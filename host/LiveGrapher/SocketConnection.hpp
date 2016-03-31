// =============================================================================
// Description: Wrapper around graph client socket descriptors
// Author: FRC Team 3512, Spartatroniks
// =============================================================================

#ifndef SOCKET_CONNECTION_HPP
#define SOCKET_CONNECTION_HPP

#include <cstdint>
#include <queue>
#include <string>
#include <vector>

class SocketConnection {
public:
    enum selector {
        Read = 1,
        Write = 2,
        Error = 4
    };

    SocketConnection(int nfd, int ipcWriteSock);
    ~SocketConnection();
    SocketConnection(const SocketConnection&) = delete;
    SocketConnection& operator=(const SocketConnection&) = delete;

    int recvData(char* buf, size_t length);
    int readPackets();
    void sendList();
    void writePackets();

    template <class T>
    void queueWrite(T& buf);

    void queueWrite(const char* buf, size_t length);

    int fd;
    uint8_t selectflags = Read | Error;
    std::vector<uint8_t> datasets;

private:
    int m_ipcfd_w;

    // Write buffer currently being written
    std::string m_writebuf; // The buffer that needs to be written into the socket
    size_t m_writebufoffset = 0; // How much has been written so far
    bool m_writedone = true;
    std::queue<std::string> m_writequeue;
};

#include "SocketConnection.inl"

#endif // SOCKET_CONNECTION_HPP
