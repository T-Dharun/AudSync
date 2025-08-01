
#pragma once

#include "AudioBuffer.h"
#include <portaudio.h>
#include <functional>
#include <atomic>

class AudioProcessor {
  public:
      AudioProcessor();
      ~AudioProcessor();

      bool initialize(int deviceId, int sample_rate, int channels, int frames_per_buffer = 256);
      void cleanup();

      bool startRecording();
      bool startPlayback();
      void stop();

      // Set callback for when audio data is captured
      void setAudioCaptureCallback(std::function<void(const float*, size_t)> callback);
      bool addPlaybackData(const float* data, size_t samples);

      bool isRecording() const { return recording_; }
      bool isPlaying() const { return playing_; }

  private:
      PaStream* input_stream_;
      PaStream* output_stream_;
      
      AudioBuffer* playback_buffer_;
      std::function<void(const float*, size_t)> capture_callback_;
      
      std::atomic<bool> recording_;
      std::atomic<bool> playing_;
      std::atomic<bool> initialized_;

      int sample_rate_;
      int frames_per_buffer_;
      int deviceId_;
      int channels_;

      static int recordCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
      static int playCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData);
};

