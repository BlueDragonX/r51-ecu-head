#ifndef _R51_BRIDGE_CONFIG_
#define _R51_BRIDGE_CONFIG_

#define SERIAL_DEVICE Serial
#define SERIAL_BAUDRATE 115200
#define SERIAL_WAIT true

// Uncomment the following line to enable debug output.
#define DEBUG_ENABLE

// Uncomment to enable Console over serial.
#define CONSOLE_ENABLE

// Uncomment the following line to enable defog heater control.
#define DEFOG_HEATER_ENABLE
#define DEFOG_HEATER_PIN 6
#define DEFOG_HEATER_MS 200

// Uncomment the following line to enable steering keypad.
#define STEERING_KEYPAD_ENABLE
#define STEERING_PIN_A A2
#define STEERING_PIN_B A3

// Uncomment to enable Bluetooth via SPI.
#define BLUETOOTH_ENABLE
#define BLUETOOTH_SPI_CS_PIN 24
#define BLUETOOTH_SPI_IRQ_PIN 23

// Uncomment to enable RealDash serial.
#define REALDASH_ENABLE
#define REALDASH_SERIAL ble
#define REALDASH_FRAME_ID 0x01

#endif  // _R51_BRIDGE_CONFIG_