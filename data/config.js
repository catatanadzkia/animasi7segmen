// config.js - Perintah Langsung & Operasional Real-time
document.addEventListener('DOMContentLoaded', () => {
    muatJadwalKeWeb();
    setupEventListeners();
});

// Pengaturan WIFI
function simpanWifi() {
    const ssid = document.getElementById('wifi-ssid').value;
    const pass = document.getElementById('wifi-pass').value;

    if (!ssid) {
        alert("Pilih atau isi SSID dulu, Bosku!");
        return;
    }

    // Konfirmasi ke user karena ESP akan restart
    if (confirm(`Jam akan mencoba konek ke "${ssid}" dan Restart. Lanjut?`)) {
        
        // Gunakan FormData agar cocok dengan request->getParam(..., true) di C++
        let formData = new FormData();
        formData.append("ssid", ssid);
        formData.append("pass", pass);

        // Tampilkan loading pada tombol (opsional agar keren)
        const btn = event.target;
        const originalText = btn.innerHTML;
        btn.disabled = true;
        btn.innerHTML = "Menyimpan...";

        fetch('/set-wifi', {
            method: 'POST',
            body: formData
        })
        .then(response => {
            if (response.ok) return response.text();
            throw new Error('Gagal mengirim data');
        })
        .then(pesan => {
            alert("Berhasil: " + pesan);
            // Redirect ke halaman utama setelah 3 detik karena ESP restart
            setTimeout(() => { window.location.href = "/"; }, 3000);
        })
        .catch(err => {
            alert("Error: " + err.message);
            btn.disabled = false;
            btn.innerHTML = originalText;
        });
    }
}
function updateStatusWifi() {
    fetch('/status-wifi')
        .then(res => res.json())
        .then(data => {
            document.getElementById('cur-ssid').innerText = data.ssid_aktif || "Terputus";
            document.getElementById('cur-ip').innerText = data.ip || "0.0.0.0";
        });
}

function scanWiFi() {
    const resBox = document.getElementById('scan-result');
    const loading = document.getElementById('scan-loading');
    
    resBox.innerHTML = "";
    loading.classList.remove('d-none');

    fetch('/scan-wifi')
        .then(res => res.json())
        .then(data => {
            loading.classList.add('d-none');
            data.forEach(net => {
                let btn = document.createElement('button');
                btn.className = "list-group-item list-group-item-action d-flex justify-content-between align-items-center";
                btn.innerHTML = `${net.s} <span class="badge badge-primary">${net.r} dBm</span>`;
                btn.onclick = () => {
                    document.getElementById('wifi-ssid').value = net.s;
                    document.getElementById('wifi-pass').focus();
                };
                resBox.appendChild(btn);
            });
        })
        .catch(() => {
            loading.classList.add('d-none');
            alert("Gagal scan, coba lagi Bos!");
        });
}

function muatJadwalKeWeb() {
    fetch('/get-jadwal')
        .then(response => response.json())
        .then(data => {
            if (data.jadwal) {
                const j = data.jadwal;
                // Update teks di dalam box berdasarkan ID
                document.getElementById('disp_0').innerHTML = `IMSAK<br>${j.Imsak || '--:--'}`;
                document.getElementById('disp_1').innerHTML = `SUBUH<br>${j.Subuh || '--:--'}`;
                document.getElementById('disp_2').innerHTML = `DZUHUR<br>${j.Luhur || '--:--'}`; // Sesuaikan Luhur
                document.getElementById('disp_3').innerHTML = `ASHAR<br>${j.Ashar || '--:--'}`;
                document.getElementById('disp_4').innerHTML = `MAGRIB<br>${j.Magrib || '--:--'}`; // Sesuaikan Magrib
                document.getElementById('disp_5').innerHTML = `ISYA<br>${j.Isya || '--:--'}`;
                
                console.log("Jadwal Web Diperbarui!");
            }
        })
        .catch(err => {
            console.error("Gagal mengambil jadwal:", err);
            // Opsional: Tampilkan status error di UI
        });
}
function gasSekarang() {
    const btn = document.getElementById('btnGas');
    const originalText = btn.innerHTML;
    
    btn.disabled = true;
    btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i>';

    fetch('/sinkron-gas-manual')
        .then(res => res.text())
        .then(txt => {
            alert("Perintah Sinkron Terkirim!");
            // Tunggu 3 detik baru cek status update terakhir
            setTimeout(ambilStatusSync, 3000);
        })
        .catch(err => alert("Gagal koneksi ke ESP"))
        .finally(() => {
            btn.disabled = false;
            btn.innerHTML = originalText;
        });
}
function showJam() {
    // Mematikan mode cek jadwal dan kembali ke jam normal
    fetch('/cmd?act=showJam')
        .then(response => {
            if(response.ok) {
                console.log("Display kembali ke Jam Normal");
            }
        });
}
function showJadwal() {
    // Mematikan mode cek jadwal dan kembali ke jam normal
    fetch('/cmd?act=cekJadwal')
        .then(response => {
            if(response.ok) {
                console.log("Display kembali ke Jam Normal");
            }
        });
}
let timerSimulasi;

function setModeAnimasi(status) {
    // Jika status false, hentikan semua simulasi
    if (!status) {
        clearInterval(timerSimulasi);
        fetch(`/cmd?act=animasi&aktif=0&id=0`); // Kirim sinyal STOP ke alat
        console.log("Simulasi Dihentikan");
        return;
    }

    // Jika status true, mulai RUN ALL (Slot 1 sampai 15)
    let currentSlot = 0;
    const totalSlot = 15;
    
    // Hentikan dulu kalau ada timer yang masih jalan
    clearInterval(timerSimulasi);

    // Jalankan Slot Pertama Seketika
    kirimKeAlat(currentSlot);

    // Jalankan Slot berikutnya setiap 5 detik
    timerSimulasi = setInterval(() => {
        currentSlot++;
        if (currentSlot >= totalSlot) {
            currentSlot = 0; // Ulang lagi dari awal kalau sudah sampai slot 15
        }
        kirimKeAlat(currentSlot);
    }, 5000); 
}

// Fungsi khusus buat nembak ke ESP32
function kirimKeAlat(id) {
    console.log("Simulasi Running Slot: " + (id + 1));
    fetch(`/cmd?act=animasi&aktif=1&id=${id}`)
    .catch(err => console.error("Koneksi ESP32 Putus!"));
}
// Fungsi untuk respon instan (ON/OFF)
function liveUpdateHama() {
    // 1. Ambil status checkbox (true/false)
    const status = document.getElementById('u_on').checked;
    
    // 2. Ubah jadi angka (1 atau 0) agar sesuai dengan C++ Bos
    const v = status ? 1 : 0;
    
    console.log("Mengirim perintah Hama, v =", v);

    // 3. Kirim ke ESP dengan argumen ?act=setUltra&v=...
    fetch(`/cmd?act=setUltra&v=${v}`)
        .then(response => {
            if(response.ok) {
                console.log("Berhasil: Hama diatur ke " + v);
            }
        })
        .catch(err => {
            console.error("Gagal kirim perintah instan:", err);
        });
}
function simpanHama() {
    // Ambil semua nilai dari inputan di Card Hama
    const dataHama = {
        on: document.getElementById('u_on').checked,
        fMin: document.getElementById('u_fMin').value,
        fMax: document.getElementById('u_fMax').value,
        int: document.getElementById('u_interval').value,
        malam: document.getElementById('u_malam').checked
    };

    // Kirim data ini ke ESP
    fetch('/save-hama', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(dataHama)
    })
    .then(res => {
        if(res.ok) alert("Sip Bos! Data Hama Terupdate.");
    });
}
// 2. KONTROL KECERAHAN LIVE (br)
let delayBr;
function liveBr(val) {
    document.getElementById('valBr').innerText = val;
    
    // Teknik Debounce: Tunggu user berhenti geser baru kirim request
    clearTimeout(delayBr);
    delayBr = setTimeout(() => {
        fetch(`/cmd?act=setBr&v=${val}`).catch(err => console.error("ESP Offline"));
    }, 100); 
}

// 4. Setup Listener untuk Input (Switch & Button)
function setupEventListeners() {
    // Listener untuk Switch Ultrasonik
    const ultraSwitch = document.getElementById('u_on');
    if (ultraSwitch) {
        ultraSwitch.addEventListener('change', function() {
            let v = this.checked ? 1 : 0;
            kirimCmd('setUltra', v);
        });
    }
}
function kirimCmd(act, val = 0) {
    fetch(`/cmd?act=${act}&v=${val}`)
        .then(res => {
            if (res.ok) console.log(`Command ${act} sukses!`);
        })
        .catch(err => console.error("Gagal kirim command:", err));
}



let skorA = 0;
let skorB = 0;
let babak = 1;

// 1. Fungsi Utama Kirim Perintah (Pakai Gatekeeper/Satpam)
function kirimCmd(params) {
    // Ambil status tombol skor_s (Switch Aktif Papan Skor)
    const isSkorAktif = document.getElementById('skor_s').checked;

    // JANGAN KIRIM kalau switch skor_s mati (kecuali kalau mau matiin via showJam)
    if (!isSkorAktif && !params.includes('act=showJam')) {
        console.log("Perintah ditahan, Mode Skor belum ON!");
        return; 
    }

    fetch(`/cmd?${params}`)
        .then(res => {
            if(res.ok) console.log("Perintah Terkirim: " + params);
        })
        .catch(err => console.error("Koneksi ke ESP Putus!", err));
}

// 2. Kontrol On/Off Papan Skor (Gunakan ID skor_s)
document.getElementById('skor_s').addEventListener('change', function() {
    if (this.checked) {
        // Begitu di ON-kan, langsung kirim data skor saat ini
        kirimCmd(`act=skor&a=${skorA}&b=${skorB}&bk=${babak}`);
    } else {
        // Kalau di OFF-kan, paksa ESP balik ke Jam
        fetch('/cmd?act=showJam'); 
    }
});

// 3. Fungsi Tambah Skor Tetap Sama (Tapi sekarang otomatis lewat satpam kirimCmd)
function setSkor(tim, poin) {
    if(tim === 'a') {
        skorA = Math.max(0, skorA + poin);
        document.getElementById('skor_a').innerText = String(skorA).padStart(2, '0');
    } else {
        skorB = Math.max(0, skorB + poin);
        document.getElementById('skor_b').innerText = String(skorB).padStart(2, '0');
    }
    // Kirim data (Akan dicek oleh kirimCmd apakah skor_s aktif atau tidak)
    kirimCmd(`act=skor&a=${skorA}&b=${skorB}&bk=${babak}`);
}

// 3. Tukar Posisi Skor (Swap)
function swapSkor() {
    let temp = skorA;
    skorA = skorB;
    skorB = temp;
    
    document.getElementById('skor_a').innerText = String(skorA).padStart(2, '0');
    document.getElementById('skor_b').innerText = String(skorB).padStart(2, '0');
    
    kirimCmd(`act=skor&a=${skorA}&b=${skorB}&bk=${babak}`);
}

// 4. Set Babak / Period
function setBabak(val) {
    babak = val;
    // Misal ada elemen UI untuk babak
    if(document.getElementById('label_babak')) {
        document.getElementById('label_babak').innerText = babak;
    }
    kirimCmd(`act=skor&a=${skorA}&b=${skorB}&bk=${babak}`);
}

// 4. PERINTAH SISTEM (Restart, Ganti Mode, dll)
function cmd(action) {
    console.log("Command:", action);
    fetch(`/cmd?act=${action}`)
        .then(res => {
            if(action === 'restart' && res.ok) {
                alert("Perangkat sedang restart...");
                location.reload();
            }
        })
        .catch(err => alert("Koneksi ke ESP terputus"));
}

let frameWeb = 0;

// Variabel global untuk menyimpan data terbaru dari ESP
let configESP = {
    idEfekTes_mode: 0,
    modeEfekAktif: false,
    kecerahan: 255
};

function updateJamUI() {
    const display = document.getElementById('jamDigit');
    const bg = document.getElementById('jamBackground');
    if (!display) return;

    // 1. Ambil Waktu
    const sekarang = new Date();
    const h = sekarang.getHours().toString().padStart(2, '0');
    const m = sekarang.getMinutes().toString().padStart(2, '0');
    const s = sekarang.getSeconds().toString().padStart(2, '0');
    const titik = (sekarang.getSeconds() % 2 === 0) ? ":" : " ";

    // 2. DETEKTIF: Cek jumlah digit dari objek konfig di main.js
    // Kita coba ambil dari konfig.global.dg (milik main.js) 
    // atau configESP.ls (yang kita oper tadi)
    let jmlDigit = 6;
    if (typeof konfig !== 'undefined' && konfig.global) {
        jmlDigit = parseInt(konfig.global.dg);
    } else if (typeof configESP !== 'undefined') {
        jmlDigit = parseInt(configESP.ls);
    }

    // Console log biar kelihatan di F12
    console.log("Preview Digit:", jmlDigit);

    // 3. POTONG TEKS
    let teksFinal = "";
    if (jmlDigit === 4) {
        teksFinal = `${h}${titik}${m}`;
        if (bg) bg.innerText = "88:88";
    } else {
        teksFinal = `${h}${titik}${m}${titik}${s}`;
        if (bg) bg.innerText = "88:88:88";
    }

    // 4. LOGIKA ANIMASI
    // Kita ambil mode dari elemen UI langsung biar realtime
    const modeAktif = document.getElementById('modeAnimasi')?.value || 0;
    const isTesting = (typeof configESP !== 'undefined') ? configESP.modeEfekAktif : false;
    const mode = isTesting ? parseInt(modeAktif) : 0;

    switch (mode) {
        case 1: // CHASE
            let shift = Math.floor(Date.now() / 200) % teksFinal.length;
            let arr = teksFinal.split('');
            for(let i=0; i<shift; i++) arr.push(arr.shift());
            display.innerText = arr.join('');
            display.style.opacity = 1;
            break;
        case 5: // STROBO
            display.style.opacity = (Date.now() % 100 < 50) ? "1" : "0";
            display.innerText = teksFinal;
            break;
        case 6: // KEDIP
            display.style.opacity = (Date.now() % 1000 < 500) ? "1" : "0";
            display.innerText = teksFinal;
            break;
        default:
            display.style.opacity = (konfig.global?.br || 255) / 255;
            display.innerText = teksFinal;
            break;
    }
}
setInterval(updateJamUI, 50);

