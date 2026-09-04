// ==========================================
// KONFIGURASI MQTT BROKER
// ==========================================
const MQTT_CONFIG = {
    server: "broker.emqx.io",
    port: 8084, // Menggunakan Port 8084 untuk WebSocket Secure (WSS)
    topic: "bosbesar/iot/data",
    clientId: "WebDashboard_" + Math.random().toString(16).substr(2, 8),
    useSSL: true
};