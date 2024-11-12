#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define DHTPIN 4         // Pin untuk sensor DHT
#define DHTTYPE DHT22    // Tipe sensor DHT

DHT dht(DHTPIN, DHTTYPE);

// Ganti dengan SSID dan password Wi-Fi Anda
const char* ssid = "iphone"; 
const char* password = "12345"; 

// Pengaturan MQTT dengan TLS dan autentikasi
const char* mqtt_server = "3cb050ddb0144d72af2240083624150a.s1.eu.hivemq.cloud"; //sesuaikan dengan mqtt anda
const int mqtt_port = 8883;
const char* mqtt_user = "esp32";
const char* mqtt_password = "esp32";
const char* mqtt_topic = "coba";  // Topik tempat mengirim data

WiFiClientSecure espClient;  // Menggunakan WiFiClientSecure untuk TLS
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nTerhubung ke Wi-Fi!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_wifi() {
  // Loop sampai terhubung kembali ke Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi terputus, mencoba menghubungkan kembali...");
    WiFi.begin(ssid, password);
    delay(5000);  // Tunggu 5 detik sebelum mencoba kembali
  }
  Serial.println("Terhubung kembali ke Wi-Fi!");
}

void reconnect_mqtt() {
  // Loop sampai terhubung kembali ke MQTT
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT broker...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Terhubung ke MQTT broker");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" Coba lagi dalam 5 detik.");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Atur sertifikat root CA untuk koneksi TLS
  espClient.setInsecure();  // Menggunakan setInsecure() jika tanpa sertifikat CA
  
  // Inisialisasi Wi-Fi
  setup_wifi();
  
  // Inisialisasi client MQTT dan set server broker
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  // Cek status Wi-Fi, jika terputus coba hubungkan ulang
  if (WiFi.status() != WL_CONNECTED) {
    reconnect_wifi();

    // Jika terhubung kembali, kirim status Wi-Fi ke MQTT
    String wifiStatusPayload = "WiFi terhubung kembali";
    client.publish(mqtt_topic, wifiStatusPayload.c_str());
  }

  // Cek koneksi ke MQTT dan coba hubungkan jika terputus
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  // Baca kelembapan dan suhu
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Cek jika pembacaan gagal
  if (isnan(h) || isnan(t)) {
    Serial.println("Gagal membaca dari sensor!");
    return;
  }

  // Ambil alamat MAC dan IP
  String macAddress = WiFi.macAddress();
  String ipAddress = WiFi.localIP().toString();

  // Tampilkan data pada Serial Monitor
  Serial.print("Kelembapan: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Suhu: ");
  Serial.print(t);
  Serial.print(" *C\t");
  Serial.print("Alamat MAC: ");
  Serial.print(macAddress);
  Serial.print("\tAlamat IP: ");
  Serial.println(ipAddress);

  // Kirim data ke MQTT
  String payload = "Kelembapan: " + String(h) + " %, Suhu: " + String(t) + " *C, Alamat MAC: " + macAddress + ", Alamat IP: " + ipAddress;
  client.publish(mqtt_topic, payload.c_str());

  delay(2000); // Tunggu sebelum mengirim data lagi
}
