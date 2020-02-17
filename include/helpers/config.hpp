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

#pragma once

#include <functional>

#include "helpers/json.hpp"

namespace tsl {

    using nlohmann::json;

    class Config {
    public:
        static json read();
        static void update(std::function<void(json &)> modify);

    private:
        static json initialize();

    private:
        Config() {}

    private:
        static const char* FILE_PATH;
    };

}
