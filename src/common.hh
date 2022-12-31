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

namespace usb_facade {
    /// @brief Gets string using libusb_get_string_descriptor_ascii, modifies string reference passed
    int get_string(libusb_device_handle* handle, uint8_t index, std::string& string, size_t max_length=100);

    /// @brief converts the USB hex value from libusb to human readable one '0x210' -> '2.1'
    std::string get_usb_string(int usb_code);

    /// @public
    /// @brief Struct that is provided in callback
    struct TransferData {
        using CallbackFn = void(unsigned char*, int, TransferData*);

        /// @brief function that is ran on interrupt
        CallbackFn* callback = nullptr;

        /// @brief if set to true the interrupt will not be called
        bool pause = false;
    };

    /// @brief Struct containing pointers to all libusb descriptors for the device
    struct Device {
        TransferData data;
        libusb_device_handle* handle = nullptr;
        libusb_config_descriptor* config_descriptor = nullptr;
        libusb_transfer* transfer = nullptr;
        unsigned char* buffer = nullptr;
        int buffer_length = 0;
        int interface_index = 0;
        int error = 0;
    };

    void close_device_interrupt(Device* device);
    Device* open_device_interrupt(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData data);
    int listen_device_interrupt(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData data);
}
