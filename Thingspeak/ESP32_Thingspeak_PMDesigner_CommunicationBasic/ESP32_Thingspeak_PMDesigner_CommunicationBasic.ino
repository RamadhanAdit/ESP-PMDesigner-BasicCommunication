#ifdef ESP32
#include <WiFi.h>             // Library untuk koneksi WiFi ESP32
#elif defined(ESP8266)
#include <ESP8266WiFi.h>      // Library untuk koneksi WiFi ESP8266
#endif 
#include <ThingSpeak.h>                   // Library untuk berkomunikasi dengan platform IoT ThingSpeak
#include <ModbusIP_ESP8266.h>             // Library Modbus TCP/IP (digunakan meskipun ini untuk ESP8266, masih bisa untuk ESP32 jika kompatibel)

const char* ssid = "&& || !";             // Nama WiFi (SSID) yang akan dikoneksikan
const char* password = "AND_OR_NOT";      // Password WiFi
unsigned long channelID = 2983309;        // ID channel ThingSpeak
const char* readAPIKey = "W9DCTD0PCND96YZM";  // API Key untuk membaca data dari ThingSpeak
const char* writeAPIKey = "2XRUXGPTHCBPOYT8"; // API Key untuk menulis data ke ThingSpeak

WiFiClient client;                        // Objek client untuk komunikasi internet

ModbusIP mb;                              // Objek Modbus server (ESP32 sebagai Modbus TCP server)
uint16_t HoldingRegister40001 = 0;        // Variabel untuk menyimpan nilai Holding Register 40001
uint16_t HoldingRegister40002 = 0;        // Variabel untuk menyimpan nilai Holding Register 40002
bool Coil00001 = 0;                       // Variabel untuk menyimpan status Coil 00001
bool Coil00002 = 0;                       // Variabel untuk menyimpan status Coil 00002

unsigned long waktuSekarang = 0;          // Variabel untuk menyimpan waktu saat ini
unsigned long waktuSebelumnyaModbus = 0;  // Waktu terakhir Modbus dijalankan
unsigned long waktuSebelumnyaThingspeak = 0; // Waktu terakhir Thingspeak dijalankan
const unsigned long intervalModbus = 1000;    // Interval pembacaan Modbus (dalam milidetik)
const unsigned long intervalThingspeak = 15000; // Interval kirim/baca ThingSpeak (15 detik)

void readThingspeak() {
  // Membaca data dari ThingSpeak: Coil di field 3, Holding Register di field 1
  Coil00001 = ThingSpeak.readIntField(channelID, 3, readAPIKey);
  HoldingRegister40001 = ThingSpeak.readIntField(channelID, 1, readAPIKey);
}

void writeThingspeak() {
  // Mengirim HoldingRegister40002 ke field 2
  ThingSpeak.writeField(channelID, 2, HoldingRegister40002, writeAPIKey);
  Serial.print("Mengirim nilai ke Thingspeak field2: ");
  Serial.println(HoldingRegister40002);

  // Mengirim Coil00002 ke field 4
  ThingSpeak.writeField(channelID, 4, Coil00002, writeAPIKey);
  Serial.print("Mengirim nilai ke Thingspeak field4: ");
  Serial.println(Coil00002);
}

void sendData() {
  // Menyimpan nilai ke Holding Register 40001 di Modbus TCP
  mb.Hreg(0, HoldingRegister40001);
  Serial.print("Mengirim nilai ke register 40001: ");
  Serial.println(HoldingRegister40001);

  // Menyimpan status ke Coil 00001 di Modbus TCP
  mb.Coil(0, Coil00001);
  Serial.print("Mengirim nilai ke Coil 00001: ");
  Serial.println(Coil00001);
}

void receiveData() {
  // Membaca nilai dari Holding Register 40002 dari HMI/client Modbus
  HoldingRegister40002 = mb.Hreg(1);
  Serial.print("Nilai dari HMI di register 40002: ");
  Serial.println(HoldingRegister40002);

  // Membaca status dari Coil 00002 dari HMI/client Modbus
  Coil00002 = mb.Coil(1);
  Serial.print("Status coil 00001: ");
  Serial.println(Coil00002 ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);                  // Inisialisasi komunikasi serial dengan baudrate 115200
  WiFi.begin(ssid, password);            // Memulai koneksi ke jaringan WiFi

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);                         // Tunggu 1 detik tiap loop jika belum terkoneksi
    Serial.print(".");                   // Tampilkan titik sebagai indikator koneksi
  }

  Serial.println("\nWiFi Terhubung!");   // Tampilkan pesan setelah koneksi berhasil
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());        // Tampilkan IP address perangkat

  Serial.println("Terhubung ke WiFi");
  ThingSpeak.begin(client);              // Mulai koneksi ke ThingSpeak dengan client

  mb.server();                           // Atur perangkat sebagai Modbus TCP server

  // Tambahkan Holding Register dan Coil pada alamat 0 dan 1
  mb.addHreg(0);
  mb.addHreg(1);
  mb.addCoil(0);
  mb.addCoil(1);
}

void loop() {
  waktuSekarang = millis();              // Update waktu sekarang

  // Periksa apakah saatnya update ThingSpeak
  if (waktuSekarang - waktuSebelumnyaThingspeak >= intervalThingspeak) {
    waktuSebelumnyaThingspeak = waktuSekarang;  // Simpan waktu update terakhir
    readThingspeak();                   // Baca data dari ThingSpeak
    writeThingspeak();                 // Kirim data ke ThingSpeak
  }

  // Periksa apakah saatnya komunikasi Modbus
  if (waktuSekarang - waktuSebelumnyaModbus >= intervalModbus) {
    waktuSebelumnyaModbus = waktuSekarang; // Simpan waktu Modbus terakhir
    sendData();                             // Kirim data ke client Modbus (HMI)
    receiveData();                          // Terima data dari client Modbus
    mb.task();                              // Jalankan tugas Modbus (handle request dari client)
  }
}
