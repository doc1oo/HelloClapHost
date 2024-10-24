#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <chrono>
#include <thread>



double g_phase = 0.0;

ma_context context;
ma_device device;
ma_context_config context_config;
int device_open_result = -9999;
int select_audio_device_id = 2;



void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{

    //auto daw_engine = static_cast<DawEngine*>(pDevice->pUserData);
    //
    //if (daw_engine->is_processing) {
    //    daw_engine->process_audio(static_cast<const float*>(pInput), static_cast<float*>(pOutput), frameCount);
    //}


    std::cout << "data_callback() start" << std::endl;

    for (int i = 0; i < frameCount; i++)
    {
        ma_float* output = (ma_float*)pOutput;

        //output[i * 2 + 0] = daw_engine->daw_audio_output_buffer[i];   // 左チャンネル
        //output[i * 2 + 1] = daw_engine->daw_audio_output_buffer[i + BUFFER_SIZE]; // 右チャンネル

        float sample = 0.9 * sinf(g_phase);

        output[i * 2 + 0] = sample; // 左チャンネル
        output[i * 2 + 1] = sample; // 右チャンネル

        g_phase += 0.1;

        // フェーズを2πでリセット
        if (g_phase >= 2.0f * 3.14)
        {
            g_phase -= 2.0f * 3.14;
        }

    }
}


int init_audio()
{
    const bool ENABLE_WASAPI_LOW_LATENCY_MODE = false;

    // コンテキスト設定の初期化 ---------------------------------------------------
    context_config = ma_context_config_init();
    context_config.threadPriority = ma_thread_priority_highest;
    // context_config.alsa.useVerboseDeviceEnumeration = MA_TRUE;		// ALSAの場合のみ

    // WASAPIを使用するための設定 ---------------------------------------------------
    // ma_backend backends[] = { ma_backend_wasapi, ma_backend_dsound, ma_backend_winmm };
    ma_backend backends[] = { ma_backend_wasapi };

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
    // config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).
    //config.pUserData = &wave_form;
    config.pUserData = nullptr;//this;
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

    std::cout << "Device Name: {}" << device.playback.name << std::endl;


    // デバイスの開始 ---------------------------------------------------
    std::cout << "ma_device_start() start" << std::endl;
    std::cout << "Device Name: {}" << device.playback.name << std::endl;
    // ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        printf("Failed to start playback device.");
        ma_device_uninit(&device);
        return -5;
    }

    std::cout << "miniaudio Initialising all successed." << std::endl;

    return 0;
}


int main()
{
    std::cout << "Hello Clap Host! Audio initialised.\n";

    init_audio();

    
    for (int i = 0; i < 5; i++) {
        std::cout << "Thread processing: " << i << std::endl;
        Sleep(1000); // 1秒待機
    }
    

    /*
    // Sleep の代わりにメッセージループを使用
    std::cout << "Playing for 3 seconds..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 3) {
            break;
        }
        // 短いスリープで CPU 使用率を抑える
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    */

    // 終了処理
    ma_device_uninit(&device);
}
