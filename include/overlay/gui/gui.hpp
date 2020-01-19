/**
 * Copyright (C) 2020 natinusala
 * Copyright (C) 2019 WerWolv
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

#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <thread>

#include "overlay/screen.hpp"
#include "overlay/elements/element.hpp"

namespace tsl {

    using namespace std::literals::chrono_literals;
    
    using Element = element::Element;
    static auto &a = tsl::Screen::a;

    class Gui {
    public:
        Gui();
        virtual ~Gui();

        static void init(Screen *screen);
        static void exit();

        virtual bool shouldClose();

        virtual Element* createUI() = 0;
        virtual void update() {}
        
        virtual void preDraw(Screen *screen) {
            screen->fillScreen(a({ 0x0, 0x0, 0x0, 0xD }));
            screen->drawRect(15, 720 - 73, FB_WIDTH - 30, 1, a(0xFFFF));
            screen->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(0xFFFF));
        } 

        virtual void postDraw(Screen *screen) {} 

        static void tick();
        static void hidUpdate(s64 keysDown, s64 keysHeld, JoystickPosition joyStickPosLeft, JoystickPosition joyStickPosRight, u32 touchX, u32 touchY);

        static void playIntroAnimation();
        static void playOutroAnimation();

        template<typename T>
        static Gui* changeTo() { 
            return Gui::s_nextGui = new T();
        }

        static void changeTo(Gui *gui) {
            Gui::s_nextGui = gui;
        }

        static Gui* getCurrentGui() { return Gui::s_currGui; }

        static void closeGui() {
            if (Gui::s_currGui == nullptr)
                return;


            delete Gui::s_currGui;
            Gui::s_currGui = nullptr;
        }

        static void requestFocus(Element *element, FocusDirection direction);
        static void removeFocus(Element *element = nullptr);

        static bool isFocused(Element *element) { return Gui::s_focusedElement == element; }

        static auto getLastFrameTime() {
            return std::chrono::duration_cast<std::chrono::duration<u64, std::milli>>(Gui::s_lastFrameDuration);
        }

        static void setOpacity(float opacity) {
            Gui::s_screen->setOpacity(opacity);
        }

    private:
        static inline Screen *s_screen = nullptr;
        static inline Gui *s_currGui = nullptr;
        static inline Gui *s_nextGui = nullptr;
        static inline Element *s_topElement = nullptr;
        static inline Element *s_focusedElement = nullptr;

        static inline u8 s_animationCounter = 0;
        static inline bool s_introAnimationPlaying = true;
        static inline bool s_outroAnimationPlaying = true;

        static inline std::chrono::duration<s64, std::nano> s_lastFrameDuration = 0s;

    };

}