# LiveGrapher

LiveGrapher graphs data received over a network in real time (useful for tuning feedback controllers).

## Dependencies

If one is building on Windows, we recommend using the [MSYS2 compiler](https://msys2.github.io/). The following libraries are required as dependencies and should be installed using either your package manager of choice or MSYS2's pacman:

* Qt4 or Qt5 (Qt5 recommended)

## Building LiveGrapher

To build the project, first run `qmake dir` within a terminal from the desired build directory, where "dir" is the relative location of the LiveGrapher.pro file. This will generate three makefiles. If a debug build is desired, run `make -f Makefile.Debug`. The default behavior when simply running `make` is to perform a release build.

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

# GraphHost Protocol Documentation

GraphHost provides a method for sending data samples to a graphing tool on a network-connected workstation for real-time display. This can be used to perform online PID controller tuning of motors.

## Communication Protocol

Clients should connect to the TCP port specified in the constructor. Various requests can then be sent to the server. These requests may trigger the server to respond with zero or more responses. All communication with the server is asynchronous. A request may be sent at any time, even before a response has been received regarding a previous request. Therefore, the order in which responses are sent is unspecified.

Packets are classified as either client or host packets, representing by whom they are received. The first byte is split into two fields of two and six bits respectively. The first field contains an ID for the type of packet sent, and the second contains the ID of the graph whose information was sent, if applicable. The packet payload descriptions are written in terms of C standard integer typedefs and C bitfields.

### Host Packets

#### Start Sending Data

This request notifies the server that it may begin sending data points associated with the specified data set.

* uint8_t packetID : 2
  * Contains '0b00'
* uint8_t graphID : 6
  * Contains ID of graph

#### Stop Sending Data

This request notifies the server to stop sending data from the specified data set.

* uint8_t packetID : 2
  * Contains '0b01'
* uint8_t graphID : 6
  * Contains ID of graph

#### List Available Data Sets

This request triggers the host to respond with a list of names of availabe data sets from which data can be requested.

* uint8_t packetID : 2
  * Contains '0b10'
* uint8_t graphID : 6
  * Unused
  * Should be set to 0, but not required

### Client Packets

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
