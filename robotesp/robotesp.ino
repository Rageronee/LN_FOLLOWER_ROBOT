#include <WiFi.h>
#include <PubSubClient.h>

// ================= SETTING WIFI =================
const char* ssid = "Ahann";
const char* password = "08080004";
IPAddress mqtt_server(172, 20, 10, 2); // CEK IP LAPTOP!

WiFiClient espClient;
PubSubClient client(espClient);

// ================= PIN & VAR =================
#define PIN_FLAME_SENSOR 34 
#define PIN_CMD_RUN  4   
#define PIN_CMD_MODE 27  
#define PIN_BUZZER 18    

int flameThreshold = 1000; 
unsigned long timerState = 0; 
unsigned long lastMsgTime = 0; // Timer Heartbeat

enum RobotState { IDLE, SEARCHING, EXTINGUISHING, BACK_HOME };
RobotState currentState = IDLE;

void sendCommand(bool run, bool mode) {
  digitalWrite(PIN_CMD_RUN, run ? HIGH : LOW);
  digitalWrite(PIN_CMD_MODE, mode ? HIGH : LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  if (String(topic) == "robot/command") {
    if ((message == "FIRE_DETECTED" || message == "START") && currentState == IDLE) {
        Serial.println("START MISSION!");
        currentState = SEARCHING; 
    }
    else if (message == "RESET") {
      currentState = IDLE;
      sendCommand(LOW, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_FLAME_SENSOR, INPUT);
  pinMode(PIN_CMD_RUN, OUTPUT);
  pinMode(PIN_CMD_MODE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  sendCommand(LOW, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
}

void loop() {
  if (!client.connected()) {
    if (client.connect("ESP32_Robot")) client.subscribe("robot/command");
    delay(1000);
  }
  client.loop();

  int val = analogRead(PIN_FLAME_SENSOR);
  bool physicalFire = (val < flameThreshold); 

  // ================= STATE MACHINE =================
  switch (currentState) {
    case IDLE:
      sendCommand(LOW, LOW); 
      digitalWrite(PIN_BUZZER, LOW);
      break;

    case SEARCHING:
      sendCommand(HIGH, LOW); 
      if (physicalFire) {
        currentState = EXTINGUISHING;
        timerState = millis();
      }
      break;

    case EXTINGUISHING:
      sendCommand(LOW, HIGH); 
      digitalWrite(PIN_BUZZER, HIGH);
      if (!physicalFire) {
        if (millis() - timerState > 3000) { // Tunggu 3 detik padam
           digitalWrite(PIN_BUZZER, LOW);
           currentState = BACK_HOME;
           timerState = millis();
        }
      } else {
        timerState = millis();
      }
      break;

    case BACK_HOME:
      sendCommand(HIGH, HIGH); 
      // Beri waktu 20 detik ke Arduino untuk cari Home
      if (millis() - timerState > 20000) { 
        currentState = IDLE; // Reset otomatis
      }
      break;
  }

  // ================= HEARTBEAT (PENTING BUAT PYTHON) =================
  if (millis() - lastMsgTime > 1000) {
    lastMsgTime = millis();
    char valStr[8];
    itoa(val, valStr, 10);
    client.publish("robot/sensor/flame", valStr);

    String statusStr = "IDLE";
    if (currentState == SEARCHING) statusStr = "SEARCHING";
    else if (currentState == EXTINGUISHING) statusStr = "FIRE!";
    else if (currentState == BACK_HOME) statusStr = "RETURNING";
    
    client.publish("robot/status", statusStr.c_str());
  }
  
  delay(50);
}