// main.js - Master Data Controller
window.addEventListener('DOMContentLoaded', () => {
	
    console.log("Sistem Memulai...");
    
    // 1. Cek Koneksi ke ESP dulu
    fetch('/status-wifi')
    .then(res => res.text())
    .then(status => {
        const halWifi = document.getElementById('halaman-konek-wifi');
        const halBeranda = document.getElementById('halaman-beranda');

        if (status === "0") {
            // MODE AP: Tampilkan setting WiFi & Scan
            if(halWifi) halWifi.classList.remove('d-none');
            if(halBeranda) halBeranda.classList.add('d-none');
            muatDaftarWifi(); 
        } else {
            // MODE RUMAH: Tampilkan Dashboard Utama
            if(halBeranda) halBeranda.classList.remove('d-none');
            if(halWifi) halWifi.classList.add('d-none');
            
            // Baru muat data jam kalau sudah konek
            muatKonfigurasi(); 
            initEfekEditor();
        }
    })
    .catch(err => {
        console.error("ESP tidak merespon:", err);
        // Failsafe: Tetap tampilkan beranda jika gagal fetch
        document.getElementById('halaman-beranda')?.classList.remove('d-none');
    });

    // 2. Jalankan Interval Monitoring
    setInterval(monitorKesehatanESP, 5000);
    setInterval(ambilStatusSync, 30000);
    
});
let konfig = {
    global: { dev: "Jam-ESP", user: "admin", mk: 0, sk: 500, dg: 4 },
    ultra: { on: false, fMin: 22000, fMax: 45000 },
    sholat: Array(6).fill({ mP: 5, iEP: 1, cDown: 30 }),
    tanggal: { a: true, ls: 60, ll: 5 },
    skor: {},
    efek: Array(15).fill({ id: 1, m: 1, s: 150, b: 200, p: [0] }),
    
};
// Panggil fungsi ini di dalam window.onload atau saat tab WiFi dibuka
function muatDaftarWifi() {
    const selectSsid = document.getElementById('st-ssid');
    
    fetch('/scan-wifi')
    .then(res => res.json())
    .then(data => {
        selectSsid.innerHTML = '<option value="">-- Pilih WiFi --</option>';
        if(data.length === 0) {
            selectSsid.innerHTML = '<option value="">WiFi tidak ditemukan</option>';
            return;
        }
        data.forEach(wifi => {
            let opt = document.createElement('option');
            opt.value = wifi.s;
            opt.innerHTML = `${wifi.s} (${wifi.r} dBm)`;
            selectSsid.appendChild(opt);
        });
    })
    .catch(err => {
        selectSsid.innerHTML = '<option value="">Gagal memindai</option>';
    });
}

// Tambahkan pemanggilan fungsi ini di logika redirect yang kita buat tadi
// Jika status === "0" (Mode AP), langsung panggil muatDaftarWifi()
function cekStatusESP() {
    fetch('/status-wifi')
    .then(res => res.text())
    .then(status => {
        const loginSection = document.getElementById('halaman-konek-wifi');
        

        if (status === "1") { 
            // JIKA SUDAH KONEK WIFI RUMAH:
            if(loginSection) loginSection.remove(); // Hapus login dari DOM
            
            
            // Langsung tampilkan halaman utama jam
            document.getElementById('halaman-konek-wifi').classList.remove('d-none');
        } else {
            // JIKA MASIH MODE AP:
            // Biarkan halaman login dan tab wifi muncul agar bisa setting
            console.log("Mode AP Aktif: Silakan setting WiFi.");
        }
    });
}

function simpanWifiRumah() {
    const ssid = document.getElementById('st-ssid').value;
    const pass = document.getElementById('st-pass').value;

    if (!ssid) return alert("Pilih WiFi dulu, Bos!");

    let fd = new FormData();
    fd.append("ssid", ssid);
    fd.append("pass", pass);

    fetch('/set-wifi-st', { method: 'POST', body: fd })
    .then(res => res.text())
    .then(hasil => {
        if (hasil === "OK") alert("Berhasil! ESP32 akan restart dan mencoba konek.");
    });
}
function muatKonfigurasi() {
    fetch('/get-config')
    .then(res => res.json())
    .then(data => {
        window.konfig = data; // Simpan ke variabel global
        updateUI();           // Isi semua form secara otomatis
        console.log("Konfigurasi dimuat!");
    })
    .catch(err => console.log("Gagal ambil config:", err));
}

function updateUI() {
    if (!konfig || !konfig.global) return;

    // Helper untuk set value aman (biar gak error kalau ID gak ada)
    const setV = (id, val) => {
        const el = document.getElementById(id);
        if (el) {
            if (el.type === 'checkbox') el.checked = val;
            else el.value = val;
        }
    };

    // --- TAB SHOLAT ---
    const s = konfig.sholat ? konfig.sholat[0] : {}; 
    setV('menitPeringatan', s.mP || 5);
    setV('idEfekPeringatan', s.iEP || 1);
    setV('hitungMundur', s.cDown || 30);
    setV('idEfekAdzan', s.iEA || 1);
    setV('durasiAdzan', s.durA || 10);
    setV('menitHemat', s.mH || 10);
    setV('kecerahanHemat', s.brH || 10);

    // --- TAB VISUAL (Sesuaikan Key dengan JSON Bos) ---
    if (konfig.tanggal) {
        setV('tgl_aktif', konfig.tanggal.a);
        setV('tgl_setiap', konfig.tanggal.ls);
        setV('tgl_lama', konfig.tanggal.ll);
        setV('tgl_format', konfig.tanggal.ft);
        setV('tgl_efek', konfig.tanggal.id);
        setV('tgl_speed_dp', konfig.tanggal.md);
        setV('tgl_dp', konfig.tanggal.dp);
    }

    // --- TAB SISTEM ---
    setV('dev', konfig.global.dev);
    setV('user', konfig.global.user);
    setV('GApps', konfig.global.gas);
    setV('jumlahDigit', konfig.global.dg);
    
    // --- TAB ULTRA ---
    if (konfig.ultra) {
        setV('u_on', konfig.ultra.on);
        setV('u_fMin', konfig.ultra.fMin);
        setV('u_fMax', konfig.ultra.fMax);
        setV('u_interval', konfig.ultra.interval);
        setV('u_malam', konfig.ultra.malam);
    }

    // Update Header
    const devName = document.getElementById('devName');
    if (devName) devName.innerText = konfig.global.dev.toUpperCase();
}

// 2. MENYIMPAN DATA (Sesuai Struktur simpanConfig)
function simpanLaci() {
    try {
        // Pastikan objek dasar ada agar tidak 'undefined'
        if (!window.konfig) window.konfig = {};
        if (!konfig.global) konfig.global = {};

        // 1. Update data GLOBAL (Gunakan ? agar tidak crash jika ID tidak ada di HTML)
        konfig.global.mk = parseInt(document.getElementById('modeKedip')?.value || 0);
        konfig.global.sk = parseInt(document.getElementById('speedKedip')?.value || 500);
        konfig.global.ls = parseInt(document.getElementById('loop_setiap')?.value || 60);
        konfig.global.ft = parseInt(document.getElementById('fmt_tgl')?.value || 0);
        konfig.global.tz = parseInt(document.getElementById('tz')?.value || 7);
        konfig.global.dg = parseInt(document.getElementById('jumlahDigit')?.value || 4);
        konfig.global.dev = document.getElementById('dev')?.value || "Jam-ESP";
        konfig.global.user = document.getElementById('user')?.value || "admin";
        konfig.global.pass = document.getElementById('pass')?.value || ""; 
        konfig.global.gas = document.getElementById('GApps')?.value || "";

        // 2. Update data TANGGAL
        konfig.tanggal = {
            "a":  document.getElementById('tgl_aktif')?.checked || false,
            "ls": parseInt(document.getElementById('tgl_setiap')?.value || 60),
            "ll": parseInt(document.getElementById('tgl_lama')?.value || 5),
            "ft": parseInt(document.getElementById('tgl_format')?.value || 0),
            "id": parseInt(document.getElementById('tgl_efek')?.value || 1),
            "md": parseInt(document.getElementById('tgl_speed_dp')?.value || 0),
            "dp": parseInt(document.getElementById('tgl_dp')?.value || 250)
        };

        // 3. Update data ULTRA (Usir nyamuk/hama?)
        konfig.ultra = {
            on: document.getElementById('u_on')?.checked || false,
            fMin: parseInt(document.getElementById('u_fMin')?.value || 22000),
            fMax: parseInt(document.getElementById('u_fMax')?.value || 45000),
            interval: parseInt(document.getElementById('u_interval')?.value || 5000),
            malam: document.getElementById('u_malam')?.checked || false
        };

        // 4. Update data SHOLAT (6 index)
        konfig.sholat = [];
        for(let i=0; i<6; i++) {
            konfig.sholat.push({
                mP: parseInt(document.getElementById('menitPeringatan')?.value || 5),
                iEP: parseInt(document.getElementById('idEfekPeringatan')?.value || 1),
                cDown: parseInt(document.getElementById('hitungMundur')?.value || 30),
                iEA: parseInt(document.getElementById('idEfekAdzan')?.value || 1),
                durA: parseInt(document.getElementById('durasiAdzan')?.value || 10),
                mH: parseInt(document.getElementById('menitHemat')?.value || 10),
                brH: parseInt(document.getElementById('kecerahanHemat')?.value || 10)
            });
        }

        // Jalankan fungsi simpan lokal jika ada
        simpanKeObjekLokal(); 

        console.log("Kirim ke ESP32:", konfig);

        // 5. Kirim JSON utuh
        fetch('/save-config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(konfig)
        })
        .then(res => res.text()) // Pakai text() dulu biar aman kalau balasan bukan JSON
        .then(res => {
            if(res.includes("OK")) alert("Sinkron Berhasil, Bosku!");
            else alert("Respon ESP: " + res);
        })
        .catch(err => alert("Gagal Koneksi: " + err));

    } catch (e) {
        console.error("Kesalahan Script:", e);
        console.log(simpanKeObjekLokal);
        alert("Waduh Bos, ada input yang error: " + e.message);
    }
}
// --- TAMBAHAN UNTUK EFEK PAINTER ---
let slotAktif = 0;
let tempPola = []; // Penampung frame yang sedang digambar

// 1. Inisialisasi awal (Panggil ini di window.onload)
function initEfekEditor() {
    const sel = document.getElementById('slotEfek');
    if(sel && sel.options.length === 0) {
        for(let i=0; i<15; i++) sel.options[i] = new Option("Slot Efek " + (i+1), i);
    }
    
    // Logika Klik: Ganti Abu-abu ke Merah ala Bootstrap
    document.querySelectorAll('.seg').forEach(s => {
        s.onclick = function() {
            if(this.classList.contains('bg-secondary')) {
                // NYALAKAN
                this.classList.remove('bg-secondary');
                this.classList.add('bg-danger', 'active'); // 'active' cuma buat penanda di JS
            } else {
                // MATIKAN
                this.classList.add('bg-secondary');
                this.classList.remove('bg-danger', 'active');
            }
        };
    });
}

// 2. Fungsi Load Efek dari Array konfig.efek
function loadSlot(idx) {
    slotAktif = parseInt(idx);
    
    // Jika data efek belum ada di konfig, buatkan default
    if (!konfig.efek[slotAktif]) {
        konfig.efek[slotAktif] = { id: slotAktif + 1, m: 1, s: 150, b: 200, dp: false, p: [0] };
    }

    let d = konfig.efek[slotAktif];
    
    // Update Input UI Efek
    document.getElementById('modeAnimasi').value = d.m;
    document.getElementById('speedEfek').value = d.s;
    document.getElementById('brightEfek').value = d.b;
    document.getElementById('pakaiDP').checked = d.dp;
    
    tempPola = [...d.p]; // Ambil pola segmen
    renderPreviewFrames();
    
    // Reset visual painter
    document.querySelectorAll('.seg').forEach(s => s.classList.remove('active'));
}

// 3. Simpan Frame ke List
function tambahFrame() {
    let mask = 0;
    // Kita ambil semua segmen yang punya class 'active'
    const segmenAktif = document.querySelectorAll('.seg.active');
    
    segmenAktif.forEach(s => {
        let nilaiBit = parseInt(s.getAttribute('data-bit'));
        mask |= nilaiBit;
    });

    // DEBUG: Munculkan di console browser (F12) buat mastiin angkanya ada
    console.log("Segmen Aktif:", segmenAktif.length, "Nilai Mask:", mask);

    if (mask === 0) {
        alert("Gambar dulu polanya Bos, jangan kosong!");
        return;
    }
    
    if (tempPola.length >= 8) return alert("Maks 8 frame!");
    
    tempPola.push(mask);
    simpanKeObjekLokal();
    renderPreviewFrames();
}

function simpanKeObjekLokal() {
    konfig.efek[slotAktif] = {
        id: slotAktif + 1,
        m: parseInt(document.getElementById('modeAnimasi').value),
        s: parseInt(document.getElementById('speedEfek').value),
        b: parseInt(document.getElementById('brightEfek').value),
        dp: document.getElementById('pakaiDP').checked,
        p: tempPola.length > 0 ? tempPola : [0]
    };
}

function renderPreviewFrames() {
    const con = document.getElementById('previewContainer');
    if (!con) return;

    if (tempPola.length === 0) {
        con.innerHTML = '<div class="w-100 text-center py-2 text-muted small"><i>Belum ada frame...</i></div>';
        return;
    }

    // Map array tempPola menjadi deretan kotak interaktif
    con.innerHTML = tempPola.map((p, i) => `
        <div class="frame-item" onclick="hapusFrame(${i})" title="Klik untuk hapus">
            <div class="frame-head">F${i + 1}</div>
            <div class="frame-body">0x${p.toString(16).toUpperCase().padStart(2, '0')}</div>
        </div>
    `).join('');
}

// Fungsi eksekusi hapus per index
function hapusFrame(idx) {
    tempPola.splice(idx, 1); // Buang data pada urutan ke-idx
    simpanKeObjekLokal();    // Sinkronkan ke objek konfig utama
    renderPreviewFrames();   // Gambar ulang antrian
}


function monitorKesehatanESP() {
    fetch('/status-system')
        .then(res => res.json())
        .then(data => {
            // 1. Update Uptime (Format ringkas)
            if(document.getElementById('statUptime')) {
                const up = data.uptime.split(' '); // memecah "1j 20m 15d"
                document.getElementById('statUptime').innerText = (up[0] || '0j') + " " + (up[1] || '0m');
            }

            // 2. Update RAM (Konversi ke KB Bulat)
            if(document.getElementById('valRAM')) {
                // data.heap biasanya dalam Bytes dari ESP, kita bulatkan ke KB
                let ramKB = Math.round(data.heap); 
                document.getElementById('valRAM').innerText = "RAM: " + ramKB + " Kb";
            }

            // 3. Update WiFi (RSSI)
            if(document.getElementById('valWiFi')) {
                const rssi = data.rssi;
                const elWiFi = document.getElementById('valWiFi');
                elWiFi.innerText = rssi + " dB";
                
                // Bonus: Warna teks indikator sinyal biar ketahuan kalau drop
                if(rssi > -65) elWiFi.style.color = "#20c997";      // Hijau (Stabil)
                else if(rssi > -80) elWiFi.style.color = "#ffc107"; // Kuning (Sedang)
                else elWiFi.style.color = "#ff4d4d";                // Merah (Lemah)
            }

            // 4. Update Suhu (Jika ada elemennya)
            if(document.getElementById('statTemp')) {
                document.getElementById('statTemp').innerText = data.temp.toFixed(1) + "°C";
            }
            if(document.getElementById('valFS')) {
                // data.heap biasanya dalam Bytes dari ESP, kita bulatkan ke KB
                let LittleFS = Math.round(data.fsT); 
                document.getElementById('valFS').innerText = "data: " + LittleFS + " Kb";
			}
			if(document.getElementById('valFSU')) {
                // data.heap biasanya dalam Bytes dari ESP, kita bulatkan ke KB
                let LittleFSU = Math.round(data.fsU); 
                document.getElementById('valFSU').innerText = "used: " + LittleFSU + " Kb";
			}
			if(document.getElementById('valFlash')) {
                // data.heap biasanya dalam Bytes dari ESP, kita bulatkan ke KB
                let flash = Math.round(data.flash); 
                document.getElementById('valFlash').innerText = "Flash: " + flash + "MB";
			}
        })
        .catch(err => {
            console.log("ESP Monitoring Offline");
            // Jika offline, beri tanda visual
            if(document.getElementById('valWiFi')) document.getElementById('valWiFi').innerText = "OFF";
        });
}
function ambilStatusSync() {
    fetch('/status-sync')
        .then(res => res.text())
        .then(txt => {
            if(document.getElementById('lastSyncDisplay')) {
                document.getElementById('lastSyncDisplay').innerText = "UPDATE Jadwal Sholat: " + txt;
            }
        });
}
window.onload = () => {
    muatKonfigurasi();
    initEfekEditor(); // PENTING: Supaya klik segmen & daftar slot aktif
    cekStatusESP();
};
setInterval(monitorKesehatanESP, 5000); // Cek kesehatan tiap 5 detik
setInterval(ambilStatusSync, 30000);
setInterval(muatDaftarWifi, 30000);   // Cek status sync tiap 30 detik
