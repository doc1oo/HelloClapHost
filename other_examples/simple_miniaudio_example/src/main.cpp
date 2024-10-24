#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <windows.h>
#include <iostream>


double g_phase = 0.0;

// Automatically called by miniaudio when it needs more data for the playback buffer.
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    auto output = (ma_float*)pOutput;

    for (int i = 0; i < frameCount; i++)
    {
        float sample = 0.9 * sinf(g_phase);

        output[i * 2 + 0] = sample;     // Left Channel
		output[i * 2 + 1] = sample;     // Right Channel

        g_phase += 0.1;

        if (g_phase >= 2.0f * 3.14)
        {
            g_phase -= 2.0f * 3.14;
        }
    }
}

int main()
{
    std::cout << "Playing sound of sine wave for 3 seconds by miniaudio.\n\n";

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.dataCallback = data_callback;

    // Initialising Audio Device ------------------
    ma_device device;
    ma_device_init(NULL, &config, &device);
    ma_device_start(&device);

	// Wait 3 seconds -----------------------------
    for (int i = 0; i < 3; i++) {
        std::cout << "Now playing sine wave... " << i << std::endl;
		Sleep(1000);
    }
    
	// Unititialising Audio Device -----------------
    ma_device_uninit(&device);
}
