/**
 * Copyright (C) 2020 diwo
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

#include <fstream>

#include "helpers/config.hpp"

namespace tsl {

    const char* Config::FILE_PATH = "sdmc:/switch/.overlays/tesla.conf";

    json Config::read() {
        std::fstream fs;
        json j;

        fs.open(FILE_PATH, std::fstream::in);
        if (fs.fail()) {
            j = initialize();
        } else {
            j = json::parse(fs, nullptr, false);
            fs.close();
            if (j.is_discarded()) {
                j = initialize();
            }
        }

        return j;
    }

    void Config::update(std::function<void(json &)> modify) {
        json j = read();

        modify(j);

        std::fstream fs(FILE_PATH, std::fstream::out);
        fs << j.dump(2);
        fs.close();
    }

    json Config::initialize() {
        std::fstream fs(FILE_PATH, std::fstream::out | std::fstream::trunc);
        json j = json::object();
        fs << j;
        fs.close();
        return j;
    }

}
