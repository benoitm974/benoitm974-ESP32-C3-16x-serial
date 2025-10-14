#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <WebSocketsServer.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
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
     * Add character to buffer for UTF-8 processing
     * @param c Character to add to buffer
     */
    void addToBuffer(char c);

    /**
     * Force flush the character buffer
     */
    void flushBuffer();

    /**
     * Send binary data to all connected WebSocket clients
     * @param data Binary data to send
     * @param length Length of data in bytes
     */
    void broadcastBinary(const uint8_t* data, size_t length);

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

    // Character buffering for UTF-8 handling
    static const size_t BUFFER_SIZE = 256;
    static const unsigned long BUFFER_TIMEOUT_MS = 50;

    uint8_t charBuffer[BUFFER_SIZE];
    size_t bufferPos = 0;
    unsigned long lastBufferTime = 0;

    /**
     * WebSocket event handler
     */
    static void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    
    /**
     * Handle HTTP requests
     */
    void handleHTTPClient();
    
    /**
     * Serve file from LittleFS with proper MIME type and gzip handling
     * @param client WiFi client to serve to
     * @param path File path to serve
     */
    void serveFile(WiFiClient& client, const String& path);
    
    /**
     * Get MIME type for file extension
     * @param filename File name or path
     * @return MIME type string
     */
    String getMimeType(const String& filename);
    
    /**
     * Check if file exists in LittleFS (with or without .gz extension)
     * @param path File path to check
     * @return actual file path if found, empty string if not found
     */
    String findFile(const String& path);
    
    /**
     * Handle channel selection command
     */
    void handleChannelCommand(const String& command);

    /**
     * Check if buffer contains valid UTF-8 sequence
     * @param data Buffer data to check
     * @param length Buffer length
     * @return true if valid UTF-8, false otherwise
     */
    bool isValidUTF8Sequence(const uint8_t* data, size_t length);

    /**
     * Send buffered data using appropriate frame type
     */
    void sendBufferedData();
};

#endif // WEBSOCKET_SERVER_H