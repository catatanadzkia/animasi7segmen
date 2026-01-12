#ifndef DISPLAY_LOGIC_H
#define DISPLAY_LOGIC_H

#include <Arduino.h>
#include "configSystem.h" // Mengakses struct konfig

// --- TABEL KARAKTER STANDAR ---
const uint8_t angkaSegmen[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
const uint8_t segmenKosong = 0x00;
const uint8_t segmenStrip  = 0x40;

#define MAX_DIGIT 12


// --- BUFFER & TIMING ---
uint8_t mainBuffer[MAX_DIGIT];  // Buffer tetap aman meskipun digit di web diubah-ubah
unsigned long lastUpdateJam = 0;
struct tm timeinfo;
/**
 * 1. MENDAPATKAN BYTE PER DIGIT
 */
uint8_t dapatkanByte(int angka, bool pakaiDP) {
  if (angka < 0 || angka > 9) return segmenKosong;
  uint8_t pola = angkaSegmen[angka];
  if (pakaiDP) pola |= 0x80;
  return pola;
}

/**
 * 2. MENGISI BUFFER JAM (Dinamis 4 atau 6 Digit)
 */
void buatBufferWaktu(int jam, int menit, int detik, uint8_t* buffer, bool titikNyala) {
  if (konfig.jumlahDigit < 4) return;

  // Digit 0 & 1 (JAM)
  buffer[0] = dapatkanByte(jam / 10, false);
  buffer[1] = dapatkanByte(jam % 10, titikNyala); // Titik di sini

  // Digit 2 & 3 (MENIT)
  // Perbaikan: Paksa titikNyala di digit ke-3 jika cuma 4 digit agar jadi ":"
  bool titikTengah = (konfig.jumlahDigit == 4) ? titikNyala : false;
  buffer[2] = dapatkanByte(menit / 10, titikTengah); 
  buffer[3] = dapatkanByte(menit % 10, false);

  // Jika 6 digit (JAM:MENIT:DETIK)
  if (konfig.jumlahDigit >= 6) {
    buffer[3] = dapatkanByte(menit % 10, titikNyala); // Titik pemisah detik
    buffer[4] = dapatkanByte(detik / 10, false);
    buffer[5] = dapatkanByte(detik % 10, false);
  }
  
  // Bersihkan sisa digit (untuk 12 digit)
  int startClean = (konfig.jumlahDigit >= 6) ? 6 : 4;
  for(int i = startClean; i < konfig.jumlahDigit; i++) {
    if(i < MAX_DIGIT) buffer[i] = segmenKosong; 
  }
}


void buatBufferAngka(int angka, uint8_t* buf) {
  buf[0] = segmenKosong; // Matikan digit kiri
  buf[1] = segmenKosong; 
  buf[2] = dapatkanByte((angka / 10) % 10, false);
  buf[3] = dapatkanByte(angka % 10, false);
}

 //* MENGISI BUFFER TANGGAL (Format DD:MM)


void buatBufferTanggal(struct tm info, uint8_t* buf, int format) {
  int tgl = info.tm_mday;
  int bln = info.tm_mon + 1;
  int thn = info.tm_year + 1900;

  // Bersihkan buffer
  for(int i=0; i<8; i++) buf[i] = 0;

  switch (format) {
    case 0: // Format DD.MM (Contoh: 09.01)
      buf[0] = dapatkanByte(tgl / 10, false); 
      buf[1] = dapatkanByte(tgl % 10, true);  // Titik nyala di sini sebagai pemisah
      buf[2] = dapatkanByte(bln / 10, false);
      buf[3] = dapatkanByte(bln % 10, false);
      break;

    case 1: // Format MM.YY (Contoh: 01.26)
      buf[0] = dapatkanByte(bln / 10, false);
      buf[1] = dapatkanByte(bln % 10, true);  // Titik nyala di sini
      buf[2] = dapatkanByte((thn % 100) / 10, false);
      buf[3] = dapatkanByte((thn % 100) % 10, false);
      break;

    case 2: // Format YYYY (Contoh: 2026)
      buf[0] = dapatkanByte(thn / 1000, false);
      buf[1] = dapatkanByte((thn / 100) % 10, false);
      buf[2] = dapatkanByte((thn / 10) % 10, false);
      buf[3] = dapatkanByte(thn % 10, false);
      break;
      
    default: // Backup jika format salah
      buf[0] = dapatkanByte(tgl / 10, false);
      buf[1] = dapatkanByte(tgl % 10, true);
      buf[2] = dapatkanByte(bln / 10, false);
      buf[3] = dapatkanByte(bln % 10, false);
      break;
  }
}
 //* 3. MENGISI BUFFER SKOR (Papan Skor)
void buatBufferSkor(uint8_t* buffer) {
  // Pastikan setidaknya ada 2 digit untuk Skor A
  if (konfig.jumlahDigit >= 2) {
    buffer[0] = dapatkanByte(konfig.papanSkor.skorA / 10, false);
    buffer[1] = dapatkanByte(konfig.papanSkor.skorA % 10, (konfig.papanSkor.babak == 1));
  }
  
  // Pastikan ada digit ke-3 dan ke-4 untuk Skor B
  if (konfig.jumlahDigit >= 4) {
    buffer[2] = dapatkanByte(konfig.papanSkor.skorB / 10, false);
    buffer[3] = dapatkanByte(konfig.papanSkor.skorB % 10, (konfig.papanSkor.babak == 2));
  }
  
  // Digit sisa (5 ke atas) dikosongkan
  for(int i = 4; i < konfig.jumlahDigit; i++) {
    if (i < MAX_DIGIT) buffer[i] = segmenKosong;
  }
}

/**
 * 4. EKSEKUSI EFEK BERDASARKAN ID
 * Digunakan untuk Adzan, Alarm, atau Notif Agenda
 */
void setDigitRaw(int digit, uint8_t pola) {
  if (digit < konfig.jumlahDigit) {
    mainBuffer[digit] = pola;
  }
}

// Fungsi untuk mengosongkan layar
void clearDisplay() {
  for (int i = 0; i < konfig.jumlahDigit; i++) mainBuffer[i] = 0x00;
}
#endif