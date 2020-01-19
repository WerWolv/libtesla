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

#include "overlay/screen.hpp"
#include "overlay/gui/gui.hpp"

#include "overlay/overlay.hpp"

extern tsl::Overlay* overlayLoad();

extern "C" {

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_nv_transfermem_size = 0x15000;

    static tsl::Screen *screen;

    void __appInit(void) {
        smInitialize();

        fsInitialize();
        hidInitialize();
        plInitialize();
        fsdevMountSdmc();

        tsl::Screen::initialize();

        smExit();

        screen = new tsl::Screen();
        tsl::Gui::init(screen);
    }

    void __appExit(void) {
        fsExit();
        hidExit();
        plExit();

        tsl::Screen::exit();

        delete screen;
    } 

}

int main(int argc, char** argv) {
    tsl::Gui::playIntroAnimation();
    tsl::Overlay *overlay = overlayLoad();

    tsl::Gui::changeTo(overlay->onSetup());
    overlay->onOverlayShow();

    while (true) {
        tsl::Gui::tick();
        tsl::Gui *gui = tsl::Gui::getCurrentGui();

        gui->draw(screen);

        if (gui->shouldClose())
            break;

        tsl::Screen::waitForVsync();
    }

    tsl::Gui::exit();

    overlay->onOverlayHide();

    delete overlay;

    envSetNextLoad("sdmc:/switch/.overlays/tesla.ovl", *argv);
}