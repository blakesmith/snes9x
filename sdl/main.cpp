#include "util.h"

#include "control.h"

#include "port.h"

#include "snes9x.h"
#include "apu/apu.h"
#include "conffile.h"
#include "display.h"
#include "cheats.h"
#include "memmap.h"
#include "controls.h"
#include "snapshot.h"
#include "movie.h"

#include <SDL.h>

#ifdef _MSC_VER

#include <chrono>
#include <thread>
int usleep(uint32_t usec) {
    std::this_thread::sleep_for(std::chrono::microseconds(usec));
    return 0;
}

#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;

    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        tmpres /= 10;  /*convert into microseconds*/
        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}

#else

#include <sys/time.h>
#include <unistd.h>

#endif

static bool term = false;
static ConfigFile global_conf;
static char saveFilename[PATH_MAX + 1];

void S9xSetPause(uint32) {

}

void S9xClearPause(uint32) {

}

void S9xExit() {
    S9xMovieShutdown();

    S9xSetSoundMute(TRUE);
    Settings.StopEmulation = TRUE;

    Memory.SaveSRAM(S9xGetFilename(".srm", SRAM_DIR));
    S9xResetSaveTimer(FALSE);
    S9xSaveCheatFile(S9xGetFilename(".cht", CHEAT_DIR));
    S9xUnmapAllControls();
    S9xDeinitDisplay();
    Memory.Deinit();
    S9xDeinitAPU();

    SDL_Quit();

    exit(0);
}

void S9xExtraUsage(void) {

}

void S9xParseArg(char **, int &, int) {

}

void S9xParsePortConfig(ConfigFile &conf, int pass) {
    global_conf = conf;
    global_conf.LoadFile(saveFilename);

    s9x_base_dir                   = conf.GetStringDup("BaseDir",             default_dir);
    snapshot_filename              = conf.GetStringDup("SnapshotFilename",    NULL);
    play_smv_filename              = conf.GetStringDup("PlayMovieFilename",   NULL);
    record_smv_filename            = conf.GetStringDup("RecordMovieFilename", NULL);
}

void S9xSyncSpeed() {
    if (Settings.SoundSync)
    {
        return;
    }

    if (Settings.DumpStreams)
        return;

    if (Settings.HighSpeedSeek > 0)
        Settings.HighSpeedSeek--;

    if (Settings.TurboMode)
    {
        if ((++IPPU.FrameSkip >= Settings.TurboSkipFrames) && !Settings.HighSpeedSeek)
        {
            IPPU.FrameSkip = 0;
            IPPU.SkippedFrames = 0;
            IPPU.RenderThisFrame = TRUE;
        }
        else
        {
            IPPU.SkippedFrames++;
            IPPU.RenderThisFrame = FALSE;
        }

        return;
    }

    static struct timeval	next1 = { 0, 0 };
    struct timeval			now;

    while (gettimeofday(&now, NULL) == -1) ;

    // If there is no known "next" frame, initialize it now.
    if (next1.tv_sec == 0)
    {
        next1 = now;
        next1.tv_usec++;
    }

    // If we're on AUTO_FRAMERATE, we'll display frames always only if there's excess time.
    // Otherwise we'll display the defined amount of frames.
    unsigned	limit = (Settings.SkipFrames == AUTO_FRAMERATE) ? (timercmp(&next1, &now, <) ? 10 : 1) : Settings.SkipFrames;

    IPPU.RenderThisFrame = (++IPPU.SkippedFrames >= limit) ? TRUE : FALSE;

    if (IPPU.RenderThisFrame)
        IPPU.SkippedFrames = 0;
    else
    {
        // If we were behind the schedule, check how much it is.
        if (timercmp(&next1, &now, <))
        {
            unsigned	lag = (now.tv_sec - next1.tv_sec) * 1000000 + now.tv_usec - next1.tv_usec;
            if (lag >= 500000)
            {
                // More than a half-second behind means probably pause.
                // The next line prevents the magic fast-forward effect.
                next1 = now;
            }
        }
    }

    // Delay until we're completed this frame.
    // Can't use setitimer because the sound code already could be using it. We don't actually need it either.
    while (timercmp(&next1, &now, >))
    {
        // If we're ahead of time, sleep a while.
        unsigned	timeleft = (next1.tv_sec - now.tv_sec) * 1000000 + next1.tv_usec - now.tv_usec;
        usleep(timeleft);

        while (gettimeofday(&now, NULL) == -1) ;
        // Continue with a while-loop because usleep() could be interrupted by a signal.
    }

    // Calculate the timestamp of the next frame.
    next1.tv_usec += Settings.FrameTime;
    if (next1.tv_usec >= 1000000)
    {
        next1.tv_sec += next1.tv_usec / 1000000;
        next1.tv_usec %= 1000000;
    }
}

void S9xAutoSaveSRAM() {
    Memory.SaveSRAM(S9xGetFilename(".srm", SRAM_DIR));
}

void S9xProcessEvents (bool8) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_QUIT: term = true; break;
            case SDL_KEYDOWN:
                S9xReportButton(event.key.keysym.sym | (event.key.keysym.mod << 10), true);
                break;
            case SDL_KEYUP:
                S9xReportButton(event.key.keysym.sym | (event.key.keysym.mod << 10), false);
                break;
            default: break;
        }
    }
}

void S9xMessage(int, int, const char *msg) {
    printf("%s\n", msg);
}

int main(int argc, char *argv[]) {
    if (argc < 2)
        S9xUsage();

    printf("\n\nSnes9x " VERSION " for unix\n");
    const char *homedir = getenv("HOME");
#ifdef _WIN32
    if (homedir == NULL) {
        homedir = getenv("USERPROFILE");
    }
#endif

    snprintf(default_dir, PATH_MAX + 1, "%s%s%s", homedir, SLASH_STR, ".snes9x");
    s9x_base_dir = default_dir;

    memset(&Settings, 0, sizeof(Settings));
    Settings.MouseMaster = TRUE;
    Settings.SuperScopeMaster = TRUE;
    Settings.JustifierMaster = TRUE;
    Settings.MultiPlayer5Master = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.SixteenBitSound = TRUE;
    Settings.Stereo = TRUE;
    Settings.SoundPlaybackRate = 48000;
    Settings.SoundInputRate = 31950;
    Settings.SupportHiRes = TRUE;
    Settings.Transparency = TRUE;
    Settings.AutoDisplayMessages = TRUE;
    Settings.InitialInfoStringTimeout = 120;
    Settings.HDMATimingHack = 100;
    Settings.BlockInvalidVRAMAccessMaster = TRUE;
    Settings.StopEmulation = TRUE;
    Settings.WrongMovieStateProtection = TRUE;
    Settings.DumpStreamsMaxFrames = -1;
    Settings.StretchScreenshots = 1;
    Settings.SnapshotScreenshots = TRUE;
    Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.TurboSkipFrames = 15;
#if 0
    Settings.MaxSpriteTilesPerLine = 34;
    Settings.OneClockCycle = 3;
    Settings.OneSlowClockCycle = 4;
    Settings.TwoClockCycles = 6;
#endif
    Settings.CartAName[0] = 0;
    Settings.CartBName[0] = 0;

    CPU.Flags = 0;

    snprintf(saveFilename, PATH_MAX + 1, "%s%ssnes9x.conf", S9xGetDirectory(DEFAULT_DIR), SLASH_STR);

    S9xLoadConfigFiles(argv, argc);
    rom_filename = S9xParseArgs(argv, argc);
    S9xDeleteCheats();

    make_snes9x_dirs();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Snes9x: Unable to initialize SDL.\nExiting...\n");
        exit(1);
    }

    if (!Memory.Init() || !S9xInitAPU())
    {
        fprintf(stderr, "Snes9x: Memory allocation failure - not enough RAM/virtual memory available.\nExiting...\n");
        Memory.Deinit();
        S9xDeinitAPU();
        SDL_Quit();
        exit(1);
    }

    S9xInitSound(128, 0);
    S9xSetSoundMute(TRUE);

    S9xReportControllers();

    uint32	saved_flags = CPU.Flags;
    bool8	loaded = FALSE;

    if (Settings.Multi)
    {
        loaded = Memory.LoadMultiCart(Settings.CartAName, Settings.CartBName);

        if (!loaded)
        {
            char	s1[PATH_MAX + 1], s2[PATH_MAX + 1];
            char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

            s1[0] = s2[0] = 0;

            if (Settings.CartAName[0])
            {
                _splitpath(Settings.CartAName, drive, dir, fname, ext);
                snprintf(s1, PATH_MAX + 1, "%s%s%s", S9xGetDirectory(ROM_DIR), SLASH_STR, fname);
                if (ext[0] && (strlen(s1) <= PATH_MAX - 1 - strlen(ext)))
                {
                    strcat(s1, ".");
                    strcat(s1, ext);
                }
            }

            if (Settings.CartBName[0])
            {
                _splitpath(Settings.CartBName, drive, dir, fname, ext);
                snprintf(s2, PATH_MAX + 1, "%s%s%s", S9xGetDirectory(ROM_DIR), SLASH_STR, fname);
                if (ext[0] && (strlen(s2) <= PATH_MAX - 1 - strlen(ext)))
                {
                    strcat(s2, ".");
                    strcat(s2, ext);
                }
            }

            loaded = Memory.LoadMultiCart(s1, s2);
        }
    }
    else if (rom_filename)
    {
        loaded = Memory.LoadROM(rom_filename);

        if (!loaded && rom_filename[0])
        {
            char	s[PATH_MAX + 1];
            char	drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

            _splitpath(rom_filename, drive, dir, fname, ext);
            snprintf(s, PATH_MAX + 1, "%s%s%s", S9xGetDirectory(ROM_DIR), SLASH_STR, fname);
            if (ext[0] && (strlen(s) <= PATH_MAX - 1 - strlen(ext)))
            {
                strcat(s, ".");
                strcat(s, ext);
            }

            loaded = Memory.LoadROM(s);
        }
    }

    if (!loaded)
    {
        fprintf(stderr, "Error opening the ROM file.\n");
        goto quit;
    }

    S9xDeleteCheats();
    S9xCheatsEnable();
    NSRTControllerSetup();
    Memory.LoadSRAM(S9xGetFilename(".srm", SRAM_DIR));

    if (Settings.ApplyCheats)
    {
        S9xLoadCheatFile(S9xGetFilename(".cht", CHEAT_DIR));
    }

    S9xParseArgsForCheats(argv, argc);

    CPU.Flags = saved_flags;
    Settings.StopEmulation = FALSE;

    S9xInitInputDevices();
    S9xInitDisplay(argc, argv);
    S9xSetupDefaultKeymap();
    S9xTextMode();

    if (play_smv_filename)
    {
        uint32	flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
        if (S9xMovieOpen(play_smv_filename, TRUE) != SUCCESS)
            goto quit;
        CPU.Flags |= flags;
    }
    else if (record_smv_filename)
    {
        uint32	flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
        if (S9xMovieCreate(record_smv_filename, 0xFF, MOVIE_OPT_FROM_RESET, NULL, 0) != SUCCESS)
            goto quit;
        CPU.Flags |= flags;
    }
    else
    {
        if (snapshot_filename)
        {
            uint32	flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
            if (!S9xUnfreezeGame(snapshot_filename))
                goto quit;
            CPU.Flags |= flags;
        }
    }

    S9xGraphicsMode();

    sprintf(String, "\"%s\" %s: %s", Memory.ROMName, TITLE, VERSION);
    S9xSetTitle(String);

#ifdef JOYSTICK_SUPPORT
    uint32	JoypadSkip = 0;
#endif

    S9xSetSoundMute(FALSE);

#ifdef NETPLAY_SUPPORT
    bool8	NP_Activated = Settings.NetPlay;
#endif

    term = false;
    while (!term)
    {
        if (!Settings.Paused)
        {
            S9xMainLoop();
        }
        if (Settings.Paused)
            S9xSetSoundMute(TRUE);

        if (Settings.Paused)
        {
            S9xProcessEvents(FALSE);
            usleep(100000);
        }

        S9xProcessEvents(FALSE);

        if (!Settings.Paused)
            S9xSetSoundMute(FALSE);
    }

    Settings.StopEmulation = TRUE;

    if (!Settings.StopEmulation)
    {
        Memory.SaveSRAM (S9xGetFilename (".srm", SRAM_DIR));
        S9xSaveCheatFile (S9xGetFilename (".cht", CHEAT_DIR));
    }

    S9xMovieShutdown (); // must happen before saving config

    global_conf.SaveTo(saveFilename);

quit:
    S9xExit();

    return 0;
}
