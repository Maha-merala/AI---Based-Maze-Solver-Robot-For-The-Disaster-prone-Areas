#include <WiFi.h>
#include <WebServer.h>

const char* WIFI_SSID = "MazeSolver";
const char* WIFI_PASS = "12345678";

WebServer server(80);

// ---------------- MOTOR ----------------
#define ML_A   25
#define ML_B   26
#define MR_A   27
#define MR_B   14
#define ML_PWM 32
#define MR_PWM 33

#define MOTOR_SPEED  160
#define TURN_SPEED   100
#define CELL_MS      600
#define TURN_MS      350

// ---------------- ULTRASONIC ----------------
#define TF  5
#define EF  18
#define TL  19
#define EL  21
#define TR  22
#define ER  23
#define TB  12
#define EB  13

#define WALL_CM 15

// ------------------------------------------------
// MOTOR FUNCTIONS
// ------------------------------------------------
void mStop() {
  digitalWrite(ML_A, LOW); digitalWrite(ML_B, LOW);
  digitalWrite(MR_A, LOW); digitalWrite(MR_B, LOW);
  ledcWrite(ML_PWM, 0);
  ledcWrite(MR_PWM, 0);
}

void mFwd(int s) {
  digitalWrite(ML_A, HIGH); digitalWrite(ML_B, LOW);
  digitalWrite(MR_A, HIGH); digitalWrite(MR_B, LOW);
  ledcWrite(ML_PWM, s); ledcWrite(MR_PWM, s);
}

void mBwd(int s) {
  digitalWrite(ML_A, LOW); digitalWrite(ML_B, HIGH);
  digitalWrite(MR_A, LOW); digitalWrite(MR_B, HIGH);
  ledcWrite(ML_PWM, s); ledcWrite(MR_PWM, s);
}

void mTurnL(int s) {
  digitalWrite(ML_A, LOW);  digitalWrite(ML_B, HIGH);
  digitalWrite(MR_A, HIGH); digitalWrite(MR_B, LOW);
  ledcWrite(ML_PWM, s); ledcWrite(MR_PWM, s);
}

void mTurnR(int s) {
  digitalWrite(ML_A, HIGH); digitalWrite(ML_B, LOW);
  digitalWrite(MR_A, LOW);  digitalWrite(MR_B, HIGH);
  ledcWrite(ML_PWM, s); ledcWrite(MR_PWM, s);
}

// ------------------------------------------------
// ULTRASONIC
// ------------------------------------------------
long pingCm(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long d = pulseIn(echo, HIGH, 23000UL);
  return (d == 0) ? 200 : d * 17L / 1000L;
}

// ------------------------------------------------
// MOVEMENT LOGIC
// ------------------------------------------------
void doForward() {
  if (pingCm(TF, EF) < WALL_CM) {
    Serial.println("Blocked F");
    return;
  }
  mFwd(MOTOR_SPEED); delay(CELL_MS); mStop();
}

void doBackward() {
  if (pingCm(TB, EB) < WALL_CM) {
    Serial.println("Blocked B");
    return;
  }
  mBwd(MOTOR_SPEED); delay(CELL_MS); mStop();
}

void doLeft() {
  mTurnL(TURN_SPEED); delay(TURN_MS); mStop();
}

void doRight() {
  mTurnR(TURN_SPEED); delay(TURN_MS); mStop();
}

// ------------------------------------------------
// EXECUTE STRING FROM PYTHON
// ------------------------------------------------
void executeSequence(String cmd) {
  cmd.trim();
  Serial.println("Received: " + cmd);

  for (int i = 0; i < cmd.length(); i++) {
    char c = cmd[i];

    if      (c == 'F') doForward();
    else if (c == 'B') doBackward();
    else if (c == 'L') doLeft();
    else if (c == 'R') doRight();
    else if (c == 'S') mStop();

    delay(300);
  }

  Serial.println("Done!");
}

// ------------------------------------------------
// WEB SERVER
// ------------------------------------------------
void handleCmd() {
  String a = server.arg("a");

  if (a == "cmdseq") {
    String seq = server.arg("data");
    executeSequence(seq);
  }

  server.send(200, "text/plain", "OK");
}

// ------------------------------------------------
// SETUP
// ------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(ML_A, OUTPUT); pinMode(ML_B, OUTPUT);
  pinMode(MR_A, OUTPUT); pinMode(MR_B, OUTPUT);

  ledcAttach(ML_PWM, 1000, 8);
  ledcAttach(MR_PWM, 1000, 8);

  pinMode(TF,OUTPUT); pinMode(EF,INPUT);
  pinMode(TL,OUTPUT); pinMode(EL,INPUT);
  pinMode(TR,OUTPUT); pinMode(ER,INPUT);
  pinMode(TB,OUTPUT); pinMode(EB,INPUT);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  Serial.println("Connect WiFi: MazeSolver");
  Serial.println("IP: " + WiFi.softAPIP().toString());

  server.on("/cmd", handleCmd);
  server.begin();
}

void loop() {
  server.handleClient();
}