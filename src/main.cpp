#include <Arduino.h>
#include <HardwareSerial.h>

// Project includes
#include "pins.h"
#include "wifi_manager.h"
#include "oled_manager.h"
#include "websocket_server.h"
#include "multiplexer.h"

// Global instances
WiFiManager wifiManager;
OLEDManager oledManager;
WebSocketServer webSocketServer;
MultiplexerController multiplexer;

// Serial communication
HardwareSerial SerialSBC(1); // Use UART1 for SBC communication

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32-C3 Serial Multiplexer starting...");
    
    // Initialize status LED (inverted logic - HIGH = OFF, LOW = ON)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);  // Turn OFF initially
    Serial.println("Status LED initialized");
    
    // Initialize OLED display first
    if (!oledManager.init()) {
        Serial.println("OLED initialization failed");
        return;
    }
    
    // Initialize multiplexer
    multiplexer.init();
    multiplexer.selectChannel(0); // Start with SBC1
    Serial.println("Multiplexer initialized - Channel 0 selected");
    
    // Initialize SBC serial communication
    SerialSBC.begin(UART_BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("SBC Serial initialized");
    
    // Initialize WiFi
    if (!wifiManager.init()) {
        Serial.println("WiFi connection failed");
        oledManager.displayIP("ERR");
        return;
    }
    
    // Display initial status on OLED
    String ipLast3 = wifiManager.getIPLast3Digits();
    oledManager.displayStatus(ipLast3, false, 0); // WebSocket not connected yet, SBC1 selected
    Serial.print("WiFi connected - IP last 3 digits: ");
    Serial.println(ipLast3);
    
    // Initialize WebSocket server
    if (!webSocketServer.init()) {
        Serial.println("WebSocket server initialization failed");
        return;
    }
    
    // Set references for WebSocket server
    webSocketServer.setReferences(&multiplexer, &SerialSBC);
    
    Serial.println("ESP32-C3 Serial Multiplexer ready!");
    Serial.print("Access web interface at: http://");
    Serial.println(wifiManager.getIPAddress());
}

void loop() {
    static unsigned long lastDisplayUpdate = 0;
    static unsigned long lastLedBlink = 0;
    static bool ledState = false;
    
    // Handle WebSocket server
    webSocketServer.loop();
    
    // Forward data from SBC to WebSocket clients (character by character)
    if (SerialSBC.available()) {
        while (SerialSBC.available()) {
            char c = SerialSBC.read();
            String charStr = String(c);
            webSocketServer.broadcast(charStr);
            Serial.print(c); // Echo to debug console
        }
    }
    
    // Handle status LED blinking when WebSocket connected (inverted logic)
    bool wsConnected = webSocketServer.hasConnectedClients();
    if (wsConnected) {
        // Blink LED every 500ms when connected
        if (millis() - lastLedBlink > 500) {
            ledState = !ledState;
            digitalWrite(STATUS_LED_PIN, ledState ? LOW : HIGH);  // LOW = ON, HIGH = OFF
            lastLedBlink = millis();
        }
    } else {
        // Turn off LED when disconnected
        digitalWrite(STATUS_LED_PIN, HIGH);  // HIGH = OFF
        ledState = false;
    }
    
    // Update OLED display periodically (every 0.8 seconds)
    if (millis() - lastDisplayUpdate > 800) {
        if (wifiManager.isConnected()) {
            String ipLast3 = wifiManager.getIPLast3Digits();
            int currentChannel = webSocketServer.getCurrentChannel();
            oledManager.displayStatus(ipLast3, wsConnected, currentChannel);
        } else {
            oledManager.displayStatus("---", false, 0);
        }
        lastDisplayUpdate = millis();
    }
    
    // Small delay to prevent overwhelming the system
    delay(10);
}