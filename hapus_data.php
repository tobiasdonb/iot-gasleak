<?php
require 'koneksi.php';

// Menangkap ID yang dikirim dari tombol Hapus di JavaScript
$data = json_decode(file_get_contents("php://input"), true);

if ($data && isset($data['id'])) {
    $id_log = $data['id'];
    
    // Query untuk menghapus baris data secara spesifik
    $query = "DELETE FROM log_sensor WHERE id_log_sensor = '$id_log'";
    
    if (mysqli_query($conn, $query)) {
        echo json_encode(["status" => "sukses"]);
    } else {
        echo json_encode(["status" => "gagal", "error" => mysqli_error($conn)]);
    }
} else {
    echo json_encode(["status" => "invalid"]);
}
?>