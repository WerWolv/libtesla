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

#include "overlay/elements/frame.hpp"

#include "overlay/gui/gui.hpp"
#include "overlay/overlay.hpp"

namespace tsl::element {

    Frame::Frame() {

    }

    Frame::~Frame() {
        for (auto &child : this->m_children)
            delete child;

        this->m_children.clear();
    }


    Element* Frame::requestFocus(Element *oldFocus, FocusDirection direction) {
        for (auto &child : this->m_children)
            if (auto element = child->requestFocus(oldFocus, direction); element != nullptr)
                return element;
        
        return nullptr;
    }

    void Frame::draw(Screen *screen, u16 x1, u16 y1) {
        for (auto &child : this->m_children)
            child->frame(screen);
    }

    void Frame::layout() {
        this->setSize(FB_WIDTH, FB_HEIGHT);

        for (auto &child : this->m_children)
            child->layout();
    }

    void Frame::applyOpacity(float opacity) {
        for (auto &item : this->m_children)
            item->setOpacity(opacity);
    }

    bool Frame::onClick(s64 key) {
        if (key == KEY_B) {     
            Gui::goBack();
            return true;
        }

        return false;
    }


    void Frame::addElement(Element *element) {
        if (element == nullptr)
            return;

        element->setParent(this);
        this->m_children.push_back(element);
    }

}