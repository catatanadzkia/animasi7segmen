#ifndef SYSTEM_LOGIC_H
#define SYSTEM_LOGIC_H

#include <Arduino.h>
#include "configSystem.h"  // Mengakses struct konfig


extern void setKecerahan(int level);
extern void updateTampilanSegmen(uint8_t* dataArray, int jumlahIC);

void buatBufferTanggal(struct tm info, uint8_t* buf, int format);
void buatBufferAngka(int angka, uint8_t* buf);
void buatBufferWaktu(int jam, int menit, int detik, uint8_t* buf, bool dot);


void tampilkanKeSegmen(uint8_t* buf) {
  // Kita panggil mesin hardware Bos di sini
  // Kita pakai variabel 'konfig.jumlahDigit' agar otomatis menyesuaikan
  updateTampilanSegmen(buf, konfig.jumlahDigit);
}



void prosesAnimasiEfek(int idEfek) {
  // ✅ KONVERSI ID KE SLOT (ID 1 = Slot 0, ID 15 = Slot 14)
  int slot = idEfek - 1;
  
  // ✅ VALIDASI SLOT
  if (slot < 0 || slot >= 15) {
    Serial.printf("⚠️ ID Efek %d invalid (slot %d)\n", idEfek, slot);
    return;
  }
  
  if (konfig.daftarEfek[slot].jumlahFrame <= 0) {
    Serial.printf("⚠️ Efek Slot %d kosong\n", slot);
    return;
  }

  // Persiapan Variabel
  int speed = konfig.daftarEfek[slot].kecepatan;
  int mode = konfig.daftarEfek[slot].mode;
  int brDasar = konfig.daftarEfek[slot].kecerahan;
  unsigned long skrg = millis();

  // --- KUNCI: CEK APAKAH PAKAI POLA JAM ATAU CUSTOM ---
  bool pakaiPolaJam = (konfig.daftarEfek[slot].polaSegmen[0] == 0);

  // Buffer untuk angka jam (jika pakaiPolaJam = true)
  uint8_t bufferJam[MAX_DIGIT];
  if (pakaiPolaJam) {
    extern struct tm timeinfo;
    bool detikKedip = (timeinfo.tm_sec % 2 == 0);
    buatBufferWaktu(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, bufferJam, detikKedip);
  }

  // Hitung frame aktif saat ini
  int frameAktif = (skrg / speed) % konfig.daftarEfek[slot].jumlahFrame;
  int brFinal = brDasar;

  // ========================================
  // LOOP UNTUK SETIAP DIGIT
  // ========================================
  for (int d = 0; d < konfig.jumlahDigit; d++) {
    uint8_t bitmask = 0;

    switch (mode) {
      
      case 1: // MODE 1: FADE IN/OUT
        {
          float cycle = (skrg % (speed * 2)) / (float)(speed * 2);
          float brightness = (sin(cycle * 2 * PI - PI/2) + 1.0) / 2.0;
          brFinal = (uint8_t)(brightness * brDasar);
          
          // Menggunakan Ternary
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        }
        break;

      case 2: // MODE 2: SMOOTH WAVE
        {
          float offset = d * 0.5;
          float phase = (skrg / (float)speed) + offset;
          float wave = (sin(phase) + 1.0) / 2.0;
          brFinal = (uint8_t)(wave * brDasar);
          
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        }
        break;

      case 3: // MODE 3: GENTLE PULSE
        {
          float t = (skrg % speed) / (float)speed;
          float eased = (t < 0.5) ? (2 * t * t) : (1 - pow(-2 * t + 2, 2) / 2);
          
          brFinal = (uint8_t)(eased * brDasar);
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        }
        break;

      case 4: // MODE 4: RIPPLE
        {
          int center = konfig.jumlahDigit / 2;
          int distance = abs(d - center);
          float ripple = (skrg / (float)speed) - (distance * 0.3);
          float amplitude = (sin(ripple * 2) + 1.0) / 2.0;
          brFinal = (uint8_t)(amplitude * brDasar);
          
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        }
        break;

      case 5: // MODE 5: SMOOTH CHASE
        {
          // Ternary khusus untuk logika chase
          bitmask = pakaiPolaJam ? bufferJam[(d + (skrg / speed)) % konfig.jumlahDigit] 
                                 : konfig.daftarEfek[slot].polaSegmen[(frameAktif + d) % konfig.daftarEfek[slot].jumlahFrame];
        }
        break;

      case 6: // MODE 6: GENTLE BLINK
        {
          int period = 2000;
          float t = (skrg % period) / (float)period;
          float brightness = (t < 0.3) ? (t / 0.3) : ((t < 0.7) ? 1.0 : ((1.0 - t) / 0.3));
          
          brFinal = (uint8_t)(brightness * brDasar);
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        }
        break;
        
      case 7: // MODE 7: SEQUENTIAL FADE
        {
          float digitPhase = (skrg / (float)speed) - (d * 0.5);
          float fade = (sin(digitPhase * 2) + 1.0) / 2.0;
          
          // Ternary bertingkat (Nested Ternary)
          bitmask = (fade < 0.1) ? 0x00 : (pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif]);
          if (fade >= 0.1) brFinal = (uint8_t)(fade * brDasar);
        }
        break;

      case 8: // MODE 8: HEARTBEAT
        {
          int period = 2000;
          int phase = skrg % period;
          float brightness = (phase < 100) ? 1.0 : 
                             (phase < 150) ? (1.0 - ((phase - 100) / 50.0)) : 
                             (phase < 300) ? 0.2 : 
                             (phase < 400) ? 1.0 : 
                             (phase < 450) ? (1.0 - ((phase - 400) / 50.0)) : 0.2;
          
          brFinal = (uint8_t)(brightness * brDasar);
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        }
        break;

      default: // DEFAULT: STATIC
        bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[slot].polaSegmen[frameAktif];
        brFinal = brDasar;
        break;
    }

    // Tambahkan DP (Decimal Point) jika perlu
    if (!pakaiPolaJam && konfig.daftarEfek[slot].pakaiDP) {
      bitmask |= 0x80;
    }

    setDigitRaw(d, bitmask);
  }

  setKecerahan(brFinal);
}

void notifTanggal(int setiap, int durasi, bool adaAgenda, int modeDP, int speedDP) {
  // PENGAMAN: Jika 'setiap' 0, maka fitur ini mati atau tampil terus.
  // Kita buat: kalau 0, jangan jalankan logika modulo.
  if (setiap <= 0) return;

  // Ambil waktu dari RTC/Sistem
  long detikSekarangTotal = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;

  // Logika interval: Misal tampil setiap 60 detik selama 5 detik
  if ((detikSekarangTotal % setiap) < durasi) {

    // 1. Buat buffer tanggal sesuai format (0-3)
    buatBufferTanggal(timeinfo, mainBuffer, konfig.tanggal.format);

    // 2. Logika Titik (DP) Kedip
    if (adaAgenda) {
      if (speedDP < 50) speedDP = 100;  // Pengaman kecepatan

      if ((millis() / speedDP) % 2 == 0) {
        if (modeDP == 1) mainBuffer[1] |= 0x80;
        else if (modeDP == 2) mainBuffer[2] |= 0x80;
        else if (modeDP == 3) {
          mainBuffer[1] |= 0x80;
          mainBuffer[2] |= 0x80;
        }
      } else {
        mainBuffer[1] &= 0x7F;
        mainBuffer[2] &= 0x7F;
      }
    }

    // Beritahu sistem bahwa kita sedang menampilkan tanggal (agar jam utama tidak menimpa)
    konfig.modeEfekAktif = true;
  } else {
    // Jika durasi habis, kembalikan kontrol ke tampilan Jam
    konfig.modeEfekAktif = false;
  }
}

// Kita tambahkan parameter pertama: int mJadwal (Menit Jadwal)
void notifSholat(int mJadwal, int mPeringatan, int idEfekPeringatan, int detikMundur, int durasiAdzan, int idEfekAdzan, int levelRedup, int menitLamaRedup) {

  // Validasi: Jika jadwal kosong (0 atau -1), langsung keluar
  if (mJadwal <= 0) return;

  extern struct tm timeinfo;
  long sekarangMenitTotal = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
  long sekarangDetikTotal = (sekarangMenitTotal * 60) + timeinfo.tm_sec;
  long jadwalDetikTotal = mJadwal * 60;  // Mengonversi jadwal menit ke detik total

  // --- 1. PRIORITAS 1: COUNTDOWN (MISAL 30 DETIK TERAKHIR) ---
  if (sekarangDetikTotal >= (jadwalDetikTotal - detikMundur) && sekarangDetikTotal < jadwalDetikTotal) {
    int sisaDetik = jadwalDetikTotal - sekarangDetikTotal;
    buatBufferAngka(sisaDetik, mainBuffer);
    konfig.modeEfekAktif = true;
    return;
  }

  // --- 2. PRIORITAS 2: EFEK PERINGATAN (MISAL 5 MENIT SEBELUM) ---
  if (sekarangMenitTotal >= (mJadwal - mPeringatan) && sekarangMenitTotal < mJadwal) {
    prosesAnimasiEfek(idEfekPeringatan);
    konfig.modeEfekAktif = true;
    return;
  }

  // --- 3. PRIORITAS 3: SAAT WAKTU SHOLAT (EFEK ADZAN) ---
  if (sekarangMenitTotal >= mJadwal && sekarangMenitTotal < (mJadwal + durasiAdzan)) {
    prosesAnimasiEfek(idEfekAdzan);
    konfig.modeEfekAktif = true;
    return;
  }

  // --- 4. PRIORITAS 4: REDUP / HEMAT ENERGI ---
  int menitMulaiRedup = mJadwal + durasiAdzan;
  if (sekarangMenitTotal >= menitMulaiRedup && sekarangMenitTotal < (menitMulaiRedup + menitLamaRedup)) {
    setKecerahan(levelRedup);
    bool statusTitik = (timeinfo.tm_sec % 2 == 0);
    buatBufferWaktu(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, mainBuffer, statusTitik);
    konfig.modeEfekAktif = true;
    return;
  }
}


#endif