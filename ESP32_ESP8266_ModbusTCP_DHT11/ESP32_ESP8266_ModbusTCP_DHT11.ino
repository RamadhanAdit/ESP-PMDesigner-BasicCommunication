#ifdef ESP32
#include <WiFi.h>             // Library untuk koneksi WiFi ESP32
#elif defined(ESP8266)
#include <ESP8266WiFi.h>      // Library untuk koneksi WiFi ESP8266
#endif  
#include <ModbusIP_ESP8266.h> // Library Modbus TCP (meskipun untuk ESP8266, juga kompatibel dengan ESP32)
#include <Adafruit_Sensor.h> // Library sensor umum dari Adafruit
#include <DHT.h> // Library untuk sensor DHT (suhu dan kelembapan)
#include <DHT_U.h> // Versi unified sensor untuk DHT

// Konfigurasi WiFi
const char* ssid = "Demokit2"; // Nama SSID WiFi yang digunakan
const char* password = ""; // Password WiFi (kosong dalam hal ini)
IPAddress ip(192, 168, 100, 140); // Alamat IP statis untuk ESP32
IPAddress gateway(192, 168,100, 1); // Alamat gateway jaringan
IPAddress subnet(255, 255, 255, 0); // Subnet mask jaringan

#define LED 2 // Mendefinisikan pin GPIO2 sebagai LED
#define DHTPIN 5 // Pin GPIO5 digunakan untuk data sensor DHT11
#define DHTTYPE DHT11 // Tipe sensor yang digunakan adalah DHT11

DHT dht(DHTPIN, DHTTYPE); // Inisialisasi objek sensor DHT

// Inisialisasi Modbus
ModbusIP mb; // Membuat instance ModbusIP

// Variabel penyimpanan untuk holding register
uint16_t HoldingRegister40001 = 0; // Menyimpan nilai suhu (x100)
uint16_t HoldingRegister40002 = 0; // Menyimpan nilai kelembapan (x100)
uint16_t HoldingRegister40003 = 0; // Menyimpan nilai input dari HMI (simulasi)

void setup() {
  Serial.begin(115200); // Memulai komunikasi serial dengan baudrate 115200
  WiFi.config(ip, gateway, subnet); // Mengatur IP statis untuk ESP32
  WiFi.begin(ssid, password); // Menghubungkan ESP32 ke WiFi
  Serial.print("Menghubungkan ke WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) { // Menunggu hingga ESP32 terhubung
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Menampilkan alamat IP yang digunakan

  mb.server(); // Menjalankan ESP32 sebagai server Modbus TCP

  // Menambahkan holding register (alamat logis 40001 → index 0)
  mb.addHreg(0); // Holding register untuk suhu
  mb.addHreg(1); // Holding register untuk kelembapan
  mb.addHreg(2); // Holding register untuk data dari HMI

  mb.addCoil(0); // Menambahkan coil 00001 (alamat logis → index 0)

  pinMode(LED, OUTPUT); // Mengatur pin LED sebagai output
  
  dht.begin(); // Memulai sensor DHT
}

void loop() {
  float suhu = dht.readTemperature(); // Membaca suhu dari sensor DHT
  float kelembapan = dht.readHumidity(); // Membaca kelembapan dari sensor DHT

  if (isnan(suhu) || isnan(kelembapan)) { // Mengecek jika pembacaan gagal
    Serial.println("Gagal membaca dari sensor DHT!");
  } else {
    // Menampilkan suhu dan kelembapan ke Serial Monitor
    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.print("°C, Kelembapan: ");
    Serial.print(kelembapan);
    Serial.println("%");
  }

  HoldingRegister40001 = uint16_t(suhu * 100); // Konversi suhu ke format integer (2 digit desimal)
  HoldingRegister40002 = uint16_t(kelembapan * 100); // Konversi kelembapan ke integer (2 digit desimal)

  mb.Hreg(0, HoldingRegister40001); // Menyimpan nilai suhu ke holding register 40001
  Serial.print("Mengirim nilai ke register 40001: ");
  Serial.println(HoldingRegister40001);

  mb.Hreg(1, HoldingRegister40002); // Menyimpan nilai kelembapan ke holding register 40002
  Serial.print("Mengirim nilai ke register 40002: ");
  Serial.println(HoldingRegister40002);

  HoldingRegister40003 = mb.Hreg(2); // Membaca nilai dari register 40003 (dikirim dari HMI)
  Serial.print("Nilai dari HMI di register 40003: ");
  Serial.println(HoldingRegister40003);

  // Mengontrol LED berdasarkan status coil 00001
  bool coilState = mb.Coil(0); // Membaca status coil dari Modbus
  digitalWrite(LED, coilState); // Menyalakan/mematikan LED

  Serial.print("Status coil 00001: ");
  Serial.println(coilState ? "ON" : "OFF"); // Tampilkan status coil

  mb.task(); // Menjalankan task Modbus untuk menangani komunikasi

  delay(1000); // Delay 1 detik sebelum pembacaan ulang
}
