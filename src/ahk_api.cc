// Copyright 2022 Aleksandar Radivojevic
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
//
// API VERSION @usb-facade_VERSION@

#include <iostream>
#include <format>

#include "common.hh"
#include "macros.hh"
#include "usb-facade/version.hh"

using namespace usb_facade;

#define DEPRECATED __declspec(deprecated)
#define API extern "C" __declspec(dllexport)

/// @public
API int ahk_api_version() {
    return USB_FACADE_API_VERSION;
}

/// @public
API bool ahk_check_api_version(int major, int minor) {
    return USB_FACADE_VERSION_MAJOR == major && USB_FACADE_VERSION_MINOR >= minor;
}

/// @public
API int ahk_init(bool debug) {
    libusb_context* ctx;

    if (auto err = libusb_init(&ctx); err != 0) {
        std::cerr << std::format("could not initialize libusb:\n  {}\n", libusb_strerror(err));

        return err;
    }

    if (debug) {
        std::cout << "debugging mode enabled\n";
        libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    }

    return EXIT_SUCCESS;
}

API void ahk_close_device_interrupt(Device* device) {
    close_device_interrupt(device);
}

/// @public
/// listen to usb device interrupt, non blocking
/// @returns device struct pointer that needs to be freed with
///
/// @note frees automatically if any errors occur, does not free the callback
API Device* ahk_open_device_interrupt(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData::CallbackFn* callback) {
    Device* device = open_device_interrupt(vid, pid, address, max_length, TransferData { .callback = callback });

    const int error = device->error;
    if (error != 0) {
        close_device_interrupt(device);

        return nullptr;
    }

    return device;
}

/// @public
/// @brief gets error property of @ref Device
API bool ahk_is_device_open(Device* device) {
    return device != nullptr && device->error != 0;
}

/// @public @deprecated
DEPRECATED API int ahk_listen_device_cb(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData::CallbackFn* callback) {
    return listen_device_interrupt(vid, pid, address, max_length, TransferData { .callback = callback });
}
