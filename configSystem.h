#ifndef CONFIG_SYSTEM_H
#define CONFIG_SYSTEM_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
// ==========================================
// --- STRUKTUR DATA (KONFIGURASI) ---
// ==========================================

// 1. Laci Daftar Efek (Animasi)
struct EfekConfig {
  int id;                 // 1 - 99
  int mode;               // Jenis: Kedip, Putar, Jalan, dll
  int kecepatan;          // Dalam milidetik (ms)
  int kecerahan;          // 0 - 255
  uint8_t polaSegmen[8];  // Bitmask untuk data 595 (a-g + DP)
  int jumlahFrame;
  bool pakaiDP;  // Apakah titik (Decimal Point) aktif
};

// 2. Laci Pengaturan Notifikasi Sholat
struct NotifSholatConfig {
  int menitPeringatan;   // Jeda sebelum adzan (Fase Warning)
  int idEfekPeringatan;  // Merujuk ke id di EfekConfig
  int hitungMundur;      // Detik sebelum adzan (Fase Countdown)
  int idEfekAdzan;       // Merujuk ke id di EfekConfig
  int durasiAdzan;       // Berapa lama efek adzan tampil (detik)
  int menitHemat;        // Jeda setelah adzan untuk masuk fase redup
  int kecerahanHemat;    // Level redup (0-255)
};

struct NotifTanggalConfig {
  bool aktif;     // Key: "a"
  int setiap;     // Key: "ls"
  int lama;       // Key: "ll"
  int format;     // Key: "ft"
  int idEfek;     // Key: "id"
  bool adaEvent;  // Key: "ev" (Flag dari GAS)
  int alertDP;
  int modeDP;  // Key: "dp" (Opsi kedip DP)
};

// 5. Laci Pengaturan Pengusir Hama (Ultrasonic Output via 595)
struct NotifUltraConfig {
  bool aktif;
  int frekuensiMin;     // Batas bawah frekuensi (misal 20000 Hz)
  int frekuensiMax;     // Batas atas frekuensi (misal 40000 Hz)
  int intervalAcak;     // Seberapa cepat frekuensi berubah (ms)
  bool aktifMalamSaja;  // Opsi hanya aktif saat gelap/malam
};

// 6. Laci Pengaturan Papan Skor (Sederhana 4 Digit)
struct NotifSkorConfig {
  bool aktif;
  int skorA;           // 0-99 (Tampil di Digit 1 & 2)
  int skorB;           // 0-99 (Tampil di Digit 3 & 4)
  int babak;           // 1 atau 2 (Ditampilkan via DP/Titik)
  bool tampilkanSkor;  // Switch antara JAM atau SKOR
};
struct WifiConfig {
  String ssid;      // SSID Router tujuan
  String pass;      // Password Router tujuan      // Nama SSID yang dipancarkan ESP32 (Host)
  String authUser;  // Username Web Login
  String authPass;  // Password Web Login & Password WiFi AP
};

// --- INDUK SEGALA PENGATURAN (SISTEM CONFIG) ---
struct SistemConfig {
  WifiConfig wikon;
  NotifSholatConfig sholat[6];  // 0:Subuh, 1:Dzuhur, 2:Ashar, 3:Maghrib, 4:Isya       // 10 slot agenda tambahan
  NotifTanggalConfig tanggal;   // 5 slot hari spesial
  EfekConfig daftarEfek[15];
  struct {
    bool aktif;
    int idEfek;
  } modeAnimasi;                  // Wadah untuk 60 jenis animasi
  NotifUltraConfig pengusirHama;  // Ultrasonic pengusir Hama
  NotifSkorConfig papanSkor;

  // --- TAMBAHAN UNTUK VISUAL & LOOPING (Sinkron Web) ---
  int modeKedip;       // id="modeKedip"
  int speedKedip;      // id="speedKedip"
  int durasiKedip;     // id="durasiKedip"
  bool titikDuaKedip;  // id="titikDuaKedip"

  int loopSetiap;  // id="loop_setiap"
  int loopLama;    // id="loop_lama"
  int fmtTgl;      // id="fmt_tgl"
  int timezone;    // id="tz"
  // -----------------------------------------------------

  int jumlahDigit;
  int kecerahanGlobal;  // Kecerahan jam saat kondisi normal
  char namaPerangkat[32];
  char urlGAS[150];  // Nama yang muncul di WiFi/mDNS
  char terakhirSync[20];
  char authUser[16];
  char authPass[16];

  //Hanya Lewat
  bool modeEfekAktif = false;
  int idEfekTes = 0;
};

// Variabel Utama untuk digunakan di seluruh kode
SistemConfig konfig;

void standarConfig() {
  // 1. Inisialisasi Nilai Global

  konfig.kecerahanGlobal = 200;
  strlcpy(konfig.namaPerangkat, "jam", sizeof(konfig.namaPerangkat));
  strlcpy(konfig.authUser, "admin", sizeof(konfig.authUser));
  strlcpy(konfig.authPass, "1234", sizeof(konfig.authPass));
  konfig.modeKedip = 0;
  konfig.speedKedip = 500;
  konfig.durasiKedip = 10;
  konfig.titikDuaKedip = true;
  konfig.loopSetiap = 60;
  konfig.loopLama = 3;
  konfig.fmtTgl = 0;
  konfig.timezone = 7;

  konfig.tanggal.aktif = true;
  konfig.tanggal.setiap = 60;  // Muncul tiap 1 menit
  konfig.tanggal.lama = 5;     // Muncul selama 5 detik
  konfig.tanggal.format = 0;   // Format DD:MM
  konfig.tanggal.idEfek = 2;   // Pakai efek putar segmen (ID 2)
  konfig.tanggal.adaEvent = false;
  konfig.tanggal.alertDP = 0;
  konfig.tanggal.modeDP = 0;

  konfig.daftarEfek[1].id = 2;
  konfig.daftarEfek[1].mode = 2;
  konfig.daftarEfek[1].kecepatan = 100;
  konfig.daftarEfek[1].kecerahan = 200;
  konfig.daftarEfek[1].polaSegmen[0] = 0x3F;  // Isi frame pertama saja untuk standar
  konfig.daftarEfek[1].jumlahFrame = 1;
  konfig.daftarEfek[1].pakaiDP = false;



  // 3. Isi Standar Notifikasi Sholat (Imsak sampai Isya)
  // Urutan: 0:Imsak, 1:Subuh, 2:Dzuhur, 3:Ashar, 4:Maghrib, 5:Isya
  for (int i = 0; i < 6; i++) {
    konfig.sholat[i].menitPeringatan = 5;   // 5 menit sebelum adzan mulai warning
    konfig.sholat[i].idEfekPeringatan = 3;  // Pakai efek kedip DP saja
    konfig.sholat[i].hitungMundur = 10;     // 10 detik terakhir muncul countdown
    konfig.sholat[i].idEfekAdzan = 1;       // Saat adzan kedip semua (ID 1)
    konfig.sholat[i].durasiAdzan = 120;     // Efek tampil selama 2 menit
    konfig.sholat[i].menitHemat = 10;       // 10 menit setelah adzan masuk mode redup
    konfig.sholat[i].kecerahanHemat = 40;   // Sangat redup agar awet
  }

  // Hapus loop for(int i=0; i<5; i++), ganti jadi:
  konfig.tanggal.aktif = false;

  konfig.wikon.ssid = "jam";
  konfig.wikon.pass = "admin123";

  // 5. Standar Konfigurasi Pengusir Hama Acak
  konfig.pengusirHama.aktif = true;
  konfig.pengusirHama.frekuensiMin = 22000;  // 22 kHz
  konfig.pengusirHama.frekuensiMax = 50000;  // 50 kHz
  konfig.pengusirHama.intervalAcak = 1000;   // Berubah frekuensi setiap 1 detik
  konfig.pengusirHama.aktifMalamSaja = false;

  // 6. Standar Papan Skor Sederhana
  konfig.papanSkor.skorA = 0;
  konfig.papanSkor.skorB = 0;
  konfig.papanSkor.babak = 1;
  konfig.papanSkor.tampilkanSkor = false;

  // 7. Bersihkan sisa daftarEfek (indeks 3 sampai 59) agar ID-nya 0
  for (int i = 3; i < 15; i++) {
    konfig.daftarEfek[i].id = 0;
  }
  //8. Jumalah digit default
  konfig.jumlahDigit = 4;
  strlcpy(konfig.urlGAS, "", sizeof(konfig.urlGAS));
  strlcpy(konfig.terakhirSync, "Belum Sync", sizeof(konfig.terakhirSync));  //ini mengartikan bahwa tidak ada sinkronisasi
}

// ==========================================
// --- MUAT KONFIGURASI JSON ---
// ==========================================

bool muatConfig() {
  File file = LittleFS.open("/config.json", "r");

  if (!file || file.isDirectory()) {
    Serial.println("- File /config.json tidak ditemukan. Memuat standarConfig...");
    standarConfig();
    return false;
  }

  // Alokasi RAM untuk dokumen JSON (20KB)
  DynamicJsonDocument doc(10240);

  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("- Gagal parsing JSON: ");
    Serial.println(error.c_str());
    standarConfig();
    return false;
  }

  // --- 1. MAPPING GLOBAL & ULTRA ---
  konfig.jumlahDigit = doc["global"]["dg"] | 4;
  konfig.kecerahanGlobal = doc["global"]["br"] | 200;
  strlcpy(konfig.namaPerangkat, doc["global"]["dev"] | "Jam_Jadwal_Sholat", sizeof(konfig.namaPerangkat));
  strlcpy(konfig.urlGAS, doc["global"]["gas"] | "", sizeof(konfig.urlGAS));
  strlcpy(konfig.terakhirSync, doc["global"]["sncy"] | "Belum Sync", sizeof(konfig.terakhirSync));
  strlcpy(konfig.authUser, doc["global"]["user"] | "admin", sizeof(konfig.authUser));
  strlcpy(konfig.authPass, doc["global"]["pass"] | "1234", sizeof(konfig.authPass));
  konfig.modeKedip = doc["global"]["mk"] | 0;
  konfig.speedKedip = doc["global"]["sk"] | 500;
  konfig.durasiKedip = doc["global"]["dk"] | 10;
  konfig.titikDuaKedip = doc["global"]["t2k"] | true;

  konfig.loopSetiap = doc["global"]["ls"] | 60;
  konfig.loopLama = doc["global"]["ll"] | 3;
  konfig.fmtTgl = doc["global"]["ft"] | 0;
  konfig.timezone = doc["global"]["tz"] | 7;

  konfig.wikon.ssid = doc["wifi"]["idwifi"] | "Jam_dinding";
  konfig.wikon.pass = doc["wifi"]["pas"] | "admin123";

  konfig.pengusirHama.aktif = doc["ultra"]["on"] | true;
  konfig.pengusirHama.frekuensiMin = doc["ultra"]["fMin"] | 22000;
  konfig.pengusirHama.frekuensiMax = doc["ultra"]["fMax"] | 50000;
  konfig.pengusirHama.intervalAcak = doc["ultra"]["int"] | 1000;
  konfig.pengusirHama.aktifMalamSaja = doc["ultra"]["malam"] | false;

  // --- 2. MAPPING PAPAN SKOR (4 Digit) ---
  konfig.papanSkor.skorA = doc["skor"]["a"] | 0;
  konfig.papanSkor.skorB = doc["skor"]["b"] | 0;
  konfig.papanSkor.babak = doc["skor"]["bk"] | 1;
  konfig.papanSkor.tampilkanSkor = doc["skor"]["s"] | false;

  // --- 3. MAPPING SHOLAT (6 Slot: Imsak - Isya) ---
  JsonArray sholatArr = doc["sholat"];
  for (int i = 0; i < 6; i++) {
    konfig.sholat[i].menitPeringatan = sholatArr[i]["mP"] | 5;
    konfig.sholat[i].idEfekPeringatan = sholatArr[i]["iEP"] | 3;
    konfig.sholat[i].hitungMundur = sholatArr[i]["cDown"] | 10;
    konfig.sholat[i].idEfekAdzan = sholatArr[i]["iEA"] | 1;
    konfig.sholat[i].durasiAdzan = sholatArr[i]["durA"] | 120;
    konfig.sholat[i].menitHemat = sholatArr[i]["mH"] | 10;
    konfig.sholat[i].kecerahanHemat = sholatArr[i]["brH"] | 40;
  }

  // --- MAPPING TANGGAL MODERN ---
  JsonObject tglObj = doc["tanggal"];
  konfig.tanggal.aktif = tglObj["a"] | false;
  konfig.tanggal.setiap = tglObj["ls"] | 60;
  konfig.tanggal.lama = tglObj["ll"] | 5;
  konfig.tanggal.format = tglObj["ft"] | 0;
  konfig.tanggal.idEfek = tglObj["id"] | 0;
  konfig.tanggal.adaEvent = tglObj["ev"] | false;
  konfig.tanggal.alertDP = tglObj["dp"] | 0;
  konfig.tanggal.modeDP = tglObj["md"] | 0;


  // --- 6. MAPPING EFEK (Maksimal 15) ---
  // Bersihkan RAM lama dulu
  for (int i = 0; i < 15; i++) konfig.daftarEfek[i].id = 0;

  // Di dalam fungsi muatConfig() atau saat terima JSON
  JsonArray efekArr = doc["efek"];
  int i = 0;
  for (JsonObject e : efekArr) {
    if (i >= 15) break;  // Batasi maksimal 15 efek

    konfig.daftarEfek[i].id = e["id"];
    konfig.daftarEfek[i].mode = e["m"];
    konfig.daftarEfek[i].kecepatan = e["s"];
    konfig.daftarEfek[i].kecerahan = e["b"];
    konfig.daftarEfek[i].pakaiDP = e["dp"];

    // Parsing Array Pola Segmen dari JS
    JsonArray pArr = e["p"];
    int f = 0;
    for (uint8_t pVal : pArr) {
      if (f >= 8) break;  // Sesuai ukuran struct [8]
      konfig.daftarEfek[i].polaSegmen[f] = pVal;
      f++;
    }
    konfig.daftarEfek[i].jumlahFrame = f;  // Simpan berapa frame yang aktif
    i++;
  }



  Serial.println("- Konfigurasi berhasil dimuat.");
  return true;
}

// ==========================================
// --- SIMPAN KONFIGURASI KE JSON ---
// ==========================================

bool simpanConfig() {
  // 1. Siapkan Dokumen JSON
  DynamicJsonDocument doc(20480);

  // 2. Bungkus Data Global & Ultra
  JsonObject global = doc.createNestedObject("global");
  global["user"] = konfig.authUser;  // Tambahkan ini
  global["pass"] = konfig.authPass;  // Tambahkan
  global["mk"] = konfig.modeKedip;
  global["sk"] = konfig.speedKedip;
  global["dk"] = konfig.durasiKedip;
  global["t2k"] = konfig.titikDuaKedip;

  global["ls"] = konfig.loopSetiap;
  global["ll"] = konfig.loopLama;
  global["ft"] = konfig.fmtTgl;
  global["tz"] = konfig.timezone;
  global["dg"] = konfig.jumlahDigit;
  global["br"] = konfig.kecerahanGlobal;
  global["dev"] = konfig.namaPerangkat;
  global["gas"] = konfig.urlGAS;
  global["sncy"] = konfig.terakhirSync;


  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["idwifi"] = konfig.wikon.ssid;
  wifi["pas"] = konfig.wikon.pass;

  JsonObject ultra = doc.createNestedObject("ultra");
  ultra["on"] = konfig.pengusirHama.aktif;
  ultra["fMin"] = konfig.pengusirHama.frekuensiMin;
  ultra["fMax"] = konfig.pengusirHama.frekuensiMax;
  ultra["int"] = konfig.pengusirHama.intervalAcak;
  ultra["malam"] = konfig.pengusirHama.aktifMalamSaja;

  // 3. Bungkus Data Papan Skor
  JsonObject skor = doc.createNestedObject("skor");
  skor["a"] = konfig.papanSkor.skorA;
  skor["b"] = konfig.papanSkor.skorB;
  skor["bk"] = konfig.papanSkor.babak;
  skor["s"] = konfig.papanSkor.tampilkanSkor;

  // 4. Bungkus Data Sholat (Array)
  JsonArray sholatArr = doc.createNestedArray("sholat");
  for (int i = 0; i < 6; i++) {
    JsonObject s = sholatArr.createNestedObject();
    s["mP"] = konfig.sholat[i].menitPeringatan;
    s["iEP"] = konfig.sholat[i].idEfekPeringatan;
    s["cDown"] = konfig.sholat[i].hitungMundur;
    s["iEA"] = konfig.sholat[i].idEfekAdzan;
    s["durA"] = konfig.sholat[i].durasiAdzan;
    s["mH"] = konfig.sholat[i].menitHemat;
    s["brH"] = konfig.sholat[i].kecerahanHemat;
  }

  // --- SIMPAN TANGGAL MODERN ---
  JsonObject tgl = doc.createNestedObject("tanggal");
  tgl["a"] = konfig.tanggal.aktif;
  tgl["ls"] = konfig.tanggal.setiap;
  tgl["ll"] = konfig.tanggal.lama;
  tgl["ft"] = konfig.tanggal.format;
  tgl["id"] = konfig.tanggal.idEfek;
  tgl["ev"] = konfig.tanggal.adaEvent;
  tgl["dp"] = konfig.tanggal.alertDP;
  tgl["md"] = konfig.tanggal.modeDP;

  // 7. Bungkus Daftar Efek (Hanya simpan yang id > 0 untuk hemat memori)
  // SIMPAN DAFTAR EFEK
  JsonArray efekArr = doc.createNestedArray("efek");
  for (int i = 0; i < 15; i++) {
    if (konfig.daftarEfek[i].id > 0) {
      JsonObject e = efekArr.createNestedObject();
      e["id"] = konfig.daftarEfek[i].id;
      e["m"] = konfig.daftarEfek[i].mode;
      e["s"] = konfig.daftarEfek[i].kecepatan;
      e["b"] = konfig.daftarEfek[i].kecerahan;
      e["dp"] = konfig.daftarEfek[i].pakaiDP;
      e["f"] = konfig.daftarEfek[i].jumlahFrame;
      // ---------------------

      // Simpan 8 Frame Pola Segmen
      JsonArray pArr = e.createNestedArray("p");
      for (int f = 0; f < 8; f++) {
        pArr.add(konfig.daftarEfek[i].polaSegmen[f]);
      }
    }
  }

  // 8. Tulis ke File
  File file = LittleFS.open("/config.json", "w");
  if (!file) {
    Serial.println("- Gagal membuka file untuk menulis!");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("- Gagal menulis data ke file!");
    return false;
  }

  file.close();
  Serial.println("- Konfigurasi berhasil disimpan ke LittleFS.");
  return true;
}

#endif
