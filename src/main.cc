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
#include "macros.hh"

/// Gets string using libusb_get_string_descriptor_ascii, modifies string reference passed
int get_string(libusb_device_handle* handle, uint8_t index, std::string& string, size_t max_length=100) {
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

/// @brief converts the USB hex value from libusb to human readable one '0x210' -> '2.1'
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

/// @brief creates a string of spaces as indent
constexpr std::string indent(int n) {
    return std::string(n, ' ');
}

/// @brief lists all devices to stdout
int list_devices(libusb_context* ctx, argparse::ArgumentParser& prog) {
    libusb_device** devices;
    ssize_t length = libusb_get_device_list(ctx, &devices);
    if (length < 0) {
        std::cout << "could not get usb device list\n"
                    << indent(2)
                    << libusb_strerror(static_cast<int>(length))
                    << '\n';
        return EXIT_FAILURE;
    }

    DEFER([&] { libusb_free_device_list(devices, true); });

    std::cout << "found " << length << " USB devices:\n";

    // found the exact device using vid/pid
    bool found = false;

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

        if (prog.present<unsigned int>("--vid").has_value() && prog.present<unsigned int>("--pid").has_value())
            found = prog.get<unsigned int>("--vid") == desc.idVendor
                    && prog.get<unsigned int>("--pid") == desc.idProduct;

        std::cout << std::format("Device {}: USB {} (VID {:#x} PID {:#x})", i, get_usb_string(desc.bcdUSB), desc.idVendor, desc.idProduct);

        if (found)
            std::cout << " * <--- [DEVICE FOUND!]";

        std::cout << '\n';

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

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    libusb_context* ctx;

    argparse::ArgumentParser program("usb-facade", USB_FACADE_VERSION_STRING);

    program.add_description("Application that allows you to read USB devices raw\n"
                            "Also provides API for use with AutoHotkey (Windows)");
    program.add_epilog("The data from the device replace '@@' argument and are provided to the script");

    program.add_argument("-d", "--debug")
        .default_value(false)
        .implicit_value(true)
        .help("Enable debugging");

    program.add_argument("-l", "--list")
        .default_value(false)
        .implicit_value(true)
        .help("List all devices");

    program.add_argument("-p", "--pid")
        .scan<'i', unsigned int>()
        .help("Product ID of the device");

    program.add_argument("-v", "--vid")
        .scan<'i', unsigned int>()
        .help("Vendor ID of the device");

    program.add_argument("--stdout")
        .default_value(false)
        .implicit_value(true)
        .help("Use stdout instead of the command");

    program.add_argument("command")
        .default_value(std::vector<std::string>{})
        .remaining();

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return EXIT_FAILURE;
    }

    if (auto err = libusb_init(&ctx); err != 0) {
        std::cout << "could not initialize libusb:\n"
                  << indent(2)
                  << libusb_strerror(err)
                  << '\n';

        return EXIT_FAILURE;
    }

    DEFER([&] { libusb_exit(ctx); });

    if (program.get<bool>("--debug")) {
        std::cout << "debugging mode enabled\n";
        libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    }

    // TODO: do this when actually using the device
    // libusb_set_auto_detach_kernel_driver(ctx, true);

    if (program.get<bool>("--list"))
        return list_devices(ctx, program);

    if (!program.present<unsigned int>("--pid").has_value() || !program.present<unsigned int>("--vid").has_value()) {
        std::cout << "pid/vid not provided" << '\n';
        return EXIT_FAILURE;
    }

    // assume pipe if no unmatched arguments provided

    return EXIT_SUCCESS;
}
