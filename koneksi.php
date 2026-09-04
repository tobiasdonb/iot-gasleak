<?php
// Konfigurasi Database bawaan XAMPP
$host = "localhost";
$user = "root";
$pass = ""; 
$db   = "db_iot_gas"; // Nama database 

// Membuat koneksi
$conn = mysqli_connect($host, $user, $pass, $db);

// Cek koneksi
if (!$conn) {
    die("Koneksi Database Gagal: " . mysqli_connect_error());
}
?>