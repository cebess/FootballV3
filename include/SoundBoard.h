#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <map>

/**
 * @brief Plays named sound effects on an MP3-TF-16P module.
 *
 * Track audio files live on the module's own TF card, named 0001.mp3, 0002.mp3, etc.
 * Friendly names are mapped to those track numbers in a hardcoded table (see SoundBoard.cpp).
 */
class SoundBoard {
public:
    static constexpr uint8_t MP3_RX_PIN = 27;  // ESP32 RX <- module TX
    static constexpr uint8_t MP3_TX_PIN = 19;  // ESP32 TX -> module RX
    static constexpr uint32_t MP3_BAUD_RATE = 9600;
    static constexpr uint8_t MAX_VOLUME = 30;

    SoundBoard();

    /**
     * @brief Initializes the MP3 player.
     * @return true if the player responded and is ready to play.
     */
    bool begin();

    /**
     * @brief Plays the track mapped to the given sound name.
     * @param name Friendly sound name as it appears in the track table.
     * @return true if the name was found and a play command was sent.
     */
    bool playSound(const String& name);
    bool playSoundNumber(const int track);

    /**
     * @brief Returns the track filename (e.g. "0001.mp3") mapped to the given sound name.
     * @param name Friendly sound name as it appears in the track table.
     * @return The 4-digit filename with ".mp3" appended, or an empty string if not found.
     */
    String trackName(const String& name) const;

    void setVolume(uint8_t volume);
    void stop();

    /**
     * @brief Blocks until the module reports the current track finished playing.
     * @param timeoutMs Maximum time to wait before giving up.
     * @return true if a "play finished" notification was received; false on timeout.
     */
    bool waitUntilFinished(uint32_t timeoutMs = 10000);

    bool isReady() const { return playerReady; }
    size_t soundCount() const { return trackByName.size(); }

private:
    HardwareSerial mp3Serial;
    DFRobotDFPlayerMini player;
    std::map<String, uint16_t> trackByName;
    bool playerReady;
};
