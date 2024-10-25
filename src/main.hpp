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
    HelloClapHost();
    int run();
    void process_audio(const float *input, float *output, uint32_t frame_count);
    static void host_request_restart(const clap_host_t *);
    static void host_request_process(const clap_host_t *);
    static void host_request_callback(const clap_host_t *);
    static void host_log(const clap_host_t *, clap_log_severity severity, const char *msg);

    std::vector<float> daw_audio_input_buffer;
    std::vector<float> daw_audio_output_buffer;
    std::atomic<bool> is_processing;


private:
    double g_phase = 0.0;

    std::string default_clap_folder_path_str = "C:/Program Files/Common Files/CLAP/";
    std::string clap_file_name = "Odin2.clap";

    ma_context context;
    ma_device device;
    ma_context_config context_config;
    int device_open_result = -9999;
    int select_audio_device_id = 2;

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

    float *_inputs[2] = {nullptr, nullptr};
    float *_outputs[2] = {nullptr, nullptr};
};
