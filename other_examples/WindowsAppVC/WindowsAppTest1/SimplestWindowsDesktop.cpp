// HelloWindowsDesktop.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include <format>
#include <cassert>
#include <iostream>
#include <sstream>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Global variables

ma_device device;

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#define DEVICE_FORMAT       ma_format_f32
#define DEVICE_CHANNELS     2
#define DEVICE_SAMPLE_RATE  48000



// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Windows Desktop Guided Tour Application");

// Stored instance handle for use in Win32 API calls such as FindResource
HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);



int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindowEx explained:
    // WS_EX_OVERLAPPEDWINDOW : An optional extended window style.
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 100,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);


    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    return (int)msg.wParam;
}



void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_waveform* pSineWave;

    MA_ASSERT(pDevice->playback.channels == DEVICE_CHANNELS);

    pSineWave = (ma_waveform*)pDevice->pUserData;
    MA_ASSERT(pSineWave != NULL);

    ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);

    (void)pInput;   
}



int init_audio_device()
{
    ma_waveform sine_wave;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = NULL;  // NULLでデフォルトデバイスを使用
    config.playback.format = DEVICE_FORMAT;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = DEVICE_CHANNELS;               // Set to 0 to use the device's native channel count.
    config.sampleRate = DEVICE_SAMPLE_RATE;           // Set to 0 to use the device's native sample rate.
    config.dataCallback = data_callback;   // This function will be called when miniaudio needs more data.
    //config.pUserData         = pMyCustomData;   // Can be accessed from the device object (device.pUserData).
    config.pUserData         = &sine_wave;
    config.periodSizeInFrames = 2048; 

    // WASAPIを使用するように指定
    config.wasapi.noAutoConvertSRC = MA_TRUE;  // オプション: サンプルレート変換を無効化
    config.wasapi.noDefaultQualitySRC = MA_TRUE;  // オプション: デフォルトの品質設定を使用しない
    config.wasapi.noAutoStreamRouting = MA_TRUE;  // オプション: 自動ストリームルーティングを無効化
    config.wasapi.noHardwareOffloading = MA_TRUE;  // オプション: ハードウェアオフロードを無効化

    //ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return -1;  // Failed to initialize the device.
    }

    printf(std::format("Device Name: {}", device.playback.name).c_str() );

    ma_waveform_config sine_wave_config = ma_waveform_config_init(
        device.playback.format,
        device.playback.channels,
        device.sampleRate,
        ma_waveform_type_sawtooth,
        0.2,
        440
    );
    printf( std::format("ma_waveform_config_init done.").c_str() );

    ma_result result = ma_waveform_init(&sine_wave_config, &sine_wave);
    if (result != MA_SUCCESS) {
        printf(std::format("ma_waveform_init: failed").c_str() );
    }

    printf(std::format("sine_wave.config.frequency: {}", sine_wave.config.frequency).c_str() );
    
    printf(std::format("Device Name: {}", device.playback.name).c_str() );

    printf( std::format("ma_waveform_init done.").c_str() );

    //ma_device_start(&device);     // The device is sleeping by default so you'll need to start it manually.
    if (ma_device_start(&device) != MA_SUCCESS) {
        printf("Failed to start playback device.");
        ma_device_uninit(&device);
        return -5;
    }

    printf( std::format("miniaudio Initialising all successed.").c_str() );

    return 0;
}



int init_audio() {

    /// JUCE -------------------------------------------------------------
    const int request_device_type_id = 0;
    const int request_device_name_id = 0;
    std::stringstream ss;

    //MessageBox(NULL, "Starting tests of audio device setup with JUCE module!", "Info", MB_OK);

    // List up available juce::AudioIODeviceType
    //     (Example):
    //     Windows Audio
    //     Windows Audio(Exclusive Mode)
    //     Windows Audio(Low Latency Mode)
    //     DirectSound
    //const auto& device_types = adm.getAvailableDeviceTypes();
    //assert(!device_types.isEmpty());

    ss.clear();
    ss << "getAvailableDeviceTypes(): " << std::endl;

	// Enable request audio device type
    ss.clear();
    MessageBox(NULL, ss.str().c_str(), "Info", MB_OK);

    //ss << "AudioDeviceManager.setCurrentAudioDeviceType():" << std::endl <<  "   " << device_types[request_device_type_id]->getTypeName().toStdString();
    //MessageBox(NULL, ss.str().c_str(), "Info", MB_OK);


    int  device_open_result = init_audio_device();

	// Get opened audio device information

    MessageBox(NULL, "Playing sine wave sound start!,", "Info", MB_OK);
    //tone_generator.setFrequency(440);
    //player.setSource(&tone_generator);
    //adm.addAudioCallback(&player);

    MessageBox(NULL, "Playing sine wave sound finished.", "Info", MB_OK);

	// Close audio device
    //adm.closeAudioDevice();

    MessageBox(NULL, "closeAudioDevice() finished!", "Info", MB_OK);

    if (device_open_result >= 0)
    {
        ma_device_uninit(&device);
    }

    return 0;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR greeting[] = _T("Hello, Windows desktop!");

    switch (message)
    {
    case WM_CREATE:
        init_audio();


    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);

        // Here your application is laid out.
        // For this introduction, we just print out "Hello, Windows desktop!"
        // in the top left corner.
        TextOut(hdc,
            5, 5,
            greeting, _tcslen(greeting));
        // End application-specific layout section.

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        //adm.closeAudioDevice();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}