# Nordic Wi-Fi Opus Audio Demo

[![Build and Test](https://github.com/your-repo/nordic_wifi_opus_audio_demo/workflows/Build%20and%20Test%20Wi-Fi%20Opus%20Audio%20Demo/badge.svg)](https://github.com/your-repo/nordic_wifi_opus_audio_demo/actions)
[![License](https://img.shields.io/badge/License-LicenseRef--Nordic--5--Clause-blue.svg)](LICENSE)
[![NCS Version](https://img.shields.io/badge/NCS-v3.0.2-green.svg)](https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK)

## 🔍 Overview

This project demonstrates how to use Wi-Fi with UDP/TCP sockets for real-time audio streaming. It is designed to showcase low-latency audio transfer, utilizing the nRF5340 Audio DK and nRF7002 EK platforms. This sample also integrates the Opus codec for efficient audio compression and decompression, offering flexibility for various network conditions.

## 🎯 Key Features

- **Real-time Audio Streaming**: Low-latency audio transfer over Wi-Fi networks
- **Opus Codec Integration**: Efficient audio compression with configurable bitrates (6kbps to 320kbps)
- **Flexible Network Protocols**: Support for both UDP and TCP sockets
- **Multiple Wi-Fi Modes**: Station mode with credential shell or static configuration
- **Dual Device Setup**: Audio Gateway and Headset device roles
- **Battery Power Support**: Optional battery operation for headset device
- **mDNS Discovery**: Automatic device discovery within the same network

## 🧪 Test Scenarios

The demo supports the following audio streaming configurations:

| **Configuration** | **Gateway** | **Headset** | **Features** |
|-------------------|-------------|-------------|--------------|
| Basic Audio | UDP/TCP streaming | Audio playback | Standard audio streaming |
| Opus Compressed | Opus + UDP/TCP | Opus decode + playback | Compressed audio with quality control |
| Static Wi-Fi | Pre-configured credentials | Auto-connect | No runtime credential input |
| Shell Wi-Fi | Runtime credential input | Auto-discovery | Flexible network setup |

## 🔧 Hardware Requirements

### Essential Hardware
**HW:** 
- **nRF5340 Audio DK x 2** (Audio Gateway and Headset)
- **nRF7002EK x 2** (Plug in nRF5340 Audio DK as shield to support Wi-Fi)
- **USB C cable x 2**
- **Earphones/Headphones** with 3.5mm jack
- **Cable with Double 3.5mm jack Male Header(Optional)** 

### Optional Hardware Modifications for Wi-Fi Audio Headset Device
- **Enable Battery Power**: Connect nRF7002EK V5V pin to nRF5340 Audio DK TP30 testpoint.
- **Copy Audio Channel**: The device HW codec can only decode one channel from sound source by default, short nRF5340 Audio DK P14 pin1 and pin2 to output it on both headphone output channels.

### Software Requirements
**SW:** 
- **NCS v3.0.2** - Nordic Connect SDK
- **Opus v1.5.2** - Audio codec library

## 🚀 Quick Start Guide

### 1. Repository Setup

```bash
git clone https://github.com/charlieshao5189/nordic_wifi_audio_demo.git
cd nordic_wifi_opus_audio_demo/wifi_audio/src/audio/opus      
git submodule update --init
git checkout v1.5.2
```

### 2. Build and Flash Firmware

**Gateway Device:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_opus_gateway -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-opus.conf;overlay-audio-gateway.conf"
west flash --erase -d build_opus_gateway
```

**Headset Device:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_opus_headset -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-opus.conf;overlay-audio-headset.conf"
west flash --erase -d build_opus_headset
```

### 3. Connect and Configure Wi-Fi

Use the Wi-Fi credentials shell on the gateway device:
```
uart:~$ wifi cred add -s wifi_ssid -p wifi_password -k 1
uart:~$ wifi cred auto_connect
```

### 4. Start Audio Streaming

1. Set audio gateway as output device on your PC
2. Connect headphones to the headset device
3. Press play/pause on headset device to start/stop streaming
4. Use VOL+/- buttons to adjust volume

## ⚙️ Configuration Guide

### Opus Codec Configuration

Fine-tune audio quality and performance by adjusting Opus codec parameters:

| **Parameter**       | **Description**                                    | **Default Value**      | **Notes**                                                   |
|---------------------|----------------------------------------------------|-------------------------|------------------------------------------------------------|
| `Bitrate`           | Controls the quality and bandwidth usage.          | 6kbps upto 320kbps       | Higher bitrate improves quality but increases CPU usage and frame encoding time.    |
| `Frame Size`        | Duration of each audio frame in milliseconds.      | 10ms                     | Smaller frames reduce latency but increase overhead.        |
| `Complexity`        | Encoding complexity level (0-10).                  | 0                        | Lower values reduce CPU usage; higher values improve quality. |
| `Application`       | Optimization mode (VoIP, Audio, or Automatic).     | `OPUS_APPLICATION_AUDIO` | Choose based on use case (e.g., VoIP for voice).            |
| `Packet Loss (%)`   | Expected network packet loss rate.                 | 15%                      | Enables PLC (Packet Loss Concealment) to improve stability. |
| `VBR`               | Variable Bitrate mode (enabled/disabled).          | Disabled                 | Dynamically adjusts bitrate for better network adaptation.  |

### Build Configuration Options

The sample supports multiple build configurations through overlay files:

- **`overlay-opus.conf`** - Enable Opus codec support
- **`overlay-audio-gateway.conf`** - Configure device as audio gateway
- **`overlay-audio-headset.conf`** - Configure device as audio headset
- **`overlay-tcp.conf`** - Use TCP instead of UDP for audio streaming
- **`overlay-wifi-sta-static.conf`** - Use static Wi-Fi credentials

## 📋 Building

The sample has the following building options:

### 🎧 Recommended: Wi-Fi Opus Audio (Dynamic Credentials)

**Gateway:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_opus_gateway -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-opus.conf;overlay-audio-gateway.conf"
west flash --erase -d build_opus_gateway
```

**Headset:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_opus_headset -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-opus.conf;overlay-audio-headset.conf"
west flash --erase -d build_opus_headset
```

### 📶 WiFi Station Mode + WiFi CREDENTIALS SHELL (for SSID+Password Input) + UDP

**Gateway:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-audio-gateway.conf"

# Repository Setup

```bash
git clone https://github.com/charlieshao5189/nordic_wifi_audio_demo.git
cd nordic_wifi_opus_audio_demo/wifi_audio/src/audio/opus      
git submodule update --init
git checkout v1.5.2
```

# Building
The sample has following building options.

## WiFi Station Mode + WiFi CREDENTIALS SHELL(for SSID+Password Input) + UDP 

Gateway:

```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_gateway  -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-audio-gateway.conf"
west flash --erase -d build_gateway
```

**Headset:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_headset -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-audio-headset.conf"
west flash --erase -d build_headset
```

### 🔒 WiFi Station Mode + Static SSID & PASSWORD + UDP

**Gateway:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_gateway -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-gateway.conf"
west flash --erase -d build_static_gateway

west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_gateway -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf;overlay-audio-gateway.conf"
west flash --erase -d build_static_opus_gateway
```

**Headset:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_headset -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf"
west flash --erase -d build_static_headset

west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_headset -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf"
west flash --erase -d build_static_opus_headset
```

### 📡 Advanced Configuration Examples

**Use TCP instead of UDP:**
- Add `-DEXTRA_CONF_FILE=overlay-tcp.conf` to switch from UDP socket to TCP socket.

**Enable Opus codec:**
- Add `-DEXTRA_CONF_FILE=overlay-opus.conf` to turn on Opus codec.

**Example: Build audio gateway/headset with both Opus and TCP socket enabled:**
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_tcp_gateway -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-opus.conf;overlay-tcp.conf;overlay-audio-gateway.conf"
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -d build_static_opus_tcp_headset -- -DSHIELD="nrf7002ek" -DEXTRA_CONF_FILE="overlay-wifi-sta-static.conf;overlay-audio-headset.conf;overlay-opus.conf;overlay-tcp.conf"
```

## 🎮 Operation Guide

### WiFi CREDENTIALS SHELL Example

#### 1) Connect WiFi Gateway and Audio Devices with WiFi Router

```
uart:~$ wifi cred
wifi cred - Wi-Fi Credentials commands
Subcommands:
  add           : Add network to storage.
                  <-s --ssid "<SSID>">: SSID.
                  [-c --channel]: Channel that needs to be scanned for
                  connection. 0:any channel.
                  [-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]
                  [-p, --passphrase]: Passphrase (valid only for secure SSIDs)
                  [-k, --key-mgmt]: Key Management type (valid only for secure
                  SSIDs)
                  0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E,
                  5:SAE-AUTO, 6:WAPI, 7:EAP-TLS, 8:WEP, 9: WPA-PSK, 10:
                  WPA-Auto-Personal, 11: DPP
                  [-w, --ieee-80211w]: MFP (optional: needs security type to be
                  specified)
                  : 0:Disable, 1:Optional, 2:Required.
                  [-m, --bssid]: MAC address of the AP (BSSID).
                  [-t, --timeout]: Timeout for the connection attempt (in
                  seconds).
                  [-a, --identity]: Identity for enterprise mode.
                  [-K, --key-passwd]: Private key passwd for enterprise mode.
                  [-h, --help]: Print out the help for the connect command.

  auto_connect  : Connect to any stored network.
  delete        : Delete network from storage.
  list          : List stored networks.
  
uart:~$ wifi cred add -s wifi_ssid -p wifi_password -k 1
uart:~$ wifi cred auto_connect
```

The device will remember this set of credential and autoconnect to target router after reset.
Headset device will find Gateway device automatically through mDNS in the same network.

#### 2) Set Audio Gateway as Output on PC and Start Audio Streaming

After socket connection is established:
1. Make sure your host PC chooses **nRF5340 USB Audio (audio gateway)** as audio output device
2. Press **play/pause** on headset device to start/stop audio streaming
3. Use **VOL+/-** buttons to adjust volume

## 🐛 Debugging Tips

### 1. Build Warnings
Fix building warning "Setting build type to 'MinSizeRel' as none was specified.": 
- See: https://github.com/zephyrproject-rtos/picolibc/pull/7/files

### 2. Hard Fault Debugging
Command to debug hard fault:

**Linux:**
```bash
/opt/nordic/ncs/toolchains/f8037e9b83/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line -e /opt/nordic/ncs/myapps/nordic_wifi_audio_demo/wifi_audio/build_static_headset/wifi_audio/zephyr/zephyr.elf 0x00094eae
```

**Windows:**
```cmd
C:\ncs\toolchains\2d382dcd92\opt\zephyr-sdk\arm-zephyr-eabi\bin\arm-zephyr-eabi-addr2line.exe -e C:\ncs\myApps\nordic_wifi_opus_audio_demo\wifi_audio\build_opus_headset\wifi_audio\zephyr\zephyr.elf 0x0002fdfb
```

### 3. Common Issues and Solutions

| **Issue** | **Solution** |
|-----------|-------------|
| No audio output | Check USB Audio device selection on PC |
| Connection timeout | Verify Wi-Fi credentials and network connectivity |
| Audio dropouts | Adjust Opus bitrate or check network quality |
| Device not discovered | Ensure both devices are on the same network |

## 📊 Performance Characteristics

| **Configuration** | **Latency** | **CPU Usage** | **Power Consumption** | **Audio Quality** |
|-------------------|-------------|---------------|-----------------------|-------------------|
| UDP + Opus 32kbps | ~20ms | Low | Optimized | Good |
| UDP + Opus 128kbps | ~25ms | Medium | Higher | Excellent |
| TCP + Opus 128kbps | ~30ms | Medium | Higher | Excellent |
| UDP Raw Audio | ~15ms | Very Low | Lowest | High |

## 📄 License

This project is licensed under the LicenseRef-Nordic-5-Clause license. See the `LICENSE` file for details.

## 🤝 Contributing

Contributions are welcome! Please ensure all code follows the Zephyr coding style and includes appropriate license headers.

## 📞 Support

For questions and support:
- [Nordic DevZone](https://devzone.nordicsemi.com/)
- [nRF Connect SDK Documentation](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/index.html)
- [GitHub Issues](https://github.com/your-repo/nordic_wifi_opus_audio_demo/issues)
