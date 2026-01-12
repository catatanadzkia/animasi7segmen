document.addEventListener('DOMContentLoaded', () => {
    loadFileList();
});

// Mengambil daftar file dari ESP32
async function loadFileList() {
    try {
        const res = await fetch('/api/list');
        const files = await res.json();
        const tbody = document.querySelector('#fileTable tbody');
        tbody.innerHTML = '';
        
        files.forEach(file => {
            tbody.innerHTML += `
                <tr>
                    <td>${file.n}</td>
                    <td>${(file.s / 1024).toFixed(2)} KB</td>
                    <td><button class="btn btn-red" onclick="deleteFile('${file.n}')">Hapus</button></td>
                </tr>`;
        });
    } catch (e) {
        console.error("Gagal memuat daftar file", e);
    }
}

// Menangani proses upload file ke ESP32
async function uploadFile() {
    const fileInput = document.getElementById('fileInput');
    if (fileInput.files.length === 0) return alert("Silakan pilih file terlebih dahulu!");

    const formData = new FormData();
    formData.append("data", fileInput.files[0], fileInput.files[0].name);

    try {
        const res = await fetch('/api/upload', {
            method: 'POST',
            body: formData
        });
        if (res.ok) {
            alert("Upload Berhasil!");
            fileInput.value = ''; // Reset input
            loadFileList(); // Segarkan tabel
        }
    } catch (e) {
        alert("Terjadi kesalahan saat upload!");
    }
}

// Menangani penghapusan file
async function deleteFile(name) {
    if (!confirm(`Yakin ingin menghapus file "${name}"?`)) return;
    
    try {
        const res = await fetch(`/api/delete?file=${name}`);
        if (res.ok) {
            loadFileList();
        }
    } catch (e) {
        alert("Gagal menghapus file!");
    }
}
