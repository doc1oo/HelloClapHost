#define MINIAUDIO_IMPLEMENTATION
#define NOMINMAX // clap-helperのmin/maxエラー対策
#include "miniaudio.h"
#include <vector>
#include <atomic>
#include <clap/clap.h>
#include <clap-helpers/event-list.hh>

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define SAMPLE_RATE 48000
#define BUFFER_SIZE 1024

class HelloClapHost
{
public:
    HelloClapHost(const char *file_path);
    int run();
    void plugin_process(const float *input, float *output, uint32_t frame_count);
    int process_note_on(int sample_offset, int channel, int key, int velocity);
    int process_note_off(int sample_offset, int channel, int key, int velocity);

    int note_id = 0;
    const char *clap_file_path;
    std::vector<float> daw_audio_input_buffer;
    std::vector<float> daw_audio_output_buffer;
    std::atomic<bool> is_processing;

private:
    float *_inputs[2] = {nullptr, nullptr};
    float *_outputs[2] = {nullptr, nullptr};

    // for miniaudio
    ma_context context;
    ma_device device;
    ma_context_config context_config;

    // for clap
    clap_host_t *clap_host = nullptr;
    clap_plugin_factory_t *clap_plugin_factory = nullptr;
    const clap_plugin_entry_t *clap_plugin_entry = nullptr;
    const clap_plugin_t *clap_plugin = nullptr;
    const clap_plugin_descriptor_t *clap_plugin_descriptor = nullptr;

    clap_audio_buffer_t input_clap_audio_buffer{};
    clap_audio_buffer_t output_clap_audio_buffer{};
    clap::helpers::EventList _event_in;
    clap::helpers::EventList _event_out;
    clap_process_t clap_process{};

    HMODULE _handle_clap_plugin_module = nullptr;
};
