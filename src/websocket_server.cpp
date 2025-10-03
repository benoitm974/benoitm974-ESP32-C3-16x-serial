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
                String message = String((char*)payload);
                
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
            
        default:
            break;
    }
}

void WebSocketServer::handleHTTPClient() {
    WiFiClient client = httpServer->available();
    if (client) {
        String request = client.readStringUntil('\r');
        client.flush();
        
        // Send HTTP response with embedded HTML
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println(getHTMLPage());
        
        client.stop();
        Serial.println("HTTP client served");
    }
}

String WebSocketServer::getHTMLPage() {
    String html = "<!DOCTYPE html><html><head><title>ESP32 Serial Terminal</title>";
    html += "<style>";
    html += "body{margin:0;padding:0;background:#000;color:#00ff00;font-family:'Courier New',monospace;overflow:hidden;}";
    html += ".container{height:100vh;display:flex;flex-direction:column;}";
    html += ".header{background:#111;padding:10px;border-bottom:1px solid #333;}";
    html += ".header h1{margin:0;font-size:18px;color:#00ff00;}";
    html += ".channel-buttons{margin:5px 0;}";
    html += "button{background:#222;color:#00ff00;border:1px solid #00ff00;padding:5px 10px;margin:2px;cursor:pointer;font-family:inherit;}";
    html += "button:hover{background:#00ff00;color:#000;}";
    html += "button.active{background:#00ff00;color:#000;}";
    html += "#terminal{flex:1;padding:10px;overflow-y:auto;background:#000;color:#00ff00;white-space:pre-wrap;font-size:14px;line-height:1.2;}";
    html += ".status{padding:5px 10px;background:#111;border-top:1px solid #333;font-size:12px;}";
    html += ".cursor{background:#00ff00;color:#000;animation:blink 1s infinite;}";
    html += "@keyframes blink{0%,50%{opacity:1;}51%,100%{opacity:0;}}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>ESP32 Serial Terminal</h1>";
    html += "<div class='channel-buttons'>";
    html += "<button id='btn0' class='active' onclick='selectChannel(0)'>SBC1</button>";
    html += "<button id='btn1' onclick='selectChannel(1)'>SBC2</button>";
    html += "<button id='btn2' onclick='selectChannel(2)'>SBC3</button>";
    html += "<button id='btn3' onclick='selectChannel(3)'>SBC4</button>";
    html += "<button id='btn4' onclick='selectChannel(4)'>SBC5</button>";
    html += "</div></div>";
    html += "<div id='terminal'></div>";
    html += "<div class='status'>Status: <span id='status'>Connecting...</span> | Channel: <span id='channel'>SBC1</span> | Press any key to type</div>";
    html += "</div>";
    html += "<script>";
    html += "const ws=new WebSocket('ws://'+window.location.hostname+':81');";
    html += "const terminal=document.getElementById('terminal');";
    html += "const status=document.getElementById('status');";
    html += "const channelSpan=document.getElementById('channel');";
    html += "let currentChannel=0;";
    html += "ws.onopen=function(){status.textContent='Connected';};";
    html += "ws.onclose=function(){status.textContent='Disconnected';};";
    html += "ws.onerror=function(){status.textContent='Error';};";
    html += "ws.onmessage=function(event){";
    html += "let data=event.data;";
    html += "for(let i=0;i<data.length;i++){";
    html += "let char=data[i];";
    html += "if(char==='\\n'){terminal.textContent+='\\n';}";
    html += "else if(char==='\\r'){terminal.textContent+='\\n';}";
    html += "else if(char==='\\t'){terminal.textContent+='    ';}";
    html += "else if(char==='\\b'){terminal.textContent=terminal.textContent.slice(0,-1);}";
    html += "else if(char.charCodeAt(0)>=32){terminal.textContent+=char;}";
    html += "}";
    html += "terminal.scrollTop=terminal.scrollHeight;};";
    html += "function selectChannel(channel){currentChannel=channel;ws.send('CHANNEL:'+channel);";
    html += "channelSpan.textContent='SBC'+(channel+1);";
    html += "document.querySelectorAll('button').forEach(b=>b.classList.remove('active'));";
    html += "document.getElementById('btn'+channel).classList.add('active');}";
    html += "document.addEventListener('keydown',function(event){";
    html += "if(event.ctrlKey||event.altKey||event.metaKey)return;";
    html += "event.preventDefault();";
    html += "let char=event.key;";
    html += "if(char.length===1){ws.send(char);terminal.textContent+=char;}";
    html += "else if(char==='Enter'){ws.send('\\n');terminal.textContent+='\\n';}";
    html += "else if(char==='Backspace'){ws.send('\\b');terminal.textContent=terminal.textContent.slice(0,-1);}";
    html += "else if(char==='Tab'){ws.send('\\t');terminal.textContent+='\\t';}";
    html += "terminal.scrollTop=terminal.scrollHeight;});";
    html += "selectChannel(0);";
    html += "</script></body></html>";
    return html;
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