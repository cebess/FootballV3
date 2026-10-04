#ifndef DISPLAY_BOARD_H
#define DISPLAY_BOARD_H

#include <LovyanGFX.hpp>
#include "RotaryEncoder.h"
#include <Arduino.h>

// Forward declaration to avoid circular include
class MenuSystem;
/**
 * @brief Class for managing a display board with encoder
 * 
 * This class combines a LovyanGFX display with a rotary encoder
 * and push button for interactive control.
 */
class DisplayBoard {
private:
    lgfx::LGFX_Device* display;
    RotaryEncoder* encoder;
    int8_t backlightPin;
    bool backlightOn;
    String boardName;
    MenuSystem* boundMenu;
    int encoderLastPos;
    int encoderRemainder;
    int encoderCountsPerStep;
    
public:
    /**
     * @brief Constructor for DisplayBoard
     * @param displayPtr Pointer to LovyanGFX display object
     * @param encoderPtr Pointer to RotaryEncoder object
     * @param blPin GPIO pin for backlight control (-1 if not used)
     * @param name Name identifier for this board
     */
    DisplayBoard(lgfx::LGFX_Device* displayPtr, RotaryEncoder* encoderPtr, 
                 int8_t blPin = -1, const String& name = "Board");
    
    /**
     * @brief Initialize the display and encoder
     */
    void begin();
    
    /**
     * @brief Update the board state (call in loop)
     */
    void update();

    /**
     * @brief Bind a menu to this board's encoder handling.
     * @param menu Pointer to the menu controlled by this board's encoder
     * @param countsPerStep Encoder counts required for one menu step
     */
    void bindMenu(MenuSystem* menu, int countsPerStep = 2);

    /**
     * @brief Process encoder rotation and button events for the bound menu.
     */
    void processEncoderInput();
    
    /**
     * @brief Get the display object
     * @return Pointer to LovyanGFX display
     */
    lgfx::LGFX_Device* getDisplay() { return display; }
    
    /**
     * @brief Get the encoder object
     * @return Pointer to RotaryEncoder
     */
    RotaryEncoder* getEncoder() { return encoder; }
    
    /**
     * @brief Control backlight
     * @param on true to turn on, false to turn off
     */
    void setBacklight(bool on);
    
    /**
     * @brief Toggle backlight state
     */
    void toggleBacklight();
    
    /**
     * @brief Get board name
     * @return Board name string
     */
    String getName() const { return boardName; }
    
    /**
     * @brief Clear display with black color
     */
    void clearDisplay();
};

#endif // DISPLAY_BOARD_H