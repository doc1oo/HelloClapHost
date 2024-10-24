
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

//#include "clap-info-host.h"

// pInputに音が入ってくるし、pOutputに音を出力する
// プラグインの音を取ってきたかったらdaw_engineかdaw_plugin_hostのオーディオバッファを書き換えて、それをコピーするなどする？
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{ // 再生モードでは、データを pOutput にコピーします。キャプチャ モードでは、データを pInput から読み取ります。全二重モードでは、pOutput と pInput の両方が有効になり、データを pInput から pOutput に移動できます。frameCount を超えるフレームは処理しないでください。

    auto host = static_cast<HelloClapHost *>(pDevice->pUserData);

    if (host->is_processing) {
        host->process_audio(static_cast<const float *>(pInput), static_cast<float *>(pOutput), frameCount);
    }

    //std::cout << frameCount << std::endl; 1024とか

    for (int i = 0; i < frameCount; i++)
    {
        // ma_int16* output = (ma_int16*)pOutput;
        ma_float *output = (ma_float *)pOutput;

        output[i * 2 + 0] = host->daw_audio_output_buffer[i];   // 左チャンネル
        output[i * 2 + 1] = host->daw_audio_output_buffer[i+BUFFER_SIZE]; // 右チャンネル

    }
}


HelloClapHost::HelloClapHost() : is_processing(false)
{

}


void HelloClapHost::process_audio(const float *input, float *output, uint32_t frame_count){

}

int HelloClapHost::run() {
    
    std::cout << "Test playing CLAP plugin tone for 3 seconds.\n\n";

    // DAWオーディオバッファの初期化
    daw_audio_input_buffer.resize(BUFFER_SIZE * 2); // ステレオ
    daw_audio_output_buffer.resize(BUFFER_SIZE * 2);

    const bool ENABLE_WASAPI_LOW_LATENCY_MODE = false;

    // コンテキスト設定の初期化 ---------------------------------------------------
    context_config = ma_context_config_init();
    context_config.threadPriority = ma_thread_priority_highest;

    // WASAPIを使用するための設定 ---------------------------------------------------
    ma_backend backends[] = { ma_backend_wasapi, ma_backend_dsound, ma_backend_winmm };

    if (ma_context_init(backends, 3, &context_config, &context) != MA_SUCCESS)
    {
        std::cout << "ma_context_init failed." << std::endl;
    }

    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
    {
        std::cout << "ma_context_get_devices failed." << std::endl;
    }

    // Loop over each device info and do something with it. Here we just print the name with their index. You may want
    // to give the user the opportunity to choose which device they'd prefer.
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
    config.pUserData = nullptr;
    config.periodSizeInFrames = 1024;

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



    std::cout << "init_clap_plugin() start"<< std::endl;


    std::vector<std::filesystem::path> clap_file_pathes;
    clap_file_pathes.push_back(std::filesystem::path(default_clap_folder_path_str + clap_file_name));

    auto clap_path = clap_file_pathes[0];

    auto clap_file_path_string = clap_path.string();

    /*
    for (const auto &q : clap_file_pathes)
    {
        Json::Value entryJson;
        if (auto entry = clap_scanner::entryFromCLAPPath(q))
        {
            entry->init(q.string(<< std::endl;
            entryJson["path"] = q.string();

            //entryJson["clap-version"] = std::to_string(entry->clap_version.major) + "." +
            //                            std::to_string(entry->clap_version.minor) + "." +
            //                            std::to_string(entry->clap_version.revision);

            // entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
        }
    }*/

    // clap_plugin_entry = clap_scanner::entryFromCLAPPath(clap_path);

    std::cout << "LoadLibrary start"<< std::endl;
    
    auto h_module = LoadLibrary((LPCSTR)(clap_path.generic_string().c_str()));
    if (!h_module)
        return -1;

    _handle_clap_plugin_module = h_module;

    auto p_mod = GetProcAddress(_handle_clap_plugin_module, "clap_entry");
    //    std::cout << "phan is " << phan << std::endl;
    auto entry = (clap_plugin_entry_t *)p_mod;

    if (entry)
    {
        clap_plugin_entry = entry;

        // clap_plugin_entry_tに対してのinit
        // 実際のプラグインのインスタンスの生成は get_factory(CLAP_PLUGIN_FACTORY_ID);を呼んでclap_plugin_factory_tを取得してから
        entry->init(clap_path.string().c_str()); // q.string()はclapファイルのパス

        // エラー対策
        if (!entry)
        {
            std::cerr << "   clap_entry returned a nullptr\n"
                      << "   either this plugin is not a CLAP or it has exported the incorrect symbol."
                      << std::endl;
            return 3;
        }

        // プラグインファクトリーの取得
        clap_plugin_factory = (clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
        auto &fact = clap_plugin_factory; // よく使うので名前短縮のため
        auto plugin_count = fact->get_plugin_count(fact);
        if (plugin_count <= 0)
        {
            std::cerr << "Plugin factory has no plugins" << std::endl;
            return 4;
        }

        int select_plugin_index = 0;

        clap_plugin_descriptor = fact->get_plugin_descriptor(fact, select_plugin_index);
        auto &desc = clap_plugin_descriptor;

        // Now lets make an instance
        /*
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
        */

        clap_plugin = fact->create_plugin(fact, NULL, desc->id);
        auto &plugin = clap_plugin;
        if (!plugin)
        {
            std::cerr << "Unable to create plugin; inst is null" << std::endl;
            return 5;
        }

        // 実際のプラグインの初期化？
        bool result = plugin->init(plugin);
        if (!result)
        {
            std::cerr << "Unable to init plugin" << std::endl;
            return 6;
        }
    }

    std::cout << "init_clap_plugin() end"<< std::endl;

    if (!clap_plugin)
        return -1;

    auto &plugin = clap_plugin;

    // プラグインのアクティベーション
    plugin->activate(plugin, SAMPLE_RATE, BUFFER_SIZE, BUFFER_SIZE);

    bool result = plugin->start_processing(plugin);
    if (!result)
    {
        std::cout << "Unable to start_processing plugin"<< std::endl;
        return -1;
    }

    // ================================================================




    for (int i = 0; i < 3; i++) {
        std::cout << "Thread processing: " << i << std::endl;
        Sleep(1000); // 1秒待機
    }
    

    // 終了処理
    ma_device_uninit(&device);

    return 0;
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
