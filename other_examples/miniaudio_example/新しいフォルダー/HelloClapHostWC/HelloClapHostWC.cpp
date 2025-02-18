﻿#define MINIAUDIO_IMPLEMENTATION
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
    ma_float* output = (ma_float*)pOutput;

    for (int i = 0; i < frameCount; i++)
    {

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


int main()
{
    std::cout << "Test playing sine wave for 3 seconds by miniaudio.\n\n";

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

    
    for (int i = 0; i < 3; i++) {
        std::cout << "Thread processing: " << i << std::endl;
        Sleep(1000); // 1秒待機
    }
    

    // 終了処理
    ma_device_uninit(&device);
}
