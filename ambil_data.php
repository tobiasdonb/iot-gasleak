<?php
require 'koneksi.php';

$query = "SELECT * FROM log_sensor ORDER BY waktu_catat DESC LIMIT 50";
$result = mysqli_query($conn, $query);

$data_riwayat = [];

while ($row = mysqli_fetch_assoc($result)) {
    // 1. Gas
    $status_gas = "AMAN"; $badge_gas = "bg-success";
    if ($row['kadar_gas'] > 15) { $status_gas = "BAHAYA (" . $row['kadar_gas'] . "%)"; $badge_gas = "bg-danger"; }
    elseif ($row['kadar_gas'] >= 5 && $row['kadar_gas'] <= 15) { $status_gas = "WASPADA (" . $row['kadar_gas'] . "%)"; $badge_gas = "bg-warning text-dark"; }

    // 2. Api
    $status_api = ($row['status_titik_api'] == 1) ? "TERDETEKSI" : "AMAN";
    $badge_api = ($row['status_titik_api'] == 1) ? "bg-danger" : "bg-success";

    // 3. Kipas
    $kipasAktif = ($row['kadar_gas'] >= 5 && $row['status_titik_api'] == 0);
    $status_kipas = $kipasAktif ? "AKTIF / MENYALA" : "STANDBY";
    $badge_kipas = $kipasAktif ? "bg-warning text-dark" : "bg-secondary";

    // 4. Buzzer (Tempo Cepat / Lambat)
    $buzzerCepat = ($row['kadar_gas'] > 15 || $row['status_titik_api'] == 1);
    $buzzerLambat = ($row['kadar_gas'] > 10 && $row['kadar_gas'] <= 15 && $row['status_titik_api'] == 0);
    
    if ($buzzerCepat) {
        $status_buzzer = "BERBUNYI CEPAT"; $badge_buzzer = "bg-danger";
    } elseif ($buzzerLambat) {
        $status_buzzer = "BERBUNYI LAMBAT"; $badge_buzzer = "bg-warning text-dark";
    } else {
        $status_buzzer = "STANDBY"; $badge_buzzer = "bg-success";
    }

    $waktu_format = date('d M Y, H:i:s', strtotime($row['waktu_catat']));

    $data_riwayat[] = [
        'id' => $row['id_log_sensor'],
        'waktu' => $waktu_format,
        'suhu' => $row['suhu_ruangan'] . " °C",
        'lembab' => isset($row['kelembapan']) ? $row['kelembapan'] . " %" : "- %", // Mengambil data Kelembapan
        'gas_text' => $status_gas,
        'gas_badge' => $badge_gas,
        'api_text' => $status_api,
        'api_badge' => $badge_api,
        'kipas_text' => $status_kipas,
        'kipas_badge' => $badge_kipas,
        'buzzer_text' => $status_buzzer,
        'buzzer_badge' => $badge_buzzer
    ];
}

echo json_encode($data_riwayat);
?>