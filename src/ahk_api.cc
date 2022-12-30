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

#include "common.hh"

/// @public
extern "C" int ahk_listen_device_cb(uint16_t vid, uint16_t pid, uint8_t address, unsigned int max_length, TransferData::CallbackFn* callback) {
    return listen_device_cb(NULL, vid, pid, address, max_length, callback);
}
