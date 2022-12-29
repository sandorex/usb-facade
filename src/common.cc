// Copyright 2022 Aleksandar Radivojević
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>
#include <format>

#include "common.hh"

int get_string(libusb_device_handle* handle, uint8_t index, std::string& string, size_t max_length) {
    std::vector<unsigned char> buffer(max_length);

    auto length = libusb_get_string_descriptor_ascii(
        handle,
        index,
        buffer.data(),
        static_cast<int>(buffer.capacity())
    );

    // return the error code
    if (length < 0)
        return length;

    // ensure there is termination of the string
    buffer.back() = '\0';

    string = std::string(reinterpret_cast<char*>(buffer.data()), length);

    // success code
    return 0;
}

std::string get_usb_string(int usb_code) {
    auto string = std::format("{:x}", usb_code);

    // invalid argument
    if (string.length() <= 2)
        return "";

    // erase last zero
    string.pop_back();

    string.insert(string.cbegin() + 1, '.');

    return string;
}

