#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "configSystem.h"
#include <Preferences.h>
// Tambahkan baris di bawah ini agar file ini kenal variabel pref dari file utama
extern Preferences pref;

extern AsyncWebServer server;

extern void muatJadwalSekarang();
extern void syncJadwalSholat();
extern String getUptime();

extern String lastSyncTime;
extern bool modeCekJadwal;
extern unsigned long lastChangeTime;
extern bool SimpanOtomatis;


void setupServer() {
  server.on("/status-wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Kirim angka 1 jika sudah konek ke rumah (STA), 0 jika masih AP
    String status = (WiFi.status() == WL_CONNECTED) ? "1" : "0";
    request->send(200, "text/plain", status);
  });
  server.on("/scan-wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pakai scanNetworks(true) agar asinkron dan TIDAK bikin lag tampilan
    int n = WiFi.scanComplete();
    if (n == -2) {
        WiFi.scanNetworks(true); // Mulai scan di background
        request->send(200, "application/json", "[]");
    } else if (n == -1) {
        request->send(200, "application/json", "[]");
    } else {
        String json = "[";
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            json += "{\"s\":\"" + WiFi.SSID(i) + "\",\"r\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        request->send(200, "application/json", json);
        WiFi.scanDelete(); 
    }
});
  server.on("/set-wifi-st", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Proteksi login (Opsional, tapi bagus buat keamanan)
    if (!request->authenticate(konfig.authUser, konfig.authPass))
      return request->requestAuthentication();

    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      String s = request->getParam("ssid", true)->value();
      String p = request->getParam("pass", true)->value();

      // Simpan ke "Brankas" NVS
      pref.begin("akses", false);
      pref.putString("st_ssid", s);
      pref.putString("st_pass", p);
      pref.end();

      request->send(200, "text/plain", "OK");

      // Kasih jeda 2 detik lalu restart agar ESP32 mencoba konek ke WiFi baru
      Serial.println("WiFi Rumah diupdate, merestart...");
      delay(2000);
      String redirectUrl = "http://" + String(konfig.namaPerangkat) + ".local";
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Data Gagal Terkirim");
    }
  });

  // 1. API LIST FILE (Sudah Aman)
  server.on("/api/list", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();
    String json = "[";
    File root = LittleFS.open("/", "r");
    File file = root.openNextFile();
    while (file) {
      if (json != "[") json += ",";
      json += "{\"n\":\"" + String(file.name()) + "\",\"s\":" + String(file.size()) + "}";
      file = root.openNextFile();
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  // 2. API DELETE (Sudah Aman)
  server.on("/api/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();
    if (request->hasParam("file")) {
      LittleFS.remove("/" + request->getParam("file")->value());
      request->send(200, "text/plain", "Deleted");
    } else {
      request->send(400, "text/plain", "Missing Filename");
    }
  });

  // 3. API UPLOAD (Perbaikan: Cek Login di awal)
  server.on(
    "/api/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();
      request->send(200, "text/plain", "Uploaded");
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // Fungsi upload tidak mendukung Auth di tengah stream, Auth dicek di awal POST
      if (!index) {
        if (!filename.startsWith("/")) filename = "/" + filename;
        request->_tempFile = LittleFS.open(filename, "w");
      }
      if (len) request->_tempFile.write(data, len);
      if (final) {
        request->_tempFile.close();
        if (filename == "/config.json") muatConfig();
        if (filename == "/jadwal.json") muatJadwalSekarang();
      }
    });


  // Masukkan ke bagian server.on di setup()
  server.on("/status-system", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Ambil data FS langsung saat request datang agar akurat
    size_t totalFS = LittleFS.totalBytes();
    size_t usedFS = LittleFS.usedBytes();
    size_t sizeFlash = ESP.getFlashChipSize();

    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap() / 1024) + ",";
    json += "\"totalHeap\":" + String(ESP.getHeapSize() / 1024) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":\"" + getUptime() + "\",";

    // Suhu: Pakai 0 saja dulu Bos, biar aman dari error compile 'temprature_sens_read'
    json += "\"temp\":0,";  // JANGAN LUPA KOMA DI SINI BOS!

    json += "\"fsT\":" + String(totalFS / 1024) + ",";
    json += "\"fsU\":" + String(usedFS / 1024) + ",";
    json += "\"flash\":" + String(sizeFlash / (1024 * 1024));  // Baris terakhir baru tanpa koma
    json += "}";

    request->send(200, "application/json", json);
  });

  server.on("/status-sync", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", lastSyncTime);
  });

  server.on("/sinkron-gas-manual", HTTP_GET, [](AsyncWebServerRequest *request) {
    syncJadwalSholat();  // Panggil fungsi sakti Bos
    request->send(200, "text/plain", "Proses sinkronisasi dimulai...");
  });

  // 4. FILE STATIS & CONFIG (PENTING: Jangan kasih auth di static agar file pendukung bisa diload)
  // Tapi halaman index.html-nya kita kunci di handler bawah
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  // Handler jika halaman tidak ditemukan
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Waduh Bos, halamannya gak ada!");
  });

  server.on("/get-jadwal", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Pastikan file jadwalnya ada
    if (LittleFS.exists("/jadwal.json")) {
      request->send(LittleFS, "/jadwal.json", "application/json");
    } else {
      // Kalau file tidak ada, kirim JSON kosong jangan teks biasa!
      request->send(200, "application/json", "{\"jadwal\":{}}");
    }
  });

  server.on("/get-config", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();
    request->send(LittleFS, "/config.json", "application/json");
  });

  server.on(
    "/save-hama", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // 1. Terima data sepotong dari Web
      DynamicJsonDocument doc(512);
      deserializeJson(doc, (const char *)data);

      // 2. Masukkan ke variabel global 'konfig' milik Bos
      konfig.pengusirHama.aktif = doc["on"];
      konfig.pengusirHama.frekuensiMin = doc["fMin"].as<int>();
      konfig.pengusirHama.frekuensiMax = doc["fMax"].as<int>();
      konfig.pengusirHama.intervalAcak = doc["int"].as<int>();
      konfig.pengusirHama.aktifMalamSaja = doc["malam"];

      // 3. Panggil fungsi sakti punya Bos tadi!
      // Ini akan menulis ULANG seluruh file config.json dengan data terbaru
      if (simpanConfig()) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(500, "text/plain", "Gagal Tulis ke Flash");
      }
    });

  // 5. SIMPAN CONFIG (Versi Aman)
  server.on(
    "/save-config", HTTP_POST, [](AsyncWebServerRequest *request) {
      if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();
    },
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Gunakan mode "w" hanya saat awal (index == 0)
      File file = LittleFS.open("/config.json", index == 0 ? "w" : "a");
      if (file) {
        file.write(data, len);
        file.close();
      }

      if (index + len == total) {
        // HANYA muat ke memory setelah file SELESAI ditulis 100%
        if (muatConfig()) {
          Serial.println("Config Baru Berhasil Disimpan & Dimuat!");
          request->send(200, "text/plain", "OK");
        } else {
          request->send(500, "text/plain", "Format JSON Rusak Bos!");
        }
      }
    });
  // 6. HALAMAN UTAMA & UPLOAD (Gerbang Utama)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();
    request->send(LittleFS, "/upload.html", "text/html");
  });

  // 7. COMMAND CEPAT (PERBAIKAN KURUNG KURAWAL)
  server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();

    String act = request->arg("act");
    String valStr = request->arg("v");
    int val = valStr.toInt();

    if (act == "setBr") {
      konfig.kecerahanGlobal = val;
      setKecerahan(val);
    } else if (act == "restart") {
      request->send(200, "text/plain", "Restarting...");
      delay(500);
      ESP.restart();
      return;
    } else if (act == "addSkorA") {
      konfig.papanSkor.skorA++;
      konfig.papanSkor.aktif = true;
      buatBufferSkor(mainBuffer);
      updateTampilanSegmen(mainBuffer, konfig.jumlahDigit);
    } else if (act == "addSkorB") {
      konfig.papanSkor.skorB++;
      konfig.papanSkor.aktif = true;
      buatBufferSkor(mainBuffer);
      updateTampilanSegmen(mainBuffer, konfig.jumlahDigit);
    } else if (act == "skor") {
      konfig.papanSkor.skorA = request->arg("a").toInt();
      konfig.papanSkor.skorB = request->arg("b").toInt();
      konfig.papanSkor.babak = request->arg("bk").toInt();
      if (request->hasArg("s")) konfig.papanSkor.aktif = (request->arg("s") == "1");
      else konfig.papanSkor.aktif = true;
      buatBufferSkor(mainBuffer);
      updateTampilanSegmen(mainBuffer, konfig.jumlahDigit);
    } else if (act == "setUltra") {
      konfig.pengusirHama.aktif = (val == 1);
    } else if (act == "setModeEfek") {
      konfig.modeEfekAktif = (val > 0);
      konfig.idEfekTes = val;
    } else if (act == "cekJadwal") {
      modeCekJadwal = true;
    } else if (act == "showJam") {
      modeCekJadwal = false;
      konfig.modeEfekAktif = false;
      konfig.papanSkor.aktif = false;
    } else if (act == "animasi") {
      // Ambil nilai 'aktif' (1 atau 0) dan 'id' (0-14)
      konfig.modeAnimasi.aktif = request->arg("aktif").toInt();
      konfig.modeAnimasi.idEfek = request->arg("id").toInt();
    }

    lastChangeTime = millis();
    SimpanOtomatis = true;
    request->send(200, "text/plain", "OK");
  });

  Serial.print("Username Terdaftar: ");
  Serial.println(konfig.authUser);
  Serial.print("Password Terdaftar: ");
  Serial.println(konfig.authPass);

  server.begin();
  Serial.println("Server & OTA Siap!");
}









#endif