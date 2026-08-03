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

// prototype/interface header file
#include "channel_mode.hpp"

// local headers
#include "common/endian.hpp"
#include "common/snap_exception.hpp"
#include "common/utils/string_utils.hpp"

// standard headers
#include <cstdint>
#include <cstring>


namespace player
{

const char* channelModeToString(ChannelMode mode)
{
    switch (mode)
    {
        case ChannelMode::stereo:
            return "stereo";
        case ChannelMode::left:
            return "left";
        case ChannelMode::right:
            return "right";
        case ChannelMode::mono:
            return "mono";
    }
    return "stereo";
}


ChannelMode parseChannelMode(const std::string& parameter)
{
    for (const auto& option : utils::string::split(parameter, ','))
    {
        if (option.find('=') == std::string::npos && utils::string::trim_copy(option) == "channel")
            throw SnapException("Invalid channel option: channel; expected channel=<stereo|left|right|mono>");
    }

    auto params = utils::string::split_pairs(parameter, ',', '=');
    auto channel = params.find("channel");
    if (channel == params.end())
        return ChannelMode::stereo;

    if (channel->second == "stereo")
        return ChannelMode::stereo;
    if (channel->second == "left")
        return ChannelMode::left;
    if (channel->second == "right")
        return ChannelMode::right;
    if (channel->second == "mono")
        return ChannelMode::mono;

    throw SnapException("Invalid channel option: " + channel->second + "; expected stereo, left, right or mono");
}


namespace
{

template <typename T>
T readSample(const char* address)
{
    T sample;
    std::memcpy(&sample, address, sizeof(sample));
    return endian::swap(sample);
}


template <typename T>
void writeSample(char* address, T sample)
{
    sample = endian::swap(sample);
    std::memcpy(address, &sample, sizeof(sample));
}


template <typename T, typename Accumulator>
void applyTwoChannelMode(char* buffer, size_t frames, size_t frameSize, ChannelMode mode)
{
    for (size_t frame = 0; frame < frames; ++frame)
    {
        char* frameAddress = buffer + frame * frameSize;
        const T left = readSample<T>(frameAddress);
        const T right = readSample<T>(frameAddress + sizeof(T));

        T outputLeft = left;
        T outputRight = right;
        switch (mode)
        {
            case ChannelMode::left:
                outputRight = left;
                break;
            case ChannelMode::right:
                outputLeft = right;
                break;
            case ChannelMode::mono:
            {
                const Accumulator mixed = (static_cast<Accumulator>(left) + static_cast<Accumulator>(right)) / 2;
                outputLeft = static_cast<T>(mixed);
                outputRight = outputLeft;
                break;
            }
            case ChannelMode::stereo:
                break;
        }

        writeSample<T>(frameAddress, outputLeft);
        writeSample<T>(frameAddress + sizeof(T), outputRight);
    }
}

} // namespace


void applyChannelMode(char* buffer, size_t frames, const SampleFormat& format, ChannelMode mode)
{
    if (buffer == nullptr || frames == 0 || mode == ChannelMode::stereo || format.channels() != 2)
        return;

    // Snapcast stores 24-bit samples in a signed 32-bit container. Oboe's
    // three-byte packing happens later, after this function has run.
    switch (format.bits())
    {
        case 8:
            applyTwoChannelMode<int8_t, int32_t>(buffer, frames, format.frameSize(), mode);
            break;
        case 16:
            applyTwoChannelMode<int16_t, int32_t>(buffer, frames, format.frameSize(), mode);
            break;
        case 24:
            applyTwoChannelMode<int32_t, int64_t>(buffer, frames, format.frameSize(), mode);
            break;
        case 32:
            applyTwoChannelMode<int32_t, int64_t>(buffer, frames, format.frameSize(), mode);
            break;
        default:
            // Keep unknown sample formats untouched rather than risking a
            // wrong sample stride in a real-time callback.
            return;
    }
}

} // namespace player
