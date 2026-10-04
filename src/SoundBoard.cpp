#include "SoundBoard.h"

namespace {
// Function-local static avoids initialization-order issues with the global SoundBoard instance.
const std::map<String, uint16_t>& soundTable() {
    static const std::map<String, uint16_t> kSoundTable = {
        {"touchdownHome", 1},
        {"firstdown", 2},
        {"crowd_cheer", 3},
        {"field_goal", 4},
        {"turnover", 5},
        {"point_after", 6},
        {"conversion", 7},
        {"safety", 8},
        {"end_of_game",9},
        {"touchdownAway", 10},
        {"field_goal_miss", 11},
        {"disappointment", 12}
    };
    return kSoundTable;
}
}  // namespace

SoundBoard::SoundBoard() : mp3Serial(2), trackByName(soundTable()), playerReady(false) {}

bool SoundBoard::begin() {
    mp3Serial.begin(MP3_BAUD_RATE, SERIAL_8N1, MP3_RX_PIN, MP3_TX_PIN);
    playerReady = player.begin(mp3Serial, /*isACK=*/true, /*doReset=*/true);
    if (!playerReady) {
        Serial.println("SoundBoard: MP3-TF-16P not detected.");
        return false;
    }

    player.volume(20);
    Serial.printf("SoundBoard: ready with %u sound(s).\n", static_cast<unsigned>(trackByName.size()));
    return true;
}

bool SoundBoard::playSound(const String& name) {
    Serial.printf("SoundBoard::playSound: requested \"%s\"\n", name.c_str());
    if (!playerReady) {
        Serial.println("SoundBoard::playSound: player not ready.");
        return false;
    }

    auto it = trackByName.find(name);
    if (it == trackByName.end()) {
        Serial.printf("SoundBoard::playSound: \"%s\" not found in manifest.\n", name.c_str());
        return false;
    }

    Serial.printf("  track is: %u\n", static_cast<unsigned>(it->second));
    // Addresses /MP3/NNNN.mp3 by its filename number, unlike play() which uses FAT directory order.
    player.playMp3Folder(it->second);
    return true;
}

bool SoundBoard::playSoundNumber(const int track) {
    Serial.printf("SoundBoard::playSound: track \"%d\"\n", track);
    player.playMp3Folder(track);
    return true;
}

bool SoundBoard::waitUntilFinished(uint32_t timeoutMs) {
    if (!playerReady) {
        return false;
    }

    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (player.available()) {
            if (player.readType() == DFPlayerPlayFinished) {
                return true;
            }
        }
    }
    return false;
}

void SoundBoard::setVolume(uint8_t volume) {
    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    player.volume(volume);
}

void SoundBoard::stop() {
    if (playerReady) {
        player.stop();
    }
}
