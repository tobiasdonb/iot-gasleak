// ============================================================
// SISTEM PENDETEKSI KEBOCORAN GAS, API & SUHU (MODE MQTT)
// ============================================================

#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>


// ==========================================
// 1. PENGATURAN WI-FI & MQTT BROKER PUBLIK
// ==========================================
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char *topic_data = "bosbesar/iot/data";

WiFiClient espClient;
PubSubClient client(espClient);

// ==========================================
// 2. DEFINISI PIN SENSOR & RELAY
// ==========================================
const int pinFlame = 32;
const int pinGas = 34;
const int pinRelayKipas = 21;
const int pinRelayBuzzer = 22;
const int pinLedIndikator = 2;

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ==========================================
// 3. VARIABEL GLOBAL & TIMER
// ==========================================
unsigned long waktuSebelumnya = 0;
unsigned long waktuBuzzerSebelumnya = 0;

// REVISI: jedaBuzzer tidak lagi "const" agar bisa diubah kecepatannya
int jedaBuzzer = 100;
bool statusBuzzerNyala = false;

int persentaseLEL = 0;
int statusApi = 0;

// --- FUNGSI KONEKSI WI-FI ---
void setup_wifi() {
delay(10);
Serial.println();
Serial.print("Menghubungkan ke Wi-Fi: ");
Serial.println(ssid);

WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("\nWi-Fi Terhubung!");
}

// --- FUNGSI KONEKSI MQTT ---
void reconnect() {
while (!client.connected()) {
Serial.print("Menghubungkan ke MQTT Broker...");
String clientId = "ESP32_BOSBESAR_" + String(random(0xffff), HEX);
if (client.connect(clientId.c_str())) {
    Serial.println("Berhasil Terhubung!");
} else {
    Serial.print("Gagal, status=");
    Serial.print(client.state());
    Serial.println(" Coba lagi dalam 5 detik...");
    delay(5000);
}
}
}

void setup() {
Serial.begin(115200);

pinMode(pinFlame, INPUT);
pinMode(pinRelayKipas, OUTPUT);
pinMode(pinRelayBuzzer, OUTPUT);
pinMode(pinLedIndikator, OUTPUT);

digitalWrite(pinRelayKipas, HIGH);
digitalWrite(pinRelayBuzzer, HIGH);
digitalWrite(pinLedIndikator, LOW);

dht.begin();
setup_wifi();
client.setServer(mqtt_server, mqtt_port);

Serial.println("=================================================");
Serial.println(" SISTEM IOT (ALARM BEP BEP + DHT22) SIAP BEKERJA! ");
Serial.println("=================================================");
}

void loop() {
if (!client.connected()) {
reconnect();
}
client.loop();

unsigned long waktuSekarang = millis();

// [FITUR BARU] REFLEKS KEJUT SENSOR API
int bacaSensorApiSekarang = digitalRead(pinFlame);
int statusApiTerbaru = (bacaSensorApiSekarang == LOW) ? 1 : 0;

// Jika sebelumnya api=0, tapi sekarang tiba-tiba api=1, PAKSA KIRIM SEKARANG!
if (statusApiTerbaru == 1 && statusApi == 0) {
    waktuSebelumnya = 0; // Mereset timer agar blok 1 detik di bawah ini LANGSUNG jalan
}

// ==================================================
// BLOK 1: MEMBACA SENSOR & KIRIM DATA
// ==================================================
if (waktuSekarang - waktuSebelumnya > 1000) {
waktuSebelumnya = waktuSekarang;

statusApi = statusApiTerbaru;

int parameterGas = analogRead(pinGas);
persentaseLEL = map(parameterGas, 800, 4095, 0, 100);
persentaseLEL = constrain(persentaseLEL, 0, 100);

float suhu = dht.readTemperature();
float lembab = dht.readHumidity();
if (isnan(suhu) || isnan(lembab)) {
    suhu = 0.0;
    lembab = 0.0;
}

String kategoriGas = "AMAN";
if (persentaseLEL > 15) {
    kategoriGas = "BAHAYA";
} else if (persentaseLEL >= 5 && persentaseLEL <= 15) {
    kategoriGas = "WASPADA";
}

Serial.print("API: ");
Serial.print(statusApi);
Serial.print(" | GAS: ");
Serial.print(persentaseLEL);
Serial.print("% (");
Serial.print(kategoriGas);
Serial.print(") | SUHU: ");
Serial.print(suhu);
Serial.print("°C | LEMBAB: ");
Serial.print(lembab);
Serial.println("%");

int payloadKipas = (persentaseLEL >= 5 && statusApi == 0) ? 1 : 0;

String payload = "{";
payload += "\"gas\":" + String(persentaseLEL) + ",";
payload += "\"api\":" + String(statusApi) + ",";
payload += "\"suhu\":" + String(suhu, 1) + ",";
payload += "\"lembab\":" + String(lembab, 1) + ",";
payload += "\"kipas\":" + String(payloadKipas);
payload += "}";

client.publish(topic_data, payload.c_str());
digitalWrite(pinLedIndikator, !digitalRead(pinLedIndikator));
}

// ==================================================
// BLOK 2: EKSEKUSI KIPAS & TEMPO BUZZER
// ==================================================
bool kipasHarusNyala = (persentaseLEL >= 5 && statusApi == 0);

// REVISI LOGIKA BUZZER TEMPO LAMBAT & CEPAT
bool buzzerCepat = (persentaseLEL > 15 || statusApi == 1);
bool buzzerLambat =
    (persentaseLEL > 10 && persentaseLEL <= 15 && statusApi == 0);

// 1. Eksekusi Kipas
if (kipasHarusNyala) {
digitalWrite(pinRelayKipas, LOW);
} else {
digitalWrite(pinRelayKipas, HIGH);
}

// 2. Tentukan Tempo Buzzer
if (buzzerCepat) {
jedaBuzzer = 100; // Tempo sangat cepat (Bahaya / Ada Api)
} else if (buzzerLambat) {
jedaBuzzer = 500; // Tempo lambat 0.5 detik (Waspada gas > 10%)
}

// 3. Eksekusi Suara Buzzer
if (buzzerCepat || buzzerLambat) {
if (waktuSekarang - waktuBuzzerSebelumnya > jedaBuzzer) {
    waktuBuzzerSebelumnya = waktuSekarang;
    statusBuzzerNyala = !statusBuzzerNyala;

    if (statusBuzzerNyala) {
    digitalWrite(pinRelayBuzzer, LOW);
    } else {
    digitalWrite(pinRelayBuzzer, HIGH);
    }
}
} else {
digitalWrite(pinRelayBuzzer, HIGH);
statusBuzzerNyala = false;
}
}