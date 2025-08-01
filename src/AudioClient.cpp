
#include "AudioClient.h"
#include <iostream>
#include <cstring>

// Updated constructor
AudioClient::AudioClient(int inputDeviceId,
                         int sampleRate,
                         int channels,
                         SessionLogger* logger,
                         AudioRecorder* recorder,
                         JitterBuffer* jitterBuffer)
    : inputDeviceId_(inputDeviceId),
      sampleRate_(sampleRate),
      channels_(channels),
      logger_(logger),
      recorder_(recorder),
      jitterBuffer_(jitterBuffer),
      connected_(false),
      audio_active_(false),
      running_(false) {
}

AudioClient::~AudioClient() {
    disconnect();
}

bool AudioClient::connect(const std::string& server_host, int server_port) {
    if (connected_) return true;

    network_manager_.setMessageHandler(
        [this](const Message& msg, int socket) {
            handleNetworkMessage(msg, socket);
        }
    );

    if (!network_manager_.connectToServer(server_host, server_port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return false;
    }

    connected_ = true;
    running_ = true;
    
    network_thread_ = std::thread(&AudioClient::networkLoop, this);
    
    std::cout << "Connected to server at " << server_host << ":" << server_port << std::endl;
    return true;
}

void AudioClient::disconnect() {
    running_ = false;
    
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    
    stopAudio();
    network_manager_.disconnect();
    connected_ = false;
    
    std::cout << "Disconnected from server" << std::endl;
}

bool AudioClient::startAudio() {
    if (!connected_ || audio_active_) return false;

    // Initialize audio processor with user settings
    if (!audio_processor_.initialize(inputDeviceId_, sampleRate_, channels_)) {
        std::cerr << "Failed to initialize audio processor" << std::endl;
        return false;
    }

    // Set up audio capture callback
    audio_processor_.setAudioCaptureCallback(
        [this](const float* data, size_t samples) {
            onAudioCaptured(data, samples);
        }
    );

    if (!audio_processor_.startRecording()) {
        std::cerr << "Failed to start recording" << std::endl;
        return false;
    }

    if (!audio_processor_.startPlayback()) {
        std::cerr << "Failed to start playback" << std::endl;
        audio_processor_.stop();
        return false;
    }

    // Send ready message to server
    Message ready_msg;
    ready_msg.type = MessageType::CLIENT_READY;
    ready_msg.size = 0;
    network_manager_.sendMessage(ready_msg);

    audio_active_ = true;
    std::cout << "Audio system started - you can now speak!" << std::endl;
    return true;
}

void AudioClient::stopAudio() {
    if (!audio_active_) return;
    
    audio_processor_.stop();
    audio_processor_.cleanup();
    audio_active_ = false;
    
    std::cout << "Audio system stopped" << std::endl;
}

bool AudioClient::isConnected() const {
    return connected_;
}

bool AudioClient::isAudioActive() const {
    return audio_active_;
}

void AudioClient::run() {
    std::cout << "AudSync Client" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  start     - Start audio streaming" << std::endl;
    std::cout << "  stop      - Stop audio streaming" << std::endl;
    std::cout << "  logon     - Start logging" << std::endl;
    std::cout << "  logoff    - Stop logging" << std::endl;
    std::cout << "  recstart  - Start recording session" << std::endl;
    std::cout << "  recstop   - Stop recording session" << std::endl;
    std::cout << "  quit      - Disconnect and exit" << std::endl;

    std::string command;
    while (running_ && std::cin >> command) {
        if (command == "start") {
            if (!audio_active_) {
                startAudio();
            } else {
                std::cout << "Audio already active" << std::endl;
            }
        } else if (command == "stop") {
            if (audio_active_) {
                stopAudio();
            } else {
                std::cout << "Audio not active" << std::endl;
            }
        } else if (command == "logon") {
            if (logger_) {
                logger_->startLogging("client_session.log");
                std::cout << "Logging started." << std::endl;
            }
        } else if (command == "logoff") {
            if (logger_) {
                logger_->stopLogging();
                std::cout << "Logging stopped." << std::endl;
            }
        } else if (command == "recstart") {
            if (recorder_) {
                recorder_->startRecording("client_audio.wav", sampleRate_, channels_);
                std::cout << "Audio recording started." << std::endl;
            }
        } else if (command == "recstop") {
            if (recorder_) {
                recorder_->stopRecording();
                std::cout << "Audio recording stopped." << std::endl;
            }
        } else if (command == "quit") {
            break;
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
}

void AudioClient::handleNetworkMessage(const Message& message, int socket_fd) {
    (void)socket_fd; // Unused in client mode

    switch (message.type) {
        case MessageType::AUDIO_DATA:
            if (audio_active_ && message.size > 0) {
                // Convert received data to float and add to playback buffer
                const float* audio_data = reinterpret_cast<const float*>(message.data.data());
                size_t samples = message.size / sizeof(float);
                audio_processor_.addPlaybackData(audio_data, samples);

                // Jitter buffer integration (optional for client receive)
                if (jitterBuffer_) {
                    AudioPacket pkt;
                    pkt.data.assign(message.data.begin(), message.data.end());
                    pkt.timestamp = message.timestamp;
                    jitterBuffer_->addPacket(pkt);
                }
            }
            break;
            
        case MessageType::HEARTBEAT:
            // Respond to heartbeat
            {
                Message response;
                response.type = MessageType::HEARTBEAT;
                response.size = 0;
                network_manager_.sendMessage(response);
            }
            break;
            
        default:
            break;
    }
}

void AudioClient::onAudioCaptured(const float* data, size_t samples) {
    if (!connected_ || !audio_active_) return;

    // Send audio data to server
    Message audio_msg;
    audio_msg.type = MessageType::AUDIO_DATA;
    audio_msg.size = samples * sizeof(float);
    audio_msg.data.resize(audio_msg.size);
    memcpy(audio_msg.data.data(), data, audio_msg.size);
    network_manager_.sendMessage(audio_msg);

    // Logging
    if (logger_) {
        logger_->logAudioStats(audio_msg.size, sampleRate_, channels_, std::to_string(inputDeviceId_));
        logger_->logPacketMetadata(/*timestamp*/ 0, audio_msg.size);
    }

    // Recording
    if (recorder_ && recorder_->isRecording()) {
        std::vector<uint8_t> raw(reinterpret_cast<const uint8_t*>(data),
                                 reinterpret_cast<const uint8_t*>(data) + audio_msg.size);
        recorder_->writeSamples(raw);
    }
}

void AudioClient::networkLoop() {
    while (running_) {
        Message message;
        if (network_manager_.receiveMessage(message)) {
            handleNetworkMessage(message, -1);
        } else {
            // Connection lost
            std::cout << "Connection to server lost" << std::endl;
            running_ = false;
            break;
        }
    }
}

// Static utility to list input devices
std::vector<std::string> AudioClient::getInputDeviceNames() {
    // This should use PortAudio APIs to enumerate devices
    std::vector<std::string> devices;
    // ...populate devices...
    devices.push_back("Default Device"); // Placeholder
    return devices;
}
//


