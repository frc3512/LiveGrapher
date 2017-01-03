// Copyright (c) FRC Team 3512, Spartatroniks 2013-2017. All Rights Reserved.

#include "SocketConnection.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <utility>

#include "GraphHost.hpp"

#ifdef __VXWORKS__
#include <sockLib.h>
#endif

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
    /* While the current buffer isn't done sending or there are more buffers to
     * send
     */
    while (!m_writedone || !m_writequeue.empty()) {
        // Get another buffer to send
        if (m_writedone) {
            m_writebuf = std::move(m_writequeue.front());
            m_writebufoffset = 0;
            m_writedone = false;
            m_writequeue.pop();
        }

        // These descriptors are ready for writing
        m_writebufoffset +=
            send(fd, &m_writebuf[0], m_writebuf.length() - m_writebufoffset, 0);

        // Have we finished writing the buffer?
        if (m_writebufoffset == m_writebuf.length()) {
            // Reset the write buffer
            m_writebufoffset = 0;
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
