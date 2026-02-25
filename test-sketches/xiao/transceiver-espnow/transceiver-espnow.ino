#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Universal MAC Address for broadcast (sends to everyone)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure to send/receive data
// Make sure this matches the receiver structure
typedef struct struct_message {
  char text[64];
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Verification callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Optional: print validation locally if needed
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Data received callback
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  struct_message receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  // Forward received ESP-NOW message to Serial Monitor
  Serial.println(receivedData.text);
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register for Send Callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // Register for Recv Callback
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("Transceiver Ready. Type commands here to send to receiver.");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      // Copy String to char array
      input.toCharArray(myData.text, 64);
      
      // Send message via ESP-NOW
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      
      if (result == ESP_OK) {
        Serial.println("Sent: " + input);
      } else {
        Serial.println("Error sending the data");
      }
    }
  }
}
