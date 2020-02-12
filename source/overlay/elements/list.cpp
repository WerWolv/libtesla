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

#include "overlay/elements/list.hpp"
#include "overlay/gui/gui.hpp"

#include <algorithm>

namespace tsl::element {

    List::List() : List(40, 175, 6) {}

    List::List(u16 x, u16 y, u16 numShown)
    : m_numShown(numShown)
    {
        setPosition(x, y);
    }

    List::~List() {
        for (auto &item : this->m_items)
            delete item;

        this->m_items.clear();
    }


    Element* List::requestFocus(Element *oldFocus, FocusDirection direction) {
        if (this->m_items.size() == 0)
            return nullptr;

        if (oldFocus == nullptr)
            return this->m_items[0];

        auto it = std::find(this->m_items.begin(), this->m_items.end(), oldFocus);

        if (it == this->m_items.end() || direction == FocusDirection::NONE)
            return this->m_items[0];

        if (direction == FocusDirection::UP) {
            if (it == this->m_items.begin())
                return this->m_items[0];
            else {
                if (oldFocus == *(this->m_items.begin() + this->m_listOffset + 1))
                    if (this->m_listOffset > 0) {
                        this->m_listOffset--;
                        this->layout();
                    }

                return *(it - 1);
            }
        } else if (direction == FocusDirection::DOWN) {
            if (it == (this->m_items.end() - 1)) {
                if (this->m_items.size() > 0)
                    return this->m_items[this->m_items.size() - 1];
                else return nullptr;
            }
            else {
                // there are more items hidden and old focus was on second last item
                if (this->m_items.size() > size_t(this->m_listOffset + this->m_numShown) && oldFocus == *(this->m_items.begin() + this->m_listOffset + this->m_numShown - 2)) {
                    this->m_listOffset++;
                    this->layout();
                }

                return *(it + 1);
            }
        }
        
        return *it;
    }

    void List::draw(Screen *screen, u16 x, u16 y) {
        u16 i = 0;
        for (auto &item : this->m_items) {
            i++;
            if (i < this->m_listOffset + 1 || i > (this->m_listOffset + this->m_numShown))
                continue;

            item->frame(screen);
        }
    }

    void List::layout() {
        u16 y = this->getPosition().second;
        u16 i = 0;
        for (auto &item : this->m_items) {
            i++;
            if (i < this->m_listOffset + 1 || i > (this->m_listOffset + this->m_numShown))
                continue;

            item->layout();

            auto [w, h] = item->getSize();
            item->setPosition(40, y);

            y += h - 1;
        }
    }

    void List::applyOpacity(float opacity) {
        for (auto &item : this->m_items)
            item->setOpacity(opacity);
    }


    void List::addItem(ListItem *listItem) {
        if (listItem == nullptr)
            return;

        listItem->setParent(this);
        this->m_items.push_back(listItem);
    }

    void List::clear() {
        tsl::Gui::removeFocus();

        for (auto &item : this->m_items)
            delete item;

        this->m_items.clear();

        tsl::Gui::requestFocus(this, FocusDirection::NONE);
    }

}
