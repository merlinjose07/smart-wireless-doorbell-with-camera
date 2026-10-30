# Smart Wireless Doorbell with Camera

<p align="center">
  <strong>An IoT-enabled smart doorbell prototype for visitor image capture, cloud connectivity and remote notifications.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Controller-AI%20Thinker%20ESP32--CAM-E7352C?logo=espressif&logoColor=white" alt="ESP32-CAM">
  <img src="https://img.shields.io/badge/Platform-Arduino%20IDE-00979D?logo=arduino&logoColor=white" alt="Arduino IDE">
  <img src="https://img.shields.io/badge/Connectivity-Wi--Fi-blue" alt="Wi-Fi">
  <img src="https://img.shields.io/badge/Status-In%20Progress-yellow" alt="In progress">
</p>

## 1. Project overview

The **Smart Wireless Doorbell with Camera** is an embedded IoT project that extends a conventional doorbell with camera-based visitor image capture, wireless connectivity, cloud controls and image notifications.

When the physical doorbell button is pressed, the ESP32-CAM firmware plays a prerecorded voice prompt, captures a visitor image and attempts to send it to CircuitDigest Cloud and the configured WhatsApp notification endpoint. A local buzzer provides an audible indication. Remote dashboard controls can request image capture and select one of four prerecorded voice responses.

This repository contains the current firmware draft and project documentation. The system is under development; compilation and complete hardware/service testing are still pending.

## 2. Project objectives

- Develop a Wi-Fi-enabled doorbell using the AI Thinker ESP32-CAM.
- Capture a still image when the doorbell button is pressed.
- Upload the captured image to a cloud service.
- Send a visitor image notification through the configured WhatsApp API.
- Provide local buzzer and prerecorded voice indications.
- Support remote image capture and voice-response selection through a cloud dashboard.
- Develop and document a low-voltage academic prototype.

## 3. System architecture

The system is organized around the ESP32-CAM. The camera captures visitor images, the Wi-Fi connection communicates with cloud services, the I2S interface plays prerecorded audio, and a GPIO output controls the buzzer.

**Functional blocks**
- Input: physical doorbell push button and remote dashboard commands.
- Controller: AI Thinker ESP32-CAM.
- Image subsystem: onboard camera and JPEG capture.
- Connectivity: Wi-Fi, CircuitDigest Cloud and configured WhatsApp API endpoint.
- Audio subsystem: I2S output, external amplifier and speaker.
- Alert subsystem: buzzer output.

*This is a functional overview based on the supplied firmware, not a verified wiring schematic.*

## 4. Main features

| Feature | Description |
|---|---|
| Physical doorbell | Push-button input with software debounce |
| Visitor image capture | JPEG still-image capture using ESP32-CAM |
| Cloud image upload | Sends the captured image through CircuitDigest Cloud |
| WhatsApp notification | Posts the image to the configured notification endpoint |
| Audible alert | Buzzer activation with configurable delay and duration |
| Voice announcement | I2S playback of prerecorded WAV audio data |
| Remote capture | Dashboard command requests a new image |
| Remote voice responses | Four dashboard controls select prerecorded audio |
| Network time | NTP time synchronization is included |

**Current scope:** the supplied firmware captures still images. Live video streaming is not implemented.

## 5. Hardware requirements

| Component | Purpose | Current status |
|---|---|---|
| AI Thinker ESP32-CAM with camera | Main controller and image capture | Defined in code |
| Momentary push button | Doorbell trigger | Defined in code |
| Buzzer | Local audible alert | Defined in code |
| I2S-compatible amplifier | Audio output interface | Exact module to be confirmed |
| Speaker | Voice announcements | Exact model to be confirmed |
| Regulated low-voltage power supply | Power for the prototype | Rating to be finalized |
| Wi-Fi access point | Cloud connectivity | Required |

The final bill of materials and power-supply rating should be added after the exact modules are selected.

## 6. GPIO configuration in the current sketch

| Function | ESP32 GPIO |
|---|---:|
| Doorbell push button | GPIO 13 |
| Buzzer | GPIO 12 |
| I2S bit clock (BCLK) | GPIO 2 |
| I2S word select / left-right clock (LRC) | GPIO 14 |
| I2S data output (DOUT) | GPIO 15 |

**Hardware caution:** GPIO 2, GPIO 12 and GPIO 15 are ESP32 strapping pins and may affect boot behaviour depending on connected circuitry. ESP32-CAM GPIO availability is also constrained by camera and board functions. Verify the complete AI Thinker pin map and startup behaviour before wiring.

## 7. Software and services

- Arduino IDE and ESP32 board support package
- CircuitDigestCloud Arduino library
- ESP32 camera driver
- ESP32 Wi-Fi and secure client libraries
- I2S driver supported by the selected ESP32 core
- CircuitDigest Cloud device and dashboard configuration
- WhatsApp notification API credentials and approved template

Check library and API compatibility against the installed ESP32 board-package version.

## 8. Repository structure

    smart-wireless-doorbell-with-camera/
    ├── README.md
    └── Code/
        └── smart_wireless_doorbell.ino

### Required supporting files

The sketch depends on two project-specific header files that have not yet been added:

- **camera.h** — must define the AI Thinker camera pin configuration and provide the initCamera() function.
- **audio_data.h** — must provide AUDIO_SAMPLE_RATE, the prerecorded WAV arrays and their length variables:
  - voice_doorbell / voice_doorbell_len
  - voice_switch1 / voice_switch1_len
  - voice_switch2 / voice_switch2_len
  - voice_switch3 / voice_switch3_len
  - voice_switch4 / voice_switch4_len

The firmware will not compile until these dependencies are supplied and verified.

## 9. Cloud dashboard controls

| Dashboard key | Function |
|---|---|
| capture-1 | Request remote image capture |
| analog-input-1 | Play voice response 1 |
| analog-input-2 | Play voice response 2 |
| analog-input-3 | Play voice response 3 |
| analog-input-4 | Play voice response 4 |

These key names must match the device dashboard configuration in CircuitDigest Cloud.

## 10. Setup and configuration

1. Install Arduino IDE and the ESP32 board support package.
2. Install the CircuitDigestCloud library and dependencies required by the selected ESP32 core.
3. Add camera.h and audio_data.h to the sketch directory.
4. Select the correct AI Thinker ESP32-CAM board configuration.
5. Enter Wi-Fi and cloud credentials locally in the sketch.
6. Configure the dashboard keys to match Section 9.
7. Configure the WhatsApp API endpoint, destination number and approved template according to the provider's current documentation.
8. Verify low-voltage wiring and power requirements before powering the prototype.
9. Compile, upload and test each function individually before testing the complete system.

### Credential protection

Do not commit real Wi-Fi passwords, device IDs, connection keys, API keys or personal phone numbers to a public repository. Keep placeholders in public code and use a private local configuration file for actual credentials. Rotate any credentials that may already have been exposed.

## 11. Operating sequence

1. The device initializes the camera, Wi-Fi and cloud connection.
2. A visitor presses the physical button, or a remote capture command is received.
3. The firmware plays the configured doorbell voice prompt.
4. The camera captures a JPEG still image.
5. The firmware attempts to upload the image to CircuitDigest Cloud and the configured WhatsApp endpoint.
6. The buzzer is scheduled to activate after the configured delay.
7. A dashboard switch can request one of four prerecorded voice responses.

## 12. Security, reliability and validation

- The current HTTPS client uses setInsecure(), which disables TLS certificate verification. Replace it with certificate validation before deployment.
- The firmware needs a concurrency review because cloud callbacks and the main loop share state.
- Check HTTP status codes and the complete API response body; a successful TCP connection does not prove that a notification was delivered.
- Verify the cloud image-upload result and test with non-sensitive sample images first.
- Confirm camera pin assignments, boot-strapping pin behaviour and I2S compatibility with the exact board and core versions.
- Use a suitable regulated low-voltage supply. Do not connect mains voltage to the prototype.
- This academic prototype is not a certified security, surveillance or life-safety device and must not replace certified alarm or access-control equipment.

## 13. Development roadmap

- [ ] Add and verify camera.h.
- [ ] Add and verify audio_data.h and prerecorded audio data.
- [ ] Finalize amplifier, speaker and power-supply selection.
- [ ] Verify GPIO allocation and ESP32-CAM boot stability.
- [ ] Compile with the selected ESP32 board package and library versions.
- [ ] Test physical button debounce and camera capture.
- [ ] Verify CircuitDigest Cloud image upload.
- [ ] Verify WhatsApp API request, response and template configuration.
- [ ] Test buzzer timing and all four remote voice controls.
- [ ] Add a verified circuit diagram, bill of materials and hardware test results.

## 14. Project status

**Status: In Progress**

The repository currently contains the combined firmware draft and this project overview. The two required header files, complete circuit documentation, compilation checks and physical test results are pending. Features should be considered code-defined until demonstrated on the actual hardware.

---

## Author

**Merlin Jose R**  
B.E. Electrical and Electronics Engineering  
Government College of Engineering, Srirangam, Tiruchirappalli

*Academic major project exploring embedded systems, IoT connectivity and camera-based visitor notification.*
