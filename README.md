## usb-facade

Application (and dll library for AHK) for reading USB devices directly hence the
name _facade_

## But what do i do with it

Well whatever you want, if you need some inspiration or even have an idea for an
example they go in `examples/`

## AutoHotkey API

Read [ahk_api.cc](src/ahk_api.cc) file, that's the best way to learn

## Building

Windows building is simple, just install VS Tools and cmake, and run following
commands

```shell
cmake . -Bbuild
cmake --build build

# optional makes a zip file like github actions does
cmake --install build
```

### Linux

Install `libusb` using your package manager (make sure to install the `-dev`
package if it exists) then run the same commands as for windows

## License

Licensed under [Apache 2.0 License](LICENSE.txt)
