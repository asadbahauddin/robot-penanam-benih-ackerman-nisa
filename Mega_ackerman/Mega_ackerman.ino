// ============================================================
// ROBOT ACKERMAN — ATmega2560 (FINAL v5)
// Pinout:
//   L298N       : ENA=2, IN1=22, IN2=23, ENB=3, IN3=24, IN4=25
//   Encoder     : M1_C1=18(INT3), M1_C2=19, M2_C1=20(INT1), M2_C2=21
//   Stepper     : IN1=26, IN2=27, IN3=28, IN4=29
//   Servo Steer : Signal=6 | Center=75, Kiri=60, Kanan=110
//   Servo Benih : Signal=7 | Tutup=34, Buka=95
//   ESP8266     : Serial0 (pin 0/1) via SW1 SW2
//
// DIP SWITCH:
//   Upload Mega : SW3=ON, SW4=ON, semua lain OFF
//   Upload ESP  : SW5=ON, SW6=ON, SW7=ON, SW8=ON, semua lain OFF
//   Running     : SW1=ON, SW2=ON, semua lain OFF
//
// SEQUENCE TANAM:
//   1. Motor STOP
//   2. Stepper 0 -> 1380 step (bor turun)
//   3. Servo benih 34 -> 95 (buka) delay 500ms
//   4. Servo benih 95 -> 34 (dorong+tutup) delay 50ms
//   5. Stepper 1380 -> 0 (naik)
// ============================================================

#include <Servo.h>

// --- PIN DEFINITIONS ---
#define ENA           2
#define IN1           22
#define IN2           23
#define ENB           3
#define IN3           24
#define IN4           25

#define ENC1_C1       18
#define ENC1_C2       19
#define ENC2_C1       20
#define ENC2_C2       21

#define STEP_IN1      26
#define STEP_IN2      27
#define STEP_IN3      28
#define STEP_IN4      29

#define SERVO_STEER   6
#define SERVO_BENIH   7

// --- STEERING CONSTANTS (sinkron dengan nigga.py) ---
#define SERVO_CENTER  75
#define SERVO_LEFT    60
#define SERVO_RIGHT   110

// --- BENIH CONSTANTS ---
#define BENIH_TUTUP   34
#define BENIH_BUKA    95

// --- STEPPER CONSTANTS ---
#define STEP_MAX      1380  // hasil kalibrasi: turun penuh (posisi ngebor)
#define STEP_MIN      0     // hasil kalibrasi: home/naik penuh
#define STEP_DELAY_US 1500

// --- ENCODER ---
volatile long enc1_count = 0;
volatile long enc2_count = 0;

void enc1_ISR() {
  if (digitalRead(ENC1_C2) == HIGH) enc1_count++;
  else enc1_count--;
}
void enc2_ISR() {
  if (digitalRead(ENC2_C2) == HIGH) enc2_count++;
  else enc2_count--;
}

// --- SERVO ---
Servo steeringServo;
Servo benihServo;
int servoAngle = SERVO_CENTER;
int benihAngle = BENIH_TUTUP;

// --- STEPPER ---
const int stepSeq[8][4] = {
  {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
  {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};
int stepIndex     = 0;
int stepperPos    = 0;
int stepperTarget = 0;

// --- STATE ---
bool sequenceRunning = false;

void stepOnce(int dir) {
  stepIndex = (stepIndex + dir + 8) % 8;
  digitalWrite(STEP_IN1, stepSeq[stepIndex][0]);
  digitalWrite(STEP_IN2, stepSeq[stepIndex][1]);
  digitalWrite(STEP_IN3, stepSeq[stepIndex][2]);
  digitalWrite(STEP_IN4, stepSeq[stepIndex][3]);
}

// Blocking stepper — hanya untuk sequence tanam
void stepBlocking(int target) {
  while (stepperPos != target) {
    int dir = (target > stepperPos) ? 1 : -1;
    stepOnce(dir);
    stepperPos += dir;
    delayMicroseconds(STEP_DELAY_US);
  }
}

// --- RPM ---
unsigned long lastRpmTime = 0;
long lastEnc1 = 0, lastEnc2 = 0;
float rpm1 = 0, rpm2 = 0;
#define PPR 11  // VERIFIKASI ke datasheet encoder!

// --- MOTOR ---
void motorControl(int pwm1, bool fwd1, int pwm2, bool fwd2) {
  analogWrite(ENA, constrain(pwm1, 0, 255));
  digitalWrite(IN1, fwd1 ? LOW  : HIGH);
  digitalWrite(IN2, fwd1 ? HIGH : LOW);
  analogWrite(ENB, constrain(pwm2, 0, 255));
  digitalWrite(IN3, fwd2 ? LOW  : HIGH);
  digitalWrite(IN4, fwd2 ? HIGH : LOW);
}

void motorStop() {
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// --- SEQUENCE TANAM ---
void sequenceTanam() {
  sequenceRunning = true;

  // 1. Stop motor
  motorStop();

  // 2. Stepper turun
  stepBlocking(STEP_MAX);

  // 3. Buka benih (34 -> 95)
  benihAngle = BENIH_BUKA;
  benihServo.write(benihAngle);
  delay(500);

  // 4. Dorong + tutup sekalian (95 -> 34) — cepat
  benihAngle = BENIH_TUTUP;
  benihServo.write(benihAngle);
  delay(50);

  // 5. Stepper naik
  stepBlocking(0);
  stepperTarget = 0;

  sequenceRunning = false;
}

// --- PARSE COMMAND ---
void parseCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("MOTOR,")) {
    if (sequenceRunning) return;
    int p1 = cmd.indexOf(',', 6);
    int p2 = cmd.indexOf(',', p1+1);
    int p3 = cmd.indexOf(',', p2+1);
    if (p1<0||p2<0||p3<0) return;
    int  pwm1 = cmd.substring(6,    p1).toInt();
    bool fwd1 = cmd.substring(p1+1, p2).toInt();
    int  pwm2 = cmd.substring(p2+1, p3).toInt();
    bool fwd2 = cmd.substring(p3+1).toInt();
    motorControl(pwm1, fwd1, pwm2, fwd2);
  }
  else if (cmd.startsWith("SERVO,")) {
    servoAngle = constrain(cmd.substring(6).toInt(), SERVO_LEFT, SERVO_RIGHT);
    steeringServo.write(servoAngle);
  }
  else if (cmd.startsWith("BENIH,")) {
    if (sequenceRunning) return;
    benihAngle = constrain(cmd.substring(6).toInt(), 0, 180);
    benihServo.write(benihAngle);
  }
  else if (cmd.startsWith("STEPPER,")) {
    if (sequenceRunning) return;
    stepperTarget = constrain(cmd.substring(8).toInt(), STEP_MIN, STEP_MAX);
  }
  else if (cmd == "TANAM") {
    if (!sequenceRunning) sequenceTanam();
  }
  else if (cmd == "STOP") {
    if (!sequenceRunning) motorStop();
  }
  else if (cmd == "RESETENC") {
    enc1_count = 0; enc2_count = 0;
    lastEnc1 = 0; lastEnc2 = 0;
    rpm1 = 0; rpm2 = 0;
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  pinMode(ENA,OUTPUT); pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(ENB,OUTPUT); pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  motorStop();

  pinMode(ENC1_C1,INPUT_PULLUP); pinMode(ENC1_C2,INPUT_PULLUP);
  pinMode(ENC2_C1,INPUT_PULLUP); pinMode(ENC2_C2,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC1_C1), enc1_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC2_C1), enc2_ISR, RISING);

  pinMode(STEP_IN1,OUTPUT); pinMode(STEP_IN2,OUTPUT);
  pinMode(STEP_IN3,OUTPUT); pinMode(STEP_IN4,OUTPUT);

  steeringServo.attach(SERVO_STEER);
  steeringServo.write(SERVO_CENTER);

  benihServo.attach(SERVO_BENIH);
  benihServo.write(BENIH_TUTUP);

  lastRpmTime = millis();
}

// --- LOOP ---
#define SEND_INTERVAL  2000
#define CMD_QUIET_TIME   15

String espBuffer = "";
unsigned long lastSendTime = 0;
unsigned long lastCmdTime  = 0;
bool cmdPending = false;

void loop() {
  // 1. Baca command dari ESP
  while (Serial.available()) {
    char c = Serial.read();
    lastCmdTime = millis();
    if (c == '\n') {
      parseCommand(espBuffer);
      espBuffer  = "";
      cmdPending = false;
    } else {
      espBuffer += c;
      cmdPending = true;
    }
  }

  // 2. Stepper non-blocking
  if (!sequenceRunning && stepperPos != stepperTarget) {
    int dir = (stepperTarget > stepperPos) ? 1 : -1;
    stepOnce(dir);
    stepperPos += dir;
    delayMicroseconds(STEP_DELAY_US);
  }

  // 3. Kirim DATA ke ESP saat jalur sepi
  unsigned long now = millis();
  bool jalurSepi = (!cmdPending) && (now - lastCmdTime > CMD_QUIET_TIME);

  if (jalurSepi && (now - lastSendTime >= SEND_INTERVAL)) {
    lastSendTime = now;

    float dt = (now - lastRpmTime) / 1000.0;
    if (dt > 0) {
      rpm1 = ((enc1_count - lastEnc1) / (float)PPR) / dt * 60.0;
      rpm2 = ((enc2_count - lastEnc2) / (float)PPR) / dt * 60.0;
      lastEnc1    = enc1_count;
      lastEnc2    = enc2_count;
      lastRpmTime = now;
    }

    // Format: DATA,enc1,enc2,rpm1,rpm2,servo,stepper,benih,seq
    Serial.print("DATA,");
    Serial.print(enc1_count);              Serial.print(',');
    Serial.print(enc2_count);              Serial.print(',');
    Serial.print((int)rpm1);               Serial.print(',');
    Serial.print((int)rpm2);               Serial.print(',');
    Serial.print(servoAngle);              Serial.print(',');
    Serial.print(stepperPos);              Serial.print(',');
    Serial.print(benihAngle);              Serial.print(',');
    Serial.print(sequenceRunning ? 1 : 0); Serial.print('\n');
  }
}