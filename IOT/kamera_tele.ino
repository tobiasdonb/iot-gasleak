#include "esp_camera.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <base64.h>

// ==========================================
// 1. PENGATURAN WIFI & MQTT
// ==========================================
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char *mqtt_topic = "bosbesar/iot/data";

// ==========================================
// 2. PENGATURAN TELEGRAM
// ==========================================
String BOT_TOKEN = "YOUR_BOT_TOKEN";
String CHAT_ID = "YOUR_CHAT_ID"; // ID Channel / Chat Telegram

// ==========================================
// 3. PENGATURAN ROBOFLOW
// ==========================================
String RF_API_KEY = "YOUR_ROBOFLOW_API_KEY";
String RF_MODEL = "fire-ynqxb/1";

// ==========================================
// PIN KAMERA & FLASH (AI THINKER)
// ==========================================
#define FLASH_LED_PIN 4
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

WiFiClient espClient;
PubSubClient mqttClient(espClient);

bool triggerFoto = false;
int statusApiSebelumnya = 0;

// ==========================================
// FUNGSI KLASIFIKASI ROBOFLOW
// ==========================================
String klasifikasiRoboflow(camera_fb_t *fb) {
  String hasilDeteksi = "AI: Gagal menghubungi Roboflow";

  WiFiClientSecure client_rf;
  client_rf.setInsecure();
  client_rf.setTimeout(20000);

  if (client_rf.connect("detect.roboflow.com", 443)) {
    String url =
        "/" + RF_MODEL + "?api_key=" + RF_API_KEY + "&confidence=0.55&format=json";

    String base64Image = base64::encode(fb->buf, fb->len);

    client_rf.println("POST " + url + " HTTP/1.1");
    client_rf.println("Host: detect.roboflow.com");
    client_rf.println("Content-Type: application/x-www-form-urlencoded");
    client_rf.println("Content-Length: " + String(base64Image.length()));
    client_rf.println();

    // Kirim data base64 ke server
    client_rf.print(base64Image);

    String responseBody = "";
    bool isBody = false;
    unsigned long timeout = millis();
    while (client_rf.connected() || client_rf.available()) {
      if (millis() - timeout > 10000)
        break;

      if (client_rf.available()) {
        String line = client_rf.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) {
          isBody = true;
        } else if (isBody) {
          responseBody += line;
        }
      }
    }
    client_rf.stop();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, responseBody);

    if (!error) {
      if (doc.containsKey("error") || doc.containsKey("message")) {
        String errorMsg = doc["message"] | doc["error"]["message"] | "Error dari server Roboflow";
        hasilDeteksi = "⚠️ ERROR AI:\n" + errorMsg;
      } else {
        JsonArray predictions = doc["predictions"];
        String listPrediksi = "";

        if (predictions.size() > 0) {
          int bestConf = 0;
          String bestClass = "";

          for (JsonObject pred : predictions) {
            String className = pred["class"];
            float conf = pred["confidence"];
            int persentase = conf * 100;

            if (persentase > bestConf) {
              bestConf = persentase;
              bestClass = className;
            }
            listPrediksi +=
                "- " + className + " (" + String(persentase) + "%)\n";
          }

          bool isFire = (bestClass.indexOf("non") == -1) && (bestConf >= 55);

          if (isFire) {
            hasilDeteksi = "🔥 STATUS: TAK AMAN!\n(" + bestClass + ": " +
                           String(bestConf) + "%)\n" + listPrediksi;
          } else {
            hasilDeteksi = "✅ STATUS: AMAN\n(" + bestClass + ": " +
                           String(bestConf) + "%)\nRincian AI:\n" +
                           listPrediksi;
          }
        } else {
          hasilDeteksi = "STATUS:\nTidak ada objek terdeteksi.";
        }
      }
    }
  } else {
    hasilDeteksi = "AI: Gagal membaca respons (JSON Error).";
  }

  return hasilDeteksi;
} // <--- KURUNG TUTUP INI YANG SEBELUMNYA KURANG ATAU SALAH POSISI!

// ==========================================
// FUNGSI MENGIRIM FOTO KE TELEGRAM
// ==========================================
void kirimFotoTelegram() {
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);

  camera_fb_t *fb = esp_camera_fb_get();
  if (fb)
    esp_camera_fb_return(fb);

  fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    Serial.println("Gagal mengambil foto");
    return;
  }

  String hasilAI = klasifikasiRoboflow(fb);

  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();
  client_tcp.setTimeout(20000);

  if (client_tcp.connect("api.telegram.org", 443)) {
    String head = "--GeminiBot\r\nContent-Disposition: form-data; "
                  "name=\"chat_id\"\r\n\r\n" +
                  CHAT_ID + "\r\n";
    String caption = "--GeminiBot\r\nContent-Disposition: form-data; "
                     "name=\"caption\"\r\n\r\n🚨 BUKTI VISUAL KAMERA! "
                     "🚨\r\n\r\n🔍 Hasil Analisis AI:\r\n" +
                     hasilAI + "\r\n";
    String photo =
        "--GeminiBot\r\nContent-Disposition: form-data; name=\"photo\"; "
        "filename=\"bahaya.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--GeminiBot--\r\n";

    uint32_t totalLen = head.length() + caption.length() + photo.length() +
                        fb->len + tail.length();

    client_tcp.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
    client_tcp.println("Host: api.telegram.org");
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=GeminiBot");
    client_tcp.println();

    client_tcp.print(head);
    client_tcp.print(caption);
    client_tcp.print(photo);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client_tcp.write(fbBuf, remainder);
      }
    }
    client_tcp.print(tail);
    esp_camera_fb_return(fb);
    client_tcp.stop();
  } else {
    esp_camera_fb_return(fb);
  }
}

// ==========================================
// FUNGSI MENERIMA DATA DARI ESP32 UTAMA
// ==========================================
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String pesan = "";
  for (int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, pesan);

  if (!error) {
    int api_status = doc["api"];
    if (api_status == 1 && statusApiSebelumnya == 0) {
      triggerFoto = true;
    }
    statusApiSebelumnya = api_status;
  }
}

// ==========================================
// KONEKSI MQTT
// ==========================================
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    String clientId = "Sniper_BosBesar_" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      mqttClient.subscribe(mqtt_topic);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_camera_init(&config);

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 0);
  s->set_hmirror(s, 0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  HTTPClient http;
  String url = "https://api.telegram.org/bot" + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=📸+Kamera+Siaga.+Menunggu+sinyal+api+dari+sensor!";
  http.begin(url);
  http.GET();
  http.end();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  if (triggerFoto) {
    kirimFotoTelegram();
    triggerFoto = false;
  }
}