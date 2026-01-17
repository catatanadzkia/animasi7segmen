
// ===============================
// config_manager.cpp
// ===============================
#include "config_manajer.h"
#include "configSystem.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>



// ---------- UTIL ----------
static bool openJson(const char* path, DynamicJsonDocument& doc) {
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) return false;
  auto err = deserializeJson(doc, f);
  f.close();
  return !err;
}

static bool writeJson(const char* path, DynamicJsonDocument& doc) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  bool ok = serializeJson(doc, f) > 0;
  f.close();
  return ok;
}

bool reloadConfigByTarget(const String& target) {
  if (target == "system") return loadSystemConfig();
  if (target == "sholat") return loadSholatConfig();
  if (target == "tanggal") return loadTanggalConfig();
  if (target == "efek") return loadEfekConfig();
  if (target == "ultra") return loadHama();
  if (target == "skor") return loadSkor();
  return false;
}
bool getConfigPath(const String& target, String& path) {
  if (target == "system") path = "/system.json";
  else if (target == "sholat") path = "/sholat.json";
  else if (target == "tanggal") path = "/tanggal.json";
  else if (target == "efek") path = "/efek.json";
  else if (target == "ultra") path = "/ultra.json";
  else if (target == "jadwal") path = "/jadwal.json";
  else if (target == "skor") path = "/skor.json";
  else return false;
  return true;
}


// ---------- SYSTEM ----------
bool loadSystemConfig() {
  DynamicJsonDocument doc(1024);
  if (!openJson("/system.json", doc)) {
    standarConfig();
    return false;
  }

  auto g = doc["global"];
  konfig.jumlahDigit = g["dg"] | konfig.jumlahDigit;
  konfig.kecerahanGlobal = g["br"] | konfig.kecerahanGlobal;
  strlcpy(konfig.namaPerangkat, g["dev"] | konfig.namaPerangkat, sizeof(konfig.namaPerangkat));
  strlcpy(konfig.urlGAS, g["gas"] | konfig.urlGAS, sizeof(konfig.urlGAS));
  strlcpy(konfig.terakhirSync, g["sncy"] | konfig.terakhirSync, sizeof(konfig.terakhirSync));
  strlcpy(konfig.authUser, g["user"] | konfig.authUser, sizeof(konfig.authUser));
  strlcpy(konfig.authPass, g["pass"] | konfig.authPass, sizeof(konfig.authPass));
  konfig.modeKedip = g["mk"] | konfig.modeKedip;
  konfig.speedKedip = g["sk"] | konfig.speedKedip;
  konfig.durasiKedip = g["dk"] | konfig.durasiKedip;
  konfig.titikDuaKedip = g["t2k"] | konfig.titikDuaKedip;
  konfig.loopSetiap = g["ls"] | konfig.loopSetiap;
  konfig.loopLama = g["ll"] | konfig.loopLama;
  konfig.fmtTgl = g["ft"] | konfig.fmtTgl;
  konfig.timezone = g["tz"] | konfig.timezone;

  auto w = doc["wifi"];
  konfig.wikon.ssid = w["idwifi"] | konfig.wikon.ssid;
  konfig.wikon.pass = w["pas"] | konfig.wikon.pass;

  return true;
}
bool saveSystemConfig() {
  DynamicJsonDocument doc(1024);
  auto g = doc.createNestedObject("global");
  g["dg"] = konfig.jumlahDigit;
  g["br"] = konfig.kecerahanGlobal;
  g["dev"] = konfig.namaPerangkat;
  g["gas"] = konfig.urlGAS;
  g["sncy"] = konfig.terakhirSync;
  g["user"] = konfig.authUser;
  g["pass"] = konfig.authPass;
  g["mk"] = konfig.modeKedip;
  g["sk"] = konfig.speedKedip;
  g["dk"] = konfig.durasiKedip;
  g["t2k"] = konfig.titikDuaKedip;
  g["ls"] = konfig.loopSetiap;
  g["ll"] = konfig.loopLama;
  g["ft"] = konfig.fmtTgl;
  g["tz"] = konfig.timezone;

  auto w = doc.createNestedObject("wifi");
  w["idwifi"] = konfig.wikon.ssid;
  w["pas"] = konfig.wikon.pass;

  return writeJson("/system.json", doc);
}
bool loadHama() {
  DynamicJsonDocument doc(512);
  if (!openJson("/ultra.json", doc)) {
    standarConfig();
    return false;
  }

  auto ul = doc["ultra"];
  konfig.pengusirHama.aktif = ul["on"] | konfig.pengusirHama.aktif;
  konfig.pengusirHama.frekuensiMin = ul["fMin"] | konfig.pengusirHama.frekuensiMin;
  konfig.pengusirHama.frekuensiMax = ul["fMax"] | konfig.pengusirHama.frekuensiMax;
  konfig.pengusirHama.intervalAcak = ul["int"] | konfig.pengusirHama.intervalAcak;
  konfig.pengusirHama.aktifMalamSaja = ul["malam"] | konfig.pengusirHama.aktifMalamSaja;
  return true;
}

bool saveHama() {
  DynamicJsonDocument doc(512);
  auto ul = doc.createNestedObject("ultra");
  ul["on"] = konfig.pengusirHama.aktif;
  ul["fMin"] = konfig.pengusirHama.frekuensiMin;
  ul["fMax"] = konfig.pengusirHama.frekuensiMax;
  ul["int"] = konfig.pengusirHama.intervalAcak;
  ul["malam"] = konfig.pengusirHama.aktifMalamSaja;

  return writeJson("/ultra.json", doc);
}
bool loadSkor() {
  DynamicJsonDocument doc(512);
  if (!openJson("/skor.json", doc)) {
    standarConfig();
    return false;
  }

  auto sek = doc["papanSkor"];
  konfig.papanSkor.skorA = sek["skorA"] | konfig.papanSkor.skorA;
  konfig.papanSkor.skorB = sek["skorB"] | konfig.papanSkor.skorB;
  konfig.papanSkor.babak = sek["bbk"] | konfig.papanSkor.babak;
  konfig.papanSkor.aktif = sek["on"] | konfig.papanSkor.aktif;
  return true;
}

bool saveSkor() {
  DynamicJsonDocument doc(512);
  auto sek = doc.createNestedObject("papanSkor");
  sek["skorA"] = konfig.papanSkor.skorA;
  sek["skorB"] = konfig.papanSkor.skorB;
  sek["bbk"] = konfig.papanSkor.babak;
  sek["on"] = konfig.papanSkor.aktif;

  return writeJson("/skor.json", doc);
}

// ---------- SHOLAT ----------
bool loadSholatConfig() {
  DynamicJsonDocument doc(1024);
  if (!openJson("/sholat.json", doc)) {
    standarConfig();
    return false;
  }
  JsonArray a = doc.as<JsonArray>();
  for (int i = 0; i < 6 && i < (int)a.size(); i++) {
    konfig.sholat[i].menitPeringatan = a[i]["mP"] | konfig.sholat[i].menitPeringatan;
    konfig.sholat[i].idEfekPeringatan = a[i]["iEP"] | konfig.sholat[i].idEfekPeringatan;
    konfig.sholat[i].hitungMundur = a[i]["cDown"] | konfig.sholat[i].hitungMundur;
    konfig.sholat[i].idEfekAdzan = a[i]["iEA"] | konfig.sholat[i].idEfekAdzan;
    konfig.sholat[i].durasiAdzan = a[i]["durA"] | konfig.sholat[i].durasiAdzan;
    konfig.sholat[i].menitHemat = a[i]["mH"] | konfig.sholat[i].menitHemat;
    konfig.sholat[i].kecerahanHemat = a[i]["brH"] | konfig.sholat[i].kecerahanHemat;
  }
  return true;
}

bool saveSholatConfig() {
  DynamicJsonDocument doc(1024);
  JsonArray a = doc.to<JsonArray>();
  for (int i = 0; i < 6; i++) {
    JsonObject s = a.createNestedObject();
    s["mP"] = konfig.sholat[i].menitPeringatan;
    s["iEP"] = konfig.sholat[i].idEfekPeringatan;
    s["cDown"] = konfig.sholat[i].hitungMundur;
    s["iEA"] = konfig.sholat[i].idEfekAdzan;
    s["durA"] = konfig.sholat[i].durasiAdzan;
    s["mH"] = konfig.sholat[i].menitHemat;
    s["brH"] = konfig.sholat[i].kecerahanHemat;
  }
  return writeJson("/sholat.json", doc);
}

// ---------- TANGGAL ----------
bool loadTanggalConfig() {
  DynamicJsonDocument doc(1024);
  if (!openJson("/tanggal.json", doc)) {
    standarConfig();
    return false;
  }
  konfig.tanggal.aktif = doc["a"] | konfig.tanggal.aktif;
  konfig.tanggal.setiap = doc["ls"] | konfig.tanggal.setiap;
  konfig.tanggal.lama = doc["ll"] | konfig.tanggal.lama;
  konfig.tanggal.format = doc["ft"] | konfig.tanggal.format;
  konfig.tanggal.idEfek = doc["id"] | konfig.tanggal.idEfek;
  konfig.tanggal.adaEvent = doc["ev"] | konfig.tanggal.adaEvent;
  konfig.tanggal.alertDP = doc["dp"] | konfig.tanggal.alertDP;
  konfig.tanggal.modeDP = doc["md"] | konfig.tanggal.modeDP;
  return true;
}

bool saveTanggalConfig() {
  DynamicJsonDocument doc(1024);
  doc["a"] = konfig.tanggal.aktif;
  doc["ls"] = konfig.tanggal.setiap;
  doc["ll"] = konfig.tanggal.lama;
  doc["ft"] = konfig.tanggal.format;
  doc["id"] = konfig.tanggal.idEfek;
  doc["ev"] = konfig.tanggal.adaEvent;
  doc["dp"] = konfig.tanggal.alertDP;
  doc["md"] = konfig.tanggal.modeDP;
  return writeJson("/tanggal.json", doc);
}

// ---------- EFEK ----------
bool loadEfekConfig() {
  DynamicJsonDocument doc(1024);
  if (!openJson("/efek.json", doc)) {
    return false;
  }
  for (int i = 0; i < 15; i++) konfig.daftarEfek[i].id = 0;
  JsonArray a = doc.as<JsonArray>();
  int i = 0;
  for (JsonObject e : a) {
    if (i >= 15) break;
    konfig.daftarEfek[i].id = e["id"] | 0;
    konfig.daftarEfek[i].mode = e["m"] | 0;
    konfig.daftarEfek[i].kecepatan = e["s"] | 0;
    konfig.daftarEfek[i].kecerahan = e["b"] | 0;
    konfig.daftarEfek[i].pakaiDP = e["dp"] | false;
    JsonArray p = e["p"].as<JsonArray>();
    int f = 0;
    for (uint8_t v : p) {
      if (f >= 8) break;
      konfig.daftarEfek[i].polaSegmen[f++] = v;
    }
    konfig.daftarEfek[i].jumlahFrame = f;
    i++;
  }
  return true;
}

bool saveEfekConfig() {
  DynamicJsonDocument doc(1024);
  JsonArray a = doc.to<JsonArray>();
  for (int i = 0; i < 15; i++) {
    if (konfig.daftarEfek[i].id > 0) {
      JsonObject e = a.createNestedObject();
      e["id"] = konfig.daftarEfek[i].id;
      e["m"] = konfig.daftarEfek[i].mode;
      e["s"] = konfig.daftarEfek[i].kecepatan;
      e["b"] = konfig.daftarEfek[i].kecerahan;
      e["dp"] = konfig.daftarEfek[i].pakaiDP;
      JsonArray p = e.createNestedArray("p");
      for (int f = 0; f < konfig.daftarEfek[i].jumlahFrame; f++) p.add(konfig.daftarEfek[i].polaSegmen[f]);
    }
  }
  return writeJson("/efek.json", doc);
}
