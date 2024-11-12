const express = require('express');
const http = require('http');
const mqtt = require('mqtt');
const socketIo = require('socket.io');
const path = require('path');

const app = express();
const server = http.createServer(app);
const port = 3000;

// Konfigurasi MQTT tanpa autentikasi
const mqttClient = mqtt.connect('mqtt://10.109.10.249', { // Ganti dengan IP broker MQTT
    port: 1883, // Ganti port menjadi 1883
});

// Inisialisasi Socket.IO
const io = socketIo(server);

// Variabel untuk timeout deteksi
let timeoutId;

// Koneksi ke broker MQTT
mqttClient.on('connect', () => {
    console.log('Connected to MQTT broker');
    mqttClient.subscribe('coba', (err) => {
        if (!err) {
            console.log('Subscribed to topic');
        }
    });
});

// Menangani pesan dari MQTT
mqttClient.on('message', (topic, message) => {
    const messageStr = message.toString();
    
    // Menggunakan regex untuk menangkap kelembapan, suhu, MAC, dan IP Address
    const humidityMatch = messageStr.match(/Kelembapan: ([\d.]+)/);
    const temperatureMatch = messageStr.match(/Suhu: ([\d.]+)/);
    const macAddressMatch = messageStr.match(/Alamat MAC: ([0-9A-Fa-f:]+)/);
    const ipAddressMatch = messageStr.match(/Alamat IP: ([\d.]+)/);

    if (humidityMatch && temperatureMatch && macAddressMatch && ipAddressMatch) {
        const data = {
            humidity: parseFloat(humidityMatch[1]),
            temperature: parseFloat(temperatureMatch[1]),
            macAddress: macAddressMatch[1],
            ipAddress: ipAddressMatch[1],
            wifiStatus: "Connected",  // Status WiFi
            mqttStatus: "Connected"    // Status MQTT
        };

        // Kirim data ke semua klien yang terhubung
        io.emit('mqttData', data);

        // Reset timeout setiap kali pesan diterima
        resetTimeout();
    } else {
        console.log('Format pesan tidak dikenali:', messageStr);
    }
});

// Fungsi untuk reset timeout
function resetTimeout() {
    if (timeoutId) clearTimeout(timeoutId);

    // Atur timeout untuk 5 detik, jika tidak ada pesan dalam 5 detik ubah status WiFi menjadi Not Connected
    timeoutId = setTimeout(() => {
        io.emit('mqttData', {
            temperature: 'N/A',
            humidity: 'N/A',
            wifiStatus: 'Not Connected',  // Jika tidak ada pesan selama 5 detik
            mqttStatus: 'Disconnected',
            macAddress: 'N/A',
            ipAddress: 'N/A'
        });
    }, 5000);  // Timeout 5 detik
}

// Menyajikan file EJS
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

app.get('/', (req, res) => {
    res.render('index', { 
        temperature: 'N/A', 
        humidity: 'N/A', 
        wifiStatus: 'Not Connected', 
        mqttStatus: 'Not Connected', 
        macAddress: 'N/A', 
        ipAddress: 'N/A' 
    }); // Nilai default
});

// Mengatur socket.io untuk menangani koneksi
io.on('connection', (socket) => {
    console.log('A user connected');

    // Emit data awal saat pengguna terhubung
    socket.emit('mqttData', { 
        temperature: 'N/A', 
        humidity: 'N/A', 
        wifiStatus: 'Not Connected', 
        mqttStatus: 'Not Connected', 
        macAddress: 'N/A', 
        ipAddress: 'N/A' 
    });
});

// Menjalankan server
server.listen(port, () => {
    console.log(`Server is running at http://localhost:${port}`);
});
