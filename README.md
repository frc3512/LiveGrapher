GraphHost Protocol Documentation
================================

GraphHost provides a method for sending data samples to a graphing tool on a network-connected workstation for real-time display. This can be used to perform online PID controller tuning of motors.

Communication Protocol
----------------------

Clients should connect to the TCP port specified in the constructor. Various requests can then be sent to the server. These requests may trigger the server to respond with zero or more responses. All communication with the server is asynchronous. A request may be sent at any time, even before a response has been received regarding a previous request. Therefore, the order in which responses are sent is unspecified.

All requests consist of sixteen byte ASCII strings. The first byte describes the type of request. The remaining fifteen bytes describe the dataset name associated with the request (if appropriate).

All responses consist of 28 bytes arranged in various configurations. The first byte describes the type of response.

### List Available Datasets ('l') ###

This request triggers the server to respond with a list of datasets from which data can be requested.

The 'l' character is sent to the server, followed by fifteen bytes of padding. One response is sent for each available dataset.

The body of each response contains the following fields:
* The ASCII character 'l', the type of response.
* A fifteen byte string whose contents represent the name of a dataset in the list.
* A nonzero value of this one byte field indicates the datagram is the last in the sequence.
* Eleven bytes of padding.

### Begin Sending Data ('c') ###

This request notifies the server that it may begin sending data points associated with the specified dataset.

The 'c' character is sent to the server, followed by a fifteen byte string describing the dataset from which data is being requested. The server will then respond with a data point each time the *GraphHost::graphData()* function is called with the specified dataset.

The body of each response contains the following fields:
* The ASCII character 'd', the type of response.
* The fifteen byte ASCII string describing the dataset to which the data point belongs.
* A 64-bit, unsigned integer representing the X component of the data points position as specified in the call to GraphHost::graphData().
* A 32-bit, IEEE floating point number representing the Y component of the data points position as specified in the call to GraphHost::graphData().

### Stop Sending Data ('d') ###

This request notifies the server to stop sending data from the specified dataset.

The 'd' character is sent to the server, followed by a fifteen byte string describing the dataset from which to stop sending data.

