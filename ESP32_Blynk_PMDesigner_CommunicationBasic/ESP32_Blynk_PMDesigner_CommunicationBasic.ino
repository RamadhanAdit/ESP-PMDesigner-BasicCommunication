#define BLYNK_PRINT Serial // Menampilkan log Blynk melalui Serial Monitor
#define BLYNK_TEMPLATE_ID "TMPL6CSjPhCXP" // ID template dari Blynk
#define BLYNK_TEMPLATE_NAME "ESP32 PM210 HMI Communication" // Nama template
#define BLYNK_AUTH_TOKEN "sdaC5SiI5lSUfpXABanazrxso3TsOFPy" // Token autentikasi proyek Blynk

#include <WiFi.h> // Library WiFi untuk ESP32
#include <WiFiClient.h> // Library client WiFi
#include <BlynkSimpleEsp32.h> // Library Blynk untuk ESP32
#include <ModbusIP_ESP8266.h> // Library Modbus TCP, juga kompatibel dengan ESP32

BlynkTimer sendTimer; // Timer untuk mengirim data ke Modbus
BlynkTimer receiveTimer; // Timer untuk menerima data dari Modbus

char ssid[] = "y30"; // SSID WiFi
char pass[] = "00000000"; // Password WiFi

ModbusIP mb; // Inisialisasi Modbus sebagai server

// Variabel register dan coil
uint16_t HoldingRegister40001 = 0; // Register untuk dikirim dari Blynk ke HMI
uint16_t HoldingRegister40002 = 0; // Register yang diterima dari HMI
bool Coil00001 = 0; // Coil untuk dikirim ke HMI
bool Coil00002 = 0; // Coil yang diterima dari HMI

// Fungsi untuk mengirim data ke Modbus
void sendData() {
  mb.Hreg(0, HoldingRegister40001); // Kirim nilai ke holding register 40001 (index 0)
  Serial.print("Mengirim nilai ke register 40001: ");
  Serial.println(HoldingRegister40001);

  mb.Coil(0, Coil00001); // Kirim nilai ke coil 00001 (index 0)
  Serial.print("Mengirim nilai ke Coil 00001: ");
  Serial.println(Coil00001);
}

// Fungsi untuk menerima data dari Modbus
void receiveData() {
  HoldingRegister40002 = mb.Hreg(1); // Baca nilai dari register 40002 (index 1)
  Serial.print("Nilai dari HMI di register 40002: ");
  Serial.println(HoldingRegister40002);

  Coil00002 = mb.Coil(1); // Baca status coil 00002 (index 1)
  Serial.print("Status coil 00001: ");
  Serial.println(Coil00002 ? "ON" : "OFF");

  Blynk.virtualWrite(V1, HoldingRegister40002); // Tampilkan register 40002 di widget V1
  Blynk.virtualWrite(V3, Coil00002); // Tampilkan status coil 00002 di widget V3

  mb.task(); // Proses permintaan Modbus (wajib dipanggil terus)
}

// Fungsi saat Blynk mengirim data ke V0
BLYNK_WRITE(V0) {
  HoldingRegister40001 = param.asInt(); // Ambil nilai dari V0 (misalnya slider atau input)
  Serial.print("Nilai diterima dari Blynk (V0): ");
  Serial.println(HoldingRegister40001);
}

// Fungsi saat Blynk mengirim data ke V2
BLYNK_WRITE(V2) {
  Coil00001 = param.asInt(); // Ambil nilai dari V2 (misalnya tombol)
  Serial.print("Nilai diterima dari Blynk (V2): ");
  Serial.println(Coil00001);
}

void setup() {
  Serial.begin(115200); // Memulai komunikasi serial
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // Inisialisasi koneksi ke Blynk
  WiFi.begin(ssid, pass); // Koneksi ke WiFi
  Serial.print("Menghubungkan ke WiFi...");

  while (WiFi.status() != WL_CONNECTED) { // Tunggu hingga terkoneksi
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Tampilkan alamat IP ESP32

  mb.server(); // Inisialisasi ESP32 sebagai server Modbus TCP

  // Tambahkan register dan coil
  mb.addHreg(0); // Holding register 40001
  mb.addHreg(1); // Holding register 40002
  mb.addCoil(0); // Coil 00001
  mb.addCoil(1); // Coil 00002

  // Atur timer untuk kirim dan terima data setiap 1 detik
  sendTimer.setInterval(1000L, sendData);
  receiveTimer.setInterval(1000L, receiveData);
}

void loop() {
  Blynk.run(); // Jalankan layanan Blynk
  sendTimer.run(); // Jalankan fungsi pengiriman berkala
  receiveTimer.run(); // Jalankan fungsi penerimaan berkala
}
