/**
 * Copyright (C) 2020 WerWolv
 * Copyright (C) 2020 natinusala
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

#include "overlay/overlay.hpp"

namespace tsl {

    Gui::Gui() {
        Gui::s_screen->clear();
        
        Gui::reset();
    }

    Gui::~Gui() {

    }

    void Gui::init(Screen *screen) {
        Gui::s_screen = screen;
    }

    void Gui::exit() {
        if (Gui::s_currGui == nullptr)
            return;

        Gui::s_screen->clear();
        Gui::s_screen->flush();

        Gui::s_currGui = nullptr;
    }

    bool Gui::shouldHide() {
        return Gui::s_shouldHide;
    }

    bool Gui::shouldClose() {
        return Gui::s_shouldClose;
    }

    void Gui::guiChange() {
        if (Gui::s_nextGui != nullptr) {
            Gui::removeFocus();

            if (Gui::s_currGui != nullptr)
                Gui::s_previousGuis.push_back(Gui::s_currGui);

            if (Gui::s_topElement != nullptr)
                delete Gui::s_topElement;

            Gui::s_currGui = Gui::s_nextGui;
            Gui::s_nextGui = nullptr;

            Gui::s_topElement = Gui::s_currGui->createUI();
            Gui::s_topElement->layout();
            Gui::requestFocus(Gui::s_topElement, FocusDirection::NONE);
        }
    }

    void Gui::draw() {
        if (Gui::s_currGui == nullptr)
            return;

        if (Gui::s_topElement != nullptr)
            Gui::s_topElement->frame(Gui::s_screen);

        Gui::s_screen->flush();

        if (Gui::s_introAnimationPlaying) {
            if (Gui::s_animationCounter >= 6) {
                Gui::s_introAnimationPlaying = false;
                Gui::s_animationCounter = 0;
            } else {
                Gui::s_screen->setOpacity(Gui::s_animationCounter * 0.2);
                Gui::s_animationCounter++;
            }
        } else if (Gui::s_outroAnimationPlaying) {
            if (Gui::s_animationCounter >= 6) {
                Gui::s_outroAnimationPlaying = false;
                Gui::s_animationCounter = 0;

                Gui::hideGui();
            } else {
                Gui::s_screen->setOpacity(1 - Gui::s_animationCounter * 0.2);
                Gui::s_animationCounter++;
            }
        }  
    }

    void Gui::hidUpdate(s64 keysDown, s64 keysHeld, JoystickPosition joyStickPosLeft, JoystickPosition joyStickPosRight, u32 touchX, u32 touchY) {
        Element *activeElement = Gui::s_focusedElement;

        if (touchX > FB_WIDTH)
            if (Overlay::getCurrentOverlay() != nullptr && Gui::getCurrentGui() != nullptr)
                Overlay::getCurrentOverlay()->onOverlayHide(Gui::getCurrentGui());

        if (activeElement == nullptr) {
            if (Gui::s_topElement == nullptr)
                return;
            else
                activeElement = Gui::s_topElement;
        }

        bool handled;
        Element *parentElement = activeElement;
        do {
            handled = parentElement->onClick(keysDown);
            parentElement = parentElement->getParent();
        } while (!handled && parentElement != nullptr);

        if (!handled) {
            if (keysDown & KEY_UP)
                Gui::requestFocus(activeElement->getParent(), FocusDirection::UP);
            else if (keysDown & KEY_DOWN)
                Gui::requestFocus(activeElement->getParent(), FocusDirection::DOWN);
            else if (keysDown & KEY_LEFT)
                Gui::requestFocus(activeElement->getParent(), FocusDirection::LEFT);
            else if (keysDown & KEY_RIGHT)
                Gui::requestFocus(activeElement->getParent(), FocusDirection::RIGHT);
        }
    }

    void Gui::playIntroAnimation() {
        if (Gui::s_introAnimationPlaying)
            return;

        Gui::s_introAnimationPlaying = true;
        Gui::s_screen->setOpacity(0.0F);
    }

    void Gui::playOutroAnimation() {
        if (Gui::s_outroAnimationPlaying)
            return;

        Gui::s_outroAnimationPlaying = true;
        Gui::s_screen->setOpacity(1.0F);
    }

    
    void Gui::requestFocus(Element *element, FocusDirection direction) {
        if (element == nullptr)
            return;
            
        Element *oldFocus = Gui::s_focusedElement;
        Element *newFocus = element->requestFocus(oldFocus, direction);

        if (oldFocus != newFocus && newFocus != nullptr) {
            Gui::s_focusedElement = newFocus;
        } else if (oldFocus != nullptr)
            oldFocus->shakeFocus(direction);
    }

    void Gui::removeFocus(Element *element) {
        if (element == Gui::s_focusedElement || element == nullptr)
            Gui::s_focusedElement = nullptr;
    }

    void Gui::goBack() {
        Gui::s_shouldHide = false;

        if (Gui::s_previousGuis.size() > 0) {
            Gui::s_nextGui = Gui::s_previousGuis.back();
            Gui::s_previousGuis.pop_back();

            delete Gui::s_currGui;
            Gui::s_currGui = nullptr;
        }
        else {
            if (Overlay::getCurrentOverlay() != nullptr && Gui::getCurrentGui() != nullptr)
                Overlay::getCurrentOverlay()->onOverlayExit(Gui::getCurrentGui());
        }
    }

    void Gui::reset() {
        Gui::s_introAnimationPlaying = false;
        Gui::s_outroAnimationPlaying = false;
        Gui::s_animationCounter = 0;
        Gui::s_shouldHide = false;
        Gui::s_shouldClose = false;
    }

}