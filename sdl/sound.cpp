#include "sound.h"

#include "apu.h"
#include "port.h"
#include "memmap.h"

#include <SDL.h>

#define SDL_AUDIO_SAMPLES (32000 / 50)
#define BUFFER_SAMPLES (SDL_AUDIO_SAMPLES * 3 + 1)

static SDL_AudioSpec audiospec;

volatile static unsigned int ReadPos, WritePos;

// 2 channels per sample (stereo); 2 bytes per sample-channel (16-bit)
static uint8_t Buffer[BUFFER_SAMPLES * 2 * 2];
static uint32_t BytesPerSample;
static bool Muted = false; // S9xSetAudioMute(TRUE) gets undone after SNES Global Mute ends

static void _AudioCallback(void *userdata, Uint8 *stream, int len)
{
    uint32_t SamplesRequested = len / BytesPerSample, SamplesBuffered,
        LocalWritePos = WritePos /* isolate a bit against races with the main thread */,
        LocalReadPos = ReadPos /* keep a non-volatile copy at hand */;
    if (LocalReadPos <= LocalWritePos)
        SamplesBuffered = LocalWritePos - LocalReadPos;
    else
        SamplesBuffered = BUFFER_SAMPLES - (LocalReadPos - LocalWritePos);

    if (SamplesRequested > SamplesBuffered)
    {
        return;
    }

    if (Muted)
    {
        memset(stream, 0, len);
    }
    else if (LocalReadPos + SamplesRequested > BUFFER_SAMPLES)
    {
        memmove(stream, &Buffer[LocalReadPos * BytesPerSample], (BUFFER_SAMPLES - LocalReadPos) * BytesPerSample);
        memmove(&stream[(BUFFER_SAMPLES - LocalReadPos) * BytesPerSample], &Buffer[0], (SamplesRequested - (BUFFER_SAMPLES - LocalReadPos)) * BytesPerSample);
    }
    else
    {
        memmove(stream, &Buffer[LocalReadPos * BytesPerSample], len);
    }
    ReadPos = (LocalReadPos + SamplesRequested) % BUFFER_SAMPLES;
}

static void AudioGenerate(int samples) {
    uint32_t SamplesAvailable,
        LocalReadPos = ReadPos /* isolate a bit against races with the audio thread */,
        LocalWritePos = WritePos /* keep a non-volatile copy at hand */;
    if (LocalReadPos <= LocalWritePos)
        SamplesAvailable = BUFFER_SAMPLES - (LocalWritePos - LocalReadPos);
    else
        SamplesAvailable = LocalReadPos - LocalWritePos;
    if (samples >= SamplesAvailable)
        samples = SamplesAvailable - 1;
    int32_t samplesLeft = BUFFER_SAMPLES - LocalWritePos;
    if (samples > samplesLeft)
    {
        S9xMixSamples(&Buffer[LocalWritePos * BytesPerSample], samplesLeft * audiospec.channels);
        samples -= samplesLeft;
        LocalWritePos = 0;
    }
    S9xMixSamples(&Buffer[LocalWritePos * BytesPerSample], samples * audiospec.channels);
    WritePos = (LocalWritePos + samples) % BUFFER_SAMPLES;
}

void _ApuCallback(void *) {
    S9xFinalizeSamples();
    int samples_to_write = S9xGetSampleCount();
    if (samples_to_write <= 0) return;
    AudioGenerate(samples_to_write);
}

bool8 S9xOpenSoundDevice () {
    audiospec.freq = Settings.SoundPlaybackRate;
    audiospec.channels = Settings.Stereo ? 2 : 1;
    audiospec.format = AUDIO_S16;

    audiospec.samples = SDL_AUDIO_SAMPLES;

    BytesPerSample = audiospec.channels * 2;

    audiospec.callback = _AudioCallback;

    if (SDL_OpenAudio(&audiospec, NULL) < 0) {
        fprintf(stderr, "Unable to initialize audio.\n");
        return FALSE;
    }

    WritePos = ReadPos = 0;
    S9xSetSamplesAvailableCallback(_ApuCallback, NULL);
    return TRUE;
}

void S9xToggleSoundChannel(int c) {
    static uint8	sound_switch = 255;

    if (c == 8)
        sound_switch = 255;
    else
        sound_switch ^= 1 << c;

    S9xSetSoundControl(sound_switch);
}

void SoundPause(bool clearCache) {
    if (clearCache) {
        ReadPos = WritePos = 0;
        memset(Buffer, 0, sizeof(Buffer));
    }
    SDL_PauseAudio(1);
}

void SoundResume() {
    SDL_PauseAudio(0);
}

void SoundMute() {
    S9xSetSoundMute(TRUE);
    Muted = true;
}

void SoundUnmute() {
    Muted = false;
    S9xSetSoundMute(FALSE);
}

void SoundClose() {
    SDL_CloseAudio();
}
