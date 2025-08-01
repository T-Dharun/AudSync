
#pragma once

#include "NetworkManager.h"
#include "SessionLogger.h"
#include "AudioRecorder.h"
#include "JitterBuffer.h"
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

struct ClientInfo {
    SOCKET socket_fd;
    bool ready;
    std::string id;
};

class AudioServer {
  public:
    // Updated constructor with sample rate, channels, logger, recorder, jitter buffer
    AudioServer(int sampleRate,
                int channels,
                SessionLogger* logger,
                AudioRecorder* recorder,
                JitterBuffer* jitterBuffer);
    ~AudioServer();

    bool start(int port);
    void stop();

    bool isRunning() const;
    size_t getConnectedClients() const;

  private:
    NetworkManager network_manager_;
    std::vector<ClientInfo> clients_;
    std::atomic<bool> running_;

    SessionLogger* logger_;
    AudioRecorder* recorder_;
    JitterBuffer* jitterBuffer_;
    int sampleRate_;
    int channels_;

    mutable std::mutex clients_mutex;
    std::thread server_thread_;

    void handleClientMessage(const Message& message, SOCKET client_socket);
    void broadcastAudioToOthers(const Message& message, SOCKET sender_socket);
    void addClient(SOCKET socket_fd);
    void removeClient(SOCKET socket_fd);
    void serverLoop();
};
