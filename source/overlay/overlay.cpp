/**
 * Copyright (C) 2020 WerWolv
 * 
 * This file is part of libtesla.
 * 
 * libtesla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libtesla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with libtesla.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <switch.h>

#include <mutex>
#include <atomic>
#include <cstring>

#include "overlay/screen.hpp"
#include "overlay/gui/gui.hpp"

#include "overlay/overlay.hpp"

extern tsl::Overlay* overlayLoad();

extern "C" {

    u32 __nx_applet_type = AppletType_OverlayApplet;
    u32 __nx_nv_transfermem_size = 0x15000;

    static tsl::Screen *screen = nullptr;

    void __appInit(void) {
        smInitialize();

        fsInitialize();
        hidInitialize();
        plInitialize();
        fsdevMountSdmc();
        pmdmntInitialize();
        hidsysInitialize();

        tsl::Screen::initialize();

        smExit();

        screen = new tsl::Screen();
        tsl::Gui::init(screen);
    }

    void __appExit(void) {
        fsExit();
        hidExit();
        plExit();
        pmdmntExit();
        hidsysExit();

        delete screen;

        tsl::Screen::exit();
    } 

}

struct SharedThreadData {

    Event overlayComboEvent;
    std::mutex inputMutex;
    bool running;

    std::atomic<u64> keysDown, keysHeld;
    std::atomic<JoystickPosition> joyStickPosLeft;
    std::atomic<JoystickPosition> joyStickPosRight;
    std::atomic<u32> touchX, touchY;

    std::atomic<bool> overlayOpen;

};


Result hidsysEnableAppletToGetInput(bool enable, u64 aruid) {  
    const struct {
        u8 permitInput;
        u64 appletResourceUserId;
    } in = { enable != 0, aruid };

    return serviceDispatchIn(hidsysGetServiceSession(), 503, in);
}

Result focusOverlay(bool focus) {
    u64 applicationAruid = 0, appletAruid = 0;

    for (u64 programId = 0x0100000000001000ul; programId < 0x0100000000001020ul; programId++) {
        pmdmntGetProcessId(&appletAruid, programId);
        
        if (appletAruid != 0)
            hidsysEnableAppletToGetInput(!focus, appletAruid);
    }

    pmdmntGetApplicationProcessId(&applicationAruid);
    hidsysEnableAppletToGetInput(!focus, applicationAruid);

    hidsysEnableAppletToGetInput(true, 0);

    return 0;
}


void hidLoop(void *args) {
    SharedThreadData *shData = static_cast<SharedThreadData*>(args);

    JoystickPosition tmpJoyStickPosition[2] = { 0 };
    touchPosition tmpTouchPosition = { 0 };

    // Drop all inputs from the previous overlay
    hidScanInput();

    while (shData->running) {
        // Scan for button presses
        hidScanInput();

        // Read in touch positions
        if (hidTouchCount() > 0)
            hidTouchRead(&tmpTouchPosition, 0);
        else 
            tmpTouchPosition = { 0 };
        // Read in joystick values
        hidJoystickRead(&tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_LEFT], CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_LEFT);
        hidJoystickRead(&tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_RIGHT], CONTROLLER_HANDHELD, HidControllerJoystick::JOYSTICK_RIGHT);

        {
            std::scoped_lock lock(shData->inputMutex);

            shData->keysDown         = hidKeysDown(CONTROLLER_HANDHELD);
            shData->keysHeld         = hidKeysHeld(CONTROLLER_HANDHELD);

            shData->touchX           = tmpTouchPosition.px;
            shData->touchY           = tmpTouchPosition.py;

            shData->joyStickPosLeft  = tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_LEFT];
            shData->joyStickPosRight = tmpJoyStickPosition[HidControllerJoystick::JOYSTICK_RIGHT];
        }

        // Detect overlay key-combo
        if ((shData->keysHeld & (KEY_L | KEY_DDOWN)) == (KEY_L | KEY_DDOWN) && shData->keysDown & KEY_RSTICK) {
            if (shData->overlayOpen) {
                if (tsl::Overlay::getCurrentOverlay() != nullptr && tsl::Gui::getCurrentGui() != nullptr)
                    tsl::Overlay::getCurrentOverlay()->onOverlayHide(tsl::Gui::getCurrentGui());
            }
            else
                eventFire(&shData->overlayComboEvent);
        }
        
        // Sleep 20ms
        svcSleepThread(20E6); 
    }
}


int main(int argc, char** argv) {
    Thread hidThread;
    static SharedThreadData shData = { 0 };

    bool skipCombo = false;
    u32 selectedMainMenuIndex = 0;

    for (u8 i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--initial-load")
            skipCombo = true;
    } 

    shData.running = true;
    shData.overlayOpen = false;

    threadCreate(&hidThread, hidLoop, &shData, nullptr, 0x1000, 0x2C, 3);
    threadStart(&hidThread);

    eventCreate(&shData.overlayComboEvent, false);

    tsl::Overlay::setCurrentOverlay(overlayLoad());

    tsl::Gui *gui = tsl::Overlay::getCurrentOverlay()->onSetup();
    tsl::Gui::changeTo(gui);
    while (shData.running) {

        if (!skipCombo) {
            eventWait(&shData.overlayComboEvent, U64_MAX);
            eventClear(&shData.overlayComboEvent);
            tsl::Overlay::getCurrentOverlay()->onOverlayShow(gui);
        } else {
            tsl::Overlay::getCurrentOverlay()->onOverlayLoad(gui);
            skipCombo = false;
        }

        focusOverlay(true);
        shData.overlayOpen = true;

        while (true) {
            gui->preDraw(screen);

            tsl::Gui::hidUpdate(shData.keysDown, shData.keysHeld, shData.joyStickPosLeft, shData.joyStickPosRight, shData.touchX, shData.touchY);
            tsl::Gui::tick();
            gui = tsl::Gui::getCurrentGui();

            if (gui == nullptr)
                break;

            gui->postDraw(screen);

            if (gui->shouldHide())
                break;

            if (gui->shouldClose()) {
                shData.running = false;
                break;
            }

            shData.inputMutex.unlock();
            tsl::Screen::waitForVsync();
            shData.inputMutex.lock();
        }

        shData.overlayOpen = false;
        focusOverlay(false);
        eventClear(&shData.overlayComboEvent);
        shData.inputMutex.unlock();
    }
    
    tsl::Gui::exit();
    delete gui;

    tsl::Overlay::getCurrentOverlay()->onDestroy();
    threadWaitForExit(&hidThread);
    threadClose(&hidThread);

    eventClose(&shData.overlayComboEvent);

    envSetNextLoad(tsl::Overlay::getNextLoadPath().c_str(), "--initial-load");

    delete tsl::Overlay::getCurrentOverlay();
}