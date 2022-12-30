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

; run in directory with the build executable and dll
SetWorkingDir A_ScriptDir "\..\..\build"

; for debugging libusb
LIBUSB_DEBUG := false

; api version
USB_FACADE_API_VER := DllCall("usb-facade.dll\ahk_api_version", "Int")

; initialize the library
DllCall "usb-facade.dll\ahk_init", "Int", LIBUSB_DEBUG

require_api_version(major, minor) {
    ret := DllCall("usb-facade.dll\ahk_check_api_version", "Int", major, "Int", minor, "Int")
    if !ret {
        MsgBox "Incompatible usb-facade API version detected, aborting..", "API Version Mismatch", "T5 OK Iconx"
        Exit(1)
    }
}
