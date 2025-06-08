#include <WiFi.h>                           // Library untuk menghubungkan ESP32 ke WiFi
#include <ThingSpeak.h>                     // Library untuk komunikasi dengan platform ThingSpeak
#include <ModbusIP_ESP8266.h>               // Library untuk Modbus TCP/IP (berbasis ESP8266, bisa juga digunakan untuk ESP32)

const char* ssid = "&& || !";                   // Nama SSID WiFi
const char* password = "AND_OR_NOT";          // Password WiFi
unsigned long channelID = 2983309;          // ID channel ThingSpeak
const char* readAPIKey = "W9DCTD0PCND96YZM";  // API Key untuk membaca dari ThingSpeak
const char* writeAPIKey = "2XRUXGPTHCBPOYT8"; // API Key untuk menulis ke ThingSpeak

WiFiClient client;                          // Objek client untuk koneksi ke ThingSpeak

ModbusIP mb;                                // Objek Modbus sebagai server TCP
uint16_t HoldingRegister40001 = 0;          // Variabel untuk menyimpan nilai holding register 40001
uint16_t HoldingRegister40002 = 0;          // Variabel untuk menyimpan nilai holding register 40002
bool Coil00001 = 0;                         // Variabel status coil 00001
bool Coil00002 = 0;                         // Variabel status coil 00002
bool lastCoil = 0;                          // Variabel bantu untuk mendeteksi perubahan (tidak digunakan di kode ini)

const unsigned long intervalModbus = 1000;      // Interval pembacaan Modbus tiap 1 detik
const unsigned long intervalThingspeak = 15000; // Interval komunikasi dengan ThingSpeak tiap 15 detik

// Fungsi untuk membaca nilai dari ThingSpeak
void readThingspeak() {
  Coil00001 = ThingSpeak.readIntField(channelID, 3, readAPIKey);    // Baca coil dari field 3
  HoldingRegister40001 = ThingSpeak.readIntField(channelID, 1, readAPIKey); // Baca holding register dari field 1
}

// Fungsi untuk menulis nilai ke ThingSpeak
void writeThingspeak() {
  ThingSpeak.writeField(channelID, 2, HoldingRegister40002, writeAPIKey);  // Kirim holding register ke field 2
  Serial.print("Mengirim nilai ke Thingspeak field2: ");
  Serial.println(HoldingRegister40002);

  ThingSpeak.writeField(channelID, 4, Coil00002, writeAPIKey);      // Kirim coil ke field 4
  Serial.print("Mengirim nilai ke Thingspeak field4: ");
  Serial.println(Coil00002);
}

// Fungsi untuk mengirim data ke Modbus client
void sendData() {
  mb.Hreg(0, HoldingRegister40001);         // Kirim HoldingRegister40001 ke address 40001
  Serial.print("Mengirim nilai ke register 40001: ");
  Serial.println(HoldingRegister40001);

  mb.Coil(0, Coil00001);                    // Kirim Coil00001 ke coil 00001
  Serial.print("Mengirim nilai ke Coil 00001: ");
  Serial.println(Coil00001);
}

// Fungsi untuk menerima data dari Modbus client
void receiveData() {
  HoldingRegister40002 = mb.Hreg(1);        // Baca HoldingRegister40002 dari address 40002
  Serial.print("Nilai dari HMI di register 40002: ");
  Serial.println(HoldingRegister40002);

  Coil00002 = mb.Coil(1);                   // Baca Coil00002 dari coil 00002
  Serial.print("Status coil 00002: ");
  Serial.println(Coil00002);
}

// Task yang berjalan di Core 0 untuk menangani ThingSpeak
void taskThingspeak(void* pvParameters) {
  unsigned long previousTS = 0;
  for (;;) {
    if (millis() - previousTS >= intervalThingspeak) {    // Cek apakah waktunya akses ThingSpeak
      previousTS = millis();                              // Perbarui waktu terakhir akses
      readThingspeak();                                   // Baca data dari ThingSpeak
      writeThingspeak();                                  // Kirim data ke ThingSpeak
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay non-blocking agar task lain bisa berjalan
  }
}

// Task yang berjalan di Core 1 untuk menangani Modbus
void taskModbus(void* pvParameters) {
  unsigned long previousModbus = 0;
  for (;;) {
    if (millis() - previousModbus >= intervalModbus) {   // Cek apakah waktunya baca/kirim Modbus
      previousModbus = millis();                         // Perbarui waktu terakhir Modbus
      sendData();                                        // Kirim data ke client
      receiveData();                                     // Terima data dari client
    }
    mb.task();  // Wajib dipanggil terus agar server Modbus bisa merespon permintaan client
    vTaskDelay(10 / portTICK_PERIOD_MS); // Delay pendek agar tidak membebani CPU
  }
}

void setup() {
  Serial.begin(115200);            // Inisialisasi komunikasi serial
  WiFi.begin(ssid, password);      // Mulai koneksi ke WiFi
  while (WiFi.status() != WL_CONNECTED) { // Tunggu sampai terkoneksi
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // Tampilkan IP yang didapat dari router

  ThingSpeak.begin(client);        // Inisialisasi koneksi ThingSpeak

  mb.server();                     // Jalankan Modbus TCP server
  mb.addHreg(0);                   // Tambahkan holding register 0 (alamat 40001)
  mb.addHreg(1);                   // Tambahkan holding register 1 (alamat 40002)
  mb.addCoil(0);                   // Tambahkan coil 0 (alamat 00001)
  mb.addCoil(1);                   // Tambahkan coil 1 (alamat 00002)

  // Buat task yang berjalan di Core 0 untuk komunikasi dengan ThingSpeak
  xTaskCreatePinnedToCore(
    taskThingspeak,    // Fungsi task
    "TaskThingSpeak",  // Nama task
    4096,              // Ukuran stack task
    NULL,              // Parameter task
    1,                 // Prioritas task
    NULL,              // Handle task (tidak digunakan)
    0                  // Pin ke core 0
  );

  // Buat task yang berjalan di Core 1 untuk komunikasi Modbus
  xTaskCreatePinnedToCore(
    taskModbus,        // Fungsi task
    "TaskModbus",      // Nama task
    4096,              // Ukuran stack
    NULL,              // Parameter
    1,                 // Prioritas
    NULL,              // Handle task
    1                  // Pin ke core 1
  );
}

void loop() {
  // Loop kosong karena seluruh proses berjalan dalam 2 task di Core 0 dan Core 1
}
