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

include (FetchContent)

if (WIN32)
    set (libusb_VERSION 1.0.26)

    FetchContent_Declare (LIBUSB_SRC
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL      https://github.com/libusb/libusb/releases/download/v${libusb_VERSION}/libusb-${libusb_VERSION}.tar.bz2
        URL_HASH MD5=9C75660DFE1D659387C37B28C91E3160
    )

    FetchContent_Declare (LIBUSB_BINARIES
        DOWNLOAD_EXTRACT_TIMESTAMP
        URL      https://github.com/libusb/libusb/releases/download/v${libusb_VERSION}/libusb-${libusb_VERSION}-binaries.7z
        URL_HASH MD5=6736254ECADACD389E3FFD822283AC6A
    )

    FetchContent_MakeAvailable (LIBUSB_SRC)
    FetchContent_MakeAvailable (LIBUSB_BINARIES)
endif ()

FetchContent_Declare (
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG 997da9255618311d1fcb0135ce86022729d1f1cb # v2.9
)

FetchContent_MakeAvailable (argparse)

find_path (LIBUSB_INCLUDE_DIR
    NAME libusb.h
    PATHS ${libusb_src_SOURCE_DIR}
    PATH_SUFFIXES "include" "libusb" "libusb-1.0")

# for some reason does not add 'lib' prefix to names
find_library (LIBUSB_LIBRARY
    NAMES usb usb-1.0 libusb libusb-1.0 NAMES_PER_DIR
    PATHS ${libusb_binaries_SOURCE_DIR}/VS2015-x64/
    PATH_SUFFIXES "lib" "lib32" "lib64")

if (NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
    message (FATAL_ERROR "libusb not found")
endif ()

configure_file (src/version.hh.in ${PROJECT_BINARY_DIR}/include/usb-facade/version.hh)

add_library (common_options INTERFACE)
target_compile_features (common_options INTERFACE cxx_std_20)
target_include_directories (common_options INTERFACE ${LIBUSB_INCLUDE_DIR}
                                                     ${PROJECT_SOURCE_DIR}/src
                                                     ${PROJECT_BINARY_DIR}/include)
target_link_libraries (common_options INTERFACE ${LIBUSB_LIBRARY}
                                                argparse)

add_executable (usb-facade src/main.cc src/common.cc)
target_link_libraries (usb-facade PUBLIC common_options)

if (WIN32)
    add_library (usb-facade-ahk SHARED src/ahk_api.cc src/common.cc)
    set_target_properties (usb-facade-ahk PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        OUTPUT_NAME usb-facade
    )
    target_link_libraries (usb-facade-ahk PUBLIC common_options)
endif ()
