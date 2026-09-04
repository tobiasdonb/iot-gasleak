// ==========================================
// PENGATURAN JAM OTOMATIS
// ==========================================
function updateClock() {
    const now = new Date();
    const timeString = now.toLocaleTimeString('id-ID', { hour: '2-digit', minute: '2-digit', second: '2-digit' }).replace(/\./g, '.');
    const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
    const dateString = now.toLocaleDateString('id-ID', options);
    
    document.getElementById('clock').innerText = timeString;
    document.getElementById('date').innerText = dateString;
}
setInterval(updateClock, 1000);
updateClock();

// ==========================================
// INISIALISASI GRAFIK CHART.JS
// ==========================================
const ctx = document.getElementById('gasChart').getContext('2d');
const chartGas = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [], 
        datasets: [{
            label: 'Level Gas (%)',
            data: [], 
            borderColor: '#4f46e5',
            backgroundColor: 'rgba(79, 70, 229, 0.1)',
            borderWidth: 3, fill: true, tension: 0.4, 
            pointRadius: 0, pointHoverRadius: 0,
            borderJoinStyle: 'round', borderCapStyle: 'round'
        }]
    },
    options: {
        responsive: true, maintainAspectRatio: false,
        animation: { duration: 800, easing: 'linear' }, 
        scales: {
            y: { min: 0, max: 100 },
            x: { grid: { display: false }, ticks: { maxTicksLimit: 15 }, offset: false }
        },
        plugins: { legend: { display: false } },
        layout: { padding: { right: 10 } }
    }
});

// ==========================================
// FUNGSI ANIMASI ANGKA BERJALAN 
// ==========================================
let prevGas = 0, prevSuhu = 0.0, prevLembab = 0.0;

function animateValue(id, start, end, duration, suffix) {
    const obj = document.getElementById(id);
    if (!obj) return;
    let startTimestamp = null;
    
    const step = (timestamp) => {
        if (!startTimestamp) startTimestamp = timestamp;
        const progress = Math.min((timestamp - startTimestamp) / duration, 1);
        const easeOut = 1 - Math.pow(1 - progress, 3);
        const currentVal = start + (end - start) * easeOut;

        if (id === "valGas") {
            obj.innerText = Math.round(currentVal) + suffix;
        } else {
            obj.innerText = currentVal.toFixed(1) + suffix;
        }

        if (progress < 1) window.requestAnimationFrame(step);
    };
    window.requestAnimationFrame(step);
}

// ==========================================
// LOGIKA KONEKSI MQTT
// ==========================================
const client = new Paho.MQTT.Client(MQTT_CONFIG.server, MQTT_CONFIG.port, MQTT_CONFIG.clientId);
client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

client.connect({ useSSL: MQTT_CONFIG.useSSL, onSuccess: onConnect, onFailure: onFailure });

function onConnect() {
    const dot = document.getElementById("koneksiDot"), txt = document.getElementById("koneksiText");
    if(dot) dot.className = "status-dot connected";
    if(txt) txt.innerText = "Terhubung";
    client.subscribe(MQTT_CONFIG.topic);
}

function onFailure(error) {
    const txt = document.getElementById("koneksiText"), dot = document.getElementById("koneksiDot");
    if(txt) txt.innerText = "Koneksi Gagal!";
    if(dot) dot.className = "status-dot";
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        const txt = document.getElementById("koneksiText"), dot = document.getElementById("koneksiDot");
        if(txt) txt.innerText = "Koneksi Terputus...";
        if(dot) dot.className = "status-dot";
    }
}

// ==========================================
// OLAH DATA MASUK & UPDATE UI/GRAFIK
// ==========================================
let dataCount = 0;

function onMessageArrived(message) {
    try {
        const data = JSON.parse(message.payloadString);
        
        let tingkatGas = data.gas !== undefined ? data.gas : 0;
        let adaApi = data.api !== undefined ? data.api : 0;

        // 1. Update Gas
        if (data.gas !== undefined) {
            const gasPill = document.getElementById("pillGas");
            animateValue("valGas", prevGas, data.gas, 800, " %");
            prevGas = data.gas; 
            if (gasPill) {
                if (data.gas > 15) {
                    gasPill.innerText = "BAHAYA"; gasPill.className = "card-pill pill-bahaya";
                } else if (data.gas >= 5 && data.gas <= 15) {
                    gasPill.innerText = "WASPADA"; gasPill.className = "card-pill pill-waspada";
                } else {
                    gasPill.innerText = "AMAN"; gasPill.className = "card-pill pill-aman";
                }
            }
        }

        // 2. Update Api
        if (data.api !== undefined) {
            const apiEl = document.getElementById("valApi");
            const apiPill = document.getElementById("pillApi");
            if (apiEl) {
                if (data.api === 1) {
                    apiEl.innerText = "TERDETEKSI"; apiEl.style.color = "#dc2626";
                    if(apiPill) { apiPill.innerText = "BAHAYA"; apiPill.className = "card-pill pill-bahaya"; }
                } else {
                    apiEl.innerText = "-"; apiEl.style.color = "#1e293b";
                    if(apiPill) { apiPill.innerText = "AMAN"; apiPill.className = "card-pill pill-aman"; }
                }
            }
        }

        // 3. LOGIKA KIPAS & BUZZER
        const kipasEl = document.getElementById("valKipas"), kipasPill = document.getElementById("pillKipas");
        const buzzerEl = document.getElementById("valBuzzer"), buzzerPill = document.getElementById("pillBuzzer");

        let kipasAktif = (tingkatGas >= 5 && adaApi === 0);
        let buzzerCepat = (tingkatGas > 15 || adaApi === 1);
        let buzzerLambat = (tingkatGas > 10 && tingkatGas <= 15 && adaApi === 0);

        if (kipasEl) {
            if (kipasAktif) {
                kipasEl.innerText = "AKTIF";
                if(kipasPill) { kipasPill.innerText = "MENYALA"; kipasPill.className = "card-pill pill-waspada"; }
            } else {
                kipasEl.innerText = "-";
                if(kipasPill) { kipasPill.innerText = "STANDBY"; kipasPill.className = "card-pill"; }
            }
        }

        if (buzzerEl) {
            if (buzzerCepat) {
                buzzerEl.innerText = "CEPAT"; buzzerEl.style.color = "#dc2626";
                if(buzzerPill) { buzzerPill.innerText = "BERBUNYI"; buzzerPill.className = "card-pill pill-bahaya"; }
            } else if (buzzerLambat) {
                buzzerEl.innerText = "LAMBAT"; buzzerEl.style.color = "#d97706";
                if(buzzerPill) { buzzerPill.innerText = "BERBUNYI"; buzzerPill.className = "card-pill pill-waspada"; }
            } else {
                buzzerEl.innerText = "-"; buzzerEl.style.color = "#1e293b";
                if(buzzerPill) { buzzerPill.innerText = "STANDBY"; buzzerPill.className = "card-pill pill-aman"; }
            }
        }
        
        // 4. Update Suhu
        if (data.suhu !== undefined) {
            const suhuPill = document.getElementById("pillSuhu");
            animateValue("valSuhu", prevSuhu, data.suhu, 800, " °C");
            prevSuhu = data.suhu;
            if (suhuPill) {
                if (data.suhu > 35) { suhuPill.innerText = "PANAS"; suhuPill.className = "card-pill pill-waspada"; } 
                else { suhuPill.innerText = "NORMAL"; suhuPill.className = "card-pill pill-aman"; }
            }
        }

        // 5. Update Kelembapan
        if (data.lembab !== undefined) {
            const lembabPill = document.getElementById("pillLembab");
            animateValue("valLembab", prevLembab, data.lembab, 800, " %");
            prevLembab = data.lembab;
            if (lembabPill) {
                if (data.lembab < 30 || data.lembab > 70) { lembabPill.innerText = "TIDAK IDEAL"; lembabPill.className = "card-pill pill-waspada"; } 
                else { lembabPill.innerText = "NORMAL"; lembabPill.className = "card-pill pill-aman"; }
            }
        }

        // 6. Update Grafik
        if (data.gas !== undefined) {
            const now = new Date();
            const timeLabel = now.toLocaleTimeString('id-ID', { hour: '2-digit', minute: '2-digit', second: '2-digit' }).replace(/\./g, ':');
            
            chartGas.data.labels.push(timeLabel);
            chartGas.data.datasets[0].data.push(data.gas);
            dataCount++;
            
            const maxTampil = 15; 
            if (dataCount > maxTampil) {
                chartGas.options.scales.x.min = dataCount - maxTampil;
                chartGas.options.scales.x.max = dataCount - 1;
            }

            if (dataCount > 100) {
                chartGas.data.labels.splice(0, 50);
                chartGas.data.datasets[0].data.splice(0, 50);
                dataCount -= 50; 
                chartGas.options.scales.x.min = dataCount - maxTampil;
                chartGas.options.scales.x.max = dataCount - 1;
                chartGas.update('none'); 
            } else {
                chartGas.update(); 
            }
        }

        // ========================================================
        // 7. Simpan ke Database & Telegram (SISTEM TEMBAK CEPAT & BYPASS API)
        // ========================================================
        
        // ATURAN BESI: Suhu dan Kelembapan diabaikan total sebagai pemicu alarm.
        // HANYA AKAN MENGIRIM JIKA: Gas >= 5 ATAU Api terdeteksi.
        const kondisiBahayaUtama = (data.gas >= 5) || (data.api === 1);
        const waktuSekarang = Date.now();

        // 🔥 FITUR BARU: BYPASS DARURAT API 🔥
        // Jika sebelumnya tidak ada api, lalu tiba-tiba ada api, paksa kirim detik itu juga!
        let adaApiMendadak = false;
        if (data.api === 1 && window.statusApiSebelumnya === 0) {
            adaApiMendadak = true; // Mode darurat aktif!
        }
        window.statusApiSebelumnya = data.api; // Simpan status untuk perbandingan berikutnya

        // CEK JEDA WAKTU (3 Detik) ATAU ADA API MENDADAK
        if (kondisiBahayaUtama && (waktuSekarang - window.waktuTerakhirKirim > 3000 || !window.waktuTerakhirKirim || adaApiMendadak)) {
            
            window.waktuTerakhirKirim = waktuSekarang; // Reset timer

            fetch('simpan_data.php', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ gas: data.gas, suhu: data.suhu, lembab: data.lembab, api: data.api })
            })
            .then(response => response.json())
            .then(res => {
                console.log("Status Telegram & DB:", res.status);
                // Refresh tabel otomatis agar data masuk terus
                muatRiwayatData();
            })
            .catch(err => console.error("Gagal simpan ke DB:", err));
        }

    } catch (e) {
        console.error("Format JSON tidak valid: ", e);
    }
}

// ==========================================
// 8. LOGIKA TAB NAVIGASI, AMBIL & HAPUS RIWAYAT
// ==========================================
const btnDashboard = document.getElementById('btnDashboard');
const btnRiwayat = document.getElementById('btnRiwayat');
const viewDashboard = document.getElementById('viewDashboard');
const viewRiwayat = document.getElementById('viewRiwayat');
const tbodyRiwayat = document.getElementById('tbodyRiwayat');

if (btnDashboard && btnRiwayat && viewDashboard && viewRiwayat) {
    btnDashboard.addEventListener('click', () => {
        btnDashboard.classList.add('active');
        btnRiwayat.classList.remove('active');
        viewDashboard.style.display = 'block';
        viewRiwayat.style.display = 'none';
    });

    btnRiwayat.addEventListener('click', () => {
        btnRiwayat.classList.add('active');
        btnDashboard.classList.remove('active');
        viewDashboard.style.display = 'none';
        viewRiwayat.style.display = 'block';
        muatRiwayatData();
    });
}

function muatRiwayatData() {
    if (!tbodyRiwayat) return;
    tbodyRiwayat.innerHTML = '<tr><td colspan="8" class="text-center py-4">Memuat data dari database...</td></tr>';
    
    fetch('ambil_data.php')
        .then(response => response.json())
        .then(data => {
            tbodyRiwayat.innerHTML = ''; 
            
            if(data.length === 0) {
                tbodyRiwayat.innerHTML = '<tr><td colspan="8" class="text-center py-4 text-muted">Belum ada riwayat anomali (Data Aman).</td></tr>';
                return;
            }
            
            data.forEach((row) => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td class="fw-bold text-secondary">${row.waktu}</td>
                    <td class="fw-bold text-info">${row.suhu}</td>
                    <td class="fw-bold text-primary">${row.lembab}</td>
                    <td><span class="badge ${row.gas_badge} px-3 py-2">${row.gas_text}</span></td>
                    <td><span class="badge ${row.api_badge} px-3 py-2">${row.api_text}</span></td>
                    <td><span class="badge ${row.kipas_badge} px-3 py-2">${row.kipas_text}</span></td>
                    <td><span class="badge ${row.buzzer_badge} px-3 py-2">${row.buzzer_text}</span></td>
                    <td><button class="btn btn-sm btn-danger fw-bold shadow-sm" onclick="hapusRiwayat(${row.id})">Hapus</button></td>
                `;
                tbodyRiwayat.appendChild(tr);
            });
        })
        .catch(err => {
            console.error("Gagal mengambil data:", err);
            tbodyRiwayat.innerHTML = '<tr><td colspan="8" class="text-center text-danger py-4">Gagal memuat data.</td></tr>';
        });
}

function hapusRiwayat(id) {
    if(confirm("Apakah Bos Besar yakin ingin menghapus data riwayat ini?")) {
        fetch('hapus_data.php', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: id })
        })
        .then(response => response.json())
        .then(data => {
            if(data.status === 'sukses') {
                muatRiwayatData(); 
            } else {
                alert("Gagal menghapus data: " + data.error);
            }
        })
        .catch(err => console.error("Koneksi hapus gagal:", err));
    }
}

// FUNGSI BARU: Hapus Semua Riwayat
function hapusSemuaRiwayat() {
    if(confirm("PERINGATAN: yakin ingin MENGHAPUS SEMUA data riwayat secara permanen?")) {
        fetch('hapus_semua.php')
        .then(response => response.json())
        .then(data => {
            if(data.status === 'sukses') {
                alert("Seluruh riwayat berhasil dibersihkan!");
                muatRiwayatData(); 
            } else {
                alert("Gagal menghapus data: " + data.error);
            }
        })
        .catch(err => console.error("Koneksi hapus semua gagal:", err));
    }
}