// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "livegrapher/SocketConnection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#include <string_view>

SocketConnection::SocketConnection(int nfd, int ipcWriteSock) {
    fd = nfd;
    m_ipcfd_w = ipcWriteSock;
}

SocketConnection::~SocketConnection() { close(fd); }

int SocketConnection::recvData(char* buf, size_t length) {
    int error = recv(fd, buf, length, 0);

    if (error == 0 || (error == -1 && errno != EAGAIN)) {
        // recv(3) failed, so return failure so socket is closed
        return -1;
    }

    return error;
}

void SocketConnection::writePackets() {
    std::string_view writeBuf;
    size_t writeBufOffset = 0;

    /* While the current buffer isn't done sending or there are more buffers to
     * send
     */
    while (!m_writeDone || m_writeQueue.size() > 0) {
        // Get another buffer to send
        if (m_writeDone) {
            writeBuf = m_writeQueue.front();
            writeBufOffset = 0;
            m_writeDone = false;
            m_writeQueue.pop_front();
        }

        // These descriptors are ready for writing
        writeBufOffset +=
            send(fd, &writeBuf[0], writeBuf.length() - writeBufOffset, 0);

        // Have we finished writing the buffer?
        if (writeBufOffset == writeBuf.length()) {
            // Reset the write buffer
            writeBufOffset = 0;
            m_writeDone = true;
        } else {
            // We haven't finished writing, keep selecting
            return;
        }
    }

    // Stop selecting on write
    selectFlags &= ~Select::Write;
}

void SocketConnection::queueWrite(const char* buf, size_t length) {
    m_writeQueue.emplace_back(buf, length);

    // Select on write
    selectFlags |= Select::Write;
    write(m_ipcfd_w, "r", 1);
}
