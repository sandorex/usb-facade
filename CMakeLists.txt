# Copyright 2022 Aleksandar Radivojević
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 3.12)

project (usb-facade VERSION 0.1.0
                    DESCRIPTION "Facade to map USB devices to do custom things"
                    LANGUAGES CXX)

set (PROJECT_EXTERNAL ${PROJECT_SOURCE_DIR}/external)

find_path (LIBUSB_INCLUDE_DIR
    NAME libusb.h
    PATHS ${PROJECT_EXTERNAL}/libusb
    PATH_SUFFIXES "include" "libusb" "libusb-1.0")

# for some reason does not add 'lib' prefix to names
find_library (LIBUSB_LIBRARY
    NAMES usb usb-1.0 libusb libusb-1.0 NAMES_PER_DIR
    PATHS ${PROJECT_EXTERNAL}/libusb
    PATH_SUFFIXES "lib" "lib32" "lib64")

if (NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
    message (FATAL_ERROR "libusb not found")
endif ()

include (FetchContent)

FetchContent_Declare (
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG 997da9255618311d1fcb0135ce86022729d1f1cb # v2.9
)

FetchContent_MakeAvailable(argparse)

configure_file (src/version.hh.in ${PROJECT_BINARY_DIR}/include/usb-facade/version.hh)

add_library (common_options INTERFACE)
target_compile_features (common_options INTERFACE cxx_std_20)
target_include_directories (common_options INTERFACE ${LIBUSB_INCLUDE_DIR}
                                                     ${PROJECT_SOURCE_DIR}/src
                                                     ${PROJECT_BINARY_DIR}/include)
target_link_libraries (common_options INTERFACE ${LIBUSB_LIBRARY}
                                                argparse)

add_executable (usb-facade src/main.cc)
target_link_libraries (usb-facade PUBLIC common_options)

# so that AHK can use them?
set_target_properties (usb-facade PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
