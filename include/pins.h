#ifndef PINS_H
#define PINS_H

// Pin definitions for ESP32-C3 (DFRobot Beetle ESP32-C3)
//
// IMPORTANT PIN USAGE:
// - GPIO20/21: Reserved for USB/Serial communication with computer
// - GPIO0/1:   Used for SBC UART communication (via multiplexer)
// - GPIO5/6:   I2C for OLED display
// - GPIO7-10:  Multiplexer control pins
//

// I2C pins for OLED display (SSD1306 72x40 with u8g2)
#define SDA_PIN 5
#define SCL_PIN 6

// Display configuration for u8g2 SSD1306 72x40
// No additional configuration needed - u8g2 handles this natively
#define SCREEN_ADDRESS 0x3C

// Hardware Serial pins for ESP32-C3 SBC Communication
// NOTE: GPIO20/21 are used by USB/Serial - use different pins for SBCs
#define RX_PIN 0         // GPIO0 - Safe for UART RX
#define TX_PIN 1         // GPIO1 - Safe for UART TX
#define UART_TX_PIN 1    // Alias for compatibility
#define UART_RX_PIN 0    // Alias for compatibility
// Common baud rates for SBC communication
// Try these if you get garbled text:
#define UART_BAUD_RATE 115200   // Default - most common
// #define UART_BAUD_RATE 9600     // Alternative 1 - very reliable
// #define UART_BAUD_RATE 38400    // Alternative 2 - good balance
// #define UART_BAUD_RATE 57600    // Alternative 3 - faster
// #define UART_BAUD_RATE 230400   // Alternative 4 - very fast

// HP4067 Multiplexer control pins - ESP32-C3 GPIO (avoiding GPIO8 status LED)
#define MUX_S0_PIN 3    // GPIO3 - LSB (A0)
#define MUX_S1_PIN 4    // GPIO4 - A1
#define MUX_S2_PIN 9    // GPIO9 - A2
#define MUX_S3_PIN 10   // GPIO10 - MSB (A3)

// WebSocket and HTTP server configuration
#define WEBSOCKET_PORT 81
#define HTTP_PORT 80

// Status LED (ABROBOT ESP32-C3 board)
#define STATUS_LED_PIN 8  // GPIO8 - ABROBOT board status LED

// Channel configuration
#define MAX_CHANNELS 5    // SBC1 through SBC5
#define SBC1_CHANNEL 0
#define SBC2_CHANNEL 1
#define SBC3_CHANNEL 2
#define SBC4_CHANNEL 3
#define SBC5_CHANNEL 4

#endif // PINS_H