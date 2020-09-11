// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "livegrapher/SocketConnection.hpp"

#include <sys/socket.h>
#include <sys/types.h>

#ifdef __VXWORKS__
#include <sockLib.h>
#endif

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

// Write queued data to a socket when the socket becomes ready
void SocketConnection::writePackets() {
    std::string_view writebuf;
    size_t writebufoffset = 0;

    /* While the current buffer isn't done sending or there are more buffers to
     * send
     */
    while (!m_writedone || !m_writequeue.empty()) {
        // Get another buffer to send
        if (m_writedone) {
            writebuf = m_writequeue.front();
            writebufoffset = 0;
            m_writedone = false;
            m_writequeue.pop();
        }

        // These descriptors are ready for writing
        writebufoffset +=
            send(fd, &writebuf[0], writebuf.length() - writebufoffset, 0);

        // Have we finished writing the buffer?
        if (writebufoffset == writebuf.length()) {
            // Reset the write buffer
            writebufoffset = 0;
            m_writedone = true;
        } else {
            // We haven't finished writing, keep selecting
            return;
        }
    }

    // Stop selecting on write
    selectflags &= ~SocketConnection::Write;
}

void SocketConnection::queueWrite(const char* buf, size_t length) {
    m_writequeue.emplace(buf, length);

    // Select on write
    selectflags |= SocketConnection::Write;
    write(m_ipcfd_w, "r", 1);
}
