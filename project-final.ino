#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
// #include <BH1750.h>      // --- Light sensor library (commented out)
#include <Wire.h>
#include "DHT.h"
#include <Arduino_JSON.h>
#include <Servo.h>

// ----------- WIFI & FIREBASE CONFIG ------------
#define WIFI_SSID     "iPhone"
#define WIFI_PASS     "romil1234"

#define FIREBASE_HOST "plant-monitor-fb8c1-default-rtdb.firebaseio.com"
#define FIREBASE_SECRET "Sh1DSJ9IMURMt7dYwtzCFDOFGki6JNrDfa7gyGOa"  // Legacy token

// ------------ SENSOR SETUP ----------------------
#define DHT_PIN      2
#define DHT_TYPE     DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Capacitive soil‐moisture sensor v1.2
#define SOIL_PIN     A0

// // --- Light sensor object (commented out)
// BH1750 lightMeter;

#define MOTOR_PIN    3       // Servo signal pin
#define LED_OK       4
#define LED_ALERT    5
#define BUZZER_PIN   6

Servo motorServo;         // Servo motor object

// ------------ GLOBAL STATE ----------------------
WiFiSSLClient wifi;  // 🔐 Encrypted connection
HttpClient client = HttpClient(wifi, FIREBASE_HOST, 443);
String firebasePath = "/sensor_data.json";

void setup() {
  Serial.begin(9600);
  Wire.begin();  // SDA = D11, SCL = D12 on Nano 33 IoT

  dht.begin();
  delay(200);

  // // --- Light sensor initialization (commented out)
  // lightMeter.begin();

  motorServo.attach(MOTOR_PIN);  // Attach servo to D3

  pinMode(LED_OK, OUTPUT);
  pinMode(LED_ALERT, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Note: analog inputs do not require pinMode()
  // pinMode(SOIL_PIN, INPUT);

  connectWiFi();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int soilRaw = analogRead(SOIL_PIN);

  // // --- Read light sensor (commented out)
  // float lux = lightMeter.readLightLevel();

  // Debug: print sensor readings
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print(" °C, Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Soil Raw: ");
  Serial.println(soilRaw);
  
  // // --- Print light sensor value (commented out)
  // Serial.print("Light: ");
  // Serial.print(lux);
  // Serial.println(" lx");

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Sensor error: invalid readings");
    delay(2000);
    return;
  }

  // // --- Alert logic for light (commented out)
  // bool alert = (temp > 35 || hum > 70 || lux < 200);

  bool alert = (temp > 35 || hum > 70);

  // Upload sensor data (including soil moisture)
  JSONVar data;
  data["temp"] = temp;
  data["humidity"] = hum;
  data["soil"] = soilRaw;
  // // --- Send lux to Firebase (commented out)
  // data["lux"] = lux;

  Serial.println("Uploading sensor data to Firebase...");
  sendToFirebase(firebasePath, data);

  // Check motor command (expects boolean true/false in Firebase)
  String motorCommand = getFromFirebase("/commands/motor.json");
  bool motorOverride = (motorCommand == "true");

  Serial.print("Motor override from Firebase: ");
  Serial.println(motorOverride ? "true" : "false");

  // Control logic
  if (alert || motorOverride) {
    Serial.println("ALERT or override active → activating servo + buzzer");
    digitalWrite(LED_OK, LOW);
    digitalWrite(LED_ALERT, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    motorServo.write(90);  // Servo ON (adjust angle as needed)
  } else {
    Serial.println("All normal → system OK");
    digitalWrite(LED_OK, HIGH);
    digitalWrite(LED_ALERT, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    motorServo.write(0);   // Servo OFF position
  }

  delay(3000);
}

// ------------ HELPERS ----------------------

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  while (WiFi.begin(WIFI_SSID, WIFI_PASS) != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" connected");
}

void sendToFirebase(String path, JSONVar json) {
  String content = JSON.stringify(json);

  client.beginRequest();
  client.put(path + "?auth=" + FIREBASE_SECRET);
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", content.length());
  client.beginBody();
  client.print(content);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Firebase PUT Status Code: ");
  Serial.println(statusCode);
  Serial.print("Sent Data: ");
  Serial.println(content);
  Serial.print("Firebase Response: ");
  Serial.println(response);
}

String getFromFirebase(String path) {
  client.get(path + "?auth=" + FIREBASE_SECRET);
  int statusCode = client.responseStatusCode();

  Serial.print("Firebase GET Status Code: ");
  Serial.println(statusCode);

  if (statusCode == 200) {
    String response = client.responseBody();
    response.trim();
    Serial.print("Received: ");
    Serial.println(response);
    return response;
  } else {
    Serial.println("Failed to get motor command");
    return "";
  }
}



