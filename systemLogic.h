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
  int i = 0;
  bool found = false;
  while (i < 15) {
    if (konfig.daftarEfek[i].id == idEfek) {
      found = true;
      break;
    }
    i++;
  }
  if (!found || konfig.daftarEfek[i].jumlahFrame <= 0) return;

  // Persiapan Variabel
  int speed = konfig.daftarEfek[i].kecepatan;
  int mode = konfig.daftarEfek[i].mode;
  int brDasar = konfig.daftarEfek[i].kecerahan;
  unsigned long skrg = millis();

  // --- KUNCI: CEK POLA 0 ---
  bool pakaiPolaJam = (konfig.daftarEfek[i].polaSegmen[0] == 0);

  uint8_t bufferJam[MAX_DIGIT];  // Pakai MAX_DIGIT agar aman sesuai fungsi Bos
  if (pakaiPolaJam) {
    extern struct tm timeinfo;
    // Panggil fungsi milik Bos.
    // titikNyala kita set true/false sesuai detik (kedip) atau dipaksa true
    bool detikKedip = (timeinfo.tm_sec % 2 == 0);
    buatBufferWaktu(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, bufferJam, detikKedip);
  }

  int frameAktif = (skrg / speed) % konfig.daftarEfek[i].jumlahFrame;
  int brFinal = brDasar;

  for (int d = 0; d < konfig.jumlahDigit; d++) {
    uint8_t bitmask = 0;

    switch (mode) {
      case 1:  // MODE CHASE (Angka Jam Berlari)
        if (pakaiPolaJam) {
          bitmask = bufferJam[(d + frameAktif) % konfig.jumlahDigit];
        } else {
          bitmask = konfig.daftarEfek[i].polaSegmen[(frameAktif + d) % konfig.daftarEfek[i].jumlahFrame];
        }
        break;

      case 2:  // MODE RAINDROP (Jatuh Satu-satu)
        {
          int fHujan = (skrg / (speed + (d * 15))) % 2;
          uint8_t pola = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[i].polaSegmen[frameAktif];
          bitmask = (fHujan == 0) ? pola : 0;
        }
        break;

      case 4:  // MODE BREATHING (Nafas)
        {
          float pulse = (sin(skrg * 0.003) * 0.5) + 0.5;
          brFinal = pulse * brDasar;
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[i].polaSegmen[frameAktif];
        }
        break;

      case 5:  // MODE STROBO (Kedip Kilat)
        {
          brFinal = (skrg % 100 < 50) ? brDasar : 0;
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[i].polaSegmen[frameAktif];
        }
        break;

      case 6:  // MODE KEDIP (Kedip Santai)
        {
          brFinal = (skrg % 1000 < 500) ? brDasar : 0;
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[i].polaSegmen[frameAktif];
        }
        break;
      case 7:  // MODE WAVE / BREATHING (Lembut)
        {
          
          float kecepatan = (float)speed / 10.0;
          float wave = (sin(millis() * 0.005 * kecepatan) + 1.0) / 2.0;

          
          brFinal = (uint8_t)(wave * brDasar);

         
          bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[i].polaSegmen[frameAktif];
        }
        break;

      default:
        bitmask = pakaiPolaJam ? bufferJam[d] : konfig.daftarEfek[i].polaSegmen[frameAktif];
        break;
    }

    // Jika pola segmen biasa, tambahkan DP sesuai config
    if (!pakaiPolaJam && konfig.daftarEfek[i].pakaiDP) {
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