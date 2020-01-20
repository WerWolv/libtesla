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

#include "overlay/gui/gui.hpp"

namespace tsl {

    class Overlay {
    public:
        Overlay() { }
        virtual ~Overlay() { }

        virtual Gui* onSetup() { return nullptr; }
        virtual void onOverlayLoad(Gui *gui) { gui->setOpacity(1.0F); }
        virtual void onOverlayShow(Gui *gui) { tsl::Gui::playIntroAnimation(); }
        virtual void onOverlayHide(Gui *gui) { tsl::Gui::playOutroAnimation(); }
        virtual void onOverlayExit(Gui *gui) { gui->setOpacity(0.0F); Gui::closeGui(); }
        virtual void onDestroy() { }

        virtual void onDraw(tsl::Screen *screen) { }

        static Overlay *getCurrentOverlay() { return Overlay::s_currentOverlay; }
        static void setCurrentOverlay(Overlay *overlay) { Overlay::s_currentOverlay = overlay; }

        static void setNextLoadPath(std::string nextLoadPath) { Overlay::s_nextLoadPath = nextLoadPath; }
        static std::string getNextLoadPath() { return Overlay::s_nextLoadPath; }
    private:
        static inline Overlay *s_currentOverlay = nullptr;
        static inline std::string s_nextLoadPath = "sdmc:/switch/.overlays/ovlmenu.ovl";
    };

}