#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <WebSocketsServer.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include "pins.h"

// Forward declaration
class MultiplexerController;

class WebSocketServer {
public:
    /**
     * Initialize WebSocket server and HTTP server
     * @return true if initialized successfully, false otherwise
     */
    bool init();

    /**
     * Handle WebSocket and HTTP server events
     */
    void loop();

    /**
     * Set references to multiplexer and serial for channel switching
     * @param multiplexer Pointer to multiplexer controller
     * @param serial Pointer to hardware serial for SBC communication
     */
    void setReferences(MultiplexerController* multiplexer, HardwareSerial* serial);

    /**
     * Set the current serial channel (0-4 for SBC1-SBC5)
     * @param channel Channel number (0-4)
     */
    void setChannel(int channel);

    /**
     * Send data to all connected WebSocket clients
     * @param data Data to send
     */
    void broadcast(const String& data);

    /**
     * Get current WebSocket connection status
     * @return true if at least one client is connected
     */
    bool hasConnectedClients();

    /**
     * Get current SBC channel
     * @return Current channel number (0-4)
     */
    int getCurrentChannel();

private:
    WebSocketsServer* webSocket = nullptr;
    WiFiServer* httpServer = nullptr;
    int currentChannel = 0;
    bool initialized = false;

    /**
     * WebSocket event handler
     */
    static void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    
    /**
     * Handle HTTP requests
     */
    void handleHTTPClient();
    
    /**
     * Get embedded HTML page
     */
    String getHTMLPage();
    
    /**
     * Handle channel selection command
     */
    void handleChannelCommand(const String& command);
};

#endif // WEBSOCKET_SERVER_H