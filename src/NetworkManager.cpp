
#include "NetworkManager.h"
#include <iostream>
#include <cstring>
#include <chrono>

NetworkManager::NetworkManager() 
    : server_socket_(INVALID_SOCKET_VAL), client_socket_(INVALID_SOCKET_VAL), is_server_(false), running_(false) {
    initializeNetworking();
}

NetworkManager::~NetworkManager() {
    disconnect();
    stopServer();
    cleanupNetworking();
}

bool NetworkManager::initializeNetworking() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    return true;
}

void NetworkManager::cleanupNetworking() {
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkManager::connectToServer(const std::string& host, int port) {
    client_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_ == INVALID_SOCKET_VAL) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
#ifdef _WIN32
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
#else
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
#endif
        std::cerr << "Invalid address" << std::endl;
        close_socket(client_socket_);
        client_socket_ = INVALID_SOCKET_VAL;
        return false;
    }

    if (connect(client_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_VAL) {
        std::cerr << "Connection failed" << std::endl;
        close_socket(client_socket_);
        client_socket_ = INVALID_SOCKET_VAL;
        return false;
    }

    // Send connect message
    Message connect_msg;
    connect_msg.type = MessageType::CONNECT;
    connect_msg.size = 0;
    connect_msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    return sendMessage(connect_msg, client_socket_);
}

void NetworkManager::disconnect() {
    if (client_socket_ != INVALID_SOCKET_VAL) {
        // Send disconnect message
        Message disconnect_msg;
        disconnect_msg.type = MessageType::DISCONNECT;
        disconnect_msg.size = 0;
        disconnect_msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        sendMessage(disconnect_msg, client_socket_);
        
        close_socket(client_socket_);
        client_socket_ = INVALID_SOCKET_VAL;
    }
}

bool NetworkManager::startServer(int port) {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET_VAL) {
        std::cerr << "Failed to create server socket" << std::endl;
        return false;
    }

#ifdef _WIN32
    char opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#else
    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR_VAL) {
        std::cerr << "Bind failed" << std::endl;
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET_VAL;
        return false;
    }

    if (listen(server_socket_, 10) == SOCKET_ERROR_VAL) {
        std::cerr << "Listen failed" << std::endl;
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET_VAL;
        return false;
    }

    is_server_ = true;
    running_ = true;
    accept_thread_ = std::thread(&NetworkManager::acceptClients, this);

    std::cout << "Server started on port " << port << std::endl;
    return true;
}

void NetworkManager::stopServer() {
    running_ = false;
    
    if (server_socket_ != INVALID_SOCKET_VAL) {
        close_socket(server_socket_);
        server_socket_ = INVALID_SOCKET_VAL;
    }
    
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    is_server_ = false;
}

// Updated to send timestamp
bool NetworkManager::sendMessage(const Message& message, SOCKET socket_fd) {
    SOCKET target_socket = (socket_fd != INVALID_SOCKET_VAL) ? socket_fd : client_socket_;
    if (target_socket == INVALID_SOCKET_VAL) return false;

    // Send header
    uint8_t type = static_cast<uint8_t>(message.type);
    if (!sendRaw(&type, sizeof(type), target_socket)) return false;
    if (!sendRaw(&message.size, sizeof(message.size), target_socket)) return false;
    if (!sendRaw(&message.timestamp, sizeof(message.timestamp), target_socket)) return false;

    // Send data if any
    if (message.size > 0) {
        return sendRaw(message.data.data(), message.size, target_socket);
    }

    return true;
}

// Updated to receive timestamp
bool NetworkManager::receiveMessage(Message& message, SOCKET socket_fd) {
    SOCKET target_socket = (socket_fd != INVALID_SOCKET_VAL) ? socket_fd : client_socket_;
    if (target_socket == INVALID_SOCKET_VAL) return false;

    // Receive header
    uint8_t type;
    if (!receiveRaw(&type, sizeof(type), target_socket)) return false;
    if (!receiveRaw(&message.size, sizeof(message.size), target_socket)) return false;
    if (!receiveRaw(&message.timestamp, sizeof(message.timestamp), target_socket)) return false;

    message.type = static_cast<MessageType>(type);
    message.data.clear();

    // Receive data if any
    if (message.size > 0) {
        message.data.resize(message.size);
        return receiveRaw(message.data.data(), message.size, target_socket);
    }

    return true;
}

void NetworkManager::setMessageHandler(std::function<void(const Message&, SOCKET)> handler) {
    message_handler_ = handler;
}

bool NetworkManager::isConnected() const {
    return client_socket_ != INVALID_SOCKET_VAL || (is_server_ && running_);
}

void NetworkManager::acceptClients() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        SOCKET client_fd = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == INVALID_SOCKET_VAL) {
            if (running_) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        std::cout << "Client connected: " << client_fd << std::endl;
        std::thread(&NetworkManager::handleClient, this, client_fd).detach();
    }
}

void NetworkManager::handleClient(SOCKET client_fd) {
    while (running_) {
        Message message;
        if (receiveMessage(message, client_fd)) {
            if (message_handler_) {
                message_handler_(message, client_fd);
            }
            
            if (message.type == MessageType::DISCONNECT) {
                break;
            }
        } else {
            break;
        }
    }
    
    close_socket(client_fd);
    std::cout << "Client disconnected: " << client_fd << std::endl;
}

bool NetworkManager::sendRaw(const void* data, size_t size, SOCKET socket_fd) {
    size_t sent = 0;
    const char* ptr = static_cast<const char*>(data);
    
    while (sent < size) {
#ifdef _WIN32
        int result = send(socket_fd, ptr + sent, static_cast<int>(size - sent), 0);
#else
        ssize_t result = send(socket_fd, ptr + sent, size - sent, 0);
#endif
        if (result <= 0) return false;
        sent += result;
    }
    
    return true;
}

bool NetworkManager::receiveRaw(void* data, size_t size, SOCKET socket_fd) {
    size_t received = 0;
    char* ptr = static_cast<char*>(data);
    
    while (received < size) {
#ifdef _WIN32
        int result = recv(socket_fd, ptr + received, static_cast<int>(size - received), 0);
#else
        ssize_t result = recv(socket_fd, ptr + received, size - received, 0);
#endif
        if (result <= 0) return false;
        received += result;
    }
    
    return true;
}

