; Copyright 2022 Aleksandar Radivojević
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;    http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

#SingleInstance Force

#Include common/common.ahk

require_api_version(0, 1)

; these are values for my keyboard, get your own using usb-facade.exe
; it gives 8 bytes
VID := 0x4d9
PID := 0x1603
ADDR := 0x81
MAX_LEN := 8

SHIFT_MOD := 0x2

KEY_A := 0x40000

callback(buf, buf_len, tdata) {
    raw_keys := NumGet(buf, 0, "UInt64")

    switch raw_keys {
        case KEY_A | SHIFT_MOD:
            SendInput "A"
        case KEY_A:
            SendInput "a"
    }
}

; NOTE: the dllcall will block the rest of script
cb := CallbackCreate(callback, "Fast")
err := DllCall("usb-facade.dll\ahk_listen_device_cb", "UShort", vid, "UShort", pid, "UChar", addr, "UInt", max_len, "Ptr", cb, "Int")
CallbackFree(cb)
