#include "menu.h"

#include "video.h"

#include "port.h"
#include "snes9x.h"

#include <SDL.h>

#include <libintl.h>

#define _(s) gettext(s)

extern bool s9xTerm;

enum MenuAction :uint32_t {
    MA_NONE     = 0,
    MA_OK       = 1 << 0,
    MA_CANCEL   = 1 << 1,
    MA_LEAVE    = 1 << 2,
    MA_UP       = 1 << 3,
    MA_DOWN     = 1 << 4,
    MA_LEFT     = 1 << 5,
    MA_RIGHT    = 1 << 6,
    MA_FIRST    = 1 << 7,
    MA_LAST     = 1 << 8,
    MA_PAGEUP   = 1 << 9,
    MA_PAGEDOWN = 1 << 10,
};

static void (*preDrawFunc)() = NULL;

static MenuAction mapAction(SDLKey);
static void drawMenu(const MenuItem items[], int x, int valx, int y, int index, int topIndex, int count);

void MenuSetPreDrawFunc(void (*func)()) {
    preDrawFunc = func;
}

#define FIX_INDEX if (topIndex + pageCount <= index) topIndex = index - pageCount + 1; \
    else if (topIndex > index) topIndex = index
#define CALL_TRIGGER changed = true; if (cur->triggerFunc && (mr = (*cur->triggerFunc)(cur)) != 0) { \
    if (mr < 0) return mr; if (mr == MR_OK) return index; }
int MenuRun(const MenuItem items[], int x, int valx, int y, int pageCount, int index) {
    int count = 0, topIndex = 0;
    const MenuItem *cur = items;
    uint32_t actions = 0;
    MenuResult mr;
    bool changed = true;
    while (cur->type != MIT_END) {
        ++cur; ++count;
    }
    if (pageCount == 0) pageCount = count;
    if (index >= count) index = count - 1;
    FIX_INDEX;
    cur = items + index;
    SDL_Event event;
    for (;;) {
        if (changed) {
            changed = false;
            drawMenu(items, x, valx, y, index, topIndex, pageCount);
            usleep(150000);
        } else {
            usleep(50000);
        }
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    s9xTerm = true;
                    return MR_LEAVE;
                case SDL_KEYDOWN:
                    actions |= mapAction(event.key.keysym.sym);
                    break;
                case SDL_KEYUP:
                    actions &= ~mapAction(event.key.keysym.sym);
                    break;
                default: break;
            }
        }
        if (!actions) continue;
        if (actions & MA_OK) {
            actions &= ~MA_OK;
            switch (cur->type) {
                case MIT_BOOL:
                    if (!cur->value) break;
                    *(bool *)cur->value = !*(bool *)cur->value;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_BOOL8:
                    if (!cur->value) break;
                    *(bool8 *)cur->value = !*(bool8 *)cur->value;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_CLICK:
                    CALL_TRIGGER;
                    break;
                default: break;
            }
            continue;
        }
        if (actions & MA_CANCEL) {
            actions &= ~MA_CANCEL;
            return MR_CANCEL;
        }
        if (actions & MA_LEAVE) {
            actions &= ~MA_LEAVE;
            return MR_LEAVE;
        }
        if (actions & MA_UP) {
            if (--index < 0) index = count - 1;
            cur = items + index;
            changed = true;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_DOWN) {
            if (++index >= count) index = 0;
            cur = items + index;
            changed = true;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_PAGEUP) {
            if (index == 0) continue;
            index -= pageCount;
            if (index < 0) index = 0;
            cur = items + index;
            changed = true;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_PAGEDOWN) {
            if (index == count - 1) continue;
            index += pageCount;
            if (index >= count) index = count - 1;
            cur = items + index;
            changed = true;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_FIRST) {
            if (index == 0) continue;
            index = 0;
            cur = items + index;
            changed = true;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_LAST) {
            if (index == count - 1) continue;
            index = count - 1;
            cur = items + index;
            changed = true;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_LEFT) {
            actions &= ~MA_LEFT;
            int32_t val;
            switch (cur->type) {
                case MIT_BOOL:
                    if (!cur->value) break;
                    val = *(bool*)cur->value;
                    *(bool*)cur->value = !val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_BOOL8:
                    if (!cur->value) break;
                    val = *(bool8*)cur->value;
                    *(bool8*)cur->value = !val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_INT8:
                    if (!cur->value) break;
                    val = *(int8_t *)cur->value;
                    if (--val < cur->valMin) val = cur->valMax;
                    *(int8_t *)cur->value = val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_INT16:
                    if (!cur->value) break;
                    val = *(int16_t *)cur->value;
                    if (--val < cur->valMin) val = cur->valMax;
                    *(int16_t *)cur->value = val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_INT32:
                    if (!cur->value) break;
                    val = *(int32_t *)cur->value;
                    if (--val < cur->valMin) val = cur->valMax;
                    *(int32_t *)cur->value = val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                default: break;
            }
            continue;
        }
        if (actions & MA_RIGHT) {
            actions &= ~MA_RIGHT;
            int32_t val;
            switch (cur->type) {
                case MIT_BOOL:
                    if (!cur->value) break;
                    val = *(bool*)cur->value;
                    *(bool*)cur->value = !val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_BOOL8:
                    if (!cur->value) break;
                    val = *(bool8*)cur->value;
                    *(bool8*)cur->value = !val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_INT8:
                    if (!cur->value) break;
                    val = *(int8_t *)cur->value;
                    if (++val > cur->valMax) val = cur->valMin;
                    *(int8_t *)cur->value = val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_INT16:
                    if (!cur->value) break;
                    val = *(int16_t *)cur->value;
                    if (++val > cur->valMax) val = cur->valMin;
                    *(int16_t *)cur->value = val;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                case MIT_INT32:
                    if (!cur->value) break;
                    val = *(int32_t *)cur->value;
                    if (++val > cur->valMax) val = cur->valMin;
                    changed = true;
                    CALL_TRIGGER;
                    break;
                default: break;
            }
            continue;
        }
    }
}

static MenuAction mapAction(SDLKey key) {
    switch (key) {
#ifdef GCW_ZERO
    case SDLK_UP: return MA_UP;
    case SDLK_DOWN: return MA_DOWN;
    case SDLK_LEFT: return MA_LEFT;
    case SDLK_RIGHT: return MA_RIGHT;
    case SDLK_ESCAPE: case SDLK_HOME: return MA_LEAVE;
    case SDLK_TAB: return MA_PAGEUP;
    case SDLK_BACKSPACE: return MA_PAGEDOWN;
    case SDLK_PAGEUP: return MA_FIRST;
    case SDLK_PAGEDOWN: return MA_LAST;
    case SDLK_LCTRL: SDLK_RETURN: return MA_OK;
    case SDLK_LALT: return MA_CANCEL;
#else
    case SDLK_w: return MA_UP;
    case SDLK_s: return MA_DOWN;
    case SDLK_a: return MA_LEFT;
    case SDLK_d: return MA_RIGHT;
    case SDLK_ESCAPE: return MA_LEAVE;
    case SDLK_PAGEUP: return MA_PAGEUP;
    case SDLK_PAGEDOWN: return MA_PAGEDOWN;
    case SDLK_HOME: return MA_FIRST;
    case SDLK_END: return MA_LAST;
    case SDLK_l: return MA_OK;
    case SDLK_k: return MA_CANCEL;
#endif
    default: return MA_NONE;
    }
}

static void drawMenu(const MenuItem items[], int x, int valx, int y, int index, int topIndex, int count) {
    const MenuItem *cur = items + topIndex;
    int curIndex = topIndex;
    int32_t val = 0;
    MenuItemValue miv = {};
    VideoClear();
    if (preDrawFunc) (*preDrawFunc)();
    while (cur->type != MIT_END && count > 0) {
        VideoOutputString(x, y, cur->text, false, true);
        bool hasvalue = false;
        switch (cur->type) {
            case MIT_BOOL8:
                val = *(bool8*)cur->value ? 1 : 0;
                hasvalue = true;
                break;
            case MIT_BOOL:
                val = *(bool*)cur->value ? 1 : 0;
                hasvalue = true;
                break;
            case MIT_INT8:
                val = *(int8_t*)cur->value ? 1 : 0;
                hasvalue = true;
                break;
            case MIT_INT16:
                val = *(int16_t*)cur->value ? 1 : 0;
                hasvalue = true;
                break;
            case MIT_INT32:
                val = *(int32_t*)cur->value ? 1 : 0;
                hasvalue = true;
                break;
            default: break;
        }
        if (hasvalue) {
            if (cur->valueFunc && (miv = (*cur->valueFunc)(val), miv.text != NULL)) {
                VideoOutputString(valx, y, miv.text, false, true);
            }
            VideoOutputString(valx, y, val ? _("on") : _("off"), false, true);
        }
        if (index == curIndex) {
            VideoOutputString(x - 10, y, ">", false, true);
        }
        if (cur->drawFunc) {
            (*cur->drawFunc)(cur, y, index == curIndex);
        }
        y += 12;
        --count;
        ++cur;
        ++curIndex;
    }
    VideoFlip();
}
