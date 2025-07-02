#include "sounds.h"
#include "pch.h"
#include <mmsystem.h>
#include <cmath>
#include <corecrt_math_defines.h>
#include <vector>
#include <thread>

#ifdef ADVANCED_SOUNDS
uint8_t Sounds::playTone(uint16_t frequency, uint16_t durationMs, uint16_t amplitude) {
    uint16_t sampleRate = 44100;
    uint16_t numSamples = (durationMs * sampleRate) / 1000;
    WAVEFORMATEX waveFormat{};
    HWAVEOUT hWaveOut;
    WAVEHDR waveHeader{};

    // Configure wave format for stereo
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = sampleRate;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // Allocate memory for stereo sound
    std::vector<int16_t> waveData(numSamples * 2);

    // Generate sine wave with amplitude variation between channels
    for (int i = 0; i < numSamples; i++) {
        int16_t leftSample = static_cast<int16_t>((amplitude - 5000) * sin(2 * M_PI * frequency * i / sampleRate));
        int16_t rightSample = static_cast<int16_t>((amplitude + 5000) * sin(2 * M_PI * frequency * i / sampleRate));

        waveData[i * 2] = leftSample;    // Left channel
        waveData[i * 2 + 1] = rightSample; // Right channel
    }

    // Prepare wave header
    waveHeader.lpData = reinterpret_cast<LPSTR>(waveData.data());
    waveHeader.dwBufferLength = numSamples * sizeof(int16_t) * 2;
    waveHeader.dwFlags = 0;
    waveHeader.dwLoops = 0;

    // Open audio device
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        return AUDIO_DEVICE_ERROR;

    if (waveOutPrepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        waveOutClose(hWaveOut);
        return HEADER_PREPARATION_ERROR;
    }

    // Play the sound
    if (waveOutWrite(hWaveOut, &waveHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
        waveOutUnprepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        return PLAY_SOUND_ERROR;
    }

    // Wait for playback to finish
    while (waveOutUnprepareHeader(hWaveOut, &waveHeader, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
        Sleep(1);
    }

    // Cleanup
    waveOutClose(hWaveOut);
    return OK;
}
#endif // ADVANCED_SOUNDS

void Sounds::playWrong()
{
#ifdef ADVANCED_SOUNDS
    Sounds::playTone(3700, 3, 20000);
#else
    Beep(3700, 3);
#endif // ADVANCED_SOUNDS
}

void Sounds::playCorrect()
{
    bool random = false;
    uint16_t freq = random ? (rand() % 100 * 15) + 1000 : 2000;
#ifdef ADVANCED_SOUNDS
    Sounds::playTone(freq, 3);
#else
    Beep(freq, 3);
#endif // ADVANCED_SOUNDS
}

void Sounds::playEnd()
{
    const uint8_t sequenceLength = 2;
    const std::pair<uint16_t, uint16_t> sequence[] = { {500, 30}, {750, 30} };

    std::thread beep(
        [=]() {

            for (int i = 0; i < sequenceLength; i++) {
#ifdef ADVANCED_SOUNDS
                Sounds::playTone(sequence[i].first, sequence[i].second);
#else
                Beep(sequence[i].first, sequence[i].second);
#endif // ADVANCED_SOUNDS
            }
        }
    );

    beep.detach();
}

void Sounds::playStar(int idx)
{
#ifdef ADVANCED_SOUNDS
    Sounds::playTone(1500 + idx * 250, 55 + idx * 10, 30000 + idx * 3000);
#else
    Beep(1500 + idx * 250, 55 + idx * 10);
#endif // ADVANCED_SOUNDS
}
