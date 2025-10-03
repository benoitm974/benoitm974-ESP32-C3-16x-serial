#ifndef OLED_MANAGER_H
#define OLED_MANAGER_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pins.h"

// Display offset configuration for ABROBOT ESP32-C3 board
// ABROBOT specification: 72×40 effective resolution with offset (30, 12)
// Libraries require 128×64 setting but drawing limited to 72×40 to avoid clipping
#define DISPLAY_OFFSET_X 30
#define DISPLAY_OFFSET_Y 12
#define VISIBLE_WIDTH 72
#define VISIBLE_HEIGHT 40

class OLEDManager {
public:
    /**
     * Initialize OLED display
     * @return true if initialized successfully, false otherwise
     */
    bool init();

    /**
     * Display IP address last 3 digits on OLED
     * @param ipLast3 Last 3 digits of IP address (e.g., "123")
     */
    void displayIP(const String& ipLast3);

    /**
     * Display system status with IP, WebSocket connection, and SBC channel
     * @param ipLast3 Last 3 digits of IP address (e.g., "123")
     * @param wsConnected WebSocket connection status
     * @param sbcChannel Current SBC channel (0-4)
     */
    void displayStatus(const String& ipLast3, bool wsConnected, int sbcChannel);

    /**
     * Clear the display
     */
    void clear();

    /**
     * Update display with current content
     */
    void update();

private:
    Adafruit_SSD1306* display = nullptr;
    bool initialized = false;
};

#endif // OLED_MANAGER_H