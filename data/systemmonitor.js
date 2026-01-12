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

// Panggil fungsi ini di dalam window.onload atau saat tab WiFi dibuka
function muatDaftarWifi() {
    const selectSsid = document.getElementById('st-ssid');
    if (!selectSsid) return;
    
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
            
            
            document.getElementById('halaman-beranda')?.classList.remove('d-none');
            document.getElementById('halaman-konek-wifi')?.classList.add('d-none');
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
