#include <configSystem.h>

// ==========================================
// --- STRUKTUR DATA (KONFIGURASI) ---
// ==========================================

SistemConfig konfig;

bool standarConfig() {
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
  strlcpy(konfig.terakhirSync, "Belum Sync", sizeof(konfig.terakhirSync));  //ini mengartikan bahwa tidak ada sinkronisasi'
  return true;
}
