#define BLYNK_TEMPLATE_ID "TMPL31RhX-m13"
#define BLYNK_TEMPLATE_NAME "Intelligent Cloud Telemetry Architecture"
#define BLYNK_AUTH_TOKEN "BVobF9dXVAjdMjIV4GWUPRFd2TRP39SH"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Wokwi WiFi
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// Pins
#define CELL1_PIN 33
#define CELL2_PIN 35
#define CELL3_PIN 34
#define CELL4_PIN 32
#define RELAY_PIN 25
#define BUZZER_PIN 26

// Thresholds
#define LOW_LIMIT 1.00
#define HIGH_LIMIT 3.20
#define IMBALANCE_LIMIT 30.0
#define CHANGE_DELTA 0.10

// Stable timings for Wokwi + Blynk
#define ADC_INTERVAL 300
#define LCD_INTERVAL 600
#define WIFI_CHECK_INTERVAL 2000
#define CLOUD_SYNC_INTERVAL 300
#define BUZZER_INTERVAL 300
#define SERIAL_INTERVAL 1000

#define QUEUE_SIZE 15

float cell[4], oldCell[4];
float packVoltage = 0.0;
float avgVoltage = 0.0;
float maxVoltage = 0.0;
float minVoltage = 0.0;
float imbalance = 0.0;

bool lowFault = false;
bool highFault = false;
bool imbalanceFault = false;

bool lastLowFault = false;
bool lastHighFault = false;
bool lastImbalanceFault = false;

bool relayState = true;
bool buzzerState = false;

String runtimeState = "NORMAL";
String lastState = "";

unsigned long lastADC = 0;
unsigned long lastLCD = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastCloudSync = 0;
unsigned long lastBuzzer = 0;
unsigned long lastSerial = 0;

String lastLine0 = "";
String lastLine1 = "";

struct CloudEvent {
  String state;
  String message;
  float c1;
  float c2;
  float c3;
  float c4;
  int rssi;
};

CloudEvent eventQueue[QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

// ---------------- Utility ----------------
float readVoltage(int pin) {
  int adc = analogRead(pin);
  return (adc * 3.3) / 4095.0;
}

void printLCDLine(int row, String text) {
  if (text.length() > 16) text = text.substring(0, 16);
  while (text.length() < 16) text += " ";

  if (row == 0 && text != lastLine0) {
    lcd.setCursor(0, 0);
    lcd.print(text);
    lastLine0 = text;
  }

  if (row == 1 && text != lastLine1) {
    lcd.setCursor(0, 1);
    lcd.print(text);
    lastLine1 = text;
  }
}

bool wifiReady() {
  return WiFi.status() == WL_CONNECTED;
}

bool cloudReady() {
  return wifiReady() && Blynk.connected();
}

// ---------------- Event Queue ----------------
void enqueueEvent(String state, String message) {
  if (queueCount >= QUEUE_SIZE) {
    Serial.println("Queue full. Event dropped.");
    return;
  }

  eventQueue[queueTail].state = state;
  eventQueue[queueTail].message = message;
  eventQueue[queueTail].c1 = cell[0];
  eventQueue[queueTail].c2 = cell[1];
  eventQueue[queueTail].c3 = cell[2];
  eventQueue[queueTail].c4 = cell[3];
  eventQueue[queueTail].rssi = wifiReady() ? WiFi.RSSI() : 0;

  queueTail = (queueTail + 1) % QUEUE_SIZE;
  queueCount++;
}

bool dequeueEvent(CloudEvent &event) {
  if (queueCount <= 0) return false;

  event = eventQueue[queueHead];
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  queueCount--;

  return true;
}

// ---------------- Battery Tasks ----------------
void readBatteryTask() {
  cell[0] = readVoltage(CELL1_PIN);
  cell[1] = readVoltage(CELL2_PIN);
  cell[2] = readVoltage(CELL3_PIN);
  cell[3] = readVoltage(CELL4_PIN);
}

void analyzeBatteryTask() {
  packVoltage = 0.0;
  maxVoltage = cell[0];
  minVoltage = cell[0];

  for (int i = 0; i < 4; i++) {
    packVoltage += cell[i];

    if (cell[i] > maxVoltage) maxVoltage = cell[i];
    if (cell[i] < minVoltage) minVoltage = cell[i];
  }

  avgVoltage = packVoltage / 4.0;

  if (avgVoltage > 0.0) {
    imbalance = ((maxVoltage - minVoltage) / avgVoltage) * 100.0;
  } else {
    imbalance = 0.0;
  }

  lowFault = false;
  highFault = false;
  imbalanceFault = false;

  for (int i = 0; i < 4; i++) {
    if (cell[i] < LOW_LIMIT) lowFault = true;
    if (cell[i] > HIGH_LIMIT) highFault = true;
  }

  if (imbalance > IMBALANCE_LIMIT) imbalanceFault = true;

  if (lowFault || highFault || imbalanceFault) {
    runtimeState = "FAULT";
    relayState = false;
  } else {
    runtimeState = "NORMAL";
    relayState = true;
  }

  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
}

// ---------------- Event Driven Telemetry ----------------
void detectTelemetryEventsTask() {
  if (runtimeState != lastState) {
    enqueueEvent(runtimeState, "State changed to " + runtimeState);
    lastState = runtimeState;
  }

  if (lowFault && !lastLowFault) {
    enqueueEvent(runtimeState, "Low voltage threshold violation");
  }

  if (highFault && !lastHighFault) {
    enqueueEvent(runtimeState, "High voltage threshold violation");
  }

  if (imbalanceFault && !lastImbalanceFault) {
    enqueueEvent(runtimeState, "Imbalance threshold violation");
  }

  lastLowFault = lowFault;
  lastHighFault = highFault;
  lastImbalanceFault = imbalanceFault;

  for (int i = 0; i < 4; i++) {
    if (abs(cell[i] - oldCell[i]) >= CHANGE_DELTA) {
      enqueueEvent(runtimeState, "Voltage anomaly C" + String(i + 1));
      oldCell[i] = cell[i];
    }
  }
}

// ---------------- WiFi + Blynk Manager ----------------
void connectionManagerTask() {
  if (!wifiReady()) {
    WiFi.disconnect();
    WiFi.begin(ssid, pass);
    return;
  }

  if (!Blynk.connected()) {
    Blynk.connect(1000);
  }
}

// ---------------- Cloud Sync ----------------
void updateLiveCloudValues() {
  if (!cloudReady()) return;

  Blynk.virtualWrite(V0, cell[0]);
  Blynk.virtualWrite(V1, cell[1]);
  Blynk.virtualWrite(V2, cell[2]);
  Blynk.virtualWrite(V3, cell[3]);
  Blynk.virtualWrite(V4, runtimeState);
  Blynk.virtualWrite(V6, WiFi.RSSI());
  Blynk.virtualWrite(V7, queueCount);
}

void sendEventToCloud(CloudEvent event) {
  if (!cloudReady()) return;

  Blynk.virtualWrite(V0, event.c1);
  Blynk.virtualWrite(V1, event.c2);
  Blynk.virtualWrite(V2, event.c3);
  Blynk.virtualWrite(V3, event.c4);
  Blynk.virtualWrite(V4, event.state);
  Blynk.virtualWrite(V5, event.message);
  Blynk.virtualWrite(V6, WiFi.RSSI());
  Blynk.virtualWrite(V7, queueCount);
}

void syncCloudTask() {
  if (!cloudReady()) return;

  updateLiveCloudValues();

  CloudEvent event;
  if (dequeueEvent(event)) {
    sendEventToCloud(event);
  }
}

// ---------------- Local Output Tasks ----------------
void updateBuzzerTask() {
  if (runtimeState == "FAULT") {
    if (millis() - lastBuzzer >= BUZZER_INTERVAL) {
      lastBuzzer = millis();
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
    }
  } else {
    buzzerState = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void updateLCDTask() {
  if (runtimeState == "FAULT") {
    printLCDLine(0, "CLOUD FAULT");
  } else {
    printLCDLine(0, "STATE:NORMAL");
  }

  if (cloudReady()) {
    printLCDLine(1, "Cloud:ONLINE");
  } else if (wifiReady()) {
    printLCDLine(1, "Blynk:OFFLINE");
  } else {
    printLCDLine(1, "WiFi:OFFLINE");
  }
}

void printSerialTask() {
  Serial.println("==================================");
  Serial.println("INTELLIGENT CLOUD TELEMETRY");
  Serial.println("==================================");

  Serial.print("Cell1: "); Serial.println(cell[0], 2);
  Serial.print("Cell2: "); Serial.println(cell[1], 2);
  Serial.print("Cell3: "); Serial.println(cell[2], 2);
  Serial.print("Cell4: "); Serial.println(cell[3], 2);

  Serial.print("Pack Voltage: "); Serial.println(packVoltage, 2);
  Serial.print("Average: "); Serial.println(avgVoltage, 2);
  Serial.print("Imbalance: "); Serial.println(imbalance, 2);

  Serial.print("Low Fault: "); Serial.println(lowFault ? "YES" : "NO");
  Serial.print("High Fault: "); Serial.println(highFault ? "YES" : "NO");
  Serial.print("Imbalance Fault: "); Serial.println(imbalanceFault ? "YES" : "NO");

  Serial.print("Runtime State: "); Serial.println(runtimeState);
  Serial.print("Relay: "); Serial.println(relayState ? "ON" : "OFF");

  Serial.print("WiFi: "); Serial.println(wifiReady() ? "CONNECTED" : "DISCONNECTED");
  Serial.print("Blynk: "); Serial.println(Blynk.connected() ? "CONNECTED" : "DISCONNECTED");

  Serial.print("RSSI: ");
  if (wifiReady()) Serial.println(WiFi.RSSI());
  else Serial.println("N/A");

  Serial.print("Queue Count: "); Serial.println(queueCount);
  Serial.println();
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();

  printLCDLine(0, "Cloud Telemetry");
  printLCDLine(1, "Stable Connect");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  Blynk.config(BLYNK_AUTH_TOKEN, "blynk.cloud", 80);
  Blynk.connect(3000);

  readBatteryTask();
  analyzeBatteryTask();

  for (int i = 0; i < 4; i++) {
    oldCell[i] = cell[i];
  }

  lastState = runtimeState;
  enqueueEvent(runtimeState, "System booted");
}

// ---------------- Loop ----------------
void loop() {
  unsigned long now = millis();

  if (wifiReady()) {
    Blynk.run();
  }

  if (now - lastADC >= ADC_INTERVAL) {
    lastADC = now;
    readBatteryTask();
    analyzeBatteryTask();
    detectTelemetryEventsTask();
  }

  if (now - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
    lastWiFiCheck = now;
    connectionManagerTask();
  }

  if (now - lastCloudSync >= CLOUD_SYNC_INTERVAL) {
    lastCloudSync = now;
    syncCloudTask();
  }

  if (now - lastLCD >= LCD_INTERVAL) {
    lastLCD = now;
    updateLCDTask();
  }

  if (now - lastSerial >= SERIAL_INTERVAL) {
    lastSerial = now;
    printSerialTask();
  }

  updateBuzzerTask();
}
