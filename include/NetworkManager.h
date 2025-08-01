
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <cstdint>

// Cross-platform socket includes
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define close_socket closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int SOCKET;
    #define SOCKET_ERROR_VAL -1
    #define INVALID_SOCKET_VAL -1
    #define close_socket close
#endif

enum class MessageType : uint8_t {
    CONNECT = 1,
    DISCONNECT = 2, 
    AUDIO_DATA = 3,
    HEARTBEAT = 4, 
    CLIENT_READY = 5
};

// Message struct now includes a timestamp for jitter/logging
struct Message {
    MessageType type;
    uint32_t size;
    std::vector<uint8_t> data;
    uint64_t timestamp; // Added for jitter/logging/recording
};

class NetworkManager {
  public: 
    NetworkManager();
    ~NetworkManager();

    //client Methods
    bool connectToServer(const std::string& host, int port);
    void disconnect();

    //server methods
    bool startServer(int port);
    void stopServer();
    
    //common methods
    bool sendMessage(const Message& message, SOCKET socket_fd = INVALID_SOCKET_VAL);
    bool receiveMessage(Message& message, SOCKET socket_fd = INVALID_SOCKET_VAL);
    
    void setMessageHandler(std::function<void(const Message&, SOCKET)> handler);
    bool isConnected() const;

    SOCKET getClientSocket() const {return client_socket_;}

  private:
    SOCKET server_socket_;
    SOCKET client_socket_;
    std::atomic<bool> is_server_;
    std::atomic<bool> running_;

    std::thread accept_thread_;
    std::function<void(const Message&, SOCKET)> message_handler_;

    void acceptClients();
    void handleClient(SOCKET client_fd);
    bool sendRaw(const void* data, size_t size, SOCKET socket_fd);
    bool receiveRaw(void* data, size_t size, SOCKET socket_fd);
    
    // Cross-platform socket initialization
    bool initializeNetworking();
    void cleanupNetworking();
};
