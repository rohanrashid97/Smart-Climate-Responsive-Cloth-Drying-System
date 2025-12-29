#include <Servo.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
// ------------------- Pins -------------------
#define DHT_PIN D5 //humidity and temparature
#define SERVO1_PIN D4 //shade
#define SERVO2_PIN D2 //wiper
#define LDR_PIN D0  //day night           // as used before
#define RAIN_SENSOR_PIN D6  //rain
#define RAIN_SENSOR_VCC D7  //rain sensor power
#define DHT_TYPE DHT22 //Ty

// ------------------- Config -------------------
const char* ssid = "We Are CSEian_2.4G";
const char* password = "@wercseian";

const char* firebaseHost = "protection-system-b6781-default-rtdb.firebaseio.com";
const char* firebaseAuth = "Gdd1dXrsUmxet4wfj0ERZGCAiEC8geTP2G7aI2D6";

const char* sensorsPath = "/sensors.json";
const char* controlsPath = "/controls.json";

const int lightThreshold = 300;            // LOW = day
const unsigned long dhtInterval = 2000;    // DHT read interval

// DHT protection thresholds
const float TEMP_PROTECT_C = 30.0;         // temp > 30°C → protect
const float HUM_PROTECT_PCT = 98.0;        // humidity ≥ 98% → protect

// Servo2 (wiper) efficiency config
const unsigned long WIPE_GAP = 5000;     // 2 minutes gap
unsigned long lastWipeTime = 0;

// ------------------- Globals -------------------
Servo servo1;
Servo servo2;
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long lastDhtRead = 0;
int currentLogicalServo1Angle = -1;

WiFiClientSecure client;

bool systemEnabled = true;
bool servo1Enabled = true;
bool servo2Enabled = true;
// ------------------- Servo1 helper (direction fixed) -------------------
void setServo1(int angle) {
  angle = constrain(angle, 0, 180);
  if (angle == currentLogicalServo1Angle) return;
  currentLogicalServo1Angle = angle;
  servo1.write(180 - angle);   // inverted direction
}

void setup() {
  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN);

  setServo1(0);
  servo2.write(0);

  pinMode(RAIN_SENSOR_VCC, OUTPUT);
  digitalWrite(RAIN_SENSOR_VCC, HIGH);
  pinMode(RAIN_SENSOR_PIN, INPUT);

  Serial.begin(9600);
  delay(50);

  dht.begin();
  Serial.println("System started. Final efficient logic running.");
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  client.setInsecure(); // skip SSL cert check (OK for demo)
}

// -------- Send Sensor Data --------
void sendSensorData(float temp, float hum, bool rain, int ldr) {
  if (!client.connect(firebaseHost, 443)) {
    Serial.println("Firebase connection failed");
    return;
  }

  StaticJsonDocument<200> doc;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["rain"] = rain;
  doc["ldr"] = ldr;

  String json;
  serializeJson(doc, json);

  client.println("PUT " + String(sensorsPath) + "?auth=" + firebaseAuth + " HTTP/1.1");
  client.println("Host: " + String(firebaseHost));
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(json.length()));
  client.println();
  client.println(json);

  Serial.println("Sensor data sent to Firebase");
}

// -------- Read Control Data --------
void readControls() {
  if (!client.connect(firebaseHost, 443)) return;

  client.println("GET " + String(controlsPath) + "?auth=" + firebaseAuth + " HTTP/1.1");
  client.println("Host: " + String(firebaseHost));
  client.println();
  
  while (client.connected() && !client.available()) delay(1);

  String response = client.readString();

  
  int jsonStart = response.indexOf('{');
  if (jsonStart == -1) return;

  String json = response.substring(jsonStart);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) return;

  systemEnabled = doc["system"];
  servo1Enabled = doc["servo1"];
  servo2Enabled = doc["servo2"];

  Serial.println("Controls:");
  Serial.println(response);
}

void loop() {
  unsigned long now = millis();

  // -------- Sensor Reads --------
  int ldrValue = analogRead(LDR_PIN);
  int rainValue = digitalRead(RAIN_SENSOR_PIN); // LOW = rain

  static float lastTemp = NAN;
  static float lastHum  = NAN;

  if (now - lastDhtRead >= dhtInterval) {
    lastDhtRead = now;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (!isnan(h) && !isnan(t)) {
      lastTemp = t;
      lastHum = h;
      Serial.print("Temp: ");
      Serial.print(t);
      Serial.print(" C | Humidity: ");
      Serial.print(h);
      Serial.println(" %");
    }
  }
  
  // -------- Read Controls from Firebase --------
  readControls();

  // -------- System OFF check --------
  if (!systemEnabled) {
    setServo1(0);
    servo2.write(0);
    delay(1000);
    return; // skip rest of loop
  }

  // -------- Core Logic (UNCHANGED) --------
  bool isDay = (ldrValue <= lightThreshold); // LOW = day
  bool isRain = (rainValue == LOW);

  bool tempProtect = (!isnan(lastTemp) && lastTemp > TEMP_PROTECT_C);
  bool humProtect  = (!isnan(lastHum)  && lastHum >= HUM_PROTECT_PCT);

  bool protect = (!isDay) || isRain || tempProtect || humProtect;

  // -------- Servo1 Control (UNCHANGED) --------
if (servo1Enabled || protect) {
     setServo1(180);
  } else {
    setServo1(0);
  }

  // -------- Servo2 NEW Efficient Wiper Logic --------
  // ONLY: Day + Rain → wipe once every 2 minutes
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

  // -------- Debug --------
  Serial.print("LDR: "); Serial.print(ldrValue);
  Serial.print(" | Rain: "); Serial.print(isRain ? "YES" : "NO");
  Serial.print(" | Day: "); Serial.print(isDay ? "YES" : "NO");
  Serial.print(" | Protect: "); Serial.println(protect ? "YES" : "NO");
 
  // -------- Send Sensor Data to Firebase --------
  if (!isnan(lastTemp) && !isnan(lastHum)) {
    sendSensorData(lastTemp, lastHum, isRain, ldrValue);
  }
  delay(500);
}
