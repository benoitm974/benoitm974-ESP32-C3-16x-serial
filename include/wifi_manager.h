#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
    /**
     * Initialize and connect to WiFi using credentials.h
     * @return true if connected successfully, false otherwise
     */
    bool init();

    /**
     * Check if WiFi is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    /**
     * Get IP address as string
     * @return IP address string
     */
    String getIPAddress() const;

    /**
     * Get last 3 digits of IP address for OLED display
     * @return last 3 digits as string (e.g., "123" from "192.168.1.123")
     */
    String getIPLast3Digits() const;

private:
    bool connected = false;
};

#endif // WIFI_MANAGER_H