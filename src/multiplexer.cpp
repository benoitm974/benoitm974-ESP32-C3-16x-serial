#include "multiplexer.h"

const uint8_t MultiplexerController::channelBits[MAX_CHANNELS] = {
    0b0000,  // Channel 0: S3=0, S2=0, S1=0, S0=0
    0b0001,  // Channel 1: S3=0, S2=0, S1=0, S0=1
    0b0010,  // Channel 2: S3=0, S2=0, S1=1, S0=0
    0b0011,  // Channel 3: S3=0, S2=0, S1=1, S0=1
    0b0100   // Channel 4: S3=0, S2=1, S1=0, S0=0
};

void MultiplexerController::init() {
    // Configure control pins as outputs
    pinMode(MUX_S0_PIN, OUTPUT);
    pinMode(MUX_S1_PIN, OUTPUT);
    pinMode(MUX_S2_PIN, OUTPUT);
    pinMode(MUX_S3_PIN, OUTPUT);
    
    // Set initial state to all high (no channel selected)
    digitalWrite(MUX_S0_PIN, HIGH);
    digitalWrite(MUX_S1_PIN, HIGH);
    digitalWrite(MUX_S2_PIN, HIGH);
    digitalWrite(MUX_S3_PIN, HIGH);
    
    currentChannel = 255;  // Invalid state
}

bool MultiplexerController::selectChannel(uint8_t channel) {
    if (!isValidChannel(channel)) {
        return false;
    }

    // Check minimum delay between switches to prevent data corruption
    unsigned long currentTime = millis();
    if (currentTime - lastSwitchTime < MIN_SWITCH_DELAY) {
        // Wait for minimum delay
        delay(MIN_SWITCH_DELAY - (currentTime - lastSwitchTime));
    }

    // Perform the actual channel switch
    if (setChannelBits(channel)) {
        lastSwitchTime = millis();
        return true;
    }

    return false;
}

bool MultiplexerController::forceSelectChannel(uint8_t channel) {
    if (!isValidChannel(channel)) {
        return false;
    }

    // Force immediate switch without timing checks
    return setChannelBits(channel);
}

bool MultiplexerController::setChannelBits(uint8_t channel) {
    uint8_t bits = channelBits[channel];

    // Set S0 (LSB)
    digitalWrite(MUX_S0_PIN, (bits & 0b0001) ? HIGH : LOW);
    // Set S1
    digitalWrite(MUX_S1_PIN, (bits & 0b0010) ? HIGH : LOW);
    // Set S2
    digitalWrite(MUX_S2_PIN, (bits & 0b0100) ? HIGH : LOW);
    // Set S3 (MSB)
    digitalWrite(MUX_S3_PIN, (bits & 0b1000) ? HIGH : LOW);

    currentChannel = channel;

    // Delay for multiplexer settling time
    delayMicroseconds(SETTLING_DELAY_US);

    return true;
}

bool MultiplexerController::isValidChannel(uint8_t channel) const {
    return channel < MAX_CHANNELS;
}

uint8_t MultiplexerController::getCurrentChannel() const {
    return currentChannel;
}