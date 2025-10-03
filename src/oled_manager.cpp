#include "oled_manager.h"

bool OLEDManager::init() {
    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Create display object
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    
    // Initialize display
    if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        return false;
    }
    
    // Clear display and set text properties
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    
    // Show initialization message (using proper text alignment)
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    
    // Center "WiFi" text within visible area (moved down 6px)
    int16_t x1, y1;
    uint16_t w, h;
    display->getTextBounds("WiFi", 0, 0, &x1, &y1, &w, &h);
    display->setCursor(DISPLAY_OFFSET_X + (VISIBLE_WIDTH - w) / 2, DISPLAY_OFFSET_Y + 14);
    display->println("WiFi");
    
    // Center "Starting..." text within visible area (moved down 6px)
    display->getTextBounds("Starting...", 0, 0, &x1, &y1, &w, &h);
    display->setCursor(DISPLAY_OFFSET_X + (VISIBLE_WIDTH - w) / 2, DISPLAY_OFFSET_Y + 26);
    display->println("Starting...");
    display->display();
    
    initialized = true;
    Serial.println("OLED display initialized");
    return true;
}

void OLEDManager::displayIP(const String& ipLast3) {
    if (!initialized || !display) {
        return;
    }
    
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    
    // WiFi IP display using proper text alignment
    int16_t x1, y1;
    uint16_t w, h;
    
    // Center "WiFi IP:" label within visible area (moved down 6px)
    display->setTextSize(1);
    display->getTextBounds("WiFi IP:", 0, 0, &x1, &y1, &w, &h);
    display->setCursor(DISPLAY_OFFSET_X + (VISIBLE_WIDTH - w) / 2, DISPLAY_OFFSET_Y + 14);
    display->println("WiFi IP:");
    
    // Center IP digits within visible area (larger text, moved down 6px)
    display->setTextSize(2);
    display->getTextBounds(ipLast3, 0, 0, &x1, &y1, &w, &h);
    display->setCursor(DISPLAY_OFFSET_X + (VISIBLE_WIDTH - w) / 2, DISPLAY_OFFSET_Y + 30);
    display->println(ipLast3);
    
    display->display();
}

void OLEDManager::displayStatus(const String& ipLast3, bool wsConnected, int sbcChannel) {
    if (!initialized || !display) {
        return;
    }
    
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);
    
    int16_t x1, y1;
    uint16_t w, h;
    
    // Top: IP address last 3 digits (centered within visible area, moved down 6px)
    display->setTextSize(2);
    display->getTextBounds(ipLast3, 0, 0, &x1, &y1, &w, &h);
    display->setCursor(DISPLAY_OFFSET_X + (VISIBLE_WIDTH - w) / 2, DISPLAY_OFFSET_Y + 14);
    display->print(ipLast3);
    
    // Bottom: SBC status (centered within visible area, moved down 6px)
    display->setTextSize(2);
    String statusText = wsConnected ? String(sbcChannel + 1) : "X";
    display->getTextBounds(statusText, 0, 0, &x1, &y1, &w, &h);
    display->setCursor(DISPLAY_OFFSET_X + (VISIBLE_WIDTH - w) / 2, DISPLAY_OFFSET_Y + 34);
    display->print(statusText);
    
    display->display();
}

void OLEDManager::clear() {
    if (!initialized || !display) {
        return;
    }
    
    display->clearDisplay();
    display->display();
}

void OLEDManager::update() {
    if (!initialized || !display) {
        return;
    }
    
    display->display();
}