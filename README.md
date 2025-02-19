A bare metal debugger built for the rasberry pi pico.

# Install
You will need [cmake](https://cmake.org/download/) for building and [picotool](https://github.com/raspberrypi/picotool) for flashing. Picotool may be available on some package managers, or you can build from source.

After cloning the repo, make a build directory:
```
$ mkdir build
```
Go into the directory and run `cmake` on the parent directory.
```
$ cd build
$ cmake ..
```
