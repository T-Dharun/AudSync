
#include "AudioClient.h"
#include "SessionLogger.h"
#include "AudioRecorder.h"
#include "JitterBuffer.h"
#include <iostream>
#include <string>
#include <vector>

void printDeviceList(const std::vector<std::string>& devices) {
    std::cout << "Available Audio Devices:\n";
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i] << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string server_host = "127.0.0.1";
    int server_port = 8080;

    // Parse command line arguments
    if (argc >= 2) server_host = argv[1];
    if (argc >= 3) server_port = std::stoi(argv[2]);

    std::cout << "AudSync Client - Real-time Audio Streaming\n";
    std::cout << "Connecting to Server: " << server_host << ":" << server_port << "\n";

    // Device selection
    std::vector<std::string> inputDevices = AudioClient::getInputDeviceNames();
    printDeviceList(inputDevices);
    int inputDeviceId = 0;
    std::cout << "Select input device ID: ";
    std::cin >> inputDeviceId;

    // Sample rate selection
    int sampleRate = 44100;
    std::cout << "Enter desired sample rate (e.g., 44100): ";
    std::cin >> sampleRate;

    // Channel selection
    int channels = 1;
    std::cout << "Enter number of channels (1=Mono, 2=Stereo, etc.): ";
    std::cin >> channels;

    // Initialize logger, recorder, jitter buffer
    SessionLogger logger;
    AudioRecorder recorder;
    JitterBuffer jitterBuffer;

    AudioClient client(inputDeviceId, sampleRate, channels, &logger, &recorder, &jitterBuffer);

    if (!client.connect(server_host, server_port)) {
        std::cerr << "Failed to connect to server. Make sure the server is running.\n";
        return 1;
    }

    std::cout << "Successfully connected to Server!\n";
    std::cout << "Type commands during session:\n";
    std::cout << "  'start'  - Begin audio streaming\n";
    std::cout << "  'stop'   - Pause audio streaming\n";
    std::cout << "  'logon'  - Start logging\n";
    std::cout << "  'logoff' - Stop logging\n";
    std::cout << "  'recstart' - Start recording session\n";
    std::cout << "  'recstop'  - Stop recording session\n";
    std::cout << "  'quit'   - Exit client\n";

    // Run the client main loop (should handle commands and pass to logger/recorder)
    client.run();

    std::cout << "Client shutting down...\n";
    return 0;
}


