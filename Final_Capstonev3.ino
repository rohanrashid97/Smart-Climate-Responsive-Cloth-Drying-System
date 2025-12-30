#include <Servo.h>              // Library to control servo motors
#include <ESP8266WiFi.h>        // WiFi control library for ESP8266
#include <DHT.h>                // Library for DHT temperature & humidity sensor
#include <ArduinoJson.h>        // Used to encode/decode JSON data (Firebase)
#include <WiFiClientSecure.h>   // Enables HTTPS communication with Firebase

// =====================================================
// ------------------- PIN DEFINITIONS -----------------
// =====================================================

// DHT22 sensor pin
// Reads Temperature (°C) and Humidity (%)
#define DHT_PIN D5

// Servo motor pins
#define SERVO1_PIN D4           // Servo1 → Shade / Roof / Protection Cover
#define SERVO2_PIN D2           // Servo2 → Wiper mechanism

// LDR sensor pin
// Used to detect DAY or NIGHT
#define LDR_PIN D0              // Lower value = brighter light (day)

// Rain sensor pins
#define RAIN_SENSOR_PIN D6      // Digital rain signal (LOW = rain)
#define RAIN_SENSOR_VCC D7      // Power supply for rain sensor

// DHT sensor type
#define DHT_TYPE DHT22

// =====================================================
// ---------------- WIFI & FIREBASE CONFIG --------------
// =====================================================

// WiFi credentials (ESP8266 requires 2.4GHz WiFi)
const char* ssid = "We Are CSEian_2.4G";
const char* password = "@wercseian";

// Firebase Realtime Database host and authentication token
const char* firebaseHost = "protection-system-b6781-default-rtdb.firebaseio.com";
const char* firebaseAuth = "Gdd1dXrsUmxet4wfj0ERZGCAiEC8geTP2G7aI2D6";

// Firebase database paths
// sensorsPath  → ESP uploads sensor values here
// controlsPath → ESP reads control commands from here
const char* sensorsPath  = "/sensors.json";
const char* controlsPath = "/controls.json";

// =====================================================
// ------------------- SYSTEM PARAMETERS ----------------
// =====================================================

// LDR threshold value
// If LDR value ≤ threshold → DAY
// If LDR value > threshold → NIGHT
const int lightThreshold = 300;

// Interval for reading DHT sensor (milliseconds)
const unsigned long dhtInterval = 2000;

// Environmental protection thresholds
const float TEMP_PROTECT_C = 30.0;   // Above 30°C → protection required
const float HUM_PROTECT_PCT = 98.0;  // Above 98% humidity → protection required

// Servo2 (wiper) timing configuration
// Prevents continuous wiping to save power
const unsigned long WIPE_GAP = 5000; // Delay between wipes
unsigned long lastWipeTime = 0;

// =====================================================
// ------------------- GLOBAL OBJECTS -------------------
// =====================================================

// Servo objects
Servo servo1;     // Controls shade/cover
Servo servo2;     // Controls wiper

// DHT sensor object
DHT dht(DHT_PIN, DHT_TYPE);

// HTTPS client for Firebase
WiFiClientSecure client;

// Stores last DHT read timestamp
unsigned long lastDhtRead = 0;

// Stores last servo1 angle to avoid repeated movement
int currentLogicalServo1Angle = -1;

// Control flags read from Firebase dashboard
bool systemEnabled = true;   // Master ON/OFF switch
bool servo1Enabled = true;   // Permission for servo1
bool servo2Enabled = true;   // Permission for servo2

// =====================================================
// ---------------- SERVO1 HELPER FUNCTION --------------
// =====================================================
// This function:
// 1. Limits angle between 0–180
// 2. Avoids unnecessary servo movement
// 3. Fixes inverted mechanical mounting
void setServo1(int angle) {

  // Ensure valid servo angle
  angle = constrain(angle, 0, 180);

  // If the servo is already at this angle, do nothing
  if (angle == currentLogicalServo1Angle) return;

  // Save current angle
  currentLogicalServo1Angle = angle;

  // Write inverted angle due to mechanical orientation
  servo1.write(180 - angle);
}

// =====================================================
// ----------------------- SETUP ------------------------
// =====================================================
void setup() {

  // Attach servo motors with pulse width limits
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN);

  // Set initial servo positions
  setServo1(0);     // Shade open
  servo2.write(0);  // Wiper stopped

  // Power and configure rain sensor
  pinMode(RAIN_SENSOR_VCC, OUTPUT);
  digitalWrite(RAIN_SENSOR_VCC, HIGH);
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // Start serial communication for debugging
  Serial.begin(9600);
  delay(50);

  // Initialize DHT sensor
  dht.begin();
  Serial.println("System started. Final efficient logic running.");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  // Disable SSL certificate verification (demo/testing purpose)
  client.setInsecure();
}

// =====================================================
// ------------- SEND SENSOR DATA TO FIREBASE -----------
// =====================================================
// Sends temperature, humidity, rain and LDR values
void sendSensorData(float temp, float hum, bool rain, int ldr) {

  // Connect to Firebase server
  if (!client.connect(firebaseHost, 443)) {
    Serial.println("Firebase connection failed");
    return;
  }

  // Create JSON payload
  StaticJsonDocument<200> doc;
  doc["temperature"] = temp;
  doc["humidity"]    = hum;
  doc["rain"]        = rain;
  doc["ldr"]         = ldr;

  String json;
  serializeJson(doc, json);

  // HTTP PUT request to Firebase
  client.println("PUT " + String(sensorsPath) + "?auth=" + firebaseAuth + " HTTP/1.1");
  client.println("Host: " + String(firebaseHost));
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(json.length()));
  client.println();
  client.println(json);

  Serial.println("Sensor data sent to Firebase");
}

// =====================================================
// ------------- READ CONTROL DATA FROM FIREBASE --------
// =====================================================
// Reads system, servo1 and servo2 ON/OFF flags
void readControls() {

  if (!client.connect(firebaseHost, 443)) return;

  // HTTP GET request
  client.println("GET " + String(controlsPath) + "?auth=" + firebaseAuth + " HTTP/1.1");
  client.println("Host: " + String(firebaseHost));
  client.println();

  while (client.connected() && !client.available()) delay(1);

  String response = client.readString();

  // Extract JSON data from HTTP response
  int jsonStart = response.indexOf('{');
  if (jsonStart == -1) return;

  String json = response.substring(jsonStart);

  // Parse JSON
  StaticJsonDocument<200> doc;
  if (deserializeJson(doc, json)) return;

  // Update control flags
  systemEnabled = doc["system"];
  servo1Enabled = doc["servo1"];
  servo2Enabled = doc["servo2"];
}

// =====================================================
// ------------------------ LOOP ------------------------
// =====================================================
void loop() {

  unsigned long now = millis();

  // -------- Sensor Reading --------
  int ldrValue  = analogRead(LDR_PIN);          // Day/Night detection
  int rainValue = digitalRead(RAIN_SENSOR_PIN); // Rain detection

  static float lastTemp = NAN;
  static float lastHum  = NAN;

  // Read DHT sensor periodically
  if (now - lastDhtRead >= dhtInterval) {
    lastDhtRead = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      lastTemp = t;
      lastHum  = h;
    }
  }

  // -------- Read Dashboard Controls --------
  readControls();

  // -------- System OFF Condition --------
  if (!systemEnabled) {
    setServo1(0);
    servo2.write(0);
    delay(1000);
    return;
  }

  // -------- Decision Logic --------
  bool isDay  = (ldrValue <= lightThreshold);
  bool isRain = (rainValue == LOW);

  bool tempProtect = (!isnan(lastTemp) && lastTemp > TEMP_PROTECT_C);
  bool humProtect  = (!isnan(lastHum)  && lastHum  >= HUM_PROTECT_PCT);

  bool protect = (!isDay) || isRain || tempProtect || humProtect;

  // -------- Servo1 (Shade) Logic --------
  if (servo1Enabled || protect) {
    setServo1(180);   // Close shade
  } else {
    setServo1(0);     // Open shade
  }

  // -------- Servo2 (Wiper) Logic --------
  if (servo2Enabled && isDay && isRain) {
    if (now - lastWipeTime >= WIPE_GAP) {
      lastWipeTime = now;
      servo2.write(90);
      delay(600);
      servo2.write(0);
    }
  } else {
    servo2.write(0);
  }

  // -------- Upload Sensor Data --------
  if (!isnan(lastTemp) && !isnan(lastHum)) {
    sendSensorData(lastTemp, lastHum, isRain, ldrValue);
  }

  delay(500);
}
