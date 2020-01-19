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

#include "overlay/elements/toggle_list_item.hpp"

namespace tsl::ovl::element {

    ToggleListItem::ToggleListItem(std::string text, bool defaultState) : ListItem(text), m_state(defaultState) {

    }

    ToggleListItem::~ToggleListItem() {

    }


    Element* ToggleListItem::requestFocus(Element *oldFocus, FocusDirection direction) {
        return ListItem::requestFocus(oldFocus, direction);
    }

    void ToggleListItem::draw(ovl::Screen *screen, u16 x1, u16 y1) {
        const auto [x, y] = this->getPosition();
        const auto [w, h] = this->getSize();

        ListItem::draw(screen, x1, y1);
        screen->drawString(this->m_state ? "On" : "Off", false, w - 8, y + 42, 19, this->m_state ? a({ 0x5, 0xC, 0xA, 0xF }) : a({ 0x6, 0x6, 0x6, 0xF }));
    }

    void ToggleListItem::layout() {
        ListItem::layout();
    }

    bool ToggleListItem::onClick(s64 key) {
        if (key & KEY_A) {
            this->m_state = !this->m_state;

            if (this->m_stateChangeListener != nullptr)
                this->m_stateChangeListener(this->m_state);
                
            return true;
        }

        return false;
    }

}