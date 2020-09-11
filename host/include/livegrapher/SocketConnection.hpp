// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <unistd.h>

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
    void queueWrite(T& buf) {
        m_writequeue.emplace(reinterpret_cast<const char*>(&buf), sizeof(T));

        // Select on write
        selectflags |= SocketConnection::Write;
        write(m_ipcfd_w, "r", 1);
    }

    void queueWrite(const char* buf, size_t length);

    int fd;
    uint8_t selectflags = Read | Error;
    std::vector<uint8_t> dataSets;

private:
    int m_ipcfd_w;

    bool m_writedone = true;
    std::queue<std::string> m_writequeue;
};
