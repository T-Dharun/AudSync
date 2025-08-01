
#include "AudioProcessor.h"
#include <iostream>
#include <cstring>

AudioProcessor::AudioProcessor()
    : input_stream_(nullptr),
      output_stream_(nullptr),
      playback_buffer_(nullptr),
      recording_(false),
      playing_(false),
      initialized_(false),
      sample_rate_(44100),
      frames_per_buffer_(256),
      deviceId_(0),
      channels_(1) // default to mono
{}

AudioProcessor::~AudioProcessor() {
    cleanup();
}

bool AudioProcessor::initialize(int deviceId, int sample_rate, int channels, int frames_per_buffer) {
    deviceId_ = deviceId;
    sample_rate_ = sample_rate;
    channels_ = channels;
    frames_per_buffer_ = frames_per_buffer;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    playback_buffer_ = new AudioBuffer(sample_rate_ * 2);
    initialized_ = true;
    return true;
}

void AudioProcessor::cleanup() {
    stop();
    if (playback_buffer_) {
        delete playback_buffer_;
        playback_buffer_ = nullptr;
    }
    if (initialized_) {
        Pa_Terminate();
        initialized_ = false;
    }
}

bool AudioProcessor::startRecording() {
    if (!initialized_ || recording_) return false;
    PaStreamParameters inputParameters;
    inputParameters.device = deviceId_;
    if (inputParameters.device == paNoDevice) {
        std::cerr << "No input device" << std::endl;
        return false;
    }

    inputParameters.channelCount = channels_;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&input_stream_,
                               &inputParameters,
                               nullptr,
                               sample_rate_,
                               frames_per_buffer_,
                               paClipOff,
                               recordCallback,
                               this);

    if (err != paNoError) {
        std::cerr << "Failed to open input stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    err = Pa_StartStream(input_stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start input stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(input_stream_);
        input_stream_ = nullptr;
        return false;
    }

    recording_ = true;
    std::cout << "Recording started " << std::endl;
    return true;
}

bool AudioProcessor::startPlayback() {
    if (!initialized_ || playing_) return false;

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        std::cerr << "No default output device" << std::endl;
        return false;
    }

    outputParameters.channelCount = channels_;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&output_stream_,
                               nullptr,
                               &outputParameters,
                               sample_rate_,
                               frames_per_buffer_,
                               paClipOff,
                               playCallback,
                               this);

    if (err != paNoError) {
        std::cerr << "Failed to open output stream: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    err = Pa_StartStream(output_stream_);
    if (err != paNoError) {
        std::cerr << "Failed to start output stream: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(output_stream_);
        output_stream_ = nullptr;
        return false;
    }

    playing_ = true;
    std::cout << "Playback started " << std::endl;
    return true;
}

void AudioProcessor::stop() {
    if (recording_ && input_stream_) {
        Pa_StopStream(input_stream_);
        Pa_CloseStream(input_stream_);
        input_stream_ = nullptr;
        recording_ = false;
        std::cout << "Recording stopped " << std::endl;
    }

    if (playing_ && output_stream_) {
        Pa_StopStream(output_stream_);
        Pa_CloseStream(output_stream_);
        output_stream_ = nullptr;
        playing_ = false;
        std::cout << "Playback stopped " << std::endl;
    }
}

void AudioProcessor::setAudioCaptureCallback(std::function<void(const float*, size_t)> callback) {
    capture_callback_ = callback;
}

bool AudioProcessor::addPlaybackData(const float* data, size_t samples) {
    if (!playback_buffer_) return false;
    return playback_buffer_->write(data, samples);
}

int AudioProcessor::recordCallback(const void* inputBuffer, void* outputBuffer,
                                   unsigned long framesPerBuffer,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags statusFlags,
                                   void* userData) {
    (void)outputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    const float* input = static_cast<const float*>(inputBuffer);

    if (processor->capture_callback_) {
        processor->capture_callback_(input, framesPerBuffer);
    }

    return paContinue;
}

int AudioProcessor::playCallback(const void* inputBuffer, void* outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void* userData) {
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;

    AudioProcessor* processor = static_cast<AudioProcessor*>(userData);
    float* output = static_cast<float*>(outputBuffer);

    if (processor->playback_buffer_) {
        processor->playback_buffer_->read(output, framesPerBuffer);
    } else {
        memset(output, 0, framesPerBuffer * sizeof(float));
    }

    return paContinue;
}

