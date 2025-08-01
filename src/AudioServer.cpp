
#include "AudioServer.h"
#include <iostream>
#include <algorithm>

// Updated constructor
AudioServer::AudioServer(int sampleRate,
                         int channels,
                         SessionLogger* logger,
                         AudioRecorder* recorder,
                         JitterBuffer* jitterBuffer)
    : sampleRate_(sampleRate),
      channels_(channels),
      logger_(logger),
      recorder_(recorder),
      jitterBuffer_(jitterBuffer),
      running_(false) {}

AudioServer::~AudioServer() {
    stop();
}

bool AudioServer::start(int port){
    if (running_) return true;
  
    network_manager_.setMessageHandler(
        [this] (const Message& msg, int socket) {
            handleClientMessage(msg, socket);
        }
    );
    if (!network_manager_.startServer(port)) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&AudioServer::serverLoop, this);
    std::cout << "AudSync Server started on port " << port << std::endl;
    return true;
}

void AudioServer::stop() {
    if (!running_) return;
    running_ = false;
  
    network_manager_.stopServer();

    if(server_thread_.joinable()){
        server_thread_.join();
    }
  
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients_.clear();
    std::cout <<"Server stopped" << std::endl;
}

bool AudioServer::isRunning() const {
    return running_;
}

size_t AudioServer::getConnectedClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients_.size();
}

void AudioServer::handleClientMessage(const Message& message, SOCKET client_socket) {
    switch (message.type) {
        case MessageType::CONNECT:
            addClient(client_socket);
            std::cout << "Client " << client_socket << " connected. Total clients: " 
                      << getConnectedClients() << std::endl;
            break;
            
        case MessageType::DISCONNECT:
            removeClient(client_socket);
            std::cout << "Client " << client_socket << " disconnected. Total clients: " 
                      << getConnectedClients() << std::endl;
            break;
            
        case MessageType::CLIENT_READY:
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                auto it = std::find_if(clients_.begin(), clients_.end(),
                    [client_socket](const ClientInfo& client) {
                        return client.socket_fd == client_socket;
                    });
                if (it != clients_.end()) {
                    it->ready = true;
                    std::cout << "Client " << client_socket << " is ready for audio" << std::endl;
                }
            }
            break;
            
        case MessageType::AUDIO_DATA:
            // Logging and recording
            if (logger_) {
                logger_->logAudioStats(message.size, sampleRate_, channels_, std::to_string(client_socket));
                logger_->logPacketMetadata(/*timestamp*/ 0, message.size);
            }
            if (recorder_ && recorder_->isRecording()) {
                std::vector<uint8_t> raw(message.data.begin(), message.data.end());
                recorder_->writeSamples(raw);
            }
            // Jitter buffer integration
            if (jitterBuffer_) {
                AudioPacket pkt;
                pkt.data = message.data;
                pkt.timestamp = message.timestamp;
                jitterBuffer_->addPacket(pkt);
            }
            broadcastAudioToOthers(message, client_socket);
            break;
            
        case MessageType::HEARTBEAT:
            network_manager_.sendMessage(message, client_socket);
            break;
            
        default:
            break;
    }
}

void AudioServer::broadcastAudioToOthers(const Message& message, SOCKET sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for(const auto& client: clients_  ){
        if(client.socket_fd != sender_socket && client.ready) {
            network_manager_.sendMessage(message, client.socket_fd);
        }
    }
}

void AudioServer::addClient(SOCKET socket_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    ClientInfo client;
    client.socket_fd = socket_fd;
    client.ready = false;
    client.id = "client_" + std::to_string(socket_fd);
    clients_.push_back(client);
}

void AudioServer::removeClient(SOCKET socket_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients_.erase(
        std::remove_if(clients_.begin(), clients_.end(),
            [socket_fd](const ClientInfo& client) {
                return client.socket_fd == socket_fd;
            }),
        clients_.end()
    );
}

void AudioServer::serverLoop() {
    std::cout << "Server loop started. Waiting for clients..." << std::endl;
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        static int counter = 0;
        if (++counter >= 30) {
            counter = 0;
            std::cout << "Server status: " << getConnectedClients() << " clients connected" << std::endl;
        }
    }
}


