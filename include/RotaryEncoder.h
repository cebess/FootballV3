#pragma once

#include <Arduino.h>

/**
 * @brief Class for handling rotary encoder with push button
 *
 * This class manages the EC11 rotary encoder including:
 * - Increment/decrement detection
 * - Push button press detection
 * - Debouncing for reliable operation
 */
class RotaryEncoder {
private:
    uint8_t pinA, pinB, pinPush;
    int32_t position;
    int32_t lastPosition;
    uint32_t lastButtonReadTime;
    uint32_t lastEncoderReadTime;
    bool lastButtonState;
    bool buttonPressed;
    volatile int16_t encoderState;
    int8_t transitionAccumulator;
    static const uint32_t DEBOUNCE_DELAY = 50; // milliseconds

public:
    /**
     * @brief Constructor for RotaryEncoder
     * @param aPin GPIO pin for encoder channel A
     * @param bPin GPIO pin for encoder channel B
     * @param pushPin GPIO pin for push button
     */
    RotaryEncoder(uint8_t aPin, uint8_t bPin, uint8_t pushPin);

    /**
     * @brief Initialize the encoder pins and state
     */
    void begin();

    /**
     * @brief Update encoder state (call in loop)
     */
    void update();

    /**
     * @brief Get current encoder position
     * @return Current position value
     */
    int32_t getPosition() const { return position; }

    /**
     * @brief Check if encoder position changed
     * @return true if position changed
     */
    bool hasChanged() {
        if (position != lastPosition) {
            lastPosition = position;
            return true;
        }
        return false;
    }

    /**
     * @brief Check if push button was pressed (single press)
     * @return true if button was pressed
     */
    bool isButtonPressed();

    /**
     * @brief Reset encoder position to zero
     */
    void resetPosition() { position = 0; lastPosition = 0; }
};
