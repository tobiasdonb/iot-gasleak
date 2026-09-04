<?php
require 'koneksi.php';

// Menangkap data JSON yang dikirim oleh JavaScript
$data = json_decode(file_get_contents("php://input"), true);

if ($data) {
    $id_perangkat = 1;
    $gas = $data['gas'];
    $suhu = $data['suhu'];
    $api = $data['api'];
    
    // Menangkap data lembab dari JS
    $lembab = isset($data['lembab']) ? $data['lembab'] : 0; 
    
    // Mengatur zona waktu (WITA)
    date_default_timezone_set('Asia/Makassar'); 
    $waktu = date('Y-m-d H:i:s'); 
    
    // Format Waktu untuk Telegram
    $hari_inggris = date('l');
    $daftar_hari = ['Sunday' => 'Minggu', 'Monday' => 'Senin', 'Tuesday' => 'Selasa', 'Wednesday' => 'Rabu', 'Thursday' => 'Kamis', 'Friday' => 'Jumat', 'Saturday' => 'Sabtu'];
    $hari_ini = $daftar_hari[$hari_inggris];
    $tanggal_indo = date('d F Y');
    $jam_indo = date('H:i') . ' WITA';

    // ==========================================
    // LOGIKA PENENTUAN STATUS
    // ==========================================
    $status_gas = "AMAN ✅";
    if ($gas > 15) { $status_gas = "BAHAYA ($gas%) 🔴"; } 
    elseif ($gas >= 5 && $gas <= 15) { $status_gas = "WASPADA ($gas%) 🟡"; }

    $status_api = ($api == 1) ? "TERDETEKSI 🔥" : "AMAN ✅";
    
    // Aturan Kipas
    $kipasAktif = ($gas >= 5 && $api == 0);
    $status_kipas = $kipasAktif ? "AKTIF / MENYALA 🌀💨" : "STANDBY ⏸";

    // Aturan Buzzer
    $buzzerCepat = ($gas > 15 || $api == 1);
    $buzzerLambat = ($gas > 10 && $gas <= 15 && $api == 0);
    
    if ($buzzerCepat) {
        $status_buzzer = "BERBUNYI CEPAT 🚨🚨";
    } elseif ($buzzerLambat) {
        $status_buzzer = "BERBUNYI LAMBAT 🔔";
    } else {
        $status_buzzer = "STANDBY 🔇";
    }

    // Header Pesan Dinamis
    $tingkat_bahaya = ($gas > 15 || $api == 1) ? "🔴 STATUS: KRITIS / BAHAYA 🔴" : "🟡 STATUS: WASPADA 🟡";

    // ==========================================
    // KIRIM PESAN KE TELEGRAM
    // ==========================================
    $env = is_file(__DIR__ . '/.env') ? parse_ini_file(__DIR__ . '/.env') : [];
    $token_bot = $env['TELEGRAM_BOT_TOKEN'] ?? getenv('TELEGRAM_BOT_TOKEN');
    $chat_id = "-1004376908090";

    $pesan_telegram = "🚨 *LAPORAN DARURAT IoT* 🚨\n\n";
    $pesan_telegram .= "Halo *BOS BESAR SEKALI*! Sistem mendeteksi anomali pada lingkungan pantau:\n\n";
    $pesan_telegram .= "*$tingkat_bahaya*\n\n";
    $pesan_telegram .= "🗓️ *Waktu Kejadian:*\n";
    $pesan_telegram .= "├ Hari: $hari_ini, $tanggal_indo\n";
    $pesan_telegram .= "└ Jam: $jam_indo\n\n";
    $pesan_telegram .= "📊 *Pembacaan Sensor:*\n";
    $pesan_telegram .= "├ 💨 Gas LEL: $status_gas\n";
    $pesan_telegram .= "├ 🌡️ Suhu: $suhu °C\n";
    $pesan_telegram .= "├ 💧 Lembab: $lembab %\n"; // Tambahan Info Lembab di Telegram
    $pesan_telegram .= "└ 🔥 Titik Api: $status_api\n\n";
    $pesan_telegram .= "⚙️ *Respon Aktuator:*\n";
    $pesan_telegram .= "├ 🌀 Exhaust Fan: $status_kipas\n";
    $pesan_telegram .= "└ 🔊 Buzzer Alarm: $status_buzzer\n\n";
    $pesan_telegram .= "👁️‍🗨️ _Mohon segera akses ESP32-CAM untuk verifikasi visual di lokasi!_ 🏃‍♂️💨";

    // Proses pengiriman via API Telegram jika token tersedia di environment.
    if ($token_bot) {
        $url_telegram = "https://api.telegram.org/bot" . $token_bot . "/sendMessage?chat_id=" . $chat_id . "&text=" . urlencode($pesan_telegram) . "&parse_mode=Markdown";

        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $url_telegram);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, 5);
        curl_exec($ch);
        curl_close($ch);
    }

    // ==========================================
    // SIMPAN KE DATABASE (Tabel log_sensor)
    // ==========================================
    $query = "INSERT INTO log_sensor (id_perangkat, waktu_catat, kadar_gas, suhu_ruangan, kelembapan, status_titik_api) 
              VALUES ('$id_perangkat', '$waktu', '$gas', '$suhu', '$lembab', '$api')";

    if (mysqli_query($conn, $query)) {
        echo json_encode(["status" => "sukses menyimpan dan kirim telegram"]);
    } else {
        echo json_encode(["status" => "gagal database", "error" => mysqli_error($conn)]);
    }
} else {
    echo json_encode(["status" => "tidak ada data masuk"]);
}
?>