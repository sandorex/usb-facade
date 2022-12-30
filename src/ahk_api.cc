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
//
// this code is windows only obviously

#include <iostream>
#include <format>

#include "common.hh"
#include "macros.hh"
#include "usb-facade/version.hh"

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
API int ahk_api_version_minor() {
    return USB_FACADE_VERSION_MINOR;
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

/// @public
API int ahk_listen_device_cb(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData::CallbackFn* callback) {
    return listen_device_cb(vid, pid, address, max_length, TransferData { .callback = callback });
}
