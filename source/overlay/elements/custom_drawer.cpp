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

#include "overlay/elements/custom_drawer.hpp"

namespace tsl::element {

    CustomDrawer::CustomDrawer(u16 x, u16 y, u16 w, u16 h, std::function<void(u16 x, u16 y, Screen *screen)> drawer) : m_drawer(drawer) {
        this->setPosition(x, y);
        this->setSize(w, h);
    }

    CustomDrawer::~CustomDrawer() {

    }

    void CustomDrawer::draw(Screen *screen, u16 x1, u16 y1) {
        const auto [x, y] = this->getPosition();

        if (this->m_drawer != nullptr)
            this->m_drawer(x, y, screen);
    }

    void CustomDrawer::layout() {

    }

}