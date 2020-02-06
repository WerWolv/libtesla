/**
 * Copyright (C) 2020 natinusala
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

#pragma once

#include <memory>
#include <vector>
#include <thread>

#include "overlay/screen.hpp"
#include "overlay/elements/element.hpp"

namespace tsl {

    using Element = element::Element;
    static auto &a = tsl::Screen::a;

    class Gui {
    public:
        Gui();
        virtual ~Gui();

        void setTitle(std::string title) { this->m_title = title; }
        void setSubtitle(std::string subtitle) { this->m_subtitle = subtitle; }

        static void init(Screen *screen);
        static void exit();

        virtual bool shouldHide();
        virtual bool shouldClose();

        virtual Element* createUI() = 0;
        virtual void update() {}
        virtual void handleInputs(s64 keysDown, s64 keysHeld, JoystickPosition joyStickPosLeft, JoystickPosition joyStickPosRight, u32 touchX, u32 touchY) {}
        
        virtual void preDraw(Screen *screen) {
            screen->fillScreen(a({ 0x0, 0x0, 0x0, 0xD }));

            screen->drawString(this->m_title.c_str(), false, 20, 50, 30, tsl::a(0xFFFF));
            screen->drawString(this->m_subtitle.c_str(), false, 20, 70, 15, tsl::a(0xFFFF));

            screen->drawRect(15, 720 - 73, FB_WIDTH - 30, 1, a(0xFFFF));
            screen->drawString("\uE0E1  Back     \uE0E0  OK", false, 30, 693, 23, a(0xFFFF));
        } 

        virtual void postDraw(Screen *screen) {} 

        static void hidUpdate(s64 keysDown, s64 keysHeld, JoystickPosition joyStickPosLeft, JoystickPosition joyStickPosRight, u32 touchX, u32 touchY);
        static void guiChange();
        static void draw();

        static void playIntroAnimation();
        static void playOutroAnimation();

        template<typename T>
        static Gui* changeTo() { 
            return Gui::changeTo(new T());
        }

        static Gui* changeTo(Gui *gui) {
            Gui::s_shouldHide = false;
            Gui::s_nextGui = gui;

            return gui;
        }

        static void goBack();

        static Gui* getCurrentGui() { return Gui::s_currGui; }
        static Element* getTopElement() { return Gui::s_topElement; }

        static void hideGui() {
            Gui::s_shouldHide = true;
        }

        static void closeGui() {
            Gui::s_shouldClose = true;
        }

        static void requestFocus(Element *element, FocusDirection direction);
        static void removeFocus(Element *element = nullptr);

        static bool isFocused(Element *element) { return Gui::s_focusedElement == element; }

        static void setOpacity(float opacity) {
            Gui::s_screen->setOpacity(opacity);
        }

        static void reset();

    private:
        std::string m_title;
        std::string m_subtitle;

        static inline Screen *s_screen = nullptr;
        static inline Gui *s_currGui = nullptr;
        static inline Gui *s_nextGui = nullptr;
        static inline Element *s_topElement = nullptr;
        static inline Element *s_focusedElement = nullptr;

        static inline u8 s_animationCounter = 0;
        static inline bool s_introAnimationPlaying = true;
        static inline bool s_outroAnimationPlaying = true;

        static inline bool s_shouldHide = false;
        static inline bool s_shouldClose  = false;

        static inline std::vector<Gui*> s_previousGuis;
    };

}