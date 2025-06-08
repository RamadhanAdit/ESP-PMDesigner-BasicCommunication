#ifdef ESP32
#include <WiFi.h>             // Library untuk koneksi WiFi ESP32
#elif defined(ESP8266)
#include <ESP8266WiFi.h>      // Library untuk koneksi WiFi ESP8266
#endif             
#include <PubSubClient.h>      // Library untuk komunikasi MQTT
#include <ModbusIP_ESP8266.h>  // Library untuk Modbus TCP/IP
#include <ArduinoJson.h>       // Library untuk manipulasi JSON

// ------------------- Konfigurasi -------------------
// Informasi WiFi
const char* ssid = "&& || !";         // Nama WiFi
const char* password = "AND_OR_NOT";  // Password WiFi

// Informasi MQTT untuk ThingsBoard
const char* mqtt_server = "mqtt.thingsboard.cloud";  // Server MQTT ThingsBoard
const char* access_token = "1lj2n3d5m8526rham81i";   // Token akses untuk autentikasi perangkat

// Inisialisasi objek WiFi dan MQTT
WiFiClient espClient;
PubSubClient client(espClient);
ModbusIP mb;  // ESP32 sebagai server Modbus TCP/IP

// ------------------- Alamat Register -------------------
// Definisi alamat register Modbus
const uint16_t HR_40001 = 0;    // Holding Register pertama
const uint16_t HR_40002 = 1;    // Holding Register kedua
const uint16_t COIL_00001 = 0;  // Coil pertama
const uint16_t COIL_00002 = 1;  // Coil kedua

// ------------------- Fungsi MQTT Callback -------------------
// Fungsi yang dipanggil saat menerima pesan dari MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<200> doc;                                         // Membuat objek JSON dengan kapasitas 200 byte
  DeserializationError error = deserializeJson(doc, payload, length);  // Parsing JSON dari payload

  // Jika parsing gagal, cetak pesan error
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  serializeJsonPretty(doc, Serial);  // Menampilkan JSON yang diterima untuk debugging
  Serial.println();

  // Mengecek apakah JSON berisi parameter yang dikirim melalui RPC
  if (doc.containsKey("params")) {
    JsonObject params = doc["params"];

    // Mengecek apakah terdapat coil00001 dan mengubah statusnya
    if (params.containsKey("coil00001")) {
      bool val = params["coil00001"];
      Serial.print("coil00001 (from params): ");
      Serial.println(val);
      mb.Coil(COIL_00001, val);  // Memperbarui nilai coil di Modbus
    }

    // Mengecek apakah terdapat holding40001 dan mengubah nilainya
    if (params.containsKey("holding40001")) {
      int val = params["holding40001"];
      Serial.print("holding40001 (via RPC): ");
      Serial.println(val);
      mb.Hreg(HR_40001, val);  // Memperbarui nilai holding register di Modbus
    }
  }
}

// ------------------- Publish ke ThingsBoard -------------------
// Fungsi untuk mengirim data ke ThingsBoard
void publishToThingsBoard() {
  uint16_t hr40002 = mb.Hreg(HR_40002);  // Membaca nilai dari Holding Register 40002
  bool coil00002 = mb.Coil(COIL_00002);  // Membaca status dari Coil 00002

  // Menyusun data JSON untuk dikirim ke ThingsBoard
  String payload = "{";
  payload += "\"holding40002\":" + String(hr40002) + ",";
  payload += "\"coil00002\":" + String(coil00002);
  payload += "}";

  // Mengirim data melalui MQTT ke ThingsBoard
  client.publish("v1/devices/me/telemetry", payload.c_str());
  Serial.println("Published: " + payload);  // Menampilkan data yang dikirim
}

// ------------------- Setup MQTT -------------------
// Fungsi untuk menghubungkan kembali ke ThingsBoard jika terputus
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard...");

    // Mencoba menghubungkan ke server MQTT dengan Access Token
    if (client.connect("ESP32Client", access_token, nullptr)) {
      Serial.println("connected");

      // Berlangganan topik atribut dan RPC pada MQTT
      client.subscribe("v1/devices/me/attributes");
      client.subscribe("v1/devices/me/rpc/request/+");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);  // Tunggu 5 detik sebelum mencoba kembali
    }
  }
}

// ------------------- Setup -------------------
// Fungsi yang dijalankan saat ESP32 dinyalakan pertama kali
void setup() {
  Serial.begin(115200);  // Memulai komunikasi serial

  WiFi.begin(ssid, password);  // Menghubungkan ke jaringan WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");  // Cetak jika berhasil terhubung ke WiFi
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Konfigurasi server MQTT untuk ThingsBoard
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);  // Menentukan callback untuk menerima pesan MQTT

  // Inisialisasi Modbus Server
  mb.server();

  // Menambahkan register Modbus
  mb.addHreg(HR_40001, 0);
  mb.addHreg(HR_40002, 0);
  mb.addCoil(COIL_00001, 0);
  mb.addCoil(COIL_00002, 0);
}

// ------------------- Loop -------------------
// Fungsi utama yang berjalan terus-menerus
unsigned long lastPublish = 0;  // Variabel untuk menyimpan waktu terakhir publish data
const long interval = 5000;     // Interval waktu untuk pengiriman data (5 detik)

void loop() {
  if (!client.connected()) {
    reconnect();  // Jika MQTT terputus, coba hubungkan kembali
  }
  client.loop();  // Menjalankan loop MQTT untuk menerima pesan baru
  mb.task();      // Menjalankan tugas Modbus

  // Mengecek apakah sudah waktunya mengirim data ke ThingsBoard
  if (millis() - lastPublish > interval) {
    lastPublish = millis();
    publishToThingsBoard();  // Mengirim data ke ThingsBoard
  }
}