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

#include <iostream>
#include <argparse/argparse.hpp>
#include <libusb.h>
#include <format>

#include "usb-facade/version.hh"
#include "common.hh"
#include "macros.hh"

libusb_context* ctx;

/// @brief lists all devices to stdout, calls `std::exit` afterwards
void cmd_list_devices() {
    libusb_device** devices;
    ssize_t length = libusb_get_device_list(ctx, &devices);
    if (length < 0) {
        std::cerr << std::format("could not get usb device list\n  {}\n\n", libusb_strerror(static_cast<int>(length)));

        std::exit(EXIT_FAILURE);
    }

    DEFER([&] { libusb_free_device_list(devices, true); });

    std::cout << "found " << length << " USB devices:\n";

    libusb_device* device;
    libusb_device_descriptor desc;
    libusb_device_handle* handle;
    for (ssize_t i = 0; i < length; ++i) {
        device = devices[i];
        if (int err = libusb_get_device_descriptor(device, &desc); err != 0) {
            std::cout << std::format("could not get device descriptor from device, skipped\n  {}\n\n", libusb_strerror(err));
            continue;
        }

        std::cout << std::format("Device {}: USB {} (VID {:#x} PID {:#x})", i, get_usb_string(desc.bcdUSB), desc.idVendor, desc.idProduct) << '\n';
        std::cout << std::format("Class {:#x} Subclass {:#x} Protocol {:#x}", desc.bDeviceClass, desc.bDeviceSubClass, desc.bDeviceProtocol) << '\n';

        if (int err = libusb_open(device, &handle); err != 0) {
            // we got all data we could so just go to the next device
            if (err == LIBUSB_ERROR_NOT_SUPPORTED) {
                std::cout << '\n';
                continue;
            }

            std::cerr << std::format("could not open the device, skipped\n  {}\n\n", libusb_strerror(err));

            continue;
        }

        DEFER([&] { libusb_close(handle); });

        std::string product_name, manufacturer, serial;

        // i do not care about errors, if they exist they exist..
        get_string(handle, desc.iProduct, product_name);
        get_string(handle, desc.iManufacturer, manufacturer);
        get_string(handle, desc.iSerialNumber, serial);

        std::cout << std::format("Name '{}' made by '{}'", product_name, manufacturer) << '\n';

        if (serial.length() > 0)
            std::cout << std::format("SN '{}'", serial) << '\n';

        libusb_config_descriptor* config_desc;
        if (int err = libusb_get_active_config_descriptor(device, &config_desc); err != 0) {
            if (err == LIBUSB_ERROR_NOT_FOUND)
                std::cout << "device is in unconfigured state\n\n";
            else
                std::cout << std::format("could not get the config descriptor, skipped\n  {}\n\n", libusb_strerror(err));

            continue;
        }

        DEFER([&] { libusb_free_config_descriptor(config_desc); });

        // i have no idea what this is in a device
        std::string configuration;
        if (int err = get_string(handle, config_desc->iConfiguration, configuration); err == 0)
            std::cout << std::format("Configuration: {}", configuration) << '\n';

        std::cout << std::format("Found {} interfaces", config_desc->bNumInterfaces) << '\n';

        for (ssize_t j = 0; j < config_desc->bNumInterfaces; ++j) {
            libusb_interface interface = config_desc->interface[j];

            // NOTE: i tried iterating over all altsetting but theres nothing most of the time
            const libusb_interface_descriptor* i_desc = interface.altsetting;

            std::cout << std::format("Interface {}: Class {:#x} Endpoints ({}):\n", j, i_desc->bInterfaceClass, i_desc->bNumEndpoints);

            for (ssize_t k = 0; k < i_desc->bNumEndpoints; ++k) {
                libusb_endpoint_descriptor e_desc = i_desc->endpoint[k];

                std::cout << std::format("  {}: Address {:#x} {}\n", k, e_desc.bEndpointAddress,
                    e_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN
                        ? "IN (DEVICE-TO-HOST)"
                        : "OUT (HOST-TO-DEVICE)"
                );
            }

            std::cout << '\n';
        }
    }

    std::exit(EXIT_SUCCESS);
}

struct TransferData {
    using Callback = std::function<void(unsigned char*, int)>;

    Callback* callback;
};

void cmd_listen(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData::Callback* callback) {
    int err;

    libusb_device_handle* handle = libusb_open_device_with_vid_pid(ctx, vid, pid);
    if (handle == NULL) {
        std::cerr << "error opening the device\n";
        std::exit(EXIT_FAILURE);
    }

    DEFER([&] { libusb_close(handle); });

    // for linux, does nothing on windows
    libusb_set_auto_detach_kernel_driver(handle, true);

    libusb_config_descriptor* cfg;
    if (err = libusb_get_active_config_descriptor(libusb_get_device(handle), &cfg); err != 0) {
        std::cerr << "error getting config descriptor\n"
                  << indent(2)
                  << libusb_strerror(err)
                  << '\n';
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
        std::exit(EXIT_FAILURE);
    }

    if (err = libusb_claim_interface(handle, interface_index); err != 0) {
        std::cerr << "error claiming interface:"
                  << '\n'
                  << indent(2)
                  << libusb_strerror(err)
                  << '\n';

        std::exit(EXIT_FAILURE);
    }

    DEFER([&] { libusb_release_interface(handle, interface_index); });

    std::vector<unsigned char> data(max_length);

    // libusb_transfer transfer;
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    DEFER([&] { libusb_free_transfer(transfer); });

    auto interrupt = [](libusb_transfer* transfer) {
        auto data = reinterpret_cast<TransferData*>(transfer->user_data);

        // call the callback with the data pointer and length
        if (data != NULL)
            (*data->callback)(transfer->buffer, transfer->actual_length);

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
        std::cerr << "error submitting transfer:"
                  << '\n'
                  << indent(2)
                  << libusb_strerror(err)
                  << '\n';

        std::exit(EXIT_FAILURE);
    }

    while(err == 0) {
        err = libusb_handle_events_completed(ctx, &err);
    }

    if (err != 0) {
        std::cerr << std::format("error handling events?:\n  {}\n", libusb_strerror(err));

        std::exit(EXIT_FAILURE);
    }

    std::exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    argparse::ArgumentParser prog("usb-facade", USB_FACADE_VERSION_STRING);

    prog.add_description("Read data from USB devices raw, let your imagination run wild");

    prog.add_argument("-d", "--debug")
        .default_value(false)
        .implicit_value(true)
        .help("Enable debugging");

    argparse::ArgumentParser list_cmd("list");
    list_cmd.add_description("List all USB devices and their information");

    argparse::ArgumentParser listen_cmd("listen");
    listen_cmd.add_description("Reads from device to stdout");
    listen_cmd.add_epilog("To find vid, pid, address of a device use 'list' command");

    listen_cmd.add_argument("--max-length")
              .scan<'d', unsigned int>()
              .default_value(5U)
              .help("Maximum length of data you want to receive (in bytes), 255 is the maximum");

    listen_cmd.add_argument("vid")
              .scan<'i', uint16_t>()
              .help("Vendor ID of the device");

    listen_cmd.add_argument("pid")
              .scan<'i', uint16_t>()
              .help("Product ID of the device");

    listen_cmd.add_argument("addr")
              .scan<'i', uint8_t>()
              .help("Endpoint address (direction must be IN aka DEVICE TO HOST)");

    argparse::ArgumentParser ahk_cmd("ahk");
    ahk_cmd.add_description("AutoHotkey specific commands");
    ahk_cmd.add_epilog("For more information go to https://github.com/sandorex/usb-facade");

    prog.add_subparser(list_cmd);
    prog.add_subparser(listen_cmd);
    prog.add_subparser(ahk_cmd);

    try {
        prog.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << std::format("error while parsing args:\n  {}\n\n", err.what()) << prog;

        return EXIT_FAILURE;
    }

    if (auto err = libusb_init(&ctx); err != 0) {
        std::cerr << std::format("could not initialize libusb:\n  {}\n", libusb_strerror(err));

        std::exit(EXIT_FAILURE);
    }

    DEFER([&] { libusb_exit(ctx); });

    if (prog.get<bool>("--debug")) {
        std::cout << "debugging mode enabled\n";
        libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    }

    if (prog.is_subcommand_used("list"))
        cmd_list_devices();

    if (prog.is_subcommand_used("listen")) {
        TransferData::Callback cb = [](unsigned char* data, int length) {
            for (size_t i = 0; i < length; ++i)
                std::cout << std::format("{:#x} ", data[i]);

            std::cout << '\n';
        };

        cmd_listen(
            listen_cmd.get<uint16_t>("vid"),
            listen_cmd.get<uint16_t>("pid"),
            listen_cmd.get<uint8_t>("addr"),
            listen_cmd.get<unsigned int>("--max-length"),
            &cb
        );
    }

    return EXIT_SUCCESS;
}
