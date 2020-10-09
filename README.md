# LiveGrapher

LiveGrapher graphs data received over a network in real time (useful for tuning feedback controllers).

## Dependencies

### Windows

If one is building on Windows, we recommend using the [MSYS2 compiler](https://msys2.github.io/). The following libraries are required as dependencies and should be installed using either your package manager of choice or MSYS2's pacman:

* Qt 5.15 or higher
* CMake

### Ubuntu

As of the time of writing, the latest version of Ubuntu (20.04)'s qt5-default package only provides Qt 5.12, which is is incompatible with LiveGrapher. We need to install a newer version of Qt ourselves.

#### Building Qt

This can be achieved by downloading the open source installer from [Qt's download page](https://www.qt.io/download) under "Downloads for open source users". Note: There are two methods for installing Qt. One is through the installer, which needs an account. Another is downloading the source package and building it yourself (doesn't require an account). If you wish to build the packages, you can visit the offline installer [download page](https://www.qt.io/offline-installers) here. Instructions on building it is avaliable [here](https://doc.qt.io/qt-5/linux-building.html) and the build requirements are avaliable [here](https://doc.qt.io/qt-5/linux-requirements.html) as well.

#### Setting Qt's PATH

Note: The same instructions here are similar to those on this [wiki page](https://doc.qt.io/qt-5/linux-building.html) mentioned earlier.

For LiveGrapher to find your Qt package, we need to add it to the PATH environment variable. Append the following to `~/.profile`.

```
export PATH=$PATH:"Qt location including bin"
```

## Building LiveGrapher

To build the project, first run `cmake dir` within a terminal from the desired build directory, where "dir" is the relative location of the CMakeLists.txt file. If a release build is desired, append `-DCMAKE_BUILD_TYPE=Release` to the cmake call. Then run `make`.

To cross-compile from Linux to Windows, install the MinGW-w64 toolchain (`mingw-w64-gcc` and a MinGW-w64 build of Qt 5 (`mingw-w64-qt5-base`), then run `publish-win32.sh` to build LiveGrapher and create a .zip of the application binary and necessary files.

## Configuration

The IPSettings.txt in the root directory of the project should be kept with the executable since it looks for it in its current directory.

### IPSettings.txt

The required entries in IPSettings.txt are as follows:

#### `robotIP`

Robot's IP address

#### `robotGraphPort`

Port from which graph data is received

#### `xHistory`

This entry is the length of time over which to maintain X axis history in seconds.

## Protocol documentation

LiveGrapher provides a method for sending data samples to a graphing tool on a network-connected workstation for real-time display. This can be used to perform online PID controller tuning of motors.

Clients should connect to the TCP port specified in the constructor. Various requests can then be sent to the server. These requests may trigger the server to respond with zero or more responses. All communication with the server is asynchronous. A request may be sent at any time, even before a response has been received regarding a previous request. Therefore, the order in which responses are sent is unspecified.

Packets are classified as either client or host packets, representing by whom they are received. The first byte is split into two fields of two and six bits respectively. The first field contains an ID for the type of packet sent, and the second contains the ID of the graph whose information was sent, if applicable. The packet payload descriptions are written in terms of C standard integer typedefs and C bitfields.

### Host packets

#### Start sending data

This request notifies the server that it may begin sending data points associated with the specified data set.

* uint8_t packetID : 2
  * Contains '0b00'
* uint8_t graphID : 6
  * Contains ID of graph

#### Stop sending data

This request notifies the server to stop sending data from the specified data set.

* uint8_t packetID : 2
  * Contains '0b01'
* uint8_t graphID : 6
  * Contains ID of graph

#### List available data sets

This request triggers the host to respond with a list of names of availabe data sets from which data can be requested.

* uint8_t packetID : 2
  * Contains '0b10'
* uint8_t graphID : 6
  * Unused
  * Should be set to 0, but not required

### Client packets

#### Data

This packet contains a point of data from the given data set.

* uint8_t packetID : 2
  * Contains '0b00'
* uint8_t graphID : 6
  * Contains ID of graph
* uint64_t x
  * X component of data point
* float y
  * Y component of data point
  * It is assumed to be a 32-bit IEEE 754 floating point number

#### List

One response of this packet type is sent for each available data set after sending a request for the list of available data sets. This packet contains the name of the data set on the host.

* uint8_t packetID : 2
  * Contains '0b01'
* uint8_t graphID : 6
  * Contains ID of graph
* uint8_t length
  * Contains length of name. Max length is 255.
* uint8_t name[]
  * Contains name which is 'length' bytes long (not NULL terminated)
* uint8_t eof
  * 1 indicates the packet is the last in the sequence; 0 otherwise

## Issue backlog

* Write protocol and CSV export tests?
* Protocol change from "uint64_t x" to "float x" to support 2D plots
  * Overload with just "float y" will still exist for time-based plots
