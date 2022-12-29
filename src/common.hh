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

#pragma once

#include <libusb.h>
#include <string>

/// @brief Gets string using libusb_get_string_descriptor_ascii, modifies string reference passed
int get_string(libusb_device_handle* handle, uint8_t index, std::string& string, size_t max_length=100);

/// @brief converts the USB hex value from libusb to human readable one '0x210' -> '2.1'
std::string get_usb_string(int usb_code);

/// @brief creates a string of spaces as indent
constexpr std::string indent(int n) {
    return std::string(n, ' ');
}
