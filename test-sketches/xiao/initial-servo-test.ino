#include <WiFi.h>
#include <ETH.h>

const char* ssid = "ROMNEY";
const char* password = "TikkaHunter";

// Define the pins exactly for WT32-ETH01
#define ETH_PHY_ADDR     1
#define ETH_PHY_POWER    16
#define ETH_PHY_MDC      23
#define ETH_PHY_MDIO     18
#define ETH_PHY_TYPE     ETH_PHY_LAN8720
#define ETH_CLK_MODE     ETH_CLOCK_GPIO0_IN // If this fails, try ETH_CLOCK_GPIO17_OUT

bool wifiSuccess = false;
bool ethSuccess = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- WT32-ETH01 Hardware Isolation Test ---");

  // 1. TEST WIFI FIRST
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter < 15) {
    delay(500); Serial.print("."); counter++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[PASS] WiFi Connected.");
    wifiSuccess = true;
    // IMPORTANT: Now we turn it OFF to give Ethernet full power/bus access
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disabled to isolate Ethernet hardware...");
    delay(1000); 
  }

  // 2. HARD RESET ETHERNET
  pinMode(ETH_PHY_POWER, OUTPUT);
  digitalWrite(ETH_PHY_POWER, LOW);
  delay(500);
  digitalWrite(ETH_PHY_POWER, HIGH);
  delay(500);

  // 3. TEST ETHERNET
  Serial.println("Testing Ethernet...");
  // Using the Core 3.x specific begin
  bool ethStarted = ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_MDC, ETH_PHY_MDIO, -1, ETH_CLK_MODE);

  counter = 0;
  while (ETH.localIP().toString() == "0.0.0.0" && counter < 20) {
    delay(500); Serial.print("."); counter++;
  }

  if (ETH.localIP().toString() != "0.0.0.0") {
    Serial.println("\n[PASS] Ethernet Connected!");
    ethSuccess = true;
  } else {
    Serial.println("\n[FAIL] Ethernet still timing out.");
  }

  // Final Results
  Serial.println("\n========= FINAL RESULTS =========");
  Serial.print("WIFI (Initial): "); Serial.println(wifiSuccess ? "SUCCESS" : "FAILED");
  Serial.print("ETHERNET:       "); Serial.println(ethSuccess ? "SUCCESS" : "FAILED");
}

void loop() {}