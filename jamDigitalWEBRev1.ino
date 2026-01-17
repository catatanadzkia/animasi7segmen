#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <NetworkClientSecure.h>  // Penting untuk HTTPS/GAS
#include <ArduinoOTA.h>
#include "time.h"
#include "configSystem.h"
#include "config_manajer.h"
#include "displayLogic.h"
#include "systemLogic.h"
#include "webHandler.h"




// --- PINOUT ---
const int LATCH_7Seg = 18;
const int CLOCK_7Seg = 22;
const int DATA_7Seg = 21;
const int OE_7Seg = 19;

// --- TAMBAHKAN DI BAGIAN ATAS (PINOUT BARU) ---
const int PIN_DATA_ULTRA = 14;
const int PIN_LATCH_ULTRA = 12;
const int PIN_CLOCK_ULTRA = 13;

unsigned long lastUltraPulse = 0;
bool ultraStatus = false;
unsigned long lastChangeTime = 0;
bool SimpanOtomatis = false;
bool modeCekJadwal = false;        // <--- Tambahkan ini Bos
unsigned long timerCekJadwal = 0;  // Untuk hitung mundur otomatis mati
int detikSekarang;                 // Deklarasikan agar bisa dipakai di loop dan cekDanTampilkan
String lastSyncTime = "Belum Sinkron";
unsigned long lastCekWiFi = 0;
Preferences pref;
bool mdnsOn = false;
bool scanOn = false;


TaskHandle_t TaskUltra;


AsyncWebServer server(80);


// Fungsi pembantu Uptime
String getUptime() {
  unsigned long sec = millis() / 1000;
  int jam = sec / 3600;
  int sisa = sec % 3600;
  int menit = sisa / 60;
  int detik = sisa % 60;
  return String(jam) + "j " + String(menit) + "m " + String(detik) + "d";
}


// === MEMUAT TAMPILAN SEGMEN===
void updateTampilanSegmen(uint8_t *dataArray, int jumlahIC) {
  // Proteksi: Jangan kirim lebih dari batas fisik buffer kita
  if (jumlahIC > MAX_DIGIT) jumlahIC = MAX_DIGIT;
  if (jumlahIC <= 0) return;

  digitalWrite(LATCH_7Seg, LOW);

  // Kirim data secara berurutan
  for (int i = jumlahIC - 1; i >= 0; i--) {
    shiftOut(DATA_7Seg, CLOCK_7Seg, MSBFIRST, dataArray[i]);
  }

  digitalWrite(LATCH_7Seg, HIGH);
}

// ===MEMUAT SUARA ULTRASONIC===
void UltraTaskCode(void *pvParameters) {
  for (;;) {
    // 1. Baca status aktif dari global konfig (hasil sinkron JSON)
    if (konfig.pengusirHama.aktif) {

      // 2. Tambahan: Logika Malam (Jika diperlukan)
      // Kita asumsikan 'timeinfo.tm_hour' diupdate oleh Core 1 secara berkala
      int jamSekarang = timeinfo.tm_hour;
      bool isMalam = (jamSekarang >= 18 || jamSekarang < 4);
      bool bolehNyala = true;

      if (konfig.pengusirHama.aktifMalamSaja && !isMalam) {
        bolehNyala = false;
      }

      if (bolehNyala) {
        static unsigned long lastChange = 0;
        static int currentFreq = 22000;
        static unsigned long lastPulse = 0;
        static bool status = false;

        // Logika Ganti Frekuensi sesuai IntervalAcak (ms) dari JSON
        if (millis() - lastChange >= (unsigned long)konfig.pengusirHama.intervalAcak) {
          lastChange = millis();
          currentFreq = random(konfig.pengusirHama.frekuensiMin, konfig.pengusirHama.frekuensiMax + 1);
          if (currentFreq < 100) currentFreq = 22000;
        }

        // Generator Pulsa
        unsigned long halfPeriod = 500000UL / currentFreq;
        if (micros() - lastPulse >= halfPeriod) {
          lastPulse = micros();
          status = !status;

          digitalWrite(PIN_LATCH_ULTRA, LOW);
          shiftOut(PIN_DATA_ULTRA, PIN_CLOCK_ULTRA, MSBFIRST, status ? 0x01 : 0x00);
          digitalWrite(PIN_LATCH_ULTRA, HIGH);
        }
      } else {
        // Jika mode malam aktif tapi sekarang siang, pastikan pin mati
        digitalWrite(PIN_LATCH_ULTRA, LOW);
        shiftOut(PIN_DATA_ULTRA, PIN_CLOCK_ULTRA, MSBFIRST, 0x00);
        digitalWrite(PIN_LATCH_ULTRA, HIGH);
      }
    } else {
      // Jika status aktif di JSON bernilai FALSE, matikan total
      digitalWrite(PIN_LATCH_ULTRA, LOW);
      shiftOut(PIN_DATA_ULTRA, PIN_CLOCK_ULTRA, MSBFIRST, 0x00);
      digitalWrite(PIN_LATCH_ULTRA, HIGH);
    }

    // PENTING: Jangan hapus delay ini agar Core 0 tidak hang (WDT)
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
void setKecerahan(int nilai) {
  // Misal Pin OE (Output Enable) ada di pin 5
  // Kita petakan nilai 0-255 dari Web
  ledcWrite(OE_7Seg, 255 - nilai);
}
void inisialisasiHardware();
void setupWiFi();
void setupServer();
void cekDanTampilkan();
void WiFiEvent(arduino_event_id_t event);

void setup() {
  Serial.begin(115200);

  // 1. Mulai LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("Gagal Mount LittleFS!");
    return;
  }
  
  // 2. MUAT CONFIG DULU (PENTING!)
  // --- SYSTEM ----
  loadSystemConfig();
  // --- HAMA ---
  loadHama();
  // --- SHOLAT ---
  loadSholatConfig();
  // --- TANGGAL ---
  loadTanggalConfig();
  // --- EFEK ----
  loadEfekConfig();

  // 3. Inisialisasi Hardware & WiFi
  inisialisasiHardware();
  WiFi.onEvent(WiFiEvent);
  setupWiFi();
  

  // 4. Sinkronisasi Waktu NTP
  configTime(konfig.timezone * 3600, 0, "id.pool.ntp.org", "time.google.com", "pool.ntp.org");

  // 5. CEK SYNC PERDANA (Setelah Config & WiFi siap)

  delay(1000);
  if (!LittleFS.exists("/jadwal.json") || strlen(konfig.terakhirSync) < 5) {
    Serial.println("- Data kosong/awal, mencoba sinkronisasi perdana...");
    syncJadwalSholat();
  }

  // 6. Muat Jadwal ke RAM
  muatJadwalSekarang();

  ArduinoOTA.setHostname(konfig.namaPerangkat);
  ArduinoOTA.begin();

  xTaskCreatePinnedToCore(UltraTaskCode, "TaskUltra", 10000, NULL, 1, &TaskUltra, 0);
  setupServer();

  Serial.println("Sistem Siap!");
}


//=== Handle Goodle Appscript===
void syncJadwalSholat() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("- Sync Gagal: WiFi tidak terkoneksi.");
    return;
  }

  if (strlen(konfig.urlGAS) < 10) {
    Serial.println("- Sync Gagal: URL GAS belum diatur.");
    return;
  }

  Serial.println("- Memulai Sinkronisasi Jadwal dari GAS...");

  NetworkClientSecure *client = new NetworkClientSecure;
  if (client) {
    client->setInsecure();  // Mengabaikan validasi SSL Certificate Google

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // WAJIB untuk GAS

    if (http.begin(*client, konfig.urlGAS)) {
      int httpCode = http.GET();

      if (httpCode == HTTP_CODE_OK) {
        // Buka file jadwal.json untuk ditulis
        File file = LittleFS.open("/jadwal.json", "w");
        if (file) {
          // Streaming data langsung dari HTTP ke File (Hemat RAM)
          http.writeToStream(&file);
          file.close();
          if (getLocalTime(&timeinfo)) {
            char buf[32];
            strftime(buf, sizeof(buf), "%d/%m %H:%M", &timeinfo);
            lastSyncTime = String(buf);
          }

          Serial.println("- Sinkronisasi Berhasil! Jadwal disimpan.");
        }
      } else {
        Serial.printf("- HTTP Error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
    delete client;
  }
}


int jadwalMenit[6];

void muatJadwalSekarang() {
  if (!LittleFS.exists("/jadwal.json")) {
    Serial.println("File jadwal.json tidak ada!");
    return;
  }

  File file = LittleFS.open("/jadwal.json", "r");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print("Gagal parsing jadwal: ");
    Serial.println(error.c_str());
    file.close();
    return;
  }

  // Masuk ke objek "jadwal" sesuai struktur JSON Bos
  JsonObject j = doc["jadwal"];

  auto konversi = [](const char *t) {
    if (!t) return -1;
    int h, m;
    if (sscanf(t, "%d:%d", &h, &m) == 2) {
      return (h * 60) + m;
    }
    return -1;
  };

  // Sesuaikan Key dengan JSON: Imsak, Subuh, Luhur, Ashar, Magrib, Isya
  jadwalMenit[0] = konversi(j["Imsak"] | "00:00");
  jadwalMenit[1] = konversi(j["Subuh"] | "00:00");
  jadwalMenit[2] = konversi(j["Luhur"] | "00:00");  // Pakai "Luhur" sesuai JSON Bos
  jadwalMenit[3] = konversi(j["Ashar"] | "00:00");
  jadwalMenit[4] = konversi(j["Magrib"] | "00:00");  // Pakai "Magrib" (kurang huruf 'h')
  jadwalMenit[5] = konversi(j["Isya"] | "00:00");

  file.close();

  // Debug hasil konversi ke Serial
  Serial.printf("Jadwal Dimuat (menit): I:%d, S:%d, L:%d, A:%d, M:%d, I:%d\n",
                jadwalMenit[0], jadwalMenit[1], jadwalMenit[2],
                jadwalMenit[3], jadwalMenit[4], jadwalMenit[5]);
}

void loop() {
  ArduinoOTA.handle();



  // --- TUGAS LAMBAT (Setiap 1 Detik) ---
  if (millis() - lastUpdateJam >= 1000) {
    lastUpdateJam = millis();
    if (getLocalTime(&timeinfo)) {
      detikSekarang = timeinfo.tm_sec;  // Update di sini sudah cukup
      if (timeinfo.tm_hour == 1 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) {
        syncJadwalSholat();
      }
    }

    if (SimpanOtomatis && (millis() - lastChangeTime > 10000)) {
      saveSystemConfig();
      saveHama();
      SimpanOtomatis = false;
    }
  }

  // --- TUGAS REFRESH (Visual - Diatur Kecepatannya) ---
  static unsigned long lastRefresh = 0;
  if (millis() - lastRefresh >= 33) {  // 33ms = Sekitar 30 FPS (Standar mata manusia)
    lastRefresh = millis();

    // 1. Olah data (Hanya isi buffer, jangan kirim hardware di dalam sini)
    cekDanTampilkan();

    // 2. Kirim ke hardware (Satu-satunya pintu keluar data)
    updateTampilanSegmen(mainBuffer, konfig.jumlahDigit);
  }
}

// --- DETAIL FUNGSI ---

void inisialisasiHardware() {
  // Output untuk 7-Segment (Daisy Chain 595)
  pinMode(LATCH_7Seg, OUTPUT);
  pinMode(CLOCK_7Seg, OUTPUT);
  pinMode(DATA_7Seg, OUTPUT);
  pinMode(OE_7Seg, OUTPUT);
  // Inisialisasi PWM Kecerahan
  ledcAttachChannel(OE_7Seg, 5000, 8, 0);
  // Langsung terapkan kecerahan dari memori (invert karena Active Low)
  ledcWrite(OE_7Seg, 255 - konfig.kecerahanGlobal);

  // Output untuk Ultrasonic (595 Terpisah)
  pinMode(PIN_LATCH_ULTRA, OUTPUT);
  pinMode(PIN_CLOCK_ULTRA, OUTPUT);
  pinMode(PIN_DATA_ULTRA, OUTPUT);

  Serial.println("- Hardware initialized.");
}


// ---- SETUP WIFI (DIPANGGIL SEKALI) ----
void setupWiFi() {

  pref.begin("akses", true);
  String ssid = pref.getString("st_ssid", "");
  String pass = pref.getString("st_pass", "");
  pref.end();

  String apName = konfig.namaPerangkat;
  if (apName == "") apName = "JamBos";

  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(apName.c_str());

  WiFi.setAutoReconnect(true);  // biarkan stack
  WiFi.persistent(false);

  if (ssid != "") {
    WiFi.begin(ssid.c_str(), pass.c_str());
  }
}
void WiFiEvent(arduino_event_id_t event) {

  switch (event) {

    // === STA DAPAT IP ===
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:

      if (!mdnsOn) {
        if (MDNS.begin(konfig.namaPerangkat)) {
          MDNS.addService("http", "tcp", 80);
          mdnsOn = true;
        }
      }

      // AP OFF kalau nyala
      WiFi.softAPdisconnect(true);
      break;

    // === STA PUTUS ===
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:

      if (mdnsOn) {
        MDNS.end();  // ⬅️ INI WAJIB
        mdnsOn = false;
      }

      // AP ON untuk akses darurat
      WiFi.softAP(konfig.namaPerangkat, "12345678");
      break;

    default:
      break;
  }
}

void cekDanTampilkan() {
  setKecerahan(konfig.kecerahanGlobal);

  // 1. PRIORITAS UTAMA: MODE CEK JADWAL
  if (modeCekJadwal) {
    int durasiPerSholat = konfig.tanggal.lama * 1000;
    if (durasiPerSholat < 1000) durasiPerSholat = 2000;
    int indexJadwal = (millis() / durasiPerSholat) % 6;
    int h = jadwalMenit[indexJadwal] / 60;
    int m = jadwalMenit[indexJadwal] % 60;
    buatBufferWaktu(h, m, 0, mainBuffer, true);
    if (indexJadwal < 4) mainBuffer[indexJadwal] |= 0x80;
    return;  // Kunci, jangan lanjut ke bawah
  }

  // 2. PRIORITAS KEDUA: PAPAN SKOR
  if (konfig.papanSkor.aktif) {
    buatBufferSkor(mainBuffer);
    return;  // Kunci, jangan lanjut ke bawah
  }
  if (konfig.modeAnimasi.aktif) {
    prosesAnimasiEfek(konfig.modeAnimasi.idEfek);
    return;  // Kunci, jangan lanjut ke bawah
  }
  // 3. PRIORITAS KETIGA: NOTIF SHOLAT (IMSAL - ISYA)
  konfig.modeEfekAktif = false;  // Reset flag
  for (int i = 0; i < 6; i++) {
    notifSholat(
      jadwalMenit[i],
      konfig.sholat[i].menitPeringatan,
      konfig.sholat[i].idEfekPeringatan,
      konfig.sholat[i].hitungMundur,
      konfig.sholat[i].durasiAdzan,
      konfig.sholat[i].idEfekAdzan,
      konfig.sholat[i].kecerahanHemat,
      konfig.sholat[i].menitHemat);
    if (konfig.modeEfekAktif) return;  // Jika sholat sedang aktif, kunci dan keluar
  }

  // 4. PRIORITAS KEEMPAT: TAMPIL TANGGAL
  if (konfig.tanggal.aktif) {
    notifTanggal(
      konfig.tanggal.setiap,
      konfig.tanggal.lama,
      konfig.tanggal.adaEvent,
      konfig.tanggal.modeDP,
      konfig.tanggal.alertDP);
    if (konfig.modeEfekAktif) return;  // Jika tanggal sedang tampil, kunci dan keluar
  }

  // 5. DEFAULT: JAM NORMAL (Hanya tampil jika tidak ada prioritas di atas)
  bool statusTitik = true;
  if (konfig.titikDuaKedip) {
    statusTitik = (detikSekarang % 2 == 0);
  }
  buatBufferWaktu(timeinfo.tm_hour, timeinfo.tm_min, detikSekarang, mainBuffer, statusTitik);
}