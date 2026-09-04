<?php
require 'koneksi.php';

// Menggunakan TRUNCATE agar data terhapus bersih dari akar dan ID kembali ke angka 1
if (mysqli_query($conn, "TRUNCATE TABLE log_sensor")) {
    echo json_encode(["status" => "sukses"]);
} else {
    echo json_encode(["status" => "gagal", "error" => mysqli_error($conn)]);
}
?>