#pragma once

#include <stdint.h>

struct VideoImageData {
    uint16_t *buffer;
    int w, h;
};

struct SVideoSettings {
    bool Fullscreen;
};

void VideoSetOriginResolution();
void VideoOutputString(int x, int y, const char *text, bool allowWrap = false, bool shadow = false);
void VideoOutputStringPixel(int x, int y, const char *text, bool allowWrap = false, bool shadow = false);
void VideoSetLogMessage(const char *msg, uint32_t msecs);
void VideoClear();
void VideoClearCache();
void VideoFlip();
void VideoFreeze();
void VideoUnfreeze();

/* Read image from file, return with buffer set to NULL means fail */
VideoImageData VideoLoadImageFile(const char *filename);
void VideoFreeImage(VideoImageData *data);
void VideoDrawImage(int x, int y, VideoImageData *data);
void VideoTakeScreenshot(const char *filename);

void VideoSetEnterMenu();

extern SVideoSettings VideoSettings;
