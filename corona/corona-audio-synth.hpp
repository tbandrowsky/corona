#pragma once



namespace corona
{
    // Audio function signature: (time_in_seconds, sample_rate) -> sample_value [-1.0, 1.0]
    using audio_function = std::function<float(double, int)>;

    // Common waveform generators
    namespace waveforms
    {
        // Sine wave at frequency Hz
        inline audio_function sine(double frequency)
        {
            return [frequency](double t, int sample_rate) {
                return std::sin(2.0 * std::numbers::pi * frequency * t);
                };
        }

        // Square wave at frequency Hz
        inline audio_function square(double frequency)
        {
            return [frequency](double t, int sample_rate) {
                return std::sin(2.0 * std::numbers::pi * frequency * t) >= 0.0 ? 1.0f : -1.0f;
                };
        }

        // Sawtooth wave at frequency Hz
        inline audio_function sawtooth(double frequency)
        {
            return [frequency](double t, int sample_rate) {
                double phase = std::fmod(t * frequency, 1.0);
                return static_cast<float>(2.0 * phase - 1.0);
                };
        }

        // Triangle wave at frequency Hz
        inline audio_function triangle(double frequency)
        {
            return [frequency](double t, int sample_rate) {
                double phase = std::fmod(t * frequency, 1.0);
                return static_cast<float>(4.0 * std::abs(phase - 0.5) - 1.0);
                };
        }

        // White noise
        inline audio_function noise()
        {
            return [](double t, int sample_rate) {
                return static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
                };
        }

        // Silence
        inline audio_function silence()
        {
            return [](double t, int sample_rate) {
                return 0.0f;
                };
        }
    }

    // Audio envelopes (ADSR)
    class envelope
    {
    public:
        double attack = 0.01;    // seconds
        double decay = 0.1;      // seconds
        double sustain = 0.7;    // level 0-1
        double release = 0.2;    // seconds

        float apply(double time_since_start, bool note_on) const
        {
            if (note_on)
            {
                if (time_since_start < attack) {
                    return static_cast<float>(time_since_start / attack);
                }
                else if (time_since_start < attack + decay) {
                    double t = (time_since_start - attack) / decay;
                    return static_cast<float>(1.0 - (1.0 - sustain) * t);
                }
                else {
                    return static_cast<float>(sustain);
                }
            }
            else
            {
                // Release phase
                if (time_since_start < release) {
                    return static_cast<float>(sustain * (1.0 - time_since_start / release));
                }
                return 0.0f;
            }
        }
    };

    // Modifiers for audio functions
    namespace modifiers
    {
        // Apply envelope to audio function
        inline audio_function with_envelope(audio_function fn, envelope env, double start_time, bool& note_on)
        {
            return [fn, env, start_time, &note_on](double t, int sr) {
                float sample = fn(t - start_time, sr);
                float env_value = env.apply(t - start_time, note_on);
                return sample * env_value;
                };
        }

        // Mix two audio functions
        inline audio_function mix(audio_function a, audio_function b, float balance = 0.5f)
        {
            return [a, b, balance](double t, int sr) {
                return a(t, sr) * balance + b(t, sr) * (1.0f - balance);
                };
        }

        // Multiply two audio functions (AM modulation)
        inline audio_function multiply(audio_function carrier, audio_function modulator)
        {
            return [carrier, modulator](double t, int sr) {
                return carrier(t, sr) * modulator(t, sr);
                };
        }

        // Frequency modulation
        inline audio_function fm(double carrier_freq, audio_function modulator, double mod_depth)
        {
            return [carrier_freq, modulator, mod_depth](double t, int sr) {
                float mod = modulator(t, sr);
                double freq = carrier_freq + mod * mod_depth;
                return std::sin(2.0 * std::numbers::pi * freq * t);
                };
        }

        // Apply volume
        inline audio_function volume(audio_function fn, float vol)
        {
            return [fn, vol](double t, int sr) {
                return fn(t, sr) * vol;
                };
        }

        // Low-pass filter (simple)
        inline audio_function lowpass(audio_function fn, float cutoff_ratio = 0.5f)
        {
            static float last_sample = 0.0f;
            return [fn, cutoff_ratio](double t, int sr) {
                float sample = fn(t, sr);
                last_sample = last_sample * (1.0f - cutoff_ratio) + sample * cutoff_ratio;
                return last_sample;
                };
        }
    }

    // Audio voice - a playing instance of an audio function
    class audio_voice
    {
    public:
        audio_function generator;
        double start_time = 0.0;
        double duration = -1.0; // -1 = infinite
        float volume = 1.0f;
        bool active = true;
        bool note_on = true;

        audio_voice(audio_function gen, double start, double dur = -1.0, float vol = 1.0f)
            : generator(gen), start_time(start), duration(dur), volume(vol)
        {
        }

        bool is_finished(double current_time) const
        {
            if (!active) return true;
            if (duration < 0) return false;
            return (current_time - start_time) >= duration;
        }

        float get_sample(double current_time, int sample_rate)
        {
            if (!active || is_finished(current_time)) return 0.0f;
            return generator(current_time, sample_rate) * volume;
        }

        void stop() { note_on = false; }
    };

    class audio_synth_engine
    {
    private:
        IMMDevice* audio_device = nullptr;
        IAudioClient* audio_client = nullptr;
        IAudioRenderClient* render_client = nullptr;
        WAVEFORMATEX* device_format = nullptr;
        UINT32 buffer_frame_count = 0;
        HANDLE audio_event = nullptr;
        std::thread audio_thread;
        std::atomic<bool> running = false;

        lockable voices_lock;
        std::vector<std::shared_ptr<audio_voice>> voices;

        double global_time = 0.0;
        int sample_rate = 48000;

        void audio_playback_thread();
        void generate_samples(float* output_buffer, UINT32 num_frames);

    public:
        audio_synth_engine();
        ~audio_synth_engine();

        bool initialize();
        void shutdown();

        // Play an audio function
        std::shared_ptr<audio_voice> play(audio_function generator, float volume = 1.0f, double duration = -1.0);

        // Play a note at a specific frequency
        std::shared_ptr<audio_voice> play_note(double frequency, double duration, float volume = 1.0f);

        // Stop a voice
        void stop_voice(std::shared_ptr<audio_voice> voice);

        // Stop all voices
        void stop_all();

        // Get current time
        double get_time() const { return global_time; }
    };

    // Procedural music generator
    void generate_procedural_music(std::shared_ptr<audio_synth_engine> audio)
    {
        // Pentatonic scale frequencies (C major pentatonic)
        double scale[] = { 261.63, 293.66, 329.63, 392.00, 440.00 };

        auto melody = [scale](double t, int sr) {
            int beat = (int)(t / 0.5); // 2 beats per second
            int note = (beat + (beat / 4)) % 5; // Change note pattern
            double freq = scale[note];

            double phase = std::fmod(t, 0.5) / 0.5;
            float envelope = std::exp(-phase * 4.0);

            return std::sin(2.0 * std::numbers::pi * freq * t) * envelope;
            };

        audio->play(melody, 0.4f); // Play forever
    }

    // FM synthesis bell
    void play_bell(std::shared_ptr<audio_synth_engine> audio, double pitch)
    {
        auto bell = modifiers::fm(
            pitch,
            waveforms::sine(pitch * 3.5), // Modulator at 3.5x carrier
            pitch * 2.0                     // Modulation depth
        );

        auto bell_with_decay = [bell](double t, int sr) {
            return bell(t, sr) * std::exp(-t * 2.0);
            };

        audio->play(bell_with_decay, 0.8f, 2.0);
    }

    audio_synth_engine::audio_synth_engine()
    {
    }

    audio_synth_engine::~audio_synth_engine()
    {
        shutdown();
    }

    bool audio_synth_engine::initialize()
    {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        // Get default audio endpoint
        IMMDeviceEnumerator* enumerator = nullptr;
        CoCreateInstance(
            __uuidof(MMDeviceEnumerator), NULL,
            CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
            (void**)&enumerator
        );

        enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio_device);
        enumerator->Release();

        // Activate audio client
        audio_device->Activate(
            __uuidof(IAudioClient), CLSCTX_ALL,
            NULL, (void**)&audio_client
        );

        // Get device format
        audio_client->GetMixFormat(&device_format);
        sample_rate = device_format->nSamplesPerSec;

        // Initialize audio client
        audio_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        audio_client->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
            10000000, // 1 second buffer
            0,
            device_format,
            NULL
        );

        audio_client->GetBufferSize(&buffer_frame_count);
        audio_client->SetEventHandle(audio_event);
        audio_client->GetService(__uuidof(IAudioRenderClient), (void**)&render_client);

        // Start audio thread
        running = true;
        audio_thread = std::thread(&audio_synth_engine::audio_playback_thread, this);

        audio_client->Start();

        return true;
    }

    void audio_synth_engine::shutdown()
    {
        running = false;

        if (audio_thread.joinable()) {
            SetEvent(audio_event);
            audio_thread.join();
        }

        if (audio_client) audio_client->Stop();
        if (render_client) render_client->Release();
        if (audio_client) audio_client->Release();
        if (audio_device) audio_device->Release();
        if (device_format) CoTaskMemFree(device_format);
        if (audio_event) CloseHandle(audio_event);
    }

    void audio_synth_engine::audio_playback_thread()
    {
        while (running)
        {
            WaitForSingleObject(audio_event, INFINITE);

            UINT32 padding = 0;
            audio_client->GetCurrentPadding(&padding);
            UINT32 frames_available = buffer_frame_count - padding;

            if (frames_available > 0)
            {
                BYTE* buffer = nullptr;
                render_client->GetBuffer(frames_available, &buffer);

                generate_samples((float*)buffer, frames_available);

                render_client->ReleaseBuffer(frames_available, 0);
            }
        }
    }

    void audio_synth_engine::generate_samples(float* output_buffer, UINT32 num_frames)
    {
        memset(output_buffer, 0, num_frames * device_format->nBlockAlign);

        auto lock = voices_lock.lock();

        // Generate and mix all voices
        for (auto it = voices.begin(); it != voices.end();)
        {
            auto& voice = *it;

            if (voice->is_finished(global_time))
            {
                it = voices.erase(it);
                continue;
            }

            for (UINT32 i = 0; i < num_frames; i++)
            {
                double t = global_time + (double)i / sample_rate;
                float sample = voice->get_sample(t, sample_rate);

                // Stereo output - same signal to both channels
                int stereo_index = i * 2;
                output_buffer[stereo_index] += sample;     // Left
                output_buffer[stereo_index + 1] += sample; // Right
            }

            ++it;
        }

        // Clamp samples to [-1, 1]
        for (UINT32 i = 0; i < num_frames * 2; i++)
        {
            output_buffer[i] = std::clamp(output_buffer[i], -1.0f, 1.0f);
        }

        global_time += (double)num_frames / sample_rate;
    }

    std::shared_ptr<audio_voice> audio_synth_engine::play(audio_function generator, float volume, double duration)
    {
        auto voice = std::make_shared<audio_voice>(generator, global_time, duration, volume);

        auto lock = voices_lock.lock();
        voices.push_back(voice);

        return voice;
    }

    std::shared_ptr<audio_voice> audio_synth_engine::play_note(double frequency, double duration, float volume)
    {
        bool note_on = true;
        auto generator = modifiers::with_envelope(
            waveforms::sine(frequency),
            envelope{ 0.01, 0.1, 0.7, 0.2 },
            global_time,
            note_on
        );

        return play(generator, volume, duration);
    }

    void audio_synth_engine::stop_voice(std::shared_ptr<audio_voice> voice)
    {
        if (voice) {
            voice->stop();
        }
    }

    void audio_synth_engine::stop_all()
    {
        auto lock = voices_lock.lock();
        for (auto& voice : voices)
        {
            voice->stop();
        }
        voices.clear();
    }
}
