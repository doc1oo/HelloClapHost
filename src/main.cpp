
#include "main.hpp"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <numbers>
#include <string>
#include <memory>
#include <shlobj.h>

#include "clap-info-host.h"

// pInputに音が入ってくるし、pOutputに音を出力する
// プラグインの音を取ってきたかったらdaw_engineかdaw_plugin_hostのオーディオバッファを書き換えて、それをコピーするなどする？
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{ // 再生モードでは、データを pOutput にコピーします。キャプチャ モードでは、データを pInput から読み取ります。全二重モードでは、pOutput と pInput の両方が有効になり、データを pInput から pOutput に移動できます。frameCount を超えるフレームは処理しないでください。

    auto host = static_cast<HelloClapHost *>(pDevice->pUserData);

    if (host->is_processing)
    {
        host->plugin_process(static_cast<const float *>(pInput), static_cast<float *>(pOutput), frameCount);
    }

    // std::cout << frameCount << std::endl; 1024とか

    for (int i = 0; i < frameCount; i++)
    {
        // ma_int16* output = (ma_int16*)pOutput;
        ma_float *output = (ma_float *)pOutput;

        output[i * 2 + 0] = host->daw_audio_output_buffer[i];               // 左チャンネル
        output[i * 2 + 1] = host->daw_audio_output_buffer[i + BUFFER_SIZE]; // 右チャンネル
    }
}

HelloClapHost::HelloClapHost() : is_processing(false)
{
}

void HelloClapHost::plugin_process(const float *input, float *output, uint32_t frame_count)
{
    
    // run CLAP process ---------------------------------------------------
    auto &process = clap_process;
    process.audio_inputs = &input_clap_audio_buffer;
    process.audio_inputs_count = 1;
    process.audio_outputs = &output_clap_audio_buffer;
    process.audio_outputs_count = 1;

    process.steady_time = -1; // -1は利用不可の場合　前フレームとの差分時間　次のプロセス呼び出しのために少なくとも `frames_count` だけ増加する必要があります。
    process.frames_count = frame_count;

    process.transport = nullptr;

    process.in_events = _event_in.clapInputEvents();
    process.out_events = _event_out.clapOutputEvents();

    auto ev_len = _event_in.size();
    // godot::UtilityFunctions::print(std::format).c_str());

    //_event_out.clear();
    // generatePluginInputEvents();

    // godot::UtilityFunctions::print(std::format("clap_plugin->process() start / {}, {}", frame_count, BUFFER_SIZE).c_str());

    // clap_plugin->process(clap_plugin, &process);        // 実際のCALPプラグインに処理させる。渡すデータ下手するとアプリごと落ちる

    if (clap_plugin != nullptr)
    {
        printf("processing...\n");
        clap_plugin->process(clap_plugin, &process);
    }
    else
    {
        printf("clap_plugin is NULL");
    }

    _event_in.clear();
    _event_out.clear();

}

int HelloClapHost::run()
{

    std::cout << "Test playing CLAP plugin tone for 3 seconds.\n\n";

    std::cout << "Initialising Audio Device.\n";

    // DAWオーディオバッファの初期化
    daw_audio_input_buffer.resize(BUFFER_SIZE * 2); // ステレオ
    daw_audio_output_buffer.resize(BUFFER_SIZE * 2);

    const bool ENABLE_WASAPI_LOW_LATENCY_MODE = false;

    // コンテキスト設定の初期化 ---------------------------------------------------
    context_config = ma_context_config_init();
    context_config.threadPriority = ma_thread_priority_highest;

    // WASAPIを使用するための設定 ---------------------------------------------------
    ma_backend backends[] = {ma_backend_wasapi}; //  { ma_backend_wasapi, ma_backend_dsound, ma_backend_winmm };
    if (ma_context_init(backends, 1, &context_config, &context) != MA_SUCCESS)
    {
        std::cout << "ma_context_init failed." << std::endl;
    }

    ma_device_info *pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info *pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
    {
        std::cout << "ma_context_get_devices failed." << std::endl;
    }

    // Print audio device information
    for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice += 1)
    {
        printf("%d - %s\n", iDevice, pPlaybackInfos[iDevice].name);
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback); // 再生モードで初期化

    // config.playback.pDeviceID を数値指定することで以下のようなデバイスを選択できる
    // 0 - OUT (UA-3FX)
    // 1 - DELL S2722DC (NVIDIA High Definition Audio)
    config.playback.pDeviceID = &pPlaybackInfos[select_audio_device_id].id; // オーディオデバイス選択

    // config.playback.pDeviceID = NULL;  // NULLでデフォルトデバイスを使用
    // config.playback.pDeviceID = &context.pDeviceInfos->id;  // NULLでデフォルトデバイスを使用
    // config.playback.format = DEVICE_FORMAT;   // Set to ma_format_unknown to use the device's native format.
    // config.playback.channels = DEVICE_CHANNELS;               // Set to 0 to use the device's native channel count.
    // config.sampleRate = DEVICE_SAMPLE_RATE;           // Set to 0 to use the device's native sample rate.
    config.dataCallback = data_callback; // This function will be called when miniaudio needs more data.
    config.pUserData = this;             // thisでないとcreate_plugin startで落ちる
    config.periodSizeInFrames = BUFFER_SIZE;

    // WASAPIを使用するように指定
    if (ENABLE_WASAPI_LOW_LATENCY_MODE)
    {
        config.wasapi.noAutoConvertSRC = MA_TRUE;     // オプション: サンプルレート変換を無効化
        config.wasapi.noDefaultQualitySRC = MA_TRUE;  // オプション: デフォルトの品質設定を使用しない
        config.wasapi.noAutoStreamRouting = MA_TRUE;  // オプション: 自動ストリームルーティングを無効化
        config.wasapi.noHardwareOffloading = MA_TRUE; // オプション: ハードウェアオフロードを無効化
    }

    // デバイスの初期化 ---------------------------------------------------
    std::cout << "ma_device_init() start" << std::endl;
    if (ma_device_init(&context, &config, &device) != MA_SUCCESS)
    {
        std::cout << "ma_device_init failed." << std::endl;
        return -1; // Failed to initialize the device.
    }

    std::cout << "Selected Device Name: {}" << device.playback.name << std::endl;

    // デバイスの開始 ---------------------------------------------------
    // ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        printf("Failed to start playback device.");
        ma_device_uninit(&device);
        return -5;
    }

    std::cout << "miniaudio Initialising all successed." << std::endl;

    // --------------================================================================

    // DAWオーディオバッファの初期化
    daw_audio_input_buffer.resize(BUFFER_SIZE * 2); // ステレオ
    daw_audio_output_buffer.resize(BUFFER_SIZE * 2);

    std::cout << "init_clap_plugin() start" << std::endl;

    std::vector<std::filesystem::path> clap_file_pathes;
    clap_file_pathes.push_back(std::filesystem::path(default_clap_folder_path_str + clap_file_name));

    auto clap_path = clap_file_pathes[0];

    auto clap_file_path_string = clap_path.string();

    std::cout << "LoadLibrary start" << std::endl;

    auto h_module = LoadLibrary((LPCSTR)(clap_path.generic_string().c_str()));
    if (!h_module)
        return -1;

    _handle_clap_plugin_module = h_module;

    auto p_mod = GetProcAddress(_handle_clap_plugin_module, "clap_entry");
    //    std::cout << "phan is " << phan << std::endl;
    auto entry = (clap_plugin_entry_t *)p_mod;

    if (!entry)
    {
        return -1;
    }

    clap_plugin_entry = entry;

    std::cout << "Entry init start" << std::endl;

    // clap_plugin_entry_tに対してのinit
    // 実際のプラグインのインスタンスの生成は get_factory(CLAP_PLUGIN_FACTORY_ID);を呼んでclap_plugin_factory_tを取得してから
    entry->init(clap_path.string().c_str()); // q.string()はclapファイルのパス

    std::cout << "Entry init end" << std::endl;
    // エラー対策
    if (!entry)
    {
        std::cerr << "   clap_entry returned a nullptr\n"
                  << "   either this plugin is not a CLAP or it has exported the incorrect symbol."
                  << std::endl;
        return 3;
    }

    std::cout << "get factory start" << std::endl;

    // プラグインファクトリーの取得
    clap_plugin_factory = (clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    auto &fact = clap_plugin_factory; // よく使うので名前短縮のため
    auto plugin_count = fact->get_plugin_count(fact);
    if (plugin_count <= 0)
    {
        std::cerr << "Plugin factory has no plugins" << std::endl;
        return 4;
    }

    std::cout << "get factory end" << std::endl;

    int select_plugin_index = 0;

    std::cout << "fact->get_plugin_descriptor() start" << std::endl;

    clap_plugin_descriptor = fact->get_plugin_descriptor(fact, select_plugin_index);
    auto &desc = clap_plugin_descriptor;

    std::cout << "fact->get_plugin_descriptor() end" << std::endl;

    clap_host = clap_info_host::createCLAPInfoHost();
    auto &host = clap_host;

    host->host_data = this;
    host->clap_version = CLAP_VERSION;
    host->name = "Minimal CLAP Host";
    host->version = "1.0.0";
    host->vendor = "Example";
    host->vendor = "clap";
    host->url = "https://github.com/free-audio/clap";
    // host->get_extension = DawPluginHost::clapExtension;
    host->request_restart = host_request_restart;
    host->request_process = host_request_process;
    host->request_callback = host_request_callback;

    bool result;

    std::cout << "create_plugin start" << std::endl;

    clap_plugin = fact->create_plugin(fact, host, desc->id);

    std::cout << "create_plugin end" << std::endl;

    auto &plugin = clap_plugin;
    if (!plugin)
    {
        std::cerr << "Unable to create plugin; inst is null" << std::endl;
        return 5;
    }

    std::cout << "plugin init start" << std::endl;
    // 実際のプラグインの初期化？
    
    result = plugin->init(plugin);
    if (!result)
    {
        std::cerr << "Unable to init plugin" << std::endl;
        return 6;
    }

    // Activate CLAP plugin ---------------------------------------------------
    std::cout << "init_clap_plugin() end" << std::endl;

    // プラグインのアクティベーション
    plugin->activate(plugin, SAMPLE_RATE, BUFFER_SIZE, BUFFER_SIZE);

    result = plugin->start_processing(plugin);
    if (!result)
    {
        std::cout << "Unable to start_processing plugin" << std::endl;
        return -1;
    }

    // ================================================================

    _inputs[0] = &daw_audio_input_buffer[0];
    _inputs[1] = &daw_audio_input_buffer[BUFFER_SIZE];
    _outputs[0] = &daw_audio_output_buffer[0];
    _outputs[1] = &daw_audio_output_buffer[BUFFER_SIZE];

    auto &in = input_clap_audio_buffer;
    auto &out = output_clap_audio_buffer;

    // Set ports ---------------------------------------------------
    in.channel_count = DEVICE_CHANNELS;
    in.data32 = _inputs; // DawEngine::init()で設定された入力バッファ
    in.data64 = nullptr;
    in.constant_mask = 0;
    in.latency = 0;

    out.channel_count = DEVICE_CHANNELS;
    out.data32 = _outputs; // DawEngine::init()で設定された出力バッファ
    out.data64 = nullptr;
    out.constant_mask = 0;
    out.latency = 0;


    is_processing = true;

    for (int i = 0; i < 5; i++)
    {
        std::cout << "Thread processing: " << i << std::endl;
        process_note_on(0, 0, 60, 127);
        Sleep(1000); // 1秒待機
        process_note_off(0, 0, 60, 127);

    }

    // deinitializing ---------------------------------------------------
    if (plugin)
    {
        plugin->stop_processing(plugin);
        plugin->deactivate(plugin);
        plugin->destroy(plugin);
    }

    if (clap_plugin_entry)
    {
        clap_plugin_entry->deinit();
    }

    // DLL を解放
    if (_handle_clap_plugin_module)
    {
        if (FreeLibrary(_handle_clap_plugin_module))
        {
            std::cout << "DLL successfully unloaded" << std::endl;
        }
        else
        {
            std::cerr << "Failed to unload DLL" << std::endl;
        }
    }

    // オーディオ終了処理
    ma_device_uninit(&device);



    return 0;
}

int HelloClapHost::process_note_on(int sample_offset, int channel, int key, int velocity)
{
    printf("DawPluginHost::process_note_on() start");
    // checkForAudioThread();

    clap_event_note ev;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_NOTE_ON;
    ev.header.time = sample_offset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.key = key;
    ev.channel = channel;
    ev.note_id = note_id; // -1はダメらしい。0以上のインクリメントされたユニークを入れる
    ev.velocity = velocity / 127.0;

    _event_in.push(&ev.header);

    note_id++;

    printf("DawPluginHost::process_note_on() end");
    return 0;
}

int HelloClapHost::process_note_off(int sample_offset, int channel, int key, int velocity)
{

    printf("DawPluginHost::process_note_off() start");
    // checkForAudioThread();

    clap_event_note ev;
    ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    ev.header.type = CLAP_EVENT_NOTE_OFF;
    ev.header.time = sample_offset;
    ev.header.flags = 0;
    ev.header.size = sizeof(ev);
    ev.port_index = 0;
    ev.key = key;
    ev.channel = channel;
    ev.note_id = note_id;
    ev.velocity = velocity / 127.0;

    _event_in.push(&ev.header);

    note_id++;

    printf("DawPluginHost::process_note_off() end");
    return 0;
}

void HelloClapHost::host_request_restart(const clap_host_t *) {}
void HelloClapHost::host_request_process(const clap_host_t *) {}
void HelloClapHost::host_request_callback(const clap_host_t *) {}
void HelloClapHost::host_log(const clap_host_t *, clap_log_severity severity, const char *msg)
{
    std::cerr << "Plugin log: " << msg << std::endl;
}

/*
HelloClapHost::~HelloClapHost()
{
}*/

int main()
{
    auto host = HelloClapHost();
    host.run();
}
