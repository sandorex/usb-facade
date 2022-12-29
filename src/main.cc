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
void cmd_list_devices(argparse::ArgumentParser& prog) {
    libusb_device** devices;
    ssize_t length = libusb_get_device_list(ctx, &devices);
    if (length < 0) {
        std::cout << "could not get usb device list\n"
                    << indent(2)
                    << libusb_strerror(static_cast<int>(length))
                    << '\n';

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
            std::cout << "could not get device descriptor from device, skipped\n"
                        << indent(2)
                        << libusb_strerror(err)
                        << "\n\n";
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

            std::cout << "could not open the device, skipped\n"
                        << indent(2)
                        << libusb_strerror(err)
                        << "\n\n";

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
                std::cout << "could not get the config descriptor, skipped\n"
                            << indent(2)
                            << libusb_strerror(err)
                            << "\n\n";

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

            std::cout << std::format("Interface {}:", j) << '\n';

            for (ssize_t k = 0; k < interface.num_altsetting; ++k) {
                libusb_interface_descriptor i_desc = interface.altsetting[k];

                std::cout << indent(2)
                            << std::format("Setting {}: Class {:#x} Endpoints ({}):", k, i_desc.bInterfaceClass, i_desc.bNumEndpoints)
                            << '\n';

                for (ssize_t l = 0; l < i_desc.bNumEndpoints; ++l) {
                    libusb_endpoint_descriptor e_desc = i_desc.endpoint[l];

                    std::cout << indent(4) << std::format("{}: Address {:#x}", l, e_desc.bEndpointAddress);

                    if (e_desc.bEndpointAddress & LIBUSB_ENDPOINT_IN)
                        std::cout << " IN (DEVICE-TO-HOST)" << '\n';
                    else
                        std::cout << " OUT (HOST-TO-DEVICE)" << '\n';
                }
            }

            std::cout << '\n';
        }
    }

    std::exit(EXIT_SUCCESS);
}

void interruptt(libusb_transfer* transfer) {
    std::cout << "got data: " << transfer->actual_length << '\n';

    libusb_submit_transfer(transfer);
}

void cmd_listen(argparse::ArgumentParser& prog) {
    auto vid = prog.present<uint16_t>("vid");
    auto pid = prog.present<uint16_t>("pid");

    if (!vid.has_value() || !pid.has_value()) {
        std::cout << "invalid arguments\n\n"
                  << prog;

        std::exit(EXIT_FAILURE);
    }

    libusb_device_handle* handle = libusb_open_device_with_vid_pid(ctx, vid.value(), pid.value());
    if (handle == NULL) {
        std::cout << "error opening the device\n";
        std::exit(EXIT_FAILURE);
    }

    // int err = libusb_claim_interface(handle, 0);
    // if (err < 0) {
    //     std::cout << "claiming interface failed\n";
    //     std::exit(EXIT_FAILURE);
    // }

    // DEFER([&] { libusb_release_interface(handle, 0); });

    std::vector<unsigned char> data(prog.get<uint8_t>("--max-length"));

    // libusb_transfer transfer;
    libusb_transfer* transfer = libusb_alloc_transfer(0);
    DEFER([&] { libusb_free_transfer(transfer); });

    // auto interrupt = [](libusb_transfer* transfer) {
    //     std::cout << "got data: " << transfer->actual_length << '\n';

    //     libusb_submit_transfer(transfer);
    // };

    libusb_fill_interrupt_transfer(
        transfer,
        handle,
        prog.get<int8_t>("addr"),
        data.data(),
        static_cast<int>(data.capacity()),
        &interruptt,
        NULL,
        0
    );

    int err = libusb_submit_transfer(transfer);
    if (err != 0) {
        std::cout << "error submitting transfer: " << libusb_strerror(err) << '\n';
        std::exit(EXIT_FAILURE);
    }

    while(err == 0) {
        err = libusb_handle_events_completed(ctx, &err);
    }

    if (err != 0) {
        std::cout << "error handling events?: " << libusb_strerror(err) << '\n';
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
    prog.add_subparser(list_cmd);

    argparse::ArgumentParser listen_cmd("listen");
    listen_cmd.add_description("Reads from device to stdout");
    listen_cmd.add_epilog("To find vid, pid, address of a device use 'list' command");

    listen_cmd.add_argument("--max-length")
              .default_value(5)
              .scan<'i', uint8_t>()
              .help("Maximum length of data you expect to receive (in bytes), 255 is the maximum");

    listen_cmd.add_argument("vid")
              .scan<'i', uint16_t>()
              .help("Vendor ID of the device");

    listen_cmd.add_argument("pid")
              .scan<'i', uint16_t>()
              .help("Product ID of the device");

    listen_cmd.add_argument("addr")
              .scan<'i', uint8_t>()
              .help("Endpoint address (direction must be IN aka DEVICE TO HOST)");

    prog.add_subparser(listen_cmd);

    argparse::ArgumentParser ahk_cmd("ahk");
    ahk_cmd.add_description("AutoHotkey specific commands");
    ahk_cmd.add_epilog("For more information go to https://github.com/sandorex/usb-facade");

    prog.add_subparser(ahk_cmd);

    try {
        prog.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << "error while parsing args:\n"
                  << indent(2)
                  << err.what()
                  << "\n\n"
                  << prog;
        return EXIT_FAILURE;
    }

    if (auto err = libusb_init(&ctx); err != 0) {
        std::cout << "could not initialize libusb:\n"
                  << indent(2)
                  << libusb_strerror(err)
                  << '\n';

        std::exit(EXIT_FAILURE);
    }

    DEFER([&] { libusb_exit(ctx); });

    if (prog.get<bool>("--debug")) {
        std::cout << "debugging mode enabled\n";
        libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    }

    try {
        if (prog.is_subcommand_used("list"))
            cmd_list_devices(list_cmd);

        if (prog.is_subcommand_used("listen"))
            cmd_listen(listen_cmd);
    } catch (const std::exception& err) {
        std::cerr << "error ??:\n"
                  << indent(2)
                  << err.what()
                  << "\n\n"
                  << prog;
        return EXIT_FAILURE;
    }



    // TODO: do this when actually using the device
    // libusb_set_auto_detach_kernel_driver(ctx, true);

    return EXIT_SUCCESS;
}
