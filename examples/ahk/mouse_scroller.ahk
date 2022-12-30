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

; these are values for my wireless mouse, get your own using usb-facade.exe
; 0x1 buttons x y wheel
VID := 0x248A
PID := 0x8367
ADDR := 0x82
MAX_LEN := 5

SCROLL_RATE := 30

; winapi values for mouse_event
; source: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-mouse_event
WHEELV := 0x0800
WHEELH := 0x01000

test_cb(buf, buf_len, tdata) {
    btns := NumGet(buf, 1, "Char")
    x := NumGet(buf, 2, "Char")
    y := NumGet(buf, 3, "Char")
    wheel := NumGet(buf, 4, "Char")

    if btns & 0x1 {
        if x > 0
            DllCall "mouse_event", "UInt", WHEELH, "UInt", 0, "UInt", 0, "UInt", SCROLL_RATE, "Ptr", 0
        else if x < 0
            DllCall "mouse_event", "UInt", WHEELH, "UInt", 0, "UInt", 0, "UInt", -SCROLL_RATE, "Ptr", 0
    } else {
        if y > 0
            DllCall "mouse_event", "UInt", WHEELV, "UInt", 0, "UInt", 0, "UInt", -SCROLL_RATE, "Ptr", 0
        else if y < 0
            DllCall "mouse_event", "UInt", WHEELV, "UInt", 0, "UInt", 0, "UInt", SCROLL_RATE, "Ptr", 0
    }

    if wheel > 0
        SoundSetVolume("+1")
    else if wheel < 0
        SoundSetVolume("-1")
}

; NOTE: the dllcall will block the rest of script
cb := CallbackCreate(test_cb, "Fast")
err := DllCall("usb-facade.dll\ahk_listen_device_cb", "UShort", vid, "UShort", pid, "UChar", addr, "UInt", max_len, "Ptr", cb, "Int")
CallbackFree(cb)
