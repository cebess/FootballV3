#include "DisplayBoard.h"
#include "MenuSystem.h"

/**
 * @file DisplayBoard.cpp
 * @brief Implements display-board wrapper behavior.
 *
 * High-level behavior present in this file:
 * - DisplayBoard: initializes display/encoder resources, updates encoder state, and controls optional backlight.
 * - Uses pointer checks before interacting with optional display/encoder objects.
 */

// DisplayBoard Implementation
/**
 * Function: DisplayBoard::DisplayBoard(lgfx::LGFX_Device* displayPtr, RotaryEncoder* encoderPtr, int8_t blPin, const String& name)
 * Purpose: Construct a board wrapper by storing display/encoder pointers, backlight pin, and board name.
 * Parameters:
 * - lgfx::LGFX_Device* displayPtr: Display instance used by this board.
 * - RotaryEncoder* encoderPtr: Encoder instance used by this board.
 * - int8_t blPin: Backlight control pin value (-1 disables backlight control).
 * - const String& name: Human-readable board name.
 * Returns: None (constructor).
 * Usage example: DisplayBoard board(&display, &encoder, 19, "Board 1");
 * Warnings/constraints:
 * - Pointers are stored as-is; constructor does not validate ownership/lifetime.
 * - backlightOn defaults to true.
 */
DisplayBoard::DisplayBoard(lgfx::LGFX_Device* displayPtr, RotaryEncoder* encoderPtr,
                           int8_t blPin, const String& name)
    : display(displayPtr), encoder(encoderPtr), backlightPin(blPin),
    backlightOn(true), boardName(name), boundMenu(nullptr),
    encoderLastPos(0), encoderRemainder(0), encoderCountsPerStep(2) {}

/**
 * Function: DisplayBoard::begin()
 * Purpose: Initialize display, encoder, and optional backlight pin.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: board.begin();
 * Warnings/constraints:
 * - Display initialization runs only when display pointer is non-null.
 * - Encoder initialization runs only when encoder pointer is non-null.
 * - Backlight pin setup runs only when backlightPin != -1 comparison passes.
 */
void DisplayBoard::begin() {
    if (display) {
        display->init();
        display->setRotation(0);
        display->fillScreen(TFT_BLACK);
    }
    
    if (encoder) {
        encoder->begin();
    }
    
    if (backlightPin != -1) {
        pinMode(backlightPin, OUTPUT);
        digitalWrite(backlightPin, HIGH);
    }
}

/**
 * Function: DisplayBoard::update()
 * Purpose: Update encoder state for this board.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: board.update();
 * Warnings/constraints:
 * - Does nothing when encoder pointer is null.
 */
void DisplayBoard::update() {
    if (encoder) {
        encoder->update();
    }
}

void DisplayBoard::bindMenu(MenuSystem* menu, int countsPerStep) {
    boundMenu = menu;
    encoderCountsPerStep = countsPerStep > 0 ? countsPerStep : 1;
    encoderRemainder = 0;
    encoderLastPos = encoder ? encoder->getPosition() : 0;
}

void DisplayBoard::processEncoderInput() {
    if (!encoder || !boundMenu) {
        return;
    }

    int currentPos = encoder->getPosition();
    if (currentPos != encoderLastPos) {
        int diff = currentPos - encoderLastPos;
        encoderRemainder += diff;

        if (boundMenu->isEditingValue()) {
            while (encoderRemainder >= encoderCountsPerStep) {
                boundMenu->changeValue(1);
                encoderRemainder -= encoderCountsPerStep;
            }
            while (encoderRemainder <= -encoderCountsPerStep) {
                boundMenu->changeValue(-1);
                encoderRemainder += encoderCountsPerStep;
            }
        } else {
            while (encoderRemainder >= encoderCountsPerStep) {
                boundMenu->selectDown();
                encoderRemainder -= encoderCountsPerStep;
            }
            while (encoderRemainder <= -encoderCountsPerStep) {
                boundMenu->selectUp();
                encoderRemainder += encoderCountsPerStep;
            }
        }

        encoderLastPos = currentPos;
    }

    if (encoder->isButtonPressed()) {
        boundMenu->selectCurrentItem();
    }
}

/**
 * Function: DisplayBoard::setBacklight(bool on)
 * Purpose: Store desired backlight state and write pin level when backlight pin is considered valid.
 * Parameters:
 * - bool on: true writes HIGH, false writes LOW.
 * Returns: void.
 * Usage example: board.setBacklight(false);
 * Warnings/constraints:
 * - Pin write occurs only when backlightPin != -1 comparison passes.
 */
void DisplayBoard::setBacklight(bool on) {
    backlightOn = on;
    if (backlightPin != -1) {
        digitalWrite(backlightPin, on ? HIGH : LOW);
    }
}

/**
 * Function: DisplayBoard::toggleBacklight()
 * Purpose: Invert stored backlight state and apply it via setBacklight().
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: board.toggleBacklight();
 * Warnings/constraints:
 * - Effective pin output depends on setBacklight() backlight pin condition.
 */
void DisplayBoard::toggleBacklight() {
    setBacklight(!backlightOn);
}

/**
 * Function: DisplayBoard::clearDisplay()
 * Purpose: Clear display contents to black.
 * Parameters:
 * - None.
 * Returns: void.
 * Usage example: board.clearDisplay();
 * Warnings/constraints:
 * - Does nothing when display pointer is null.
 */
void DisplayBoard::clearDisplay() {
    if (display) {
        display->fillScreen(TFT_BLACK);
    }
}