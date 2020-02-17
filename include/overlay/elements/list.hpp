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

#pragma once

#include "overlay/elements/element.hpp"
#include "overlay/elements/list_item.hpp"

#include <vector>

namespace tsl::element {

    class List : public Element {
    public:
        List();
        List(u16 x, u16 y, u16 numShown);
        ~List();

        Element* requestFocus(Element *oldFocus, FocusDirection direction) override;

        void draw(Screen *screen, u16 x, u16 y) override;
        void layout() override;
        void applyOpacity(float opacity) override;

        void addItem(ListItem *listItem);
        void clear();

    private:
        std::vector<ListItem*> m_items;
        u16 m_numShown;
        u16 m_listOffset = 0;
    };

}