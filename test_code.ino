////////////////////////////////////////////////////
//                                                //
// Proof of concept MQTT code                     //
//                                                //
////////////////////////////////////////////////////


/* 

	History:
	Sept 7, 2026 RMB Initial generation

*/


#include <WiFi.h>          
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- System Tracking ---
int currentActiveSelection = -1; // -1 means no 1-of-16 button is selected
unsigned char extraButtonsLEDs = 0x00; // Bit 0=SW17, Bit 1=SW18, Bit 2=SW19

// --- Network & MQTT Settings ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "://hivemq.com"; 

const char* topic_selection = "devices/matrix/selection";
const char* topic_extra_switches = "devices/matrix/extra_switches";

WiFiClient espClient;
PubSubClient client(espClient);

// Tracks the last processed raw key register to prevent spamming MQTT messages
unsigned char lastRawKeys = 0; 

// --- YOUR EXISTING HARDWARE ROUTINES ---
// Keep your existing definitions for these two functions intact.
unsigned char get_keys() {
  // Your existing shift-register read implementation
}

void update_display(unsigned long display_word, unsigned char keyleds) {
  // Your existing shift-register write implementation
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
  
  // Set initial physical state of the display (All Red ON, All Green OFF)
  refreshPhysicalDisplay();
}

// --- Helper: Calculate 32-bit word and push to shift registers ---
void refreshPhysicalDisplay() {
  unsigned long display_word = 0x00000000;

  if (currentActiveSelection == -1) {
    // Idle state: All 16 Red LEDs ON (bits 0-15), All Green OFF (bits 16-31)
    display_word = 0x0000FFFF; 
  } else {
    // 1-of-N Selected State:
    // 1. Turn on ALL Red LEDs except the active index bit
    unsigned long red_mask = 0x0000FFFF;
    red_mask &= ~(1UL << currentActiveSelection); 
    
    // 2. Turn on ONLY the Green LED at the active index bit (shifted left by 16)
    unsigned long green_mask = (1UL << (currentActiveSelection + 16));
    
    display_word = red_mask | green_mask;
  }

  // Push the calculated states directly to your hardware routine
  update_display(display_word, extraButtonsLEDs);
}

// --- Broker Response Callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  if (String(topic) == topic_selection) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) return;

    // Acknowledge selection verified by the broker
    currentActiveSelection = doc["active_index"]; 
    
    // Instantly update the 32-bit shift registers
    refreshPhysicalDisplay();
  }
}

void loop() {
  if (!client.connected()) { reconnect(); }
  client.loop(); 

  // --- Read Shift Registers ---
  unsigned char rawKeys = get_keys();

  // Only act if the button state changed
  if (rawKeys != lastRawKeys) {
    
    // 1. Handle the 1-of-16 Matrix Buttons (Check "Data Ready" Bit 4)
    bool dataReady = (rawKeys & 0x10) != 0;
    bool wasDataReady = (lastRawKeys & 0x10) != 0;

    // Detect a fresh press edge where Data Ready transitioned from LOW to HIGH
    if (dataReady && !wasDataReady) {
      int physicalPressedKey = rawKeys & 0x0F; // Extract lower 4 bits (0-15)
      int targetSelection;

      if (currentActiveSelection == physicalPressedKey) {
        targetSelection = -1; // Pressed a second time -> turn off completely
      } else {
        targetSelection = physicalPressedKey; // Switch to the new key choice
      }

      // Package up request to broker
      JsonDocument doc;
      doc["active_index"] = targetSelection;
      char buffer[64];
      serializeJson(doc, buffer);

      client.publish(topic_selection, buffer);
    }

    // 2. Handle Extra Buttons: SW17 (Bit 7), SW18 (Bit 6), SW19 (Bit 5)
    // Compare current state bits vs last state bits to isolate fresh presses
    unsigned char changes = rawKeys ^ lastRawKeys;
    
    if (changes & 0x80) { // SW17 changed state
      bool sw17_pressed = (rawKeys & 0x80) != 0;
      client.publish(topic_extra_switches, sw17_pressed ? "SW17_DOWN" : "SW17_UP");
    }
    if (changes & 0x40) { // SW18 changed state
      bool sw18_pressed = (rawKeys & 0x40) != 0;
      client.publish(topic_extra_switches, sw18_pressed ? "SW18_DOWN" : "SW18_UP");
    }
    if (changes & 0x20) { // SW19 changed state
      bool sw19_pressed = (rawKeys & 0x20) != 0;
      client.publish(topic_extra_switches, sw19_pressed ? "SW19_DOWN" : "SW19_UP");
    }

    lastRawKeys = rawKeys; // Save state
    delay(50); // Small debounce window for shift registers
  }
}

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ArduinoMatrixController")) {
      client.subscribe(topic_selection);
      // If the second client should also control SW17-19 LEDs, subscribe here
    } else { delay(5000); }
  }
}

