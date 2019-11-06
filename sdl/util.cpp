#include "util.h"

#include "display.h"

#include "port.h"
#include "apu/apu.h"
#include "memmap.h"

#include <unistd.h>
#include <limits.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#define mkdir(d, p) _mkdir(d)
#else
#include <sys/stat.h>
#endif

extern char s9xBaseDir[PATH_MAX + 1];
extern char romFilename[PATH_MAX + 1];

static const char *dirNames[13] =
    {
        "", // DEFAULT_DIR
        "", // HOME_DIR
        "", // ROMFILENAME_DIR
        "rom", // ROM_DIR
        "sram", // SRAM_DIR
        "savestate", // SNAPSHOT_DIR
        "screenshot", // SCREENSHOT_DIR
        "spc", // SPC_DIR
        "cheat", // CHEAT_DIR
        "patch", // PATCH_DIR
        "bios", // BIOS_DIR
        "log", // LOG_DIR
        ""
    };

#ifndef __USE_MINGW_ANSI_STDIO

void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext)
{
    *drive = 0;

    const char *slash = strrchr(path, SLASH_CHAR),
        *dot   = strrchr(path, '.');

    if (dot && slash && dot < slash)
        dot = NULL;

    if (!slash)
    {
        *dir = 0;

        strcpy(fname, path);

        if (dot)
        {
            fname[dot - path] = 0;
            strcpy(ext, dot + 1);
        }
        else
            *ext = 0;
    }
    else
    {
        strcpy(dir, path);
        dir[slash - path] = 0;

        strcpy(fname, slash + 1);

        if (dot)
        {
            fname[dot - slash - 1] = 0;
            strcpy(ext, dot + 1);
        }
        else
            *ext = 0;
    }
}

void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
{
    if (dir && *dir)
    {
        strcpy(path, dir);
        strcat(path, SLASH_STR);
    }
    else
        *path = 0;

    strcat(path, fname);

    if (ext && *ext)
    {
        strcat(path, ".");
        strcat(path, ext);
    }
}
#endif

const char * S9xSelectFilename(const char *def, const char *dir1, const char *ext1, const char *title) {
    static char s[PATH_MAX + 1];
    char buffer[PATH_MAX + 1];

    printf("\n%s (default: %s): ", title, def);
    fflush(stdout);

    if (fgets(buffer, PATH_MAX + 1, stdin))
    {
        char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

        char *p = buffer;
        while (isspace(*p))
            p++;
        if (!*p)
        {
            strncpy(buffer, def, PATH_MAX + 1);
            buffer[PATH_MAX] = 0;
            p = buffer;
        }

        char *q = strrchr(p, '\n');
        if (q)
            *q = 0;

        _splitpath(p, drive, dir, fname, ext);
        _makepath(s, drive, *dir ? dir : dir1, fname, *ext ? ext : ext1);

        return (s);
    }

    return (NULL);
}

const char * S9xGetDirectory(enum s9x_getdirtype dirtype) {
    static char s[PATH_MAX + 1];

    if (dirNames[dirtype][0])
        snprintf(s, PATH_MAX + 1, "%s%s%s", s9xBaseDir, SLASH_STR, dirNames[dirtype]);
    else
    {
        switch (dirtype)
        {
            case DEFAULT_DIR:
                strncpy(s, s9xBaseDir, PATH_MAX + 1);
                s[PATH_MAX] = 0;
                break;

            case HOME_DIR:
                strncpy(s, getenv("HOME"), PATH_MAX + 1);
                s[PATH_MAX] = 0;
                break;

            case ROMFILENAME_DIR:
                strncpy(s, Memory.ROMFilename, PATH_MAX + 1);
                s[PATH_MAX] = 0;

                for (int i = strlen(s); i >= 0; i--)
                {
                    if (s[i] == SLASH_CHAR)
                    {
                        s[i] = 0;
                        break;
                    }
                }

                break;

            default:
                s[0] = 0;
                break;
        }
    }

    return (s);
}

const char * S9xGetFilename(const char *ex, enum s9x_getdirtype dirtype) {
    static char s[PATH_MAX + 1];
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    _splitpath(Memory.ROMFilename, drive, dir, fname, ext);
    snprintf(s, PATH_MAX + 1, "%s%s%s%s", S9xGetDirectory(dirtype), SLASH_STR, fname, ex);

    return (s);
}

const char * S9xGetFilenameInc(const char *ex, enum s9x_getdirtype dirtype) {
    static char s[PATH_MAX + 1];
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    unsigned int i = 0;
    const char *d;
    struct stat buf;

    _splitpath(Memory.ROMFilename, drive, dir, fname, ext);
    d = S9xGetDirectory(dirtype);

    do
        snprintf(s, PATH_MAX + 1, "%s%s%s.%03d%s", d, SLASH_STR, fname, i++, ex);
    while (stat(s, &buf) == 0 && i < 1000);

    return (s);
}

const char * S9xBasename(const char *f) {
    const char *p;

    if ((p = strrchr(f, '/')) != NULL || (p = strrchr(f, '\\')) != NULL)
        return (p + 1);

    return (f);
}

const char * S9xChooseFilename(bool8 read_only) {
    char s[PATH_MAX + 1];
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    const char *filename;
    char title[64];

    _splitpath(Memory.ROMFilename, drive, dir, fname, ext);
    snprintf(s, PATH_MAX + 1, "%s.frz", fname);
    sprintf(title, "%s snapshot filename", read_only ? "Select load" : "Choose save");

    S9xSetSoundMute(TRUE);
    filename = S9xSelectFilename(s, S9xGetDirectory(SNAPSHOT_DIR), "frz", title);
    S9xSetSoundMute(FALSE);

    return (filename);
}

const char * S9xChooseMovieFilename(bool8 read_only) {
    char s[PATH_MAX + 1];
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    const char *filename;
    char title[64];

    _splitpath(Memory.ROMFilename, drive, dir, fname, ext);
    snprintf(s, PATH_MAX + 1, "%s.smv", fname);
    sprintf(title, "Choose movie %s filename", read_only ? "playback" : "record");

    S9xSetSoundMute(TRUE);
    filename = S9xSelectFilename(s, S9xGetDirectory(HOME_DIR), "smv", title);
    S9xSetSoundMute(FALSE);

    return (filename);
}

bool8 S9xOpenSnapshotFile(const char *filename, bool8 read_only, STREAM *file) {
    char s[PATH_MAX + 1];
    char drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], fname[_MAX_FNAME + 1], ext[_MAX_EXT + 1];

    _splitpath(filename, drive, dir, fname, ext);

    if (*drive || *dir == SLASH_CHAR || (strlen(dir) > 1 && *dir == '.' && *(dir + 1) == SLASH_CHAR))
    {
        strncpy(s, filename, PATH_MAX + 1);
        s[PATH_MAX] = 0;
    }
    else
        snprintf(s, PATH_MAX + 1, "%s%s%s", S9xGetDirectory(SNAPSHOT_DIR), SLASH_STR, fname);

    if (!*ext && strlen(s) <= PATH_MAX - 4)
        strcat(s, ".frz");

    if ((*file = OPEN_STREAM(s, read_only ? "rb" : "wb")))
        return (TRUE);

    return (FALSE);
}

void S9xCloseSnapshotFile(STREAM file) {
    CLOSE_STREAM(file);
}

const char * S9xStringInput(const char *message) {
    static char buffer[256];

    printf("%s: ", message);
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer) - 2, stdin))
        return (buffer);

    return (NULL);
}

int MakeS9xDirs()
{
    if (strlen(s9xBaseDir) + 1 + sizeof(dirNames[0]) > PATH_MAX + 1)
        return (-1);

    mkdir(s9xBaseDir, 0755);

    for (int i = 0; i < LAST_DIR; i++)
    {
        if (dirNames[i][0])
        {
            char s[PATH_MAX + 1];
            snprintf(s, PATH_MAX + 1, "%s%s%s", s9xBaseDir, SLASH_STR, dirNames[i]);
            mkdir(s, 0755);
        }
    }

    return (0);
}

uint64_t GetTicks() {
#ifndef CLOCK_MONOTONIC_COARSE
#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC
#endif
    struct timeval tv;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000ULL + static_cast<uint64_t>(ts.tv_nsec / 1000U);
}
