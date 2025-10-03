#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include "pins.h"
#include <Arduino.h>

class MultiplexerController {
public:
    /**
     * Initialize the multiplexer control pins
     */
    void init();

    /**
     * Select a specific channel (0-4 for SBC1-SBC5) with proper timing
     * @param channel The channel number (0-4)
     * @return true if successful, false if invalid channel
     */
    bool selectChannel(uint8_t channel);

    /**
     * Force immediate channel switch (bypasses timing restrictions)
     * @param channel The channel number (0-4)
     * @return true if successful, false if invalid channel
     */
    bool forceSelectChannel(uint8_t channel);

    /**
     * Check if a channel is valid
     * @param channel The channel number
     * @return true if valid (0-4), false otherwise
     */
    bool isValidChannel(uint8_t channel) const;

    /**
     * Get the currently selected channel
     * @return Current channel (0-4) or 255 if none
     */
    uint8_t getCurrentChannel() const;

private:
    uint8_t currentChannel = 255;  // Invalid initial state
    unsigned long lastSwitchTime = 0;  // Timestamp of last channel switch

    // Timing constants (in milliseconds)
    static const unsigned long MIN_SWITCH_DELAY = 50;  // Minimum delay between switches
    static const unsigned long SETTLING_DELAY_US = 100;  // Multiplexer settling time

    // Channel selection bit mapping (S3 S2 S1 S0)
    static const uint8_t channelBits[MAX_CHANNELS];

    /**
     * Internal channel selection without timing checks
     * @param channel The channel number
     * @return true if successful
     */
    bool setChannelBits(uint8_t channel);
};

#endif // MULTIPLEXER_H