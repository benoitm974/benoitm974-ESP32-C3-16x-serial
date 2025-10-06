// WebSocket connection
const ws = new WebSocket('ws://' + window.location.hostname + ':81');
const terminal = document.getElementById('terminal');
const status = document.getElementById('status');
const channelSpan = document.getElementById('channel');
let currentChannel = 0;

// WebSocket event handlers
ws.onopen = function() {
    status.textContent = 'Connected';
};

ws.onclose = function() {
    status.textContent = 'Disconnected';
};

ws.onerror = function() {
    status.textContent = 'Error';
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
    ws.send('CHANNEL:' + channel);
    channelSpan.textContent = 'SBC' + (channel + 1);
    
    // Update button states
    document.querySelectorAll('button').forEach(b => b.classList.remove('active'));
    document.getElementById('btn' + channel).classList.add('active');
}

// Keyboard event handler
document.addEventListener('keydown', function(event) {
    if (event.ctrlKey || event.altKey || event.metaKey) return;
    
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

// Initialize with channel 0
selectChannel(0);