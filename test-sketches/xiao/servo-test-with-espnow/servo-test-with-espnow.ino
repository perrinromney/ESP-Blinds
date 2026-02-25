#include <Arduino.h>
#include <ESP32Servo.h>  // Use the standard ESP32Servo library
#include <Adafruit_INA219.h>
#include <esp_now.h>
#include <WiFi.h>

// --- Pin Mapping ---
const int PIN_MOSFET = D3;
const int PIN_SERVO  = D2; // XIAO D2
const int PIN_CSN    = D0; // D0
const int PIN_GDO0   = D1;  // D1
const int PIN_SCK    = D8;
const int PIN_MISO   = D9;
const int PIN_MOSI   = D10;

// Objects
Adafruit_INA219 ina219;
Servo myServo; 

// --- ESP-NOW Globals ---
typedef struct struct_message {
  char text[64];
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;
bool msgReceived = false;
String receivedString = "";

// Universal MAC Address for broadcast
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Forward declaration
void sendTelemetry(String msg);

// Data received callback
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  msgReceived = true;
  receivedString = String(myData.text);
}

void setup() {
    pinMode(PIN_MOSFET, OUTPUT);
    digitalWrite(PIN_MOSFET, LOW);

    Serial.begin(115200);
    // while (!Serial) delay(10); // Don't block if not plugged in
    
    // --- Init ESP-NOW ---
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Register peer for sending telemetry back
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer for telemetry");
    }

    // Register callback
    esp_now_register_recv_cb(OnDataRecv);


    // 1. Setup Servo for ESP32-C3
    // We allocate a timer explicitly for the C3's LEDC hardware
    ESP32PWM::allocateTimer(0);
    myServo.setPeriodHertz(50); // Standard 50Hz for Servos
    
    // 270 Degree Mapping: 500us (0°) to 2500us (270°)
    if (!myServo.attach(PIN_SERVO, 500, 2500)) {
        Serial.println("[-] Servo Attach Failed!");
    }

    // 2. Setup INA219
    if (!ina219.begin()) Serial.println("[-] INA219 Fail!");


    Serial.println(F("\n--- SYSTEM READY (ESP-NOW RCV) ---"));
    sendTelemetry("--- SYSTEM READY (ESP-NOW RCV) ---");
}

// Function to send telemetry back
void sendTelemetry(String msg) {
  struct_message telemetry;
  msg.toCharArray(telemetry.text, 64);
  esp_now_send(broadcastAddress, (uint8_t *) &telemetry, sizeof(telemetry));
  // Keep local serial for debugging
  Serial.println(msg); 
}

void loop() {
    if (msgReceived) {
        msgReceived = false; // Reset flag
        String input = receivedString;
        input.trim();
        
        // Echo command back for confirmation
        // sendTelemetry("Received cmd: " + input);

        if (input.equalsIgnoreCase("on")) {
            digitalWrite(PIN_MOSFET, HIGH);
            sendTelemetry("[!] MOSFET CLOSED (Power ON)");
        } 
        else if (input.equalsIgnoreCase("off")) {
            digitalWrite(PIN_MOSFET, LOW);
            sendTelemetry("[!] MOSFET OPEN (Power OFF)");
        } 
        else {
            float targetAngle = input.toFloat();
            
            // Constrain input to the physical 270 limit
            if (targetAngle >= 0 && targetAngle <= 270) {
                // Formatting string for telemetry
                char buf[64];
                sprintf(buf, "Moving to: %.1f deg", targetAngle);
                sendTelemetry(buf);
                
                // The .write() function in ESP32Servo maps 0-180 
                // We use map() to translate our 0-270 request to that scale
                int pulseToMove = map(targetAngle, 0, 270, 0, 180);
                myServo.write(pulseToMove); 

                // Give it time to move (Blocking simulation)
                delay(1000); 
                
                float ma = ina219.getCurrent_mA();
                sprintf(buf, "Position reached. Current: %.1f mA", ma);
                sendTelemetry(buf);
            } else {
                sendTelemetry("[-] Out of range (0-270 only)");
            }
        }
    }
}