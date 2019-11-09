#include "input.h"
#include "sound.h"
#include "menu.h"
#include "video.h"
#include "util.h"

#include "port.h"

#include "snes9x.h"
#include "apu/apu.h"
#include "conffile.h"
#include "display.h"
#include "cheats.h"
#include "memmap.h"
#include "controls.h"
#include "snapshot.h"

#include <SDL.h>

#include <libintl.h>
#include <sys/stat.h>

#define _(s) gettext(s)

bool s9xTerm = false;
static ConfigFile global_conf;
static char saveFilename[PATH_MAX + 1];

char s9xBaseDir[PATH_MAX + 1] = {0};
char romFilename[PATH_MAX + 1] = {0};
char snapshotFilename[PATH_MAX + 1] = {0};
char defaultDir[PATH_MAX + 1] {0};

static void quickLoadState(int n);
static void quickSaveState(int n);
static void enterMainMenu();
static MenuResult enterSaveStateMenu();
static MenuResult enterLoadStateMenu();
static MenuResult enterSettingsMenu();

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

    snprintf(defaultDir, PATH_MAX + 1, "%s%s%s", homedir, SLASH_STR, ".snes9x");
    strncpy(s9xBaseDir, defaultDir, PATH_MAX + 1);

    memset(&Settings, 0, sizeof(Settings));
    Settings.MouseMaster = TRUE;
    Settings.SuperScopeMaster = TRUE;
    Settings.JustifierMaster = TRUE;
    Settings.MultiPlayer5Master = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.SixteenBitSound = TRUE;
    Settings.Stereo = TRUE;
    Settings.SoundPlaybackRate = 32000;
    Settings.SoundInputRate = 32000;
    Settings.SupportHiRes = FALSE;
    Settings.Transparency = TRUE;
    Settings.AutoDisplayMessages = TRUE;
    Settings.InitialInfoStringTimeout = 120;
    Settings.HDMATimingHack = 100;
    Settings.BlockInvalidVRAMAccessMaster = TRUE;
    Settings.StopEmulation = TRUE;
    Settings.WrongMovieStateProtection = TRUE;
    Settings.DumpStreamsMaxFrames = -1;
    Settings.StretchScreenshots = 0;
    Settings.SnapshotScreenshots = FALSE;
    Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.TurboSkipFrames = 15;
    Settings.FastSavestates = TRUE;
    Settings.DontSaveOopsSnapshot = TRUE;
    Settings.AutoSaveDelay = 5;
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
    const char *romfn = S9xParseArgs(argv, argc);
    if (romfn) strncpy(romFilename, romfn, PATH_MAX + 1);
    S9xDeleteCheats();

    MakeS9xDirs();

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
    SoundPause();
    S9xSetSoundMute(TRUE);

    S9xReportControllers();

    uint32 saved_flags = CPU.Flags;
    bool8 loaded = FALSE;

    if (Settings.Multi)
    {
        loaded = Memory.LoadMultiCart(Settings.CartAName, Settings.CartBName);

        if (!loaded)
        {
            char s1[PATH_MAX + 1], s2[PATH_MAX + 1];
            char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

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
    else if (romFilename)
    {
        loaded = Memory.LoadROM(romFilename);

        if (!loaded && romFilename[0])
        {
            char s[PATH_MAX + 1];
            char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

            _splitpath(romFilename, drive, dir, fname, ext);
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
        S9xExit();
        return 1;
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

    if (snapshotFilename[0] && !S9xUnfreezeGame(snapshotFilename)) {
        S9xExit();
        return 1;
    }

    S9xGraphicsMode();

#ifndef GCW_ZERO
    sprintf(String, "\"%s\" %s: %s", Memory.ROMName, TITLE, VERSION);
    S9xSetTitle(String);
#endif

    S9xSetSoundMute(FALSE);
    SoundResume();

    s9xTerm = false;
    while (!s9xTerm)
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

    if (VideoSettings.FrameRate == 0)
        global_conf.SetString("Settings::FrameSkip", "Auto");
    else
        global_conf.SetInt("Settings::FrameSkip", VideoSettings.FrameRate - 1);
    global_conf.SetBool("Video::Fullscreen", VideoSettings.Fullscreen);
    global_conf.SetBool("Display::DisplayFrameRate", Settings.DisplayFrameRate);
    global_conf.SetBool("Hack::AllowInvalidVRAMAccess", VideoSettings.AllowInvalidVRAMAccess);
    global_conf.SaveTo(saveFilename);

    S9xExit();

    return 0;
}

void S9xExit() {
    S9xSetSoundMute(TRUE);
    SoundPause();

    Settings.StopEmulation = TRUE;

    Memory.SaveSRAM(S9xGetFilename(".srm", SRAM_DIR));
    S9xResetSaveTimer(FALSE);
    S9xSaveCheatFile(S9xGetFilename(".cht", CHEAT_DIR));
    S9xUnmapAllControls();
    S9xDeinitDisplay();
    Memory.Deinit();
    S9xDeinitAPU();

    SoundClose();
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

    VideoSettings.Fullscreen = conf.GetBool("Video::Fullscreen", true);
    VideoSettings.AllowInvalidVRAMAccess = !Settings.BlockInvalidVRAMAccessMaster;
    VideoSettings.FrameRate = Settings.SkipFrames == AUTO_FRAMERATE ? 0 : Settings.SkipFrames;

    const char *dir = global_conf.GetString("BaseDir", defaultDir);
    if (dir) strncpy(s9xBaseDir, dir, PATH_MAX + 1);
    dir = global_conf.GetString("SnapshotFilename", NULL);
    if (dir) strncpy(snapshotFilename, dir, PATH_MAX + 1);
}

void S9xAutoSaveSRAM() {
    Memory.SaveSRAM(S9xGetFilename(".srm", SRAM_DIR));
}

void S9xProcessEvents (bool8) {
    SDL_Event event;
    bool enterMenu = false;
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT: s9xTerm = true; break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym ==
#ifdef GCW_ZERO
                SDLK_HOME
#else
                SDLK_ESCAPE
#endif
                ) {
                enterMenu = true;
                break;
            }
            S9xReportButton(event.key.keysym.sym
#ifndef GCW_ZERO
                | (event.key.keysym.mod << 10)
#endif
                , true);
            break;
        case SDL_KEYUP:
            S9xReportButton(event.key.keysym.sym
#ifndef GCW_ZERO
                | (event.key.keysym.mod << 10)
#endif
                , false);
            break;
        default: break;
        }
    }
    if (enterMenu) {
        Settings.Paused = TRUE;
        SoundMute();

        VideoFreeze();
        VideoSetOriginResolution();

        enterMainMenu();

        VideoClearCache();
        VideoUnfreeze();
        SoundUnmute();
        Settings.Paused = FALSE;
    }
}

void S9xMessage(int, int, const char *msg) {
    printf("%s\n", msg);
}

static void buildStateFilename(int n, char def[_MAX_FNAME + 1], char filename[PATH_MAX + 1]) {
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], ext[_MAX_EXT + 1];

    _splitpath(Memory.ROMFilename, drive, dir, def, ext);
    snprintf(filename, PATH_MAX + 1, "%s%s%s.%03d", S9xGetDirectory(SNAPSHOT_DIR), SLASH_STR, def, n);
}

static void quickLoadState(int n) {
    char filename[PATH_MAX + 1];
    char def[_MAX_FNAME + 1];
    buildStateFilename(n, def, filename);

    if (S9xUnfreezeGame(filename)) {
        char buf[256];
        snprintf(buf, 256, "%s.%03d loaded", def, n);
        S9xSetInfoString(buf);
    } else
        S9xMessage(S9X_ERROR, S9X_FREEZE_FILE_NOT_FOUND, "Freeze file not found");
}

static void quickSaveState(int n) {
    char filename[PATH_MAX + 1];
    char def[_MAX_FNAME + 1];
    buildStateFilename(n, def, filename);

    if (S9xFreezeGame(filename)) {
        strncat(filename, ".png", PATH_MAX);
        VideoTakeScreenshot(filename);
        char buf[256];
        snprintf(buf, 256, "%s.%03d saved", def, n);
        S9xSetInfoString(buf);
    } else
        S9xMessage(S9X_ERROR, S9X_FREEZE_FILE_NOT_FOUND, "Unable to write file");
}

static void enterMainMenu() {
    const MenuItem items[] = {
        { MIT_CLICK, NULL, _("Save state"), 0, 0,
          [](const MenuItem*)->MenuResult { return enterSaveStateMenu(); } },
        { MIT_CLICK, NULL, _("Load state"), 0, 0,
          [](const MenuItem*)->MenuResult { return enterLoadStateMenu(); } },
        { MIT_CLICK, NULL, _("Settings"), 0, 0,
          [](const MenuItem*)->MenuResult { return enterSettingsMenu(); } },
        { MIT_CLICK, NULL, _("Reset"), 0, 0,
          [](const MenuItem*)->MenuResult { S9xReset(); return MR_LEAVE; } },
        { MIT_CLICK, NULL, _("Quit"), 0, 0,
          [](const MenuItem*)->MenuResult { s9xTerm = true; return MR_LEAVE; } },
        { MIT_END }
    };
    MenuRun(items, 120, 0, 80, 0, 0);
}

static MenuResult enterSaveStateMenu() {
    MenuItem items[11] = {};
    char text[10][64];
    for (int i = 0; i < 10; ++i) {
        items[i].type = MIT_CLICK;
        snprintf(text[i], 64, _("Save to slot %d"), i);
        items[i].text = text[i];
        items[i].customData = (void*)(uintptr_t)i;
        items[i].triggerFunc = [](const MenuItem *item)->MenuResult {
            quickSaveState((int)(uintptr_t)item->customData);
            return MR_LEAVE;
        };
    }
    items[10].type = MIT_END;
    if (MenuRun(items, 80, 0, 70, 0, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}

static MenuResult enterLoadStateMenu() {
    char filename[PATH_MAX + 1];
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], def[_MAX_FNAME + 1], ext[_MAX_EXT + 1];
    _splitpath(Memory.ROMFilename, drive, dir, def, ext);

    MenuItem items[11] = {};
    char text[10][64];
    for (int i = 0; i < 10; ++i) {
        items[i].type = MIT_CLICK;
        snprintf(filename, PATH_MAX + 1, "%s%s%s.%03d", S9xGetDirectory(SNAPSHOT_DIR), SLASH_STR, def, i);
        struct stat st = {};
        if (stat(filename, &st) != 0 || !S_ISREG(st.st_mode)) {
            snprintf(text[i], 64, _("Empty slot %d"), i);
            items[i].triggerFunc = NULL;
            items[i].drawFunc = NULL;
        } else {
            snprintf(text[i], 64, _("Load from slot %d"), i);
            items[i].customData = (void*)(uintptr_t)i;
            items[i].triggerFunc = [](const MenuItem *item)->MenuResult {
                quickLoadState((int)(uintptr_t)item->customData);
                return MR_LEAVE;
            };
            items[i].drawFunc = [](const MenuItem *item, int, bool selected) {
                if (!selected) return;
                int n = (int)(uintptr_t)item->customData;
                char filename[PATH_MAX + 1];
                char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], def[_MAX_FNAME + 1], ext[_MAX_EXT + 1];
                _splitpath(Memory.ROMFilename, drive, dir, def, ext);
                snprintf(filename, PATH_MAX + 1, "%s%s%s.%03d.png", S9xGetDirectory(SNAPSHOT_DIR), SLASH_STR, def, n);
                VideoImageData vid = VideoLoadImageFile(filename);
                if (vid.buffer) {
                    VideoDrawImage(180, 64, &vid);
                    VideoFreeImage(&vid);
                }
            };
        }
        items[i].text = text[i];
    }
    items[10].type = MIT_END;
    if (MenuRun(items, 80, 0, 70, 0, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}

static MenuResult enterSettingsMenu() {
    const MenuItem items[] = {
        { MIT_BOOL, &VideoSettings.Fullscreen, _("Fullscreen") },
        { MIT_BOOL8, &Settings.DisplayFrameRate, _("Show FPS") },
        { MIT_INT32, &VideoSettings.FrameRate, _("Frame Skip"), 0, 10,
          [](const MenuItem*)->MenuResult { Settings.SkipFrames = VideoSettings.FrameRate == 0 ? AUTO_FRAMERATE : VideoSettings.FrameRate; return MR_NONE; },
          NULL,
          [](int val)->MenuItemValue {
            const char *values[11] = {_("AUTO"), "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
            return MenuItemValue { values[val < 0 || val > 10 ? 0 : val], NULL };
        }},
        { MIT_BOOL8, &VideoSettings.AllowInvalidVRAMAccess, _("AllowInvalidVRAMAccess"), 0, 0,
          [](const MenuItem*)->MenuResult { Settings.BlockInvalidVRAMAccessMaster = Settings.BlockInvalidVRAMAccess = !VideoSettings.AllowInvalidVRAMAccess; return MR_NONE; }},
        { MIT_END }
    };
    if (MenuRun(items, 70, 210, 80, 0, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}
