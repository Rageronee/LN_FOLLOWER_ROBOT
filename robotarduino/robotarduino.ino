// ================= DEFINISI PIN =================
#define IR_SENSOR_RIGHT 11
#define IR_SENSOR_LEFT 12

// KOMUNIKASI DARI ESP32
#define PIN_CMD_RUN 4   
#define PIN_CMD_MODE 3  

// OUTPUT MOTOR & KIPAS
#define MOTOR_SPEED 28    
#define SLOW_SPEED 25     
#define TURN_SPEED 50     
#define FAN_SPEED 190     

// --- TUNING WAKTU ---
#define WAKTU_PUTAR_API    800
#define WAKTU_PUTAR_HOME   800
#define DURASI_VALIDASI    400   
#define WAKTU_SAFE_MODE    2500  // (PENTING) Waktu robot "Tutup Mata" terhadap Home

// L298N PIN
int enableRightFan = 6;  
int pinMotorKanan = 7;   
int pinKipas = 8;        
int enableLeft = 5;      
int pinMotorKiri = 9;    

// VARIABEL LOGIKA
int fasePulang = 0; 
bool keluarDariBase = false; 
unsigned long timerSafety = 0; // Timer pengaman

void setup() {
  pinMode(enableRightFan, OUTPUT);
  pinMode(pinMotorKanan, OUTPUT);
  pinMode(pinKipas, OUTPUT);
  pinMode(enableLeft, OUTPUT);
  pinMode(pinMotorKiri, OUTPUT);

  pinMode(IR_SENSOR_RIGHT, INPUT);
  pinMode(IR_SENSOR_LEFT, INPUT);
  pinMode(PIN_CMD_RUN, INPUT);
  pinMode(PIN_CMD_MODE, INPUT); 

  stopTotal();   
}

void loop() {
  int cmdRun = digitalRead(PIN_CMD_RUN);   
  int cmdMode = digitalRead(PIN_CMD_MODE); 

  // ================= LOGIKA UTAMA =================
  
  // 1. IDLE / RESET
  if (cmdRun == LOW && cmdMode == LOW) {
     stopTotal();
     fasePulang = 0;
     keluarDariBase = false; 
  }
  
  // 2. PADAMKAN API
  else if (cmdRun == LOW && cmdMode == HIGH) {
     stopMotorsOnly();
     nyalakanKipas();
     fasePulang = 0;
  }

  // 3. SEARCHING
  else if (cmdRun == HIGH && cmdMode == LOW) {
     matikanKipas();
     
     int rightIR = digitalRead(IR_SENSOR_RIGHT);
     int leftIR = digitalRead(IR_SENSOR_LEFT);

     if (!keluarDariBase) {
        if (rightIR == LOW && leftIR == LOW) {
           motorKanan(true, MOTOR_SPEED); 
           motorKiri(true, MOTOR_SPEED);
        } else if (rightIR == HIGH || leftIR == HIGH) {
           keluarDariBase = true; 
        }
     } 
     else {
        runLineFollower(MOTOR_SPEED);
     }
  }

  // 4. BACK HOME
  else if (cmdRun == HIGH && cmdMode == HIGH) {
     matikanKipas();
     jalankanLogikaPulang(); 
  }
}

// ================= LOGIKA PULANG (ANTI-FALSE DETECT) =================
void jalankanLogikaPulang() {
  
  // TAHAP 0: PUTAR 200 DERAJAT
  if (fasePulang == 0) {
    putarBalik(WAKTU_PUTAR_API); // 1000 ms
    
    // Mulai Timer Pengaman
    timerSafety = millis(); 
    fasePulang = 1; 
  }
  
  // TAHAP 1: JALAN PULANG (DENGAN FILTER WAKTU)
  else if (fasePulang == 1) {
    
    // 1. Robot SELALU menjalankan Line Follower agar tetap di jalur
    runLineFollower(MOTOR_SPEED); 

    // 2. LOGIKA PENGAMAN:
    // Jika belum lewat 2.5 detik (WAKTU_SAFE_MODE), JANGAN cek sensor Home.
    // Ini mencegah robot mengira garis di dekat api sebagai Home.
    if (millis() - timerSafety > WAKTU_SAFE_MODE) {
       
       // Setelah 2.5 detik berlalu, baru kode di bawah ini boleh jalan:
       
       int rightIR = digitalRead(IR_SENSOR_RIGHT);
       int leftIR = digitalRead(IR_SENSOR_LEFT);

       // Cek Home (Hitam-Hitam)
       if (rightIR == LOW && leftIR == LOW) {
          
          // Validasi Maju Pelan
          motorKanan(true, SLOW_SPEED);
          motorKiri(true, SLOW_SPEED);
          delay(DURASI_VALIDASI); 

          rightIR = digitalRead(IR_SENSOR_RIGHT);
          leftIR = digitalRead(IR_SENSOR_LEFT);

          // Fix Home
          if (rightIR == LOW && leftIR == LOW) {
             stopTotal();
             delay(1000); 
             
             // Putar 180 Derajat (Posisi Reset)
             putarBalik(WAKTU_PUTAR_HOME); 
             
             stopTotal();
             fasePulang = 2;  // Selesai
          } 
       }
    }
  }

  // TAHAP 2: SELESAI
  else if (fasePulang == 2) {
    stopTotal();
  }
}

// ================= HELPER FUNCTIONS =================

void putarBalik(int durasi) {
  digitalWrite(pinMotorKanan, LOW); 
  analogWrite(enableRightFan, TURN_SPEED); 
  digitalWrite(pinMotorKiri, HIGH); 
  analogWrite(enableLeft, TURN_SPEED);
  
  delay(durasi); 
  
  stopMotorsOnly();
  delay(500); 
}

void runLineFollower(int speed) {
  int rightIR = digitalRead(IR_SENSOR_RIGHT);
  int leftIR = digitalRead(IR_SENSOR_LEFT);
  
  if (rightIR == LOW && leftIR == HIGH) {       
    motorKiri(true, speed); motorKanan(false, 0); 
  } 
  else if (rightIR == HIGH && leftIR == LOW) {  
    motorKanan(true, speed); motorKiri(false, 0); 
  } 
  else if (rightIR == LOW && leftIR == LOW) {   
    motorKanan(true, speed); motorKiri(true, speed); 
  } 
  else { 
    stopMotorsOnly(); 
  }
}

void nyalakanKipas() {
  digitalWrite(pinKipas, HIGH); 
  digitalWrite(pinMotorKanan, LOW); 
  analogWrite(enableRightFan, FAN_SPEED); 
}

void matikanKipas() {
  digitalWrite(pinKipas, LOW);
}

void motorKanan(bool maju, int speed) {
  if (maju) {
    digitalWrite(pinMotorKanan, HIGH);
    analogWrite(enableRightFan, speed);
  } else {
    digitalWrite(pinMotorKanan, LOW);
    if (digitalRead(pinKipas) == LOW) digitalWrite(enableRightFan, LOW);
  }
}

void motorKiri(bool maju, int speed) {
  if (maju) {
    digitalWrite(pinMotorKiri, HIGH);
    analogWrite(enableLeft, speed);
  } else {
    digitalWrite(pinMotorKiri, LOW);
    digitalWrite(enableLeft, LOW);
  }
}

void stopMotorsOnly() {
  digitalWrite(pinMotorKanan, LOW);
  digitalWrite(pinMotorKiri, LOW);
  digitalWrite(enableLeft, LOW);
  if (digitalRead(pinKipas) == LOW) digitalWrite(enableRightFan, LOW);
}

void stopTotal() {
  digitalWrite(pinKipas, LOW);
  stopMotorsOnly();
}