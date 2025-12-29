import cv2
import paho.mqtt.client as mqtt
import numpy as np
import time

# ================= KONFIGURASI =================
MQTT_BROKER = "localhost"  # Ganti IP jika beda laptop
MQTT_PORT = 1883
WINDOW_NAME = "ROBOT COMMAND CENTER"
TARGET_WIDTH = 1280
TARGET_HEIGHT = 720

# Topik
TOPIC_STATUS = "robot/status"
TOPIC_SENSOR = "robot/sensor/flame"
TOPIC_COMMAND = "robot/command"

# ================= VARIABEL STATUS =================
broker_connected = False
robot_last_seen = 0
robot_online = False
robot_current_status = "MENUNGGU..."
sensor_value = "0"

visual_fire_detected = False
last_visual_state = False

# ================= MQTT HANDLERS =================
def on_connect(client, userdata, flags, rc):
    global broker_connected
    if rc == 0:
        broker_connected = True
        print("✅ PYTHON: Terhubung ke Broker MQTT!")
        client.subscribe(TOPIC_STATUS)
        client.subscribe(TOPIC_SENSOR)
    else:
        print(f"❌ PYTHON: Gagal Konek Broker (Code: {rc})")

def on_message(client, userdata, msg):
    global robot_last_seen, robot_online, robot_current_status, sensor_value
    
    payload = msg.payload.decode()
    topic = msg.topic
    
    robot_last_seen = time.time()
    robot_online = True
    
    if topic == TOPIC_STATUS:
        robot_current_status = payload
    elif topic == TOPIC_SENSOR:
        sensor_value = payload

# ================= CV & VISUAL UTILS =================
def draw_modern_hud(frame):
    global robot_online
    
    # 1. Base Layer & Overlay
    height, width, _ = frame.shape
    overlay = frame.copy()
    
    # --- SIDE PANEL (KIRI) ---
    panel_width = 320
    cv2.rectangle(overlay, (0, 0), (panel_width, height), (20, 20, 20), -1)
    
    # --- HEADER PANEL ---
    cv2.rectangle(overlay, (0, 0), (width, 50), (10, 10, 10), -1)
    
    # Alpha Blending
    alpha = 0.7
    cv2.addWeighted(overlay, alpha, frame, 1 - alpha, 0, frame)
    
    # --- GARIS DEKORATIF ---
    cv2.line(frame, (panel_width, 0), (panel_width, height), (0, 255, 255), 2)
    cv2.line(frame, (0, 50), (width, 50), (0, 255, 255), 1)
    
    # --- HEADER TEXT ---
    cv2.putText(frame, "CONTROL SYSTEM - BATAGOR REDLOSS ROBOT", (panel_width + 20, 35), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

    # --- CROSSHAIR (TARGET) ---
    center_x, center_y = width // 2 + (panel_width // 2), height // 2
    cv2.line(frame, (center_x - 20, center_y), (center_x + 20, center_y), (0, 255, 0), 1)
    cv2.line(frame, (center_x, center_y - 20), (center_x, center_y + 20), (0, 255, 0), 1)

    # --- LOGIC STATUS ---
    if time.time() - robot_last_seen > 3.0:
        robot_online = False

    col_broker = (0, 255, 0) if broker_connected else (0, 0, 255)
    
    if robot_online:
        col_robot = (0, 255, 0)
        status_txt = "ONLINE"
    else:
        col_robot = (0, 0, 255)
        status_txt = "OFFLINE"

    # --- ISI SIDE PANEL ---
    x_pad = 20
    y_start = 100
    line_height = 40
    
    # 1. Broker Status
    cv2.putText(frame, "MQTT BROKER", (x_pad, y_start), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
    cv2.circle(frame, (x_pad + 150, y_start - 5), 8, col_broker, -1)
    
    # 2. Robot Connection
    y_start += line_height * 2
    cv2.putText(frame, "ROBOT CONNECTION", (x_pad, y_start), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
    cv2.putText(frame, status_txt, (x_pad, y_start + 30), cv2.FONT_HERSHEY_TRIPLEX, 0.8, col_robot, 1)

    # 3. Robot Mode
    y_start += int(line_height * 2.5) 
    cv2.putText(frame, "CURRENT MODE", (x_pad, y_start), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
    cv2.putText(frame, str(robot_current_status), (x_pad, y_start + 35), cv2.FONT_HERSHEY_TRIPLEX, 0.7, (0, 255, 255), 1)

    # 4. Sensor Value
    y_start += int(line_height * 2.5)
    
    try:
        val_int = int(sensor_value)
    except:
        val_int = 4095
    
    bar_width = int(((4095 - val_int) / 4095) * 250)
    if bar_width < 0: bar_width = 0
    
    cv2.putText(frame, f"FLAME SENSOR: {sensor_value}", (x_pad, y_start), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (200, 200, 200), 1)
    
    cv2.rectangle(frame, (x_pad, y_start + 15), (x_pad + 250, y_start + 35), (50, 50, 50), -1)
    bar_color = (0, 255, 0)
    if val_int < 1000: bar_color = (0, 0, 255) 
    elif val_int < 2500: bar_color = (0, 165, 255) 
    
    cv2.rectangle(frame, (x_pad, y_start + 15), (x_pad + bar_width, y_start + 35), bar_color, -1)

    # --- FOOTER INSTRUCTIONS ---
    cv2.putText(frame, "[Q] QUIT   [R] RESET ROBOT", (x_pad, height - 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (150, 150, 150), 1)
    
    return frame

# ================= BAGIAN PENTING: ALGORITMA DETEKSI API =================
def process_frame(frame):
    global visual_fire_detected
    
    # 1. Blur untuk mengurangi noise kamera
    blur = cv2.GaussianBlur(frame, (21, 21), 0)
    
    # 2. Convert ke HSV
    hsv = cv2.cvtColor(blur, cv2.COLOR_BGR2HSV)
    
    # --- LOGIKA STRICT (KETAT) UNTUK API ---
    # Api biasanya memiliki:
    # - Hue: Kuning ke Merah (0-30)
    # - Saturation: Sangat Tinggi (Api itu warnanya pekat, bukan pucat) > 130
    # - Value: Sangat Terang (Api itu sumber cahaya) > 180
    
    # Range untuk warna API (Kuning-Oranye Terang)
    lower = np.array([0, 130, 180], dtype="uint8") 
    upper = np.array([35, 255, 255], dtype="uint8")
    
    mask = cv2.inRange(hsv, lower, upper)
    
    # 3. Morphological Transformations (Pembersihan Noise)
    # Erosi untuk menghilangkan titik-titik kecil (noise cahaya lampu)
    # Dilasi untuk mempertegas area api yang sesungguhnya
    mask = cv2.erode(mask, None, iterations=2)
    mask = cv2.dilate(mask, None, iterations=2)
    
    contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    visual_fire_detected = False
    
    for cnt in contours:
        # Filter ukuran: hanya anggap api jika areanya cukup besar (> 300px)
        # Tapi jangan terlalu kecil agar tidak mendeteksi lampu led indikator
        area = cv2.contourArea(cnt)
        
        if area > 300:
            visual_fire_detected = True
            x, y, w, h = cv2.boundingRect(cnt)
            
            # --- VISUALISASI KOTAK MERAH BERKEDIP ---
            # Kotak merah
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 2)
            
            # Label
            cv2.rectangle(frame, (x, y-30), (x+150, y), (0, 0, 255), -1)
            cv2.putText(frame, f"API! ({int(area)})", (x+5, y-8), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
    return frame

# ================= MAIN LOOP =================
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print("Menghubungkan ke MQTT...")
try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
except:
    print("GAGAL KONEK BROKER. Pastikan Mosquitto jalan!")

# Setup Window
cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
cv2.resizeWindow(WINDOW_NAME, 1024, 600) 

cap = cv2.VideoCapture(0)

# Setting Exposure Manual (Opsional, agar background lebih gelap dan api lebih jelas)
# cap.set(cv2.CAP_PROP_EXPOSURE, -6.0) 

try:
    while True:
        ret, raw_frame = cap.read()
        if not ret: break
        
        frame = cv2.resize(raw_frame, (TARGET_WIDTH, TARGET_HEIGHT))
        
        # Deteksi Api yang sudah diperbaiki
        frame = process_frame(frame)
        
        # Gambar HUD
        frame = draw_modern_hud(frame)
        
        # MQTT Logic
        if visual_fire_detected and not last_visual_state:
            print("🔥 DETEKSI VISUAL: Mengirim perintah START...")
            client.publish(TOPIC_COMMAND, "FIRE_DETECTED")
        last_visual_state = visual_fire_detected
        
        cv2.imshow(WINDOW_NAME, frame)
        
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord('r'):
            print("🔄 RESET MANUAL dikirim.")
            client.publish(TOPIC_COMMAND, "RESET")

except KeyboardInterrupt:
    pass

finally:
    print("Menutup aplikasi...")
    client.publish(TOPIC_COMMAND, "RESET") 
    cap.release()
    cv2.destroyAllWindows()
    client.loop_stop()