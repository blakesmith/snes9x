#include "input.h"

#include "port.h"

#include "snes9x.h"
#include "memmap.h"
#include "controls.h"

#include <SDL.h>

void S9xInitInputDevices() {
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
}

void S9xSetupDefaultKeymap() {
#define	ASSIGN_BUTTON(n, s)	S9xMapButton (n, cmd = S9xGetCommandT(s), false)
    s9xcommand_t	cmd;
#ifdef GCW_ZERO
    ASSIGN_BUTTON(SDLK_SPACE, "Joypad1 X");
    ASSIGN_BUTTON(SDLK_LCTRL, "Joypad1 A");
    ASSIGN_BUTTON(SDLK_LALT, "Joypad1 B");
    ASSIGN_BUTTON(SDLK_LSHIFT, "Joypad1 Y");
    ASSIGN_BUTTON(SDLK_TAB, "Joypad1 L");
    ASSIGN_BUTTON(SDLK_BACKSPACE, "Joypad1 R");
    ASSIGN_BUTTON(SDLK_ESCAPE, "Joypad1 Select");
    ASSIGN_BUTTON(SDLK_RETURN, "Joypad1 Start");
    ASSIGN_BUTTON(SDLK_UP, "Joypad1 Up");
    ASSIGN_BUTTON(SDLK_DOWN, "Joypad1 Down");
    ASSIGN_BUTTON(SDLK_LEFT, "Joypad1 Left");
    ASSIGN_BUTTON(SDLK_RIGHT, "Joypad1 Right");
    ASSIGN_BUTTON(SDLK_PAGEDOWN, "EmuTurbo");
    ASSIGN_BUTTON(SDLK_PAGEUP, "Reset");
    ASSIGN_BUTTON(SDLK_KP_DIVIDE, "QuickSave010");
    ASSIGN_BUTTON(SDLK_KP_PERIOD, "QuickLoad010");
#else
    ASSIGN_BUTTON(SDLK_i, "Joypad1 X");
    ASSIGN_BUTTON(SDLK_l, "Joypad1 A");
    ASSIGN_BUTTON(SDLK_k, "Joypad1 B");
    ASSIGN_BUTTON(SDLK_j, "Joypad1 Y");
    ASSIGN_BUTTON(SDLK_q, "Joypad1 L");
    ASSIGN_BUTTON(SDLK_e, "Joypad1 R");
    ASSIGN_BUTTON(SDLK_c, "Joypad1 Select");
    ASSIGN_BUTTON(SDLK_v, "Joypad1 Start");
    ASSIGN_BUTTON(SDLK_w, "Joypad1 Up");
    ASSIGN_BUTTON(SDLK_s, "Joypad1 Down");
    ASSIGN_BUTTON(SDLK_a, "Joypad1 Left");
    ASSIGN_BUTTON(SDLK_d, "Joypad1 Right");
    ASSIGN_BUTTON(SDLK_SPACE, "EmuTurbo");
    ASSIGN_BUTTON(SDLK_RETURN, "Pause");
    ASSIGN_BUTTON(SDLK_LEFTBRACKET, "QuickSave000");
    ASSIGN_BUTTON(SDLK_RIGHTBRACKET, "QuickLoad000");
#endif
}

bool S9xPollButton (uint32 id, bool *pressed) {
    return false;
}

bool S9xPollPointer (uint32 id, int16 *x, int16 *y) {
    return false;
}

bool S9xPollAxis (uint32 id, int16 *value) {
    return false;
}

void S9xHandlePortCommand (s9xcommand_t cmd, int16 data1, int16 data2) {
}

void S9xOnSNESPadRead() {
}

void NSRTControllerSetup ()
{
    if (!strncmp((const char *) Memory.NSRTHeader + 24, "NSRT", 4))
    {
        // First plug in both, they'll change later as needed
        S9xSetController(0, CTL_JOYPAD, 0, 0, 0, 0);
        S9xSetController(1, CTL_JOYPAD, 1, 0, 0, 0);

        switch (Memory.NSRTHeader[29])
        {
            case 0x00:	// Everything goes
                break;

            case 0x10:	// Mouse in Port 0
                S9xSetController(0, CTL_MOUSE,      0, 0, 0, 0);
                break;

            case 0x01:	// Mouse in Port 1
                S9xSetController(1, CTL_MOUSE,      1, 0, 0, 0);
                break;

            case 0x03:	// Super Scope in Port 1
                S9xSetController(1, CTL_SUPERSCOPE, 0, 0, 0, 0);
                break;

            case 0x06:	// Multitap in Port 1
                S9xSetController(1, CTL_MP5,        1, 2, 3, 4);
                break;

            case 0x66:	// Multitap in Ports 0 and 1
                S9xSetController(0, CTL_MP5,        0, 1, 2, 3);
                S9xSetController(1, CTL_MP5,        4, 5, 6, 7);
                break;

            case 0x08:	// Multitap in Port 1, Mouse in new Port 1
                S9xSetController(1, CTL_MOUSE,      1, 0, 0, 0);
                // There should be a toggle here for putting in Multitap instead
                break;

            case 0x04:	// Pad or Super Scope in Port 1
                S9xSetController(1, CTL_SUPERSCOPE, 0, 0, 0, 0);
                // There should be a toggle here for putting in a pad instead
                break;

            case 0x05:	// Justifier - Must ask user...
                S9xSetController(1, CTL_JUSTIFIER,  1, 0, 0, 0);
                // There should be a toggle here for how many justifiers
                break;

            case 0x20:	// Pad or Mouse in Port 0
                S9xSetController(0, CTL_MOUSE,      0, 0, 0, 0);
                // There should be a toggle here for putting in a pad instead
                break;

            case 0x22:	// Pad or Mouse in Port 0 & 1
                S9xSetController(0, CTL_MOUSE,      0, 0, 0, 0);
                S9xSetController(1, CTL_MOUSE,      1, 0, 0, 0);
                // There should be a toggles here for putting in pads instead
                break;

            case 0x24:	// Pad or Mouse in Port 0, Pad or Super Scope in Port 1
                // There should be a toggles here for what to put in, I'm leaving it at gamepad for now
                break;

            case 0x27:	// Pad or Mouse in Port 0, Pad or Mouse or Super Scope in Port 1
                // There should be a toggles here for what to put in, I'm leaving it at gamepad for now
                break;

                // Not Supported yet
            case 0x99:	// Lasabirdie
                break;

            case 0x0A:	// Barcode Battler
                break;
        }
    }
}
