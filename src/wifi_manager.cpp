#include "wifi_manager.h"
#include "credentials.h"

bool WiFiManager::init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    const int timeout = 100;  // 10 seconds timeout (100 * 100ms)
    
    while (WiFi.status() != WL_CONNECTED && attempts < timeout) {
        delay(100);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        Serial.println();
        Serial.print("WiFi connected! IP: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println();
        Serial.println("WiFi connection failed");
        connected = false;
        return false;
    }
}

bool WiFiManager::isConnected() const {
    return connected && WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIPAddress() const {
    return WiFi.localIP().toString();
}

String WiFiManager::getIPLast3Digits() const {
    if (!isConnected()) {
        return "---";
    }
    
    String ip = WiFi.localIP().toString();
    int lastDot = ip.lastIndexOf('.');
    if (lastDot != -1) {
        return ip.substring(lastDot + 1);
    }
    return "---";
}