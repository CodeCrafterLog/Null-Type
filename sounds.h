#pragma once
#include <stdint.h>

namespace Sounds
{
#ifndef ADVANCED_SOUNDS
#define ADVANCED_SOUNDS TRUE
#endif // !ADVANCED_SOUNDS

#ifdef ADVANCED_SOUNDS

#pragma region Audio Errors
    // TODO Debug errors
    constexpr uint8_t OK                        = 0;
    constexpr uint8_t AUDIO_DEVICE_ERROR        = 1; // Failed to open audio device
    constexpr uint8_t HEADER_PREPARATION_ERROR  = 2; // Failed to prepare header
    constexpr uint8_t PLAY_SOUND_ERROR          = 3; // Failed to play sound
#pragma endregion

    uint8_t playTone(uint16_t frequency, uint16_t durationMs, uint16_t amplitude = 30000);
#endif // ADVANCED_SOUNDS

#pragma region Sounds
    void playWrong();
    void playCorrect();
    void playEnd();
    void playStar(int idx);
#pragma endregion
}