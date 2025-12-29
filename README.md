# Line Following Firefighter Robot - Control System

Sistem kontrol dan monitoring berbasis Computer Vision untuk Robot Pemadam Api (Firefighter Robot) yang menggunakan komunikasi MQTT. Proyek ini menggabungkan logika pendeteksian api menggunakan kamera (Python/OpenCV) dengan kontrol gerak robot (Arduino/ESP).

## 📋 Fitur Utama

- **Real-time Monitoring & HUD**: Tampilan antarmuka futuristik (Heads-up Display) yang menampilkan status robot, status koneksi MQTT, dan nilai sensor.
- **Visual Fire Detection**: Menggunakan algoritma pemrosesan citra (HSV filtering, Morphological Ops) untuk mendeteksi titik api secara visual melalui kamera laptop/webcam.
- **MQTT Communication**: Komunikasi dua arah antara PC (Control Center) dan Robot via protokol MQTT.
  - **Topik Status**: `robot/status`
  - **Topik Sensor**: `robot/sensor/flame`
  - **Topik Command**: `robot/command`
- **Fail-safe Logic**: Mendeteksi jika robot offline (tidak mengirim heartbeat > 3 detik).

## 📂 Struktur Direktori

```
.
├── Tampilan.py           # Program utama (Python) untuk Control Center & Computer Vision
├── robotarduino/         # Source code untuk mikrokontroler Arduino (Motor & Sensor)
│   └── robotarduino.ino
└── robotesp/             # Source code untuk modul ESP (WiFi & MQTT Bridge)
    └── robotesp.ino
```

## 🛠️ Prasyarat (Hardware & Software)

### Hardware

1. Laptop/PC dengan Webcam.
2. Robot Line Follower dengan modul ESP8266/ESP32.
3. Sensor Api (Flame Sensor) yang terhubung ke robot.

### Software

1. **Python 3.x**
2. **MQTT Broker** (contoh: Mosquitto) yang berjalan di `localhost` atau server yang ditentukan.
3. **Library Python**:
   - `opencv-python`
   - `paho-mqtt`
   - `numpy`

## 🚀 Cara Instalasi & Penggunaan

### 1. Setup Python Environment

Install library yang dibutuhkan:

```bash
pip install opencv-python paho-mqtt numpy
```

### 2. Setup MQTT Broker

Pastikan service MQTT Broker (Mosquitto) sudah berjalan di port `1883`. Jika menggunakan IP selain `localhost`, ubah variabel `MQTT_BROKER` di dalam file `Tampilan.py`:

```python
MQTT_BROKER = "192.168.x.x"  # Sesuaikan IP
```

### 3. Setup Robot

- Upload `robotarduino.ino` ke Arduino.
- Upload `robotesp.ino` ke ESP8266/ESP32 (Pastikan SSID/Password WiFi dan IP Broker sesuai).

### 4. Menjalankan Program (Control Center)

Jalankan file `Tampilan.py`:

```bash
python Tampilan.py
```

## 🎮 Kontrol Keyboard

- **`Q`**: Keluar dari aplikasi.
- **`R`**: Reset Robot (Mengirim command `RESET` via MQTT).

## 🧠 Cara Kerja Deteksi Api

Sistem menggunakan ruang warna **HSV** untuk mengisolasi warna api (Kuning-Merah terang).

1. **Blurring**: Mengurangi noise kamera.
2. **HSV Thresholding**: Memfilter warna dengan range `Lower(0, 130, 180)` sampai `Upper(35, 255, 255)`.
3. **Erosi & Dilasi**: Membersihkan noise kecil dan mempertegas bentuk api.
4. **Contour Detection**: Mendeteksi area api. Jika area > 300px, sistem menganggap ada api dan mengirim sinyal `FIRE_DETECTED` ke robot.

## 🤝 Kontribusi

Silakan fork repository ini dan buat Pull Request untuk fitur atau perbaikan bug.
