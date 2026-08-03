/***
    This file is part of snapcast
    Copyright (C) 2024  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#pragma once

// local headers
#include "common/sample_format.hpp"

// standard headers
#include <cstddef>
#include <string>


namespace player
{

enum class ChannelMode
{
    stereo,
    left,
    right,
    mono
};

/// Parse the channel=<mode> player option. Missing channel options mean stereo.
ChannelMode parseChannelMode(const std::string& parameter);

/// Return the canonical value used in --player options and diagnostics.
const char* channelModeToString(ChannelMode mode);

/// Map an interleaved PCM buffer in place without changing its frame count.
void applyChannelMode(char* buffer, size_t frames, const SampleFormat& format, ChannelMode mode);

} // namespace player
