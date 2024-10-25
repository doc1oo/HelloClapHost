#include "main.hpp"
#include "clap-info-host.h"

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <numbers>
#include <string>

int main(int argc, char *argv[])
{
    const char *clap_plugin_file_path = (argc == 2) ? argv[1] : "./clap-saw-demo-imgui.clap";

    auto host = HelloClapHost(clap_plugin_file_path);
    host.run();
}

// miniaudio =====================================================================================
// miniaudio calls this function automatically when it needs more data for the output buffer.
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    auto host = static_cast<HelloClapHost *>(pDevice->pUserData);
    if (host->is_processing)
    {
        host->plugin_process(static_cast<const float *>(pInput), static_cast<float *>(pOutput), frameCount);
    }

    for (int i = 0; i < frameCount; i++)
    {
        ma_float *output = (ma_float *)pOutput;

        output[i * 2 + 0] = host->daw_audio_output_buffer[i];               // 左チャンネル
        output[i * 2 + 1] = host->daw_audio_output_buffer[i + BUFFER_SIZE]; // 右チャンネル
    }
}

// HelloClapHost =====================================================================================
HelloClapHost::HelloClapHost(const char *file_path) : is_processing(false)
{
    clap_file_path = file_path;         // = "C:/Program Files/Common Files/CLAP/Odin2.clap";

    daw_audio_output_buffer.resize(BUFFER_SIZE * 2);        // x2 for stereo
    _outputs[0] = &daw_audio_output_buffer[0];
    _outputs[1] = &daw_audio_output_buffer[BUFFER_SIZE];
}

int HelloClapHost::run()
{
    // User settings ---------------------------------------------------
    int select_plugin_index      = 0;
    ma_backend backends[]        = {ma_backend_wasapi};        //  { ma_backend_wasapi, ma_backend_dsound, ma_backend_winmm };
    int backends_count           = 1;

    // Initialising Audio Device ----------------------------------------------------------------
    std::cout << "Initializing audio device and CLAP plugin."  << std::endl;
    context_config = ma_context_config_init();
    ma_context_init(backends, backends_count, &context_config, &context);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = NULL;
    config.dataCallback = data_callback;     // This function will be called when miniaudio needs more data.
    config.pUserData = this;                 // Pass access to this class to data_callback()
    config.periodSizeInFrames = BUFFER_SIZE;

    ma_device_init(&context, &config, &device); // Initializing audio device
    ma_device_start(&device); // Start audio device

    // CLAP Plugin Initialization --------------------------------------------------------------------
    auto h_module = LoadLibrary((LPCSTR)(clap_file_path));      // DLL Load
    _handle_clap_plugin_module = h_module;

    auto p_mod = GetProcAddress(h_module, "clap_entry");        // Get entry point
    clap_plugin_entry = (clap_plugin_entry_t *)p_mod;

    clap_plugin_entry->init(clap_file_path);                        // Init plugin entry

    clap_plugin_factory = (clap_plugin_factory_t *)clap_plugin_entry->get_factory(CLAP_PLUGIN_FACTORY_ID);  // Get plugin factory

    auto &fact = clap_plugin_factory;
    clap_plugin_descriptor = fact->get_plugin_descriptor(fact, select_plugin_index);

    clap_host = clap_info_host::createCLAPInfoHost();
    clap_plugin = fact->create_plugin(fact, clap_host, clap_plugin_descriptor->id);

    auto &plugin = clap_plugin;
    plugin->init(plugin);
    plugin->activate(plugin, SAMPLE_RATE, BUFFER_SIZE, BUFFER_SIZE);
    plugin->start_processing(plugin);

    // Set ports -------------------------------------------------------------------
    auto &out = output_clap_audio_buffer;
    out.channel_count = DEVICE_CHANNELS;
    out.data32 = _outputs; // DawEngine::init()で設定された出力バッファ
    out.data64 = nullptr;
    out.constant_mask = 0;
    out.latency = 0;

    
    // Main  //////////////////////////////////////////////////////////////////////////////////////////////////////
    is_processing = true;           // if true, data_callback() will call HelloClapHost.plugin_process()

    std::cout << "Now playing notes by the CLAP plugin. (for 8 seconds)"  << std::endl;
    int play_note_keys[] = {60, 62, 64, 65, 67, 69, 71, 72};    // Note key numbers "C-D-E-F-G-A-B-C"
    for (int i = 0; i < 8; i++)
    {
        std::cout << i << " - note key: " << play_note_keys[i] << std::endl;
        process_note_on(0, 0, play_note_keys[i], 127);
        Sleep(1000); // 1秒待機
        process_note_off(0, 0, play_note_keys[i], 127);
    }

    is_processing = false;

    // Deinitializing CLAP plugin -------------------------------------------------------------------
    plugin->stop_processing(plugin);
    plugin->deactivate(plugin);
    plugin->destroy(plugin);
    clap_plugin_entry->deinit();

      // Finish audio device ---------------------------------------------------------
    ma_device_uninit(&device);

    return 0;
}

void HelloClapHost::plugin_process(const float *input, float *output, uint32_t frame_count)
{
    auto &process = clap_process;
    process.audio_outputs = &output_clap_audio_buffer;
    process.audio_outputs_count = 1;
    process.steady_time = -1;
    process.frames_count = frame_count;
    process.transport = nullptr;
    process.in_events = _event_in.clapInputEvents();
    process.out_events = _event_out.clapOutputEvents();

    clap_plugin->process(clap_plugin, &process);            // Run CLAP Plugin main process

    _event_in.clear();
    _event_out.clear();
}

int HelloClapHost::process_note_on(int sample_offset, int channel, int key, int velocity)
{
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

    return 0;
}

int HelloClapHost::process_note_off(int sample_offset, int channel, int key, int velocity)
{
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

    return 0;
}
