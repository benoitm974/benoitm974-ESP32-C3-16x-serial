#ifndef OLED_MANAGER_H
#define OLED_MANAGER_H

#include <U8g2lib.h>
#include "pins.h"

// u8g2 library natively supports 72×40 SSD1306 displays
// No offset calculations needed - direct pixel addressing
#define DISPLAY_WIDTH 72
#define DISPLAY_HEIGHT 40

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
    U8G2_SSD1306_72X40_ER_F_HW_I2C* display = nullptr;
    bool initialized = false;
};

#endif // OLED_MANAGER_H