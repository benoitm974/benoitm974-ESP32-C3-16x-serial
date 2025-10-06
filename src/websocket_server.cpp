#include "websocket_server.h"
#include "multiplexer.h"

// Static instance for callback
static WebSocketServer* instance = nullptr;
static MultiplexerController* multiplexerInstance = nullptr;
static HardwareSerial* serialSBC = nullptr;

void WebSocketServer::setReferences(MultiplexerController* multiplexer, HardwareSerial* serial) {
    multiplexerInstance = multiplexer;
    serialSBC = serial;
}

bool WebSocketServer::init() {
    instance = this;
    
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("LittleFS initialization failed!");
        return false;
    }
    Serial.println("LittleFS initialized successfully");
    
    // Initialize WebSocket server
    webSocket = new WebSocketsServer(WEBSOCKET_PORT);
    webSocket->begin();
    webSocket->onEvent(webSocketEvent);
    
    // Initialize HTTP server
    httpServer = new WiFiServer(HTTP_PORT);
    httpServer->begin();
    
    initialized = true;
    Serial.print("WebSocket server started on port ");
    Serial.println(WEBSOCKET_PORT);
    Serial.print("HTTP server started on port ");
    Serial.println(HTTP_PORT);
    return true;
}

void WebSocketServer::loop() {
    if (!initialized) return;
    
    webSocket->loop();
    handleHTTPClient();
}

void WebSocketServer::setChannel(int channel) {
    if (channel >= 0 && channel < MAX_CHANNELS && multiplexerInstance) {
        if (multiplexerInstance->selectChannel(channel)) {
            currentChannel = channel;
            Serial.print("Switched to channel: ");
            Serial.println(channel);
        } else {
            Serial.print("Failed to switch to channel: ");
            Serial.println(channel);
        }
    }
}

void WebSocketServer::broadcast(const String& data) {
    if (!initialized) return;
    webSocket->broadcastTXT(data.c_str());
}

void WebSocketServer::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (!instance) return;
    
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("WebSocket client %u disconnected\n", num);
            break;
            
        case WStype_CONNECTED:
            Serial.printf("WebSocket client %u connected\n", num);
            break;
            
        case WStype_TEXT:
            {
                // Ensure payload is null-terminated and valid UTF-8
                if (length == 0) break;
                
                // Validate that all bytes are valid UTF-8 before processing
                bool isValidUTF8 = true;
                for (size_t i = 0; i < length; i++) {
                    if (payload[i] > 127) {
                        // For simplicity, reject any non-ASCII characters
                        // This prevents UTF-8 decoding errors from SBC boot data
                        isValidUTF8 = false;
                        break;
                    }
                }
                
                if (!isValidUTF8) {
                    Serial.printf("WebSocket client %u sent non-ASCII data - ignoring\n", num);
                    break;
                }
                
                // Create a proper null-terminated string
                char* buffer = new char[length + 1];
                memcpy(buffer, payload, length);
                buffer[length] = '\0';
                
                String message = String(buffer);
                delete[] buffer;
                
                // Handle channel commands
                if (message.startsWith("CHANNEL:")) {
                    instance->handleChannelCommand(message);
                } else {
                    // Forward character-by-character to serial SBC
                    if (serialSBC && message.length() > 0) {
                        // Send each character immediately
                        for (int i = 0; i < message.length(); i++) {
                            serialSBC->write(message[i]);
                        }
                        Serial.print("WS->SBC: ");
                        Serial.print(message);
                    }
                }
            }
            break;
            
        case WStype_BIN:
            // Handle binary data - just ignore it for now
            Serial.printf("WebSocket client %u sent binary data (%u bytes) - ignoring\n", num, length);
            break;
            
        case WStype_ERROR:
            Serial.printf("WebSocket client %u error\n", num);
            break;
            
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            // Handle fragmented messages - ignore for now
            Serial.printf("WebSocket client %u sent fragmented data - ignoring\n", num);
            break;
            
        default:
            break;
    }
}

void WebSocketServer::handleHTTPClient() {
    WiFiClient client = httpServer->available();
    if (client) {
        String request = client.readStringUntil('\r');
        client.flush();
        
        // Parse the requested path
        String path = "/";
        int pathStart = request.indexOf(' ') + 1;
        int pathEnd = request.indexOf(' ', pathStart);
        if (pathStart > 0 && pathEnd > pathStart) {
            path = request.substring(pathStart, pathEnd);
        }
        
        // Default to index.html for root path
        if (path == "/") {
            path = "/index.html";
        }
        
        // Serve the requested file
        serveFile(client, path);
        
        client.stop();
        Serial.print("HTTP client served: ");
        Serial.println(path);
    }
}

void WebSocketServer::serveFile(WiFiClient& client, const String& path) {
    // Find the actual file (may have .gz extension)
    String actualPath = findFile(path);
    
    if (actualPath.isEmpty()) {
        // File not found - send 404
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("File not found");
        return;
    }
    
    // Open the file
    File file = LittleFS.open(actualPath, "r");
    if (!file) {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Failed to open file");
        return;
    }
    
    // Get MIME type
    String mimeType = getMimeType(path);
    bool isGzipped = actualPath.endsWith(".gz");
    
    // Send HTTP headers
    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: ");
    client.println(mimeType);
    client.print("Content-Length: ");
    client.println(file.size());
    
    if (isGzipped) {
        client.println("Content-Encoding: gzip");
    }
    
    client.println("Cache-Control: max-age=86400"); // Cache for 24 hours
    client.println("Connection: close");
    client.println();
    
    // Send file content
    while (file.available()) {
        client.write(file.read());
    }
    
    file.close();
}

String WebSocketServer::getMimeType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".svg")) return "image/svg+xml";
    if (filename.endsWith(".json")) return "application/json";
    if (filename.endsWith(".txt")) return "text/plain";
    if (filename.endsWith(".ico")) return "image/x-icon";
    return "text/plain";
}

String WebSocketServer::findFile(const String& path) {
    // First try the exact path
    if (LittleFS.exists(path)) {
        return path;
    }
    
    // Then try with .gz extension
    String gzPath = path + ".gz";
    if (LittleFS.exists(gzPath)) {
        return gzPath;
    }
    
    // File not found
    return "";
}

void WebSocketServer::handleChannelCommand(const String& command) {
    int channel = command.substring(8).toInt(); // Remove "CHANNEL:" prefix
    setChannel(channel);
}

bool WebSocketServer::hasConnectedClients() {
    if (!initialized || !webSocket) return false;
    return webSocket->connectedClients() > 0;
}

int WebSocketServer::getCurrentChannel() {
    return currentChannel;
}