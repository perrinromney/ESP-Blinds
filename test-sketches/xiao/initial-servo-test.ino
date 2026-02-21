#include <Arduino.h>
#include <ESP32Servo.h>  // Use the standard ESP32Servo library
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <RadioLib.h>

// --- Pin Mapping ---
const int PIN_MOSFET = D3;
const int PIN_SERVO  = D2; // XIAO D2
const int PIN_CSN    = D0; // D0
const int PIN_GDO0   = D1;  // D1
const int PIN_SCK    = D8;
const int PIN_MISO   = D9;
const int PIN_MOSI   = D10;

// Objects
CC1101 radio = new Module(PIN_CSN, PIN_GDO0, RADIOLIB_NC, RADIOLIB_NC);
Adafruit_INA219 ina219;
Servo myServo; 

void setup() {
    pinMode(PIN_MOSFET, OUTPUT);
    digitalWrite(PIN_MOSFET, LOW);

    Serial.begin(115200);
    while (!Serial) delay(10);
    

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

    // 3. Setup Radio (RadioLib)
    SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CSN);
    int state = radio.begin(433.0, 4.8, 48.0, 10, 16, 0);
    if (state == RADIOLIB_ERR_NONE) Serial.println("[+] Radio: OK");

    Serial.println(F("\n--- SYSTEM READY (270° MOD) ---"));
    Serial.println(F("Commands: 'on', 'off', or angle '0'-'270'"));
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.equalsIgnoreCase("on")) {
            digitalWrite(PIN_MOSFET, HIGH);
            Serial.println(F("[!] MOSFET CLOSED (Power ON)"));
        } 
        else if (input.equalsIgnoreCase("off")) {
            digitalWrite(PIN_MOSFET, LOW);
            Serial.println(F("[!] MOSFET OPEN (Power OFF)"));
        } 
        else {
            float targetAngle = input.toFloat();
            
            // Constrain input to the physical 270 limit
            if (targetAngle >= 0 && targetAngle <= 270) {
                Serial.printf("Moving to: %.1f°\n", targetAngle);
                
                // The .write() function in ESP32Servo maps 0-180 
                // We use map() to translate our 0-270 request to that scale
                int pulseToMove = map(targetAngle, 0, 270, 0, 180);
                myServo.write(pulseToMove); 

                // Give it time to move (Blocking simulation)
                delay(1000); 
                
                float ma = ina219.getCurrent_mA();
                Serial.printf("Position reached. Current: %.1f mA\n", ma);
            } else {
                Serial.println("[-] Out of range (0-270 only)");
            }
        }
    }
}