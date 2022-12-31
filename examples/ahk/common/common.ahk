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

; allow the scripts to work if the dll is in the same directory
if FileExist(A_WorkingDir "\usb-facade.dll") == "" {
    ; run in directory with the build executable and dll
    SetWorkingDir A_ScriptDir "\..\..\build"
}

if FileExist(A_WorkingDir "\usb-facade.dll") == "" {
    MsgBox "Please place usb-facade.dll next to the script file (" A_ScriptDir ")", "usb-facade library not found", "OK Iconx"
    Exit(1)
}

; for debugging libusb
LIBUSB_DEBUG := false

; api version
USB_FACADE_API_VER := DllCall("usb-facade.dll\ahk_api_version", "Int")

; initialize the library
DllCall "usb-facade.dll\ahk_init", "Int", LIBUSB_DEBUG

require_api_version(major, minor) {
    ret := DllCall("usb-facade.dll\ahk_check_api_version", "Int", major, "Int", minor, "Int")
    if !ret {
        MsgBox "Incompatible usb-facade API version detected, aborting..", "API Version Mismatch", "OK Iconx"
        Exit(1)
    }
}

check_device_open(device_ptr) {
    ret := DllCall("usb-facade.dll\ahk_is_device_open", "Ptr", device_ptr, "int")
    if ret != 0 {
        MsgBox "Error opening device (code " ret "), aborting..", "usb-facade failed to open device", "OK Iconx"
        Exit(1)
    }
}
