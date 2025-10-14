// Unified terminal implementation with runtime switching
// Supports both xterm.js and basic terminal modes

// Configurable logging system
const LogLevel = {
    ERROR: 0,
    WARN: 1,
    INFO: 2,
    DEBUG: 3
};

// Configure debug level - change this to control verbosity
// 0 = Errors only, 1 = Errors + Warnings, 2 = Errors + Warnings + Info, 3 = All messages
// Can be overridden with URL parameter ?debug=level or localStorage item 'terminal_debug_level'
const DEFAULT_LOG_LEVEL = 1; // Default to warnings and errors only

// Get debug level from URL parameter, localStorage, or default
function getDebugLevel() {
    // Try URL parameter first
    const urlParams = new URLSearchParams(window.location.search);
    const urlDebugLevel = parseInt(urlParams.get('debug'));
    if (!isNaN(urlDebugLevel) && urlDebugLevel >= 0 && urlDebugLevel <= 3) {
        return urlDebugLevel;
    }
    
    // Try localStorage next
    const storedDebugLevel = parseInt(localStorage.getItem('terminal_debug_level'));
    if (!isNaN(storedDebugLevel) && storedDebugLevel >= 0 && storedDebugLevel <= 3) {
        return storedDebugLevel;
    }
    
    // Fall back to default
    return DEFAULT_LOG_LEVEL;
}

const CONFIG_LOG_LEVEL = getDebugLevel();

class Logger {
    static getCurrentLogLevel() {
        // Check if debug level has been updated in localStorage
        const storedLevel = parseInt(localStorage.getItem('terminal_debug_level'));
        if (!isNaN(storedLevel) && storedLevel >= 0 && storedLevel <= 3) {
            return storedLevel;
        }
        return CONFIG_LOG_LEVEL;
    }
    
    static log(level, message, ...args) {
        if (level <= this.getCurrentLogLevel()) {
            const prefix = level === LogLevel.ERROR ? '[ERROR]' :
                          level === LogLevel.WARN ? '[WARN]' :
                          level === LogLevel.INFO ? '[INFO]' : '[DEBUG]';
            
            switch(level) {
                case LogLevel.ERROR:
                    console.error(prefix, message, ...args);
                    break;
                case LogLevel.WARN:
                    console.warn(prefix, message, ...args);
                    break;
                case LogLevel.INFO:
                    console.info(prefix, message, ...args);
                    break;
                case LogLevel.DEBUG:
                    console.log(prefix, message, ...args);
                    break;
            }
        }
    }
    
    static error(message, ...args) { this.log(LogLevel.ERROR, message, ...args); }
    static warn(message, ...args) { this.log(LogLevel.WARN, message, ...args); }
    static info(message, ...args) { this.log(LogLevel.INFO, message, ...args); }
    static debug(message, ...args) { this.log(LogLevel.DEBUG, message, ...args); }
}

// WebSocket Connection Manager
class WebSocketConnectionManager {
    constructor() {
        this.ws = null;
        this.isConnected = false;
        this.connectionState = 'disconnected'; // 'disconnected', 'connecting', 'connected', 'reconnecting', 'failed'
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 10;
        this.reconnectDelays = [1000, 2000, 4000, 8000, 15000, 30000]; // Exponential backoff
        this.heartbeatInterval = 15000; // 15 seconds
        this.pingTimeout = 3000; // 3 seconds
        this.heartbeatTimer = null;
        this.pingTimer = null;
        this.manualReconnect = false;
        this.connectionQuality = 'good'; // 'good', 'unstable', 'poor'
        this.lastPingTime = 0;
        this.callbacks = {
            onConnect: [],
            onDisconnect: [],
            onMessage: [],
            onStateChange: []
        };
    }

    connect() {
        if (this.ws && (this.ws.readyState === WebSocket.CONNECTING || this.ws.readyState === WebSocket.OPEN)) {
            return;
        }

        this.setState('connecting');
        this.ws = new WebSocket('ws://' + window.location.hostname + ':81');
        
        this.ws.onopen = () => {
            this.isConnected = true;
            this.connectionState = 'connected';
            this.reconnectAttempts = 0;
            this.connectionQuality = 'good';
            this.startHeartbeat();
            this.setState('connected');
            this.triggerCallbacks('onConnect');
        };

        this.ws.onclose = (event) => {
            this.isConnected = false;
            this.stopHeartbeat();
            
            if (!this.manualReconnect) {
                this.setState('disconnected');
                this.triggerCallbacks('onDisconnect');
                
                // Only auto-reconnect if it wasn't a manual disconnect
                if (this.reconnectAttempts < this.maxReconnectAttempts) {
                    this.scheduleReconnect();
                } else {
                    this.setState('failed');
                }
            }
            
            this.manualReconnect = false;
        };

        this.ws.onerror = (error) => {
            Logger.error('WebSocket error:', error);
            this.connectionQuality = 'poor';
            this.updateStatusIndicator();
        };

        this.ws.onmessage = (event) => {
            // Handle pong response for heartbeat
            if (event.data === 'pong') {
                const pingTime = Date.now() - this.lastPingTime;
                this.updateConnectionQuality(pingTime);
                clearTimeout(this.pingTimer);
                return;
            }
            
            this.triggerCallbacks('onMessage', event.data);
        };
    }

    disconnect() {
        this.manualReconnect = true;
        this.stopHeartbeat();
        if (this.ws) {
            this.ws.close();
            this.ws = null;
        }
        this.isConnected = false;
        this.setState('disconnected');
    }

    forceReconnect() {
        this.disconnect();
        this.reconnectAttempts = 0;
        setTimeout(() => this.connect(), 100);
    }

    scheduleReconnect() {
        this.setState('reconnecting');
        const delay = this.getReconnectDelay();
        
        setTimeout(() => {
            this.reconnectAttempts++;
            this.connect();
        }, delay);
    }

    getReconnectDelay() {
        if (this.reconnectAttempts < this.reconnectDelays.length) {
            return this.reconnectDelays[this.reconnectAttempts];
        }
        return this.reconnectDelays[this.reconnectDelays.length - 1];
    }

    startHeartbeat() {
        this.heartbeatTimer = setInterval(() => {
            if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                this.lastPingTime = Date.now();
                this.ws.send('ping');
                
                // Set timeout for pong response
                this.pingTimer = setTimeout(() => {
                    Logger.warn('Ping timeout - connection may be unstable');
                    this.connectionQuality = 'unstable';
                    this.updateStatusIndicator();
                }, this.pingTimeout);
            }
        }, this.heartbeatInterval);
    }

    stopHeartbeat() {
        if (this.heartbeatTimer) {
            clearInterval(this.heartbeatTimer);
            this.heartbeatTimer = null;
        }
        if (this.pingTimer) {
            clearTimeout(this.pingTimer);
            this.pingTimer = null;
        }
    }

    updateConnectionQuality(pingTime) {
        if (pingTime < 100) {
            this.connectionQuality = 'good';
        } else if (pingTime < 500) {
            this.connectionQuality = 'unstable';
        } else {
            this.connectionQuality = 'poor';
        }
        this.updateStatusIndicator();
    }

    setState(newState) {
        const oldState = this.connectionState;
        this.connectionState = newState;
        this.updateStatusIndicator();
        this.triggerCallbacks('onStateChange', { oldState, newState });
    }

    updateStatusIndicator() {
        const statusElement = document.getElementById('status');
        if (!statusElement) return;

        let statusText = '';
        let statusIcon = '';

        switch (this.connectionState) {
            case 'connected':
                statusText = 'Connected';
                statusIcon = this.connectionQuality === 'good' ? '●' :
                           this.connectionQuality === 'unstable' ? '◐' : '○';
                break;
            case 'connecting':
                statusText = 'Connecting';
                statusIcon = '⟳';
                break;
            case 'reconnecting':
                statusText = `Reconnecting (${this.reconnectAttempts}/${this.maxReconnectAttempts})`;
                statusIcon = '⟳';
                break;
            case 'disconnected':
                statusText = 'Disconnected';
                statusIcon = '○';
                break;
            case 'failed':
                statusText = 'Connection Failed';
                statusIcon = '○';
                break;
        }

        statusElement.innerHTML = `${statusIcon} ${statusText}`;
        
        // Add color coding
        statusElement.className = '';
        if (this.connectionState === 'connected') {
            if (this.connectionQuality === 'good') {
                statusElement.style.color = '#00ff00';
            } else if (this.connectionQuality === 'unstable') {
                statusElement.style.color = '#ffff00';
            } else {
                statusElement.style.color = '#ff9900';
            }
        } else if (this.connectionState === 'reconnecting') {
            statusElement.style.color = '#0099ff';
        } else {
            statusElement.style.color = '#ff0000';
        }
    }

    send(data) {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.send(data);
            return true;
        }
        return false;
    }

    on(event, callback) {
        if (this.callbacks[event]) {
            this.callbacks[event].push(callback);
        }
    }

    triggerCallbacks(event, data) {
        if (this.callbacks[event]) {
            this.callbacks[event].forEach(callback => callback(data));
        }
    }
}

// Initialize xterm.js terminal
window.initXtermTerminal = function initXtermTerminal() {
    if (typeof Terminal === 'undefined') {
        Logger.error('xterm.js not available');
        return false;
    }
    
    const terminal = document.getElementById('terminal');
    terminal.innerHTML = '';
    
    const term = new Terminal({
        cursorBlink: true,
        fontSize: 14,
        fontFamily: 'Courier New, monospace',
        theme: {
            background: '#000000',
            foreground: '#00ff00',
            cursor: '#00ff00',
            selection: '#ffffff40'
        },
        cols: 80,
        rows: 24
    });

    term.open(terminal);
    window.terminalState.currentTerminal = term;
    window.terminalState.mode = 'xterm';
    
    // Update UI
    document.getElementById('terminal-type').textContent = 'xterm.js';
    document.getElementById('toggle-text').textContent = 'Switch to Basic';
    document.getElementById('local-echo-container').style.display = 'none';
    
    // Welcome message
    term.writeln('\x1b[32mESP32-C3 Serial Terminal (xterm.js)\x1b[0m');
    
    // Handle terminal input - xterm.js automatically handles Ctrl+C, Ctrl+D, etc.
    term.onData(data => {
        if (window.terminalState.connectionManager) {
            window.terminalState.connectionManager.send(data);
        }
    });
    
    // Handle window resize
    const resizeHandler = () => {
        const rect = terminal.getBoundingClientRect();
        const cols = Math.floor(rect.width / 9);
        const rows = Math.floor(rect.height / 17);
        term.resize(cols, rows);
    };
    
    window.addEventListener('resize', resizeHandler);
    setTimeout(resizeHandler, 100);
    
    // Replay any buffered messages
    replayBufferedMessages();
    
    // Focus the terminal after initialization
    setTimeout(() => focusTerminal(), 100);
    
    return true;
}

// Initialize basic terminal
function initBasicTerminal() {
    const terminal = document.getElementById('terminal');
    terminal.innerHTML = '';
    
    // Convert to basic text terminal
    terminal.style.padding = '10px';
    terminal.style.overflowY = 'auto';
    terminal.style.backgroundColor = '#000';
    terminal.style.color = '#00ff00';
    terminal.style.whiteSpace = 'pre-wrap';
    terminal.style.fontSize = '14px';
    terminal.style.lineHeight = '1.2';
    terminal.style.fontFamily = 'Courier New, monospace';
    
    window.terminalState.currentTerminal = terminal;
    window.terminalState.mode = 'basic';
    
    // Update UI
    document.getElementById('terminal-type').textContent = 'Basic';
    document.getElementById('toggle-text').textContent = window.terminalState.xtermAvailable ? 'Switch to xterm.js' : 'xterm.js unavailable';
    document.getElementById('local-echo-container').style.display = 'inline';
    
    // Welcome message
    terminal.textContent = 'ESP32-C3 Serial Terminal (Basic Mode)\n';
    
    // Setup keyboard handler for basic terminal
    setupBasicKeyboardHandler();
    
    // Replay any buffered messages
    replayBufferedMessages();
    
    return true;
}

// Setup keyboard handler for basic terminal
function setupBasicKeyboardHandler() {
    // Remove any existing handlers
    document.removeEventListener('keydown', window.basicKeyHandler);
    
    window.basicKeyHandler = function(event) {
        if (!window.terminalState.connectionManager || !window.terminalState.connectionManager.isConnected) {
            return;
        }
        
        // Handle Ctrl key combinations
        if (event.ctrlKey && !event.altKey && !event.metaKey) {
            event.preventDefault();
            let char = event.key.toLowerCase();
            let controlCode = 0;
            
            switch(char) {
                case 'c': controlCode = 3; break;
                case 'd': controlCode = 4; break;
                case 'z': controlCode = 26; break;
                case 'q': controlCode = 17; break;
                case 's': controlCode = 19; break;
                case 'a': controlCode = 1; break;
                case 'e': controlCode = 5; break;
                case 'k': controlCode = 11; break;
                case 'l': controlCode = 12; break;
                case 'u': controlCode = 21; break;
                case 'w': controlCode = 23; break;
                default:
                    if (char.length === 1 && char >= 'a' && char <= 'z') {
                        controlCode = char.charCodeAt(0) - 96;
                    }
            }
            
            if (controlCode > 0) {
                sendControlChar(controlCode);
            }
            return;
        }
        
        if (event.ctrlKey || event.altKey || event.metaKey) return;
        
        event.preventDefault();
        let char = event.key;
        let localEcho = document.getElementById('localEcho').checked;
        
        if (char.length === 1) {
            window.terminalState.connectionManager.send(char);
            if (localEcho) {
                window.terminalState.currentTerminal.textContent += char;
            }
        } else if (char === 'Enter') {
            window.terminalState.connectionManager.send('\n');
            if (localEcho) {
                window.terminalState.currentTerminal.textContent += '\n';
            }
        } else if (char === 'Backspace') {
            window.terminalState.connectionManager.send('\b');
            if (localEcho) {
                window.terminalState.currentTerminal.textContent = window.terminalState.currentTerminal.textContent.slice(0, -1);
            }
        } else if (char === 'Tab') {
            window.terminalState.connectionManager.send('\t');
            if (localEcho) {
                window.terminalState.currentTerminal.textContent += '\t';
            }
        }
        
        window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
    };
    
    document.addEventListener('keydown', window.basicKeyHandler);
}

// Initialize WebSocket with connection manager
window.initWebSocket = function initWebSocket() {
    // Create connection manager if it doesn't exist
    if (!window.terminalState.connectionManager) {
        window.terminalState.connectionManager = new WebSocketConnectionManager();
        
        // Set up connection event handlers
        window.terminalState.connectionManager.on('onConnect', () => {
            window.terminalState.isConnected = true;
            window.terminalState.ws = window.terminalState.connectionManager.ws;
            
            if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
                window.terminalState.currentTerminal.writeln('\x1b[32mWebSocket connected!\x1b[0m');
            } else if (window.terminalState.mode === 'basic') {
                window.terminalState.currentTerminal.textContent += 'WebSocket connected!\n';
                window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
            }
            
            selectChannel(0);
        });
        
        window.terminalState.connectionManager.on('onDisconnect', () => {
            window.terminalState.isConnected = false;
            
            if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
                window.terminalState.currentTerminal.writeln('\x1b[31mWebSocket disconnected!\x1b[0m');
            } else if (window.terminalState.mode === 'basic') {
                window.terminalState.currentTerminal.textContent += 'WebSocket disconnected!\n';
                window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
            }
        });
        
        window.terminalState.connectionManager.on('onMessage', (data) => {
            // Store message in buffer for terminal switching
            window.terminalState.messageBuffer.push(data);
            
            // Keep only last 1000 messages to prevent memory issues
            if (window.terminalState.messageBuffer.length > 1000) {
                window.terminalState.messageBuffer.shift();
            }
            
            displayMessage(data);
        });
        
        window.terminalState.connectionManager.on('onStateChange', ({ newState }) => {
            // Handle connection state changes if needed
            if (newState === 'failed') {
                showNotification('Connection failed. Please try reconnecting manually.', 'error');
            }
        });
    }
    
    // Start connection
    window.terminalState.connectionManager.connect();
}

// Manual reconnect function
window.manualReconnect = function manualReconnect() {
    Logger.debug('manualReconnect called, terminalState:', window.terminalState);
    Logger.debug('connectionManager available:', !!window.terminalState.connectionManager);
    Logger.debug('initWebSocket function available:', typeof window.initWebSocket);
    
    if (window.terminalState.connectionManager) {
        Logger.debug('connectionManager.forceReconnect type:', typeof window.terminalState.connectionManager.forceReconnect);
        
        if (typeof window.terminalState.connectionManager.forceReconnect === 'function') {
            Logger.debug('Calling connectionManager.forceReconnect()');
            window.terminalState.connectionManager.forceReconnect();
        } else {
            Logger.warn('forceReconnect method not found, trying manual disconnect/connect...');
            // Try to manually trigger reconnection
            Logger.debug('Attempting manual reconnection...');
            window.terminalState.connectionManager.disconnect();
            setTimeout(() => {
                window.terminalState.connectionManager.connect();
            }, 100);
        }
    } else {
        Logger.warn('Connection manager not available, attempting to initialize...');
        Logger.debug('Creating new connection manager...');
        
        // Create connection manager directly
        window.terminalState.connectionManager = new WebSocketConnectionManager();
        Logger.debug('Connection manager created:', !!window.terminalState.connectionManager);
        
        // Set up connection event handlers
        window.terminalState.connectionManager.on('onConnect', () => {
            window.terminalState.isConnected = true;
            window.terminalState.ws = window.terminalState.connectionManager.ws;
            
            if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
                window.terminalState.currentTerminal.writeln('\x1b[32mWebSocket connected!\x1b[0m');
            } else if (window.terminalState.mode === 'basic') {
                window.terminalState.currentTerminal.textContent += 'WebSocket connected!\n';
                window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
            }
        });
        
        window.terminalState.connectionManager.on('onDisconnect', () => {
            window.terminalState.isConnected = false;
            
            if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
                window.terminalState.currentTerminal.writeln('\x1b[31mWebSocket disconnected!\x1b[0m');
            } else if (window.terminalState.mode === 'basic') {
                window.terminalState.currentTerminal.textContent += 'WebSocket disconnected!\n';
                window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
            }
        });
        
        // Try to reconnect immediately
        setTimeout(() => {
            if (window.terminalState.connectionManager && typeof window.terminalState.connectionManager.forceReconnect === 'function') {
                Logger.debug('Calling forceReconnect after initialization');
                window.terminalState.connectionManager.forceReconnect();
            } else {
                Logger.error('forceReconnect still not available after direct initialization');
            }
        }, 100);
    }
}

// Handle reconnect with focus restoration
window.handleReconnect = function handleReconnect() {
    manualReconnect();
    // Restore focus after attempting reconnection
    setTimeout(() => focusTerminal(), 250);
}

// Show notification function
function showNotification(message, type = 'info') {
    // Create a simple notification that doesn't interfere with the terminal
    const notification = document.createElement('div');
    notification.style.cssText = `
        position: fixed;
        top: 10px;
        right: 10px;
        padding: 10px 15px;
        background: ${type === 'error' ? '#ff4444' : '#4444ff'};
        color: white;
        border-radius: 5px;
        font-size: 12px;
        z-index: 1000;
        opacity: 0.9;
        transition: opacity 0.3s;
    `;
    notification.textContent = message;
    
    document.body.appendChild(notification);
    
    // Auto-remove after 3 seconds
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            if (notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, 300);
    }, 3000);
}

// Display message in current terminal
function displayMessage(data) {
    if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
        window.terminalState.currentTerminal.write(data);
    } else if (window.terminalState.mode === 'basic' && window.terminalState.currentTerminal) {
        // Basic terminal with ANSI filtering
        let filteredData = data.replace(/\x1b\[[0-9;]*m/g, '');
        filteredData = filteredData.replace(/\x1b\[[0-9;]*[A-Za-z]/g, '');
        filteredData = filteredData.replace(/\x1b\[!\w*\]/g, '');
        filteredData = filteredData.replace(/\x1b\[\?\w*[hl]/g, '');
        
        for (let i = 0; i < filteredData.length; i++) {
            let char = filteredData[i];
            if (char === '\n') {
                window.terminalState.currentTerminal.textContent += '\n';
            } else if (char === '\r') {
                window.terminalState.currentTerminal.textContent += '\n';
            } else if (char === '\t') {
                window.terminalState.currentTerminal.textContent += '    ';
            } else if (char === '\b') {
                window.terminalState.currentTerminal.textContent = window.terminalState.currentTerminal.textContent.slice(0, -1);
            } else if (char.charCodeAt(0) >= 32) {
                window.terminalState.currentTerminal.textContent += char;
            }
        }
        window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
    }
}

// Replay buffered messages when switching terminals
function replayBufferedMessages() {
    // Only replay last 50 messages to avoid overwhelming the terminal
    const messagesToReplay = window.terminalState.messageBuffer.slice(-50);
    messagesToReplay.forEach(message => displayMessage(message));
}

// Utility function to focus the terminal
function focusTerminal() {
    if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
        // For xterm.js, try multiple methods to focus
        const helperTextarea = document.querySelector('.xterm-helper-textarea');
        const terminalCanvas = document.querySelector('.xterm-textarea');
        const terminalElement = document.getElementById('terminal');
        
        if (helperTextarea) {
            helperTextarea.focus();
        } else if (terminalCanvas) {
            terminalCanvas.focus();
        } else if (terminalElement) {
            terminalElement.focus();
            // Also try to focus the xterm instance directly
            try {
                if (window.terminalState.currentTerminal.focus) {
                    window.terminalState.currentTerminal.focus();
                }
            } catch (e) {
                Logger.debug('Could not focus xterm instance:', e);
            }
        }
    } else if (window.terminalState.mode === 'basic') {
        // For basic terminal, focus the terminal container
        const terminalElement = document.getElementById('terminal');
        if (terminalElement) {
            terminalElement.focus();
        }
    }
}

// Channel selection function
function selectChannel(channel) {
    window.terminalState.currentChannel = channel;
    
    if (window.terminalState.isConnected && window.terminalState.ws.readyState === WebSocket.OPEN) {
        window.terminalState.ws.send('CHANNEL:' + channel);
    }
    
    document.getElementById('channel').textContent = 'SBC' + (channel + 1);
    
    // Update button states
    document.querySelectorAll('.channel-buttons button').forEach(b => b.classList.remove('active'));
    document.getElementById('btn' + channel).classList.add('active');
    
    // Show channel change in terminal
    if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
        window.terminalState.currentTerminal.writeln(`\x1b[33mSwitched to SBC${channel + 1}\x1b[0m`);
    } else if (window.terminalState.mode === 'basic') {
        window.terminalState.currentTerminal.textContent += `Switched to SBC${channel + 1}\n`;
        window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
    }
    
    // Restore focus to terminal after channel change
    setTimeout(() => focusTerminal(), 10);
}

// Function to send control characters
function sendControlChar(charCode) {
    if (!window.terminalState.isConnected || window.terminalState.ws.readyState !== WebSocket.OPEN) {
        return;
    }
    
    let controlChar = String.fromCharCode(charCode);
    window.terminalState.ws.send(controlChar);
    
    // Show control character feedback
    let controlName = '';
    switch(charCode) {
        case 3: controlName = '^C'; break;
        case 4: controlName = '^D'; break;
        case 26: controlName = '^Z'; break;
        case 17: controlName = '^Q'; break;
        case 19: controlName = '^S'; break;
        default: controlName = '^' + String.fromCharCode(64 + charCode);
    }
    
    if (window.terminalState.mode === 'xterm' && window.terminalState.currentTerminal) {
        window.terminalState.currentTerminal.write(`\x1b[36m${controlName}\x1b[0m`);
    } else if (window.terminalState.mode === 'basic') {
        const localEcho = document.getElementById('localEcho').checked;
        if (localEcho) {
            window.terminalState.currentTerminal.textContent += controlName;
            window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
        }
    }
    
    // Restore focus to terminal after sending control character
    setTimeout(() => focusTerminal(), 10);
}

// Local echo toggle handler (basic mode only)
document.addEventListener('DOMContentLoaded', function() {
    const localEchoCheckbox = document.getElementById('localEcho');
    if (localEchoCheckbox) {
        localEchoCheckbox.addEventListener('change', function() {
            if (window.terminalState.mode === 'basic') {
                if (this.checked) {
                    window.terminalState.currentTerminal.textContent += 'Local echo enabled\n';
                } else {
                    window.terminalState.currentTerminal.textContent += 'Local echo disabled\n';
                }
                window.terminalState.currentTerminal.scrollTop = window.terminalState.currentTerminal.scrollHeight;
            }
        });
    }
});

Logger.info('Unified terminal script loaded');