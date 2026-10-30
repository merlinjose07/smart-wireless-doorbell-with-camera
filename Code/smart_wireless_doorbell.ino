#include <Arduino.h>
#include <CircuitDigestCloud.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "driver/i2s.h"
#include "audio_data.h"
#include "time.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera.h"

// Replace placeholders locally. Do not publish real credentials.
#define WIFI_SSID      "your-Wi-Fi-name"
#define WIFI_PASS      "Your-Wi-Fi-password"
#define DEVICE_ID      "Your-Device-ID"
#define CONNECTION_KEY "Your-Connection-Key"
#define API_KEY        "Your-API-Key"

#define WHATSAPP_PHONE_NUMBER "yourphone number"
#define WHATSAPP_TEMPLATE_ID  "image_capture_alert"
#define WHATSAPP_HOST         "www.circuitdigest.cloud"

#define REMOTE_CAPTURE "capture-1"
#define SLOT_SWITCH_1  "analog-input-1"
#define SLOT_SWITCH_2  "analog-input-2"
#define SLOT_SWITCH_3  "analog-input-3"
#define SLOT_SWITCH_4  "analog-input-4"

#define DOORBELL_BTN_PIN   13
#define BUZZER_PIN         12
#define DEBOUNCE_MS        50
#define BUZZER_DURATION_MS 3000
#define BUZZER_ACTIVE_HIGH 0
#define BUZZER_ON  (BUZZER_ACTIVE_HIGH ? HIGH : LOW)
#define BUZZER_OFF (BUZZER_ACTIVE_HIGH ? LOW : HIGH)
#define BUZZER_DELAY_MS 3000

#define I2S_LRC  14
#define I2S_BCLK 2
#define I2S_DOUT 15
#define I2S_NUM  I2S_NUM_1

CircuitDigestCloud CDcloud;

volatile uint32_t lastEdgeMs = 0;
bool remoteCapture = false;
volatile bool captureBusy = false;
uint32_t buzzerStartTime = 0;
bool buzzerActive = false;
bool buzzerPending = false;
uint32_t buzzerPendingSince = 0;
volatile int playResponseIndex = 0;

void IRAM_ATTR onDoorbellButton() {
  lastEdgeMs = millis();
}

void triggerBuzzer() {
  digitalWrite(BUZZER_PIN, BUZZER_ON);
  buzzerStartTime = millis();
  buzzerActive = true;
}

bool startI2SAudio() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = AUDIO_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) return false;

  err = i2s_set_pin(I2S_NUM, &pin_config);
  if (err != ESP_OK) {
    i2s_driver_uninstall(I2S_NUM);
    return false;
  }
  return true;
}

void stopI2SAudio() {
  i2s_driver_uninstall(I2S_NUM);
}

void playAudioBuffer(const unsigned char* audio_arr, size_t arr_len) {
  if (!audio_arr || arr_len <= 44) return;
  if (!startI2SAudio()) return;

  size_t bytesWritten = 0;
  const uint8_t* audio_data = audio_arr + 44; // Skip WAV header
  size_t audio_len = arr_len - 44;

  i2s_write(I2S_NUM, audio_data, audio_len, &bytesWritten, portMAX_DELAY);
  delay(100);
  stopI2SAudio();
}

void sendWhatsAppAlertWithImage(
  const uint8_t* imageBuf,
  size_t imageLen,
  const char* eventLabel
) {
  if (!imageBuf || imageLen == 0) return;

  WiFiClientSecure client;
  // Security limitation: this disables TLS certificate validation.
  // Replace with CA certificate validation before deployment.
  client.setInsecure();

  if (!client.connect(WHATSAPP_HOST, 443)) {
    Serial.println("WhatsApp: connection failed");
    return;
  }

  String boundary = "ESP32CAMBound7MA4YWxkTrZu0gW";
  String crlf = "\r\n";
  String dash = "--";

  String variablesJson =
    String("{\"event_type\":\"") + eventLabel + "\"," +
    "\"location\":\"Front Door\"," +
    "\"device_name\":\"Outdoor Doorbell\"," +
    "\"captured_time\":\"Just now\"}";

  String bodyHead =
    dash + boundary + crlf +
    "Content-Disposition: form-data; name=\"phone_number\"" + crlf + crlf +
    WHATSAPP_PHONE_NUMBER + crlf +
    dash + boundary + crlf +
    "Content-Disposition: form-data; name=\"template_id\"" + crlf + crlf +
    WHATSAPP_TEMPLATE_ID + crlf +
    dash + boundary + crlf +
    "Content-Disposition: form-data; name=\"variables\"" + crlf + crlf +
    variablesJson + crlf +
    dash + boundary + crlf +
    "Content-Disposition: form-data; name=\"image\"; filename=\"p.jpg\"" + crlf +
    "Content-Type: image/jpeg" + crlf + crlf;

  String bodyTail = crlf + dash + boundary + dash + crlf;
  size_t totalContentLength = bodyHead.length() + imageLen + bodyTail.length();

  client.println("POST /api/v1/whatsapp/send-with-image HTTP/1.1");
  client.print("Host: "); client.println(WHATSAPP_HOST);
  client.print("Authorization: "); client.println(API_KEY);
  client.print("Content-Type: multipart/form-data; boundary=");
  client.println(boundary);
  client.print("Content-Length: "); client.println(totalContentLength);
  client.println();

  client.print(bodyHead);
  client.write(imageBuf, imageLen);
  client.print(bodyTail);

  uint32_t startMs = millis();
  while ((client.connected() || client.available()) &&
         millis() - startMs < 8000) {
    if (client.available()) {
      Serial.println(client.readStringUntil('\n'));
    }
  }
  client.stop();
}

bool resetCamera() {
  esp_camera_deinit();
  delay(50);

#ifdef PWDN_GPIO_NUM
  if (PWDN_GPIO_NUM != -1) {
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, HIGH);
    delay(100);
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(100);
  }
#endif

  return initCamera();
}

void handleCapture(float value) {
  if ((bool)value && !captureBusy) remoteCapture = true;
  CDcloud.publish(REMOTE_CAPTURE, 0.0f);
}

void handleSwitch1(float value) {
  if ((bool)value) {
    playResponseIndex = 1;
    CDcloud.publish(SLOT_SWITCH_1, 0.0f);
  }
}

void handleSwitch2(float value) {
  if ((bool)value) {
    playResponseIndex = 2;
    CDcloud.publish(SLOT_SWITCH_2, 0.0f);
  }
}

void handleSwitch3(float value) {
  if ((bool)value) {
    playResponseIndex = 3;
    CDcloud.publish(SLOT_SWITCH_3, 0.0f);
  }
}

void handleSwitch4(float value) {
  if ((bool)value) {
    playResponseIndex = 4;
    CDcloud.publish(SLOT_SWITCH_4, 0.0f);
  }
}

static void captureAndUpload() {
  if (captureBusy) return;
  captureBusy = true;

  buzzerPending = true;
  buzzerPendingSince = millis();

  playAudioBuffer(voice_doorbell, voice_doorbell_len);

  sensor_t* s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_bpc(s, 1);
    s->set_wpc(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_special_effect(s, 0);
    s->set_saturation(s, 0);
  }

  camera_fb_t* dummy_fb = esp_camera_fb_get();
  if (dummy_fb) esp_camera_fb_return(dummy_fb);
  dummy_fb = esp_camera_fb_get();
  if (dummy_fb) esp_camera_fb_return(dummy_fb);
  delay(200);

  Serial.println("Capturing photo...");
  camera_fb_t* fb = esp_camera_fb_get();

  if (!fb) {
    bool recovered = false;
    for (int attempt = 1; attempt <= 2 && !recovered; attempt++) {
      if (resetCamera()) {
        delay(300);
        fb = esp_camera_fb_get();
        if (fb) recovered = true;
      }
    }

    if (!recovered) {
      Serial.println("Camera recovery failed; restarting.");
      ESP.restart();
    }
  }

  if (!fb) {
    captureBusy = false;
    return;
  }

  bool uploaded = CDcloud.sendImage(fb->buf, fb->len, "image/jpeg");
  Serial.println(uploaded ? "Cloud image upload successful"
                          : "Cloud image upload failed");

  sendWhatsAppAlertWithImage(fb->buf, fb->len, "Doorbell Pressed");
  esp_camera_fb_return(fb);
  captureBusy = false;
}

void waitForNTPSync() {
  Serial.print("Syncing time via NTP");
  configTime(0, 0, "time.google.com", "pool.ntp.org", "1.1.1.1");

  time_t now = time(nullptr);
  uint32_t startMs = millis();
  while (now < 8 * 3600 * 2 && millis() - startMs < 15000) {
    delay(250);
    now = time(nullptr);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(DOORBELL_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DOORBELL_BTN_PIN),
                  onDoorbellButton, FALLING);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);
  delay(300);

  bool camOk = initCamera();
  for (int attempt = 1; !camOk && attempt <= 3; attempt++) {
    delay(500);
    camOk = initCamera();
  }
  if (!camOk) ESP.restart();

  WiFi.mode(WIFI_STA);
  if (!CDcloud.begin(WIFI_SSID, WIFI_PASS, DEVICE_ID,
                     CONNECTION_KEY, API_KEY)) {
    Serial.println("Cloud connection failed");
    while (true) delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED) WiFi.setSleep(false);
  waitForNTPSync();

  CDcloud.subscribe(REMOTE_CAPTURE, handleCapture);
  CDcloud.subscribe(SLOT_SWITCH_1, handleSwitch1);
  CDcloud.subscribe(SLOT_SWITCH_2, handleSwitch2);
  CDcloud.subscribe(SLOT_SWITCH_3, handleSwitch3);
  CDcloud.subscribe(SLOT_SWITCH_4, handleSwitch4);

  Serial.println("Smart Wireless Doorbell Started");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    static uint32_t lastReconnectAttempt = 0;
    uint32_t currentMs = millis();

    if (currentMs - lastReconnectAttempt >= 5000) {
      lastReconnectAttempt = currentMs;
      WiFi.disconnect();
      WiFi.reconnect();
    }
    delay(100);
    return;
  }

  CDcloud.loop();

  if (buzzerPending &&
      millis() - buzzerPendingSince >= BUZZER_DELAY_MS) {
    buzzerPending = false;
    triggerBuzzer();
  }

  if (buzzerActive &&
      millis() - buzzerStartTime >= BUZZER_DURATION_MS) {
    digitalWrite(BUZZER_PIN, BUZZER_OFF);
    buzzerActive = false;
  }

  if (lastEdgeMs && millis() - lastEdgeMs >= DEBOUNCE_MS) {
    lastEdgeMs = 0;
    if (digitalRead(DOORBELL_BTN_PIN) == LOW) {
      captureAndUpload();
    }
  }

  if (remoteCapture) {
    remoteCapture = false;
    captureAndUpload();
  }

  if (playResponseIndex > 0) {
    int choice = playResponseIndex;
    playResponseIndex = 0;

    switch (choice) {
      case 1: playAudioBuffer(voice_switch1, voice_switch1_len); break;
      case 2: playAudioBuffer(voice_switch2, voice_switch2_len); break;
      case 3: playAudioBuffer(voice_switch3, voice_switch3_len); break;
      case 4: playAudioBuffer(voice_switch4, voice_switch4_len); break;
    }
  }
}
