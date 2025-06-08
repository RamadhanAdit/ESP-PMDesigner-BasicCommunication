#include <ModbusIP_ESP8266.h>  // Library untuk komunikasi Modbus TCP/IP pada ESP8266
#include <ESP8266WiFi.h>       // Library untuk koneksi WiFi pada ESP8266

// ------------------- Konfigurasi WiFi -------------------
const char* ssid = "&& || !";         // SSID (nama jaringan WiFi)
const char* password = "AND_OR_NOT";  // Password WiFi

// ------------------- Inisialisasi Modbus -------------------
ModbusIP server;  // Deklarasi objek server Modbus untuk ESP8266

// ------------------- Variabel untuk Holding Register dan Coil -------------------
uint16_t reg40001 = 0;  // Variabel untuk Holding Register 40001
uint16_t reg40002 = 0;  // Variabel untuk Holding Register 40002
bool coil00001 = false; // Variabel untuk Coil 00001 (true/false)
bool coil00002 = false; // Variabel untuk Coil 00002 (true/false)

void setup() {
  Serial.begin(9600);  // Memulai komunikasi serial dengan baud rate 9600 (ke ESP32)
  
  // Menghubungkan ESP8266 ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);  // Menunggu koneksi terhubung

  // Memulai server Modbus TCP/IP pada ESP8266
  server.server();

  // ------------------- Inisialisasi Holding Register dan Coil -------------------
  server.addHreg(0, reg40001);  // Menambahkan Holding Register 40001 dengan nilai awal
  server.addHreg(1, reg40002);  // Menambahkan Holding Register 40002 dengan nilai awal
  server.addCoil(0, coil00001); // Menambahkan Coil 00001 dengan status awal OFF (false)
  server.addCoil(1, coil00002); // Menambahkan Coil 00002 dengan status awal OFF (false)
}

void loop() {
  server.task();  // Menjalankan tugas Modbus untuk menangani komunikasi data

  // ------------------- Perbarui Nilai Register dan Coil -------------------
  reg40002 = server.Hreg(1);  // Membaca nilai Holding Register 40002 dari server Modbus
  coil00002 = server.Coil(1); // Membaca status Coil 00002 dari server Modbus

  // ------------------- Kirim Nilai Register dan Coil ke ESP32 -------------------
  static uint32_t lastSend = 0;  // Variabel untuk menyimpan waktu terakhir pengiriman data
  if (millis() - lastSend > 1000) {  // Mengecek apakah sudah 1 detik sejak pengiriman terakhir
    lastSend = millis();  // Memperbarui waktu pengiriman terakhir

    Serial.print("H:"); Serial.println(reg40002); // Mencetak nilai Holding Register 40002
    Serial.print("C:"); Serial.println(coil00002 ? 1 : 0); // Mencetak status Coil 00002
  }

  // ------------------- Menerima Data dari ESP32 dan Simpan ke Modbus -------------------
  while (Serial.available()) {  // Mengecek apakah ada data yang diterima dari ESP32
    String line = Serial.readStringUntil('\n');  // Membaca data hingga karakter '\n' ditemukan

    // Jika data mengandung "HR40001:", maka ubah nilai Holding Register 40001
    if (line.startsWith("HR40001:")) {
      reg40001 = line.substring(8).toInt();  // Mengambil angka setelah "HR40001:"
      server.Hreg(0, reg40001); // Memperbarui nilai Holding Register 40001
    }
    
    // Jika data mengandung "C00001:", maka ubah status Coil 00001
    else if (line.startsWith("C00001:")) {
      coil00001 = line.substring(7).toInt() > 0; // Mengubah data string menjadi boolean (true/false)
      server.Coil(0, coil00001); // Memperbarui status Coil 00001
    }

    // ------------------- Debug: Menampilkan Data yang Diterima -------------------
    Serial.print("From ESP32: "); 
    Serial.println(line);  // Menampilkan data yang diterima dari ESP32
  }
}