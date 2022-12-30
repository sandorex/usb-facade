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
#include <iostream>

#include "macros.hh"
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

int listen_device_cb(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData::CallbackFn* callback) {
    int err;

    libusb_device_handle* handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (handle == NULL) {
        std::cerr << "error opening the device\n";

        return EXIT_FAILURE;
    }

    DEFER([&] { libusb_close(handle); });

    // for linux, does nothing on windows
    libusb_set_auto_detach_kernel_driver(handle, true);

    libusb_config_descriptor* cfg;
    if (err = libusb_get_active_config_descriptor(libusb_get_device(handle), &cfg); err != 0) {
        std::cerr << std::format("error getting config descriptor\n  {}\n", libusb_strerror(err));

        return EXIT_FAILURE;
    }

    DEFER([&] { libusb_free_config_descriptor(cfg); });

    bool found = false;
    int interface_index = 0;
    for (int i = 0; i < cfg->bNumInterfaces; ++i) {
        auto& altsetting = cfg->interface[i].altsetting;
        for (int j = 0; j < altsetting->bNumEndpoints; ++j) {
            auto& endpoint = altsetting->endpoint[j];
            if (endpoint.bEndpointAddress == address) {
                interface_index = i;
                found = true;
                break;
            }
        }

        if (found)
            break;
    }

    if (!found) {
        std::cerr << std::format("error interface with endpoint address {:#x} not found", address) << '\n';

        return EXIT_FAILURE;
    }

    if (err = libusb_claim_interface(handle, interface_index); err != 0) {
        std::cerr << std::format("error claiming interface:\n  {}\n", libusb_strerror(err));

        return EXIT_FAILURE;
    }

    DEFER([&] { libusb_release_interface(handle, interface_index); });

    std::vector<unsigned char> data(max_length);

    // libusb_transfer transfer;
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    DEFER([&] { libusb_free_transfer(transfer); });

    auto interrupt = [](libusb_transfer* transfer) {
        auto data = reinterpret_cast<TransferData*>(transfer->user_data);

        if (data->callback != NULL) {
            // call the callback with the buffer pointer, buffer length and the struct itself
            (*data->callback)(
                transfer->buffer,
                transfer->actual_length,
                data
            );
        }

        libusb_submit_transfer(transfer);
    };

    // expandable for futureproofing
    TransferData transfer_data {
        .callback = callback
    };

    libusb_fill_interrupt_transfer(
        transfer,
        handle,
        address,
        data.data(),
        static_cast<int>(data.capacity()),
        interrupt,
        &transfer_data,
        0
    );

    if (err = libusb_submit_transfer(transfer); err != 0) {
        std::cerr << std::format("error submitting transfer:\n  {}\n", libusb_strerror(err));

        return EXIT_FAILURE;
    }

    while(err == 0) {
        err = libusb_handle_events_completed(NULL, &err);
    }

    if (err != 0) {
        std::cerr << std::format("error handling events?:\n  {}\n", libusb_strerror(err));

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
