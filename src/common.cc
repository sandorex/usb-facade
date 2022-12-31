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

namespace usb_facade {
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

    /// @brief clears up allocated memory when opening a device, stops the interrupt
    ///
    /// @note does not free device->data.callback you have to do that yourself
    void close_device_interrupt(Device* device) {
        if (device == nullptr)
            return;

        if (device->buffer != nullptr) {
            delete[] device->buffer;
            device->buffer = nullptr;
        }

        if (device->transfer != nullptr) {
            libusb_free_transfer(device->transfer);
            device->transfer = nullptr;
        }

        if (device->interface_index >= 0) {
            libusb_release_interface(device->handle, device->interface_index);
            device->interface_index = -1;
        }

        if (device->config_descriptor != nullptr) {
            libusb_free_config_descriptor(device->config_descriptor);
            device->config_descriptor = nullptr;
        }

        if (device->handle != nullptr) {
            libusb_close(device->handle);
            device->handle = nullptr;
        }

        delete device;
    }

    /// @brief open a device and read interrupts using the callback in data
    Device* open_device_interrupt(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData data) {
        Device* device = new Device {
            .data = TransferData {
                .callback = data.callback
            },
            .interface_index = -1,
            .error = EXIT_FAILURE,
        };

        device->handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
        if (device->handle == NULL) {
            std::cerr << "error opening the device\n";

            device->error = EXIT_FAILURE;
            return device;
        }

        // for linux, does nothing on windows
        libusb_set_auto_detach_kernel_driver(device->handle, true);

        if (device->error = libusb_get_active_config_descriptor(libusb_get_device(device->handle), &device->config_descriptor); device->error != 0) {
            std::cerr << std::format("error getting config descriptor\n  {}\n", libusb_strerror(device->error));

            device->config_descriptor = nullptr;
            return device;
        }

        bool found = false;
        for (int i = 0; i < device->config_descriptor->bNumInterfaces; ++i) {
            auto& altsetting = device->config_descriptor->interface[i].altsetting;
            for (int j = 0; j < altsetting->bNumEndpoints; ++j) {
                auto& endpoint = altsetting->endpoint[j];
                if (endpoint.bEndpointAddress == address) {
                    device->interface_index = i;
                    found = true;
                    break;
                }
            }

            if (found)
                break;
        }

        if (!found) {
            std::cerr << std::format("error interface with endpoint address {:#x} not found", address) << '\n';

            return device;
        }

        if (device->error = libusb_claim_interface(device->handle, device->interface_index); device->error != 0) {
            std::cerr << std::format("error claiming interface:\n  {}\n", libusb_strerror(device->error));

            return device;
        }

        device->transfer = libusb_alloc_transfer(0);
        device->buffer = new unsigned char[max_length];

        libusb_fill_interrupt_transfer(
            device->transfer,
            device->handle,
            address,
            device->buffer,
            max_length,
            [](libusb_transfer* transfer) {
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
            },
            &device->data,
            0
        );

        if (device->error = libusb_submit_transfer(device->transfer); device->error != 0) {
            std::cerr << std::format("error submitting transfer:\n  {}\n", libusb_strerror(device->error));

            return device;
        }

        device->error = EXIT_SUCCESS;
        return device;
    }

    /// @brief same as @ref open_device_interrupt but blocking
    int listen_device_interrupt(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData data) {
        Device* device = open_device_interrupt(vid, pid, address, max_length, data);

        DEFER([&]{ close_device_interrupt(device); });
        if (device->error != 0)
            return EXIT_FAILURE;

        // block
        while(device->error == 0)
            device->error = libusb_handle_events_completed(NULL, &device->error);

        if (device->error != 0) {
            std::cerr << std::format("error handling events?:\n  {}\n", libusb_strerror(device->error));

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
}
