#pragma once

// Pin definitions
#define SDA_PIN                3     // I2C SDA pin
#define SCL_PIN                7     // I2C SCL pin
#define PULSE_LED_PIN         15     // LED indicator pin
#define ENC_CLK               17     // Encoder CLK pin
#define ENC_DT                21     // Encoder DT pin
#define ENC_SW                34     // Encoder switch pin
#define PROTECTION_MOSFET_PIN  1     // Output MOSFET control pin
#define DC_CONTROL_PIN        40     // PWM for feedback low side
#define NTC_ADC_PIN            2     // ADC channel for thermistor

// Local UI (touch buttons and backlight)
#define LED_UI_PIN            39     // WS2812 LED backlight pin
#define NUM_LEDS               2     // Number of backlight LEDs
#define TOUCH1_PAD     TOUCH_PAD_NUM12 // Touch pad 1
#define TOUCH2_PAD     TOUCH_PAD_NUM11 // Touch pad 2

// INA226 configuration
#define INA226_I2C_ADDRESS    0x40   // INA226 I2C address
#define SHUNT_RESISTANCE_OHMS 0.0053f // Shunt resistance (ohms)
#define SHUNT_MAX_CURRENT_A   3.2f   // Max measurable current (A)

// NTC thermistor parameters
#define NTC_NOMINAL_RES      10000.0f // Nominal resistance at 25°C (ohms)
#define NTC_BETA_COEFF       3470.0f  // Beta coefficient of thermistor
#define NTC_SERIES_RESISTOR  3300.0f  // Series resistor value (ohms)
#define NTC_NOMINAL_TEMP_C   25.0f   // Nominal temperature (°C)