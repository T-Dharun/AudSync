# AudSync - Cross-Platform Real-Time Audio Streaming

AudSync is a real-time audio streaming application that allows you to stream audio between devices over a network. It supports multiple operating systems including Linux, Windows, and macOS.

## Features

- Real-time audio streaming with low latency
- Cross-platform support (Linux, Windows, macOS)
- Client-server architecture
- Multiple client support
- PortAudio-based audio processing

## Supported Platforms

- **Linux** (Ubuntu, Debian, Fedora, Arch Linux, CentOS/RHEL)
- **Windows** (Windows 10/11 with Visual Studio or MinGW)
- **macOS** (macOS 10.15+)

## Prerequisites

### All Platforms
- CMake 3.10 or higher
- C++17 compatible compiler
- PortAudio library

### Platform-Specific Requirements

#### Linux
- GCC or Clang compiler
- ALSA development libraries
- pkg-config

#### Windows
- Visual Studio 2019 or newer (recommended)
- OR MinGW-w64
- vcpkg package manager (recommended for dependencies)

#### macOS
- Xcode Command Line Tools
- Homebrew (recommended for dependencies)

## Installation

### Automated Installation

We provide automated scripts to install dependencies:

#### Linux/macOS:
```bash
chmod +x install_deps.sh
./install_deps.sh
```

#### Windows (Command Prompt):
```cmd
install_deps.bat
```

#### Windows (PowerShell):
```powershell
./install_deps.ps1
```

### Manual Installation

#### Linux (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libasound2-dev portaudio19-dev
```

#### Linux (Fedora):
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake pkg-config alsa-lib-devel portaudio-devel
```

#### Linux (Arch):
```bash
sudo pacman -S base-devel cmake pkg-config alsa-lib portaudio
```

#### Windows (vcpkg):
```cmd
vcpkg install portaudio
```

#### macOS (Homebrew):
```bash
brew install cmake pkg-config portaudio
```

## Building

### Automated Build

#### Linux/macOS:
```bash
chmod +x build.sh
./build.sh
```

#### Windows:
```cmd
build.bat
```

### Manual Build

#### All Platforms:
```bash
mkdir build
cd build
cmake ..
```

#### Linux/macOS:
```bash
make -j$(nproc)
```

#### Windows (Visual Studio):
```cmd
cmake --build . --config Release
```

## Usage

### Starting the Server

#### Linux/macOS:
```bash
./build/audsync_server
```

#### Windows:
```cmd
build\Release\audsync_server.exe
```

The server will start on default port 8080. You can specify a different port:
```bash
./audsync_server 9090
```

### Starting the Client

#### Linux/macOS:
```bash
./build/audsync_client
```

#### Windows:
```cmd
build\Release\audsync_client.exe
```

The client will attempt to connect to localhost:8080 by default. You can specify a different server:
```bash
./audsync_client 192.168.1.100 9090
```

## Network Configuration

- Default port: 8080
- Protocol: TCP
- The server can handle multiple clients simultaneously
- Clients automatically synchronize audio playback

## Troubleshooting

### Common Issues

#### PortAudio Not Found
- **Linux**: Install portaudio development packages
- **Windows**: Make sure vcpkg is properly set up and PortAudio is installed
- **macOS**: Install PortAudio via Homebrew

#### Build Errors
- Ensure all dependencies are installed
- Check that your compiler supports C++17
- On Windows, make sure you're using the correct architecture (x64/x86)

#### Network Connection Issues
- Check firewall settings
- Ensure the server port is not blocked
- Verify the server IP address and port

#### Audio Issues
- Check that your audio devices are properly configured
- Ensure no other application is using the audio device exclusively
- Try different sample rates or buffer sizes

### Platform-Specific Issues

#### Windows
- If using MinGW, ensure it's in your PATH
- For Visual Studio, use the Developer Command Prompt
- vcpkg integration must be properly set up

#### Linux
- Make sure your user is in the audio group: `sudo usermod -a -G audio $USER`
- Check ALSA configuration if you have audio issues

#### macOS
- Grant microphone permissions when prompted
- Check Audio MIDI Setup for device configuration

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes ensuring cross-platform compatibility
4. Test on multiple platforms if possible
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Architecture

The project consists of:

- **NetworkManager**: Cross-platform networking abstraction
- **AudioProcessor**: PortAudio-based audio handling
- **AudioServer**: Server-side audio streaming coordination
- **AudioClient**: Client-side audio capture and playback
- **AudioBuffer**: Thread-safe audio data buffering

## Performance Notes

- Designed for low-latency real-time audio streaming
- Buffer sizes can be adjusted for different latency requirements
- Network performance depends on your local network infrastructure
- Audio quality settings can be modified in the source code
- 
##Functionalities
<img width="1773" height="661" alt="image" src="https://github.com/user-attachments/assets/55a58816-bb71-4e7c-9976-29285470eafb" />
