#include "webHandler.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "configSystem.h"
#include "config_manajer.h"
#include <AsyncJson.h>


extern AsyncWebServer server;

extern String lastSyncTime;
extern bool modeCekJadwal;
extern unsigned long lastChangeTime;
extern bool SimpanOtomatis;
extern bool scanOn;
extern uint8_t mainBuffer[];

// Deklarasi fungsi eksternal agar bisa dipanggil di sini
extern String getUptime();
extern void syncJadwalSholat();
extern void muatJadwalSekarang();
extern void setKecerahan(int val);
extern void updateTampilanSegmen(uint8_t *buf, int len);
extern void buatBufferSkor(uint8_t *buf);

static void setupPageHandlers();
static void setupWifiHandlers();
static void setupStatusHandlers();
static void setupConfigHandlers();
static void setupCommandHandlers();

static String fName = "";



void setupServer() {
  setupPageHandlers();
  setupWifiHandlers();
  setupStatusHandlers();
  setupConfigHandlers();
  setupCommandHandlers();


  server.begin();
}

//---SetUp index page Handler----

static void setupPageHandlers() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.serveStatic("/", LittleFS, "/");
    if (LittleFS.exists("/index.html")) {
      request->send(LittleFS, "/index.html", "text/html");
    } else {
      request->send(404, "text/plain", "Error: index.html not found");
    }
  });

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Cek Login
    if (!request->authenticate(konfig.authUser, konfig.authPass)) {
      return request->requestAuthentication();
    }

    if (LittleFS.exists("/upload.html")) {
      request->send(LittleFS, "/upload.html", "text/html");
    } else {
      request->send(404, "text/plain", "Error: upload.html not found");
    }
  });

  // --------------------------------------------------
  // 3. API: LIST FILES (JSON)
  // URL: /list-files
  // --------------------------------------------------
  server.on("/list-files", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();

    File root = LittleFS.open("/");
    String json = "[";
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      while (file) {
        if (json != "[") json += ",";
        json += "{\"name\":\"" + String(file.name()) + "\",\"size\":" + String(file.size()) + "}";
        file = root.openNextFile();
      }
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  // --------------------------------------------------
  // 4. API: DELETE FILE
  // URL: /delete?file=/namafile.txt
  // --------------------------------------------------
  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass)) return request->requestAuthentication();

    if (request->hasParam("file")) {
      String filename = request->getParam("file")->value();
      if (LittleFS.exists(filename)) {
        LittleFS.remove(filename);
        request->send(200, "text/plain", "File deleted");
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "Missing param: file");
    }
  });

  // --------------------------------------------------
  // 5. PROSES UPLOAD FILE (POST)
  // URL: /upload (Method: POST)
  // --------------------------------------------------
  server.on(
    "/upload", HTTP_POST,
    // (A) Bagian Respon Selesai
    [](AsyncWebServerRequest *request) {
      String pesan = "Berhasil upload: " + fName;
      request->send(200, "text/plain", pesan);
    },
    // (B) Bagian Stream Data (Proses Fisik File)
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // Cek Keamanan
      if (!request->authenticate(konfig.authUser, konfig.authPass)) return;

      // -- Awal Upload (Paket Pertama) --
      if (index == 0) {
        // Format nama file
        if (!filename.startsWith("/")) filename = "/" + filename;

        // Simpan ke variabel global
        fName = filename;

        // Hapus file lama & Buka file baru
        if (LittleFS.exists(fName)) LittleFS.remove(fName);
        request->_tempFile = LittleFS.open(fName, "w");

        Serial.printf("[Upload Start] %s\n", fName.c_str());
      }

      // -- Tulis Data --
      if (request->_tempFile) {
        request->_tempFile.write(data, len);
      }

      // -- Akhir Upload (Tutup & Reload) --
      if (final) {
        if (request->_tempFile) {
          request->_tempFile.close();
          Serial.printf("[Upload Selesai] Ukuran: %u bytes\n", index + len);

          // LOGIKA AUTO RELOAD CONFIG
          // Ambil nama dari fName ("/sholat.json" -> "sholat")
          String target = fName;
          target.replace("/", "");
          target.replace(".json", "");

          // Coba Reload
          if (reloadConfigByTarget(target)) {
            Serial.printf("[Auto-Reload] Config '%s' berhasil diperbarui di RAM!\n", target.c_str());
          }
        }
      }
    });

  // --------------------------------------------------
  // 6. NOT FOUND HANDLER (404)
  // --------------------------------------------------
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text/plain", "Halaman tidak ditemukan (404)");
    }
  });
}

//---SetUp WIFI Handler----
static void setupWifiHandlers() {

  // Endpoint Tunggal: /wifi
  server.on("/wifi", HTTP_ANY, [](AsyncWebServerRequest *request) {
    // 2. Cek Parameter "mode"
    if (!request->hasParam("mode")) {
      request->send(400, "text/plain", "Error: Missing mode parameter");
      return;
    }

    String mode = request->getParam("mode")->value();
    if (mode == "scan") {
      int n = WiFi.scanNetworks();
      String json = "[";
      for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"enc\":\"" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Secured") + "\"";
        json += "}";
      }
      json += "]";
      request->send(200, "application/json", json);
      WiFi.scanDelete();
    }
    else if (mode == "save") {
      if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {

        String newSSID = request->getParam("ssid", true)->value();
        String newPass = request->getParam("pass", true)->value();

        // --- SIMPAN KE PREFERENCES ---
        Preferences pref;
        if (pref.begin("akses", false)) {

          pref.putString("st_ssid", newSSID);
          pref.putString("st_pass", newPass);
          pref.end();  // Tutup preferences

          request->send(200, "text/plain", "WiFi Disimpan ke NVS. Restarting...");

          delay(1000);
          ESP.restart();  // Restart agar konek ke WiFi baru

        } else {
          request->send(500, "text/plain", "Gagal membuka Preferences");
        }
        // -----------------------------

      } else {
        request->send(400, "text/plain", "Data SSID/Pass kurang");
      }
    }

    else if (mode == "status") {
      String status = (WiFi.status() == WL_CONNECTED) ? "1" : "0";
      request->send(200, "text/plain", status);
    }

    // D. Lainnya
    else {
      request->send(400, "text/plain", "Mode tidak dikenal");
    }
  });
}
//---SetUp Status ESP----
static void setupStatusHandlers() {

 server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    
    float rawTemp = temperatureRead(); 
    
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":\"" + getUptime() + "\",";
    json += "\"temp\":" + String(rawTemp, 1) + ",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"fs_used\":" + String(LittleFS.usedBytes()) + ",";
    json += "\"fs_total\":" + String(LittleFS.totalBytes());    
    json += "}";

    request->send(200, "application/json", json);
  });
}

//---Save and Load----
static void setupConfigHandlers() {

  server.on("/load", HTTP_GET, [](AsyncWebServerRequest *request) {
    
    // 1. Cek parameter "set"
    if (request->hasParam("set")) {
      String tNmae = request->getParam("set")->value(); // Contoh: "ultra"
      String fsPath;

      // 2. Dapatkan path file (misal: "/ultra.json")
      if (getConfigPath(tNmae, fsPath)) {
        
        if (LittleFS.exists(fsPath)) {
          // Kirim file JSON langsung
          request->send(LittleFS, fsPath, "application/json");
        } else {
          // File belum ada, kirim object kosong
          request->send(200, "application/json", "{}");
        }
        
      } else {
        // Jika target 'set' tidak dikenal
        request->send(400, "application/json", "{\"error\":\"Config name (set) not found\"}");
      }
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing parameter: set\"}");
    }
  });
  
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/save", 
    [](AsyncWebServerRequest *request, JsonVariant &json) {
    
    // 1. Cek parameter "set"
    if (!request->hasParam("set")) {
      request->send(400, "application/json", "{\"msg\":\"Missing parameter: set\"}");
      return;
    }

    String tNmae = request->getParam("set")->value();
    String fsPath;

    // 2. Validasi target & ambil path
    if (!getConfigPath(tNmae, fsPath)) {
      request->send(400, "application/json", "{\"msg\":\"Invalid config name\"}");
      return;
    }

    // 3. Tulis JSON ke File (FIXED: Selalu Overwrite "w")
    // Hapus logika 'index == 0', langsung paksa mode "w"
    File file = LittleFS.open(fsPath, "w"); 
    
    if (file) {
      // serializeJson otomatis menulis data baru ke file kosong (karena mode w)
      serializeJson(json, file);
      file.close();
      
      // 4. RELOAD RAM
      if (reloadConfigByTarget(tNmae)) {
         Serial.printf("[Config] '%s' Saved & Reloaded.\n", tNmae.c_str());
         request->send(200, "application/json", "{\"status\":\"ok\",\"msg\":\"Saved & Reloaded\"}");
      } else {
         Serial.printf("[Config] '%s' Saved but Reload FAILED.\n", tNmae.c_str());
         request->send(200, "application/json", "{\"status\":\"warning\",\"msg\":\"Saved but Reload Failed\"}");
      }

    } else {
      Serial.println("[Config] Write Error");
      request->send(500, "application/json", "{\"status\":\"error\",\"msg\":\"Filesystem Write Error\"}");
    }
  });

  server.addHandler(handler);
}
static void setupCommandHandlers() {

  server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request) {
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
}



/*
void setupServer() {
  server.on("/status-wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain",
                  (WiFi.status() == WL_CONNECTED) ? "1" : "0");
  });

  server.on("/scan-wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == -2 && !scanOn) {
      scanOn = true;
      WiFi.scanNetworks(true);
      request->send(200, "application/json", "[]");
    } else if (n == -1) {
      request->send(200, "application/json", "[]");
    } else {
      char json[512];
      size_t pos = 0;

      pos += snprintf(json + pos, sizeof(json) - pos, "[");

      for (int i = 0; i < n; i++) {
        if (i) pos += snprintf(json + pos, sizeof(json) - pos, ",");

        pos += snprintf(
          json + pos, sizeof(json) - pos,
          "{\"s\":\"%s\",\"r\":%d}",
          WiFi.SSID(i).c_str(),
          WiFi.RSSI(i));

        if (pos >= sizeof(json) - 1) break;  // proteksi overflow
      }

      pos += snprintf(json + pos, sizeof(json) - pos, "]");

      WiFi.scanDelete();
      scanOn = false;

      request->send(200, "application/json", json);
    }
  });


  server.on("/set-wifi-st", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(konfig.authUser, konfig.authPass))
      return request->requestAuthentication();

    if (!request->hasParam("ssid", true) || !request->hasParam("pass", true)) {
      request->send(400, "text/plain", "ERR");
      return;
    }

    pref.begin("akses", false);
    pref.putString("st_ssid", request->getParam("ssid", true)->value());
    pref.putString("st_pass", request->getParam("pass", true)->value());
    pref.end();

    request->send(200, "text/plain", "OK");

    esp_timer_handle_t t;
    esp_timer_create_args_t cfg = {
      .callback = [](void *) {
        ESP.restart();
      },
      .arg = nullptr,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "rst"
    };
    esp_timer_create(&cfg, &t);
    esp_timer_start_once(t, 2000000);
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
    "/save-hama", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      static char jsonBuf[512];
      static size_t received = 0;

      if (index == 0) {
        received = 0;
        memset(jsonBuf, 0, sizeof(jsonBuf));
      }

      if (received + len >= sizeof(jsonBuf)) {
        request->send(413, "text/plain", "Payload terlalu besar");
        return;
      }

      memcpy(jsonBuf + received, data, len);
      received += len;

      if (received < total) return;

      StaticJsonDocument<512> doc;
      DeserializationError err = deserializeJson(doc, jsonBuf);
      if (err) {
        request->send(400, "text/plain", "JSON tidak valid");
        return;
      }

      konfig.pengusirHama.aktif = doc["on"] | false;
      konfig.pengusirHama.frekuensiMin = doc["fMin"] | 0;
      konfig.pengusirHama.frekuensiMax = doc["fMax"] | 0;
      konfig.pengusirHama.intervalAcak = doc["int"] | 0;
      konfig.pengusirHama.aktifMalamSaja = doc["malam"] | false;

      if (saveHama()) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      } else {
        request->send(500, "text/plain", "Gagal Tulis ke Flash");
      }
    });


  // 5. SIMPAN CONFIG (Versi Aman)
  server.on(
  "/save-config", HTTP_POST,
  [](AsyncWebServerRequest *request) {},
  NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {

    if (!request->hasParam("target")) {
      request->send(400, "text/plain", "target missing");
      return;
    }

    String target = request->getParam("target")->value();
    String path;

    if (!getConfigPath(target, path)) {
      request->send(400, "text/plain", "invalid target");
      return;
    }

    File file = LittleFS.open(path, index == 0 ? "w" : "a");
    if (!file) {
      request->send(500, "text/plain", "FS error");
      return;
    }

    file.write(data, len);
    file.close();

    if (index + len == total) {
      if (reloadConfigByTarget(target)) {
        request->send(200, "text/plain", "OK");
      } else {
        request->send(500, "text/plain", "JSON invalid");
      }
    }
  }
);



  // 7. COMMAND CEPAT (PERBAIKAN KURUNG KURAWAL)
  server.on("/cmd", HTTP_GET, [](AsyncWebServerRequest *request) {
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

  server.begin();
  Serial.println("Server & OTA Siap!");
}


*/