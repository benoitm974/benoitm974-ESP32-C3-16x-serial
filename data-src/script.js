// WebSocket connection
const ws = new WebSocket('ws://' + window.location.hostname + ':81');
const terminal = document.getElementById('terminal');
const status = document.getElementById('status');
const channelSpan = document.getElementById('channel');
let currentChannel = 0;
let isConnected = false;

// WebSocket event handlers
ws.onopen = function() {
    status.textContent = 'Connected';
    isConnected = true;
    // Initialize with channel 0 only after connection is established
    selectChannel(0);
};

ws.onclose = function() {
    status.textContent = 'Disconnected';
    isConnected = false;
};

ws.onerror = function(error) {
    status.textContent = 'Error';
    isConnected = false;
    console.error('WebSocket error:', error);
};

ws.onmessage = function(event) {
    let data = event.data;
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
    document.querySelectorAll('button').forEach(b => b.classList.remove('active'));
    document.getElementById('btn' + channel).classList.add('active');
}

// Keyboard event handler
document.addEventListener('keydown', function(event) {
    if (event.ctrlKey || event.altKey || event.metaKey) return;
    
    // Only handle keyboard input if WebSocket is connected
    if (!isConnected || ws.readyState !== WebSocket.OPEN) {
        return;
    }
    
    event.preventDefault();
    let char = event.key;
    
    if (char.length === 1) {
        ws.send(char);
        terminal.textContent += char;
    } else if (char === 'Enter') {
        ws.send('\n');
        terminal.textContent += '\n';
    } else if (char === 'Backspace') {
        ws.send('\b');
        terminal.textContent = terminal.textContent.slice(0, -1);
    } else if (char === 'Tab') {
        ws.send('\t');
        terminal.textContent += '\t';
    }
    
    terminal.scrollTop = terminal.scrollHeight;
});

// Note: selectChannel(0) is now called in ws.onopen to ensure connection is ready