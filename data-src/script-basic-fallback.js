// Basic terminal fallback implementation
// Used when xterm.js CDN is not available (offline mode)

// WebSocket connection
const ws = new WebSocket('ws://' + window.location.hostname + ':81');
const terminal = document.getElementById('terminal');
const status = document.getElementById('status');
const channelSpan = document.getElementById('channel');
const localEchoCheckbox = document.getElementById('localEcho');
let currentChannel = 0;
let isConnected = false;

// Convert terminal div to basic text terminal
terminal.style.padding = '10px';
terminal.style.overflowY = 'auto';
terminal.style.backgroundColor = '#000';
terminal.style.color = '#00ff00';
terminal.style.whiteSpace = 'pre-wrap';
terminal.style.fontSize = '14px';
terminal.style.lineHeight = '1.2';
terminal.style.fontFamily = 'Courier New, monospace';

// Add welcome message
terminal.textContent = 'ESP32-C3 Serial Terminal (Basic Mode)\nConnecting to WebSocket...\n';

// WebSocket event handlers
ws.onopen = function() {
    status.textContent = 'Connected';
    isConnected = true;
    terminal.textContent += 'WebSocket connected!\n';
    // Initialize with channel 0 only after connection is established
    selectChannel(0);
};

ws.onclose = function() {
    status.textContent = 'Disconnected';
    isConnected = false;
    terminal.textContent += 'WebSocket disconnected!\n';
};

ws.onerror = function(error) {
    status.textContent = 'Error';
    isConnected = false;
    terminal.textContent += 'WebSocket error!\n';
    console.error('WebSocket error:', error);
};

ws.onmessage = function(event) {
    let data = event.data;
    
    // Simple ANSI escape sequence handling
    // Remove common ANSI color and formatting codes for cleaner display
    data = data.replace(/\x1b\[[0-9;]*m/g, ''); // Remove color codes like [0;32m, [0m
    data = data.replace(/\x1b\[[0-9;]*[A-Za-z]/g, ''); // Remove cursor control codes
    data = data.replace(/\x1b\[!\w*\]/g, ''); // Remove capability queries like [!p]
    data = data.replace(/\x1b\[\?\w*[hl]/g, ''); // Remove mode settings like [?7h
    
    for (let i = 0; i < data.length; i++) {
        let char = data[i];
        if (char === '\n') {
            terminal.textContent += '\n';
        } else if (char === '\r') {
            terminal.textContent += '\n';
        } else if (char === '\t') {
            terminal.textContent += '    ';
        } else if (char === '\b') {
            terminal.textContent = terminal.textContent.slice(0, -1);
        } else if (char.charCodeAt(0) >= 32) {
            terminal.textContent += char;
        }
    }
    terminal.scrollTop = terminal.scrollHeight;
};

// Channel selection function
function selectChannel(channel) {
    currentChannel = channel;
    
    // Only send if WebSocket is connected
    if (isConnected && ws.readyState === WebSocket.OPEN) {
        ws.send('CHANNEL:' + channel);
    }
    
    channelSpan.textContent = 'SBC' + (channel + 1);
    
    // Update button states
    document.querySelectorAll('.channel-buttons button').forEach(b => b.classList.remove('active'));
    document.getElementById('btn' + channel).classList.add('active');
    
    // Show channel change in terminal
    terminal.textContent += `Switched to SBC${channel + 1}\n`;
    terminal.scrollTop = terminal.scrollHeight;
}

// Function to send control characters
function sendControlChar(charCode) {
    if (!isConnected || ws.readyState !== WebSocket.OPEN) {
        return;
    }
    
    let controlChar = String.fromCharCode(charCode);
    ws.send(controlChar);
    
    // Show control character in terminal for feedback
    let controlName = '';
    switch(charCode) {
        case 3: controlName = '^C'; break;
        case 4: controlName = '^D'; break;
        case 26: controlName = '^Z'; break;
        case 17: controlName = '^Q'; break;
        case 19: controlName = '^S'; break;
        default: controlName = '^' + String.fromCharCode(64 + charCode);
    }
    
    if (localEchoCheckbox.checked) {
        terminal.textContent += controlName;
        terminal.scrollTop = terminal.scrollHeight;
    }
}

// Keyboard event handler
document.addEventListener('keydown', function(event) {
    // Only handle keyboard input if WebSocket is connected
    if (!isConnected || ws.readyState !== WebSocket.OPEN) {
        return;
    }
    
    // Handle Ctrl key combinations
    if (event.ctrlKey && !event.altKey && !event.metaKey) {
        event.preventDefault();
        let char = event.key.toLowerCase();
        let controlCode = 0;
        
        // Map common control characters
        switch(char) {
            case 'c': controlCode = 3; break;   // Ctrl+C
            case 'd': controlCode = 4; break;   // Ctrl+D
            case 'z': controlCode = 26; break;  // Ctrl+Z
            case 'q': controlCode = 17; break;  // Ctrl+Q
            case 's': controlCode = 19; break;  // Ctrl+S
            case 'a': controlCode = 1; break;   // Ctrl+A
            case 'e': controlCode = 5; break;   // Ctrl+E
            case 'k': controlCode = 11; break;  // Ctrl+K
            case 'l': controlCode = 12; break;  // Ctrl+L
            case 'u': controlCode = 21; break;  // Ctrl+U
            case 'w': controlCode = 23; break;  // Ctrl+W
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
    
    // Skip other modifier keys
    if (event.ctrlKey || event.altKey || event.metaKey) return;
    
    event.preventDefault();
    let char = event.key;
    let localEcho = localEchoCheckbox.checked;
    
    if (char.length === 1) {
        ws.send(char);
        if (localEcho) {
            terminal.textContent += char;
        }
    } else if (char === 'Enter') {
        ws.send('\n');
        if (localEcho) {
            terminal.textContent += '\n';
        }
    } else if (char === 'Backspace') {
        ws.send('\b');
        if (localEcho) {
            terminal.textContent = terminal.textContent.slice(0, -1);
        }
    } else if (char === 'Tab') {
        ws.send('\t');
        if (localEcho) {
            terminal.textContent += '\t';
        }
    }
    
    terminal.scrollTop = terminal.scrollHeight;
});

// Local echo toggle handler
localEchoCheckbox.addEventListener('change', function() {
    if (this.checked) {
        terminal.textContent += 'Local echo enabled\n';
    } else {
        terminal.textContent += 'Local echo disabled\n';
    }
    terminal.scrollTop = terminal.scrollHeight;
});

console.log('Basic terminal fallback loaded');