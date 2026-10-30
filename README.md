# Smart Wireless Doorbell with Camera

An ESP32-CAM-based smart doorbell prototype with Wi-Fi connectivity, visitor image capture, CircuitDigest Cloud integration, WhatsApp image notifications, buzzer alerts, prerecorded voice announcements and remote dashboard controls.

> **Project status: In Progress.** The firmware is assembled from the code shared so far. It has not yet been compile-verified or tested on hardware.

## Features

- Physical doorbell button input
- Visitor image capture using the AI Thinker ESP32-CAM
- Image upload through CircuitDigest Cloud
- WhatsApp image alert through the configured HTTPS endpoint
- Buzzer alert with configurable delay and duration
- Prerecorded voice announcements using I2S output
- Remote image capture from the cloud dashboard
- Four dashboard switches for prerecorded audio responses
- NTP time synchronization

## Hardware identified

| Component | Purpose |
|---|---|
| AI Thinker ESP32-CAM with OV2640 | Main controller and camera |
| Momentary push button | Doorbell input |
| Buzzer | Local audible alert |
| I2S-compatible audio amplifier and speaker | Voice playback; exact module to be confirmed |
| Wi-Fi network | Internet connectivity |

## Software and services

- Arduino IDE / ESP32 Arduino core
- CircuitDigestCloud library
- ESP32 camera driver
- Wi-Fi, secure client and I2S libraries
- WhatsApp image notification API configured in the sketch
- NTP time synchronization

## Repository structure

```text
Smart-Wireless-Doorbell-With-Camera/
├── README.md
└── Code/
    └── smart_wireless_doorbell.ino
```

The sketch currently depends on two project-specific headers that are still pending:

- `camera.h` — AI Thinker camera pin mapping and the `initCamera()` implementation.
- `audio_data.h` — prerecorded WAV arrays, their lengths and `AUDIO_SAMPLE_RATE`.

## Configuration

Replace the placeholder Wi-Fi, CircuitDigest device and connection credentials, API key, and WhatsApp destination number locally before testing.

**Do not commit real passwords, API keys, connection keys or personal phone numbers to a public repository.** Keep placeholders in public source code.

## Important hardware and firmware checks

1. Verify all GPIO assignments against the complete AI Thinker ESP32-CAM pin map. GPIO 2, GPIO 12 and GPIO 15 are boot-strapping pins and can affect startup.
2. Confirm that the selected ESP32 Arduino core supports the I2S API used by the sketch.
3. Confirm the I2S amplifier wiring and power requirements before connecting a speaker.
4. Review the camera header and camera pin definitions before compiling.
5. Protect shared state accessed by cloud callbacks and the main loop; the current draft still needs a concurrency review.
6. Check HTTP response codes and cloud upload results rather than assuming an upload was accepted.
7. The current WhatsApp HTTPS client uses `setInsecure()`, which disables TLS certificate verification. Replace it with proper certificate validation before deployment.
8. Use a suitable regulated low-voltage power supply. Do not connect mains voltage to the prototype.

## Functional flow

1. A visitor presses the physical doorbell button or a remote capture command is received.
2. The ESP32-CAM plays the doorbell voice prompt.
3. The camera captures a JPEG image.
4. The image is uploaded to CircuitDigest Cloud and sent to the configured WhatsApp endpoint.
5. The buzzer activates after the configured delay.
6. Dashboard switches can request one of four prerecorded voice responses.

## Current limitations

- This is an academic prototype, not a certified security or access-control product.
- Live video streaming is not implemented in the supplied code; the current workflow captures still images.
- The two supporting header files are not yet included.
- Compilation, camera operation, cloud delivery and WhatsApp delivery have not been independently verified.

## Next development steps

- Add and verify `camera.h` and `audio_data.h`.
- Confirm the audio amplifier/speaker module and final GPIO wiring.
- Compile against the selected ESP32 board package and library versions.
- Test button debounce, camera recovery, image upload and cloud callbacks.
- Add the circuit diagram, bill of materials, test results and project photographs.

---
Academic major project in Electrical and Electronics Engineering.
