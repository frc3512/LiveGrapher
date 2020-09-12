// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "wpi/static_circular_buffer.h"

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

    /**
     * Write queued data to a socket when the socket becomes ready.
     */
    void writePackets();

    template <class T>
    void queueWrite(T& buf) {
        m_writeQueue.emplace_back(reinterpret_cast<const char*>(&buf),
                                  sizeof(T));

        // Select on write
        selectFlags |= SocketConnection::Write;
        write(m_ipcfd_w, "r", 1);
    }

    void queueWrite(const char* buf, size_t length);

    int fd;
    uint8_t selectFlags = Read | Error;
    std::vector<uint8_t> dataSets;

private:
    int m_ipcfd_w;

    bool m_writeDone = true;
    wpi::static_circular_buffer<std::string, 32> m_writeQueue;
};
