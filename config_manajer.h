#ifndef CONFIG_MANAJER_H
#define CONFIG_MANAJER_H
// ===============================
// config_manager.h
// ===============================
#include <Arduino.h>

// --- SYSTEM ----
bool loadSystemConfig();
bool saveSystemConfig();
// --- HAMA ---
bool loadHama();
bool saveHama();
// --- SHOLAT ---
bool loadSholatConfig();
bool saveSholatConfig();
// --- TANGGAL ---
bool loadTanggalConfig();
bool saveTanggalConfig();
// --- EFEK ----
bool loadEfekConfig();
bool saveEfekConfig();

bool loadSkor();

//---LOAD JSON---
bool reloadConfigByTarget(const String& target);
bool getConfigPath(const String& target, String& path);

#endif