#include "oled_manager.h"

bool OLEDManager::init() {
    // Create u8g2 display object with hardware I2C
    // Constructor: u8g2_ssd1306_72x40_er_f_hw_i2c(rotation, reset_pin, scl_pin, sda_pin)
    display = new U8G2_SSD1306_72X40_ER_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);
    
    // Initialize display
    display->begin();
    
    // Clear display and set font
    display->clearBuffer();
    display->setFont(u8g2_font_6x10_tf);  // Small, clear font for status display
    
    // Show initialization message
    display->clearBuffer();
    
    // Center "WiFi" text
    const char* wifi_text = "WiFi";
    int wifi_width = display->getStrWidth(wifi_text);
    display->setCursor((DISPLAY_WIDTH - wifi_width) / 2, 16);
    display->print(wifi_text);
    
    // Center "Starting..." text
    const char* start_text = "Starting...";
    int start_width = display->getStrWidth(start_text);
    display->setCursor((DISPLAY_WIDTH - start_width) / 2, 28);
    display->print(start_text);
    
    display->sendBuffer();
    
    initialized = true;
    Serial.println("OLED display initialized with u8g2");
    return true;
}

void OLEDManager::displayIP(const String& ipLast3) {
    if (!initialized || !display) {
        return;
    }
    
    display->clearBuffer();
    
    // Display "WiFi IP:" label
    const char* label = "WiFi IP:";
    int label_width = display->getStrWidth(label);
    display->setCursor((DISPLAY_WIDTH - label_width) / 2, 16);
    display->print(label);
    
    // Display IP digits in larger font
    display->setFont(u8g2_font_10x20_tf);  // Larger font for IP
    int ip_width = display->getStrWidth(ipLast3.c_str());
    display->setCursor((DISPLAY_WIDTH - ip_width) / 2, 35);
    display->print(ipLast3);
    
    // Reset to normal font
    display->setFont(u8g2_font_6x10_tf);
    
    display->sendBuffer();
}

void OLEDManager::displayStatus(const String& ipLast3, bool wsConnected, int sbcChannel) {
    if (!initialized || !display) {
        return;
    }
    
    display->clearBuffer();
    
    // Top: IP address last 3 digits (larger font)
    display->setFont(u8g2_font_10x20_tf);
    int ip_width = display->getStrWidth(ipLast3.c_str());
    display->setCursor((DISPLAY_WIDTH - ip_width) / 2, 20);
    display->print(ipLast3);
    
    // Bottom: SBC status (larger font)
    String statusText = wsConnected ? String(sbcChannel + 1) : "X";
    int status_width = display->getStrWidth(statusText.c_str());
    display->setCursor((DISPLAY_WIDTH - status_width) / 2, 38);
    display->print(statusText);
    
    // Reset to normal font
    display->setFont(u8g2_font_6x10_tf);
    
    display->sendBuffer();
}

void OLEDManager::clear() {
    if (!initialized || !display) {
        return;
    }
    
    display->clearBuffer();
    display->sendBuffer();
}

void OLEDManager::update() {
    if (!initialized || !display) {
        return;
    }
    
    display->sendBuffer();
}