// Copyright (c) 2013-2017 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <queue>
#include <string>
#include <vector>

/**
 * Wrapper around graph client socket descriptors
 */
class SocketConnection {
public:
    enum selector { Read = 1, Write = 2, Error = 4 };

    SocketConnection(int nfd, int ipcWriteSock);
    ~SocketConnection();
    SocketConnection(const SocketConnection&) = delete;
    SocketConnection& operator=(const SocketConnection&) = delete;

    int recvData(char* buf, size_t length);
    int readPackets();
    void writePackets();

    template <class T>
    void queueWrite(T& buf);

    void queueWrite(const char* buf, size_t length);

    int fd;
    uint8_t selectflags = Read | Error;
    std::vector<uint8_t> dataSets;

private:
    int m_ipcfd_w;

    // The buffer that needs to be written into the socket
    std::string m_writebuf;

    // How much has been written so far
    size_t m_writebufoffset = 0;

    bool m_writedone = true;
    std::queue<std::string> m_writequeue;
};

#include "SocketConnection.inl"
