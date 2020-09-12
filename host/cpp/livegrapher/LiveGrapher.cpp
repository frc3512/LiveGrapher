// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "livegrapher/LiveGrapher.hpp"

#include <endian.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>

#include <algorithm>
#include <cstring>

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

LiveGrapher::LiveGrapher(int port) {
    // Listen on a socket
    m_listenFd = Listen(port, 0);
    if (m_listenFd == -1) {
        throw -1;
    }

    // Create a pipe for IPC with the thread
    int pipeFd[2];
    if (pipe(pipeFd) == -1) {
        throw -1;
    }

    m_ipcFdReader = pipeFd[0];
    m_ipcFdWriter = pipeFd[1];

    m_thread = std::thread([this] { ThreadMain(); });
}

LiveGrapher::~LiveGrapher() {
    // Tell the other thread to stop
    write(m_ipcFdWriter, "x", 1);

    // Join to the other thread
    m_thread.join();

    // Close file descriptors and clean up
    close(m_ipcFdReader);
    close(m_ipcFdWriter);
}

void LiveGrapher::AddData(const std::string& dataset, float value) {
    // This will only work if ints are the same size as floats
    static_assert(sizeof(float) == sizeof(uint32_t),
                  "float isn't 32 bits long");

    auto currentTime =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch())
            .count();

    auto i = m_graphList.find(dataset);

    if (i == m_graphList.end()) {
        m_graphList.emplace(dataset, m_graphList.size());
    }

    ClientDataPacket packet;
    packet.ID = kClientDataPacket | i->second;

    // Change to network byte order
    // Swap bytes in x, and copy into the payload struct
    uint64_t xtmp;
    std::memcpy(&xtmp, &currentTime, sizeof(xtmp));
    xtmp = be64toh(xtmp);
    std::memcpy(&packet.x, &xtmp, sizeof(xtmp));

    // Swap bytes in y, and copy into the payload struct
    uint32_t ytmp;
    std::memcpy(&ytmp, &value, sizeof(ytmp));
    ytmp = htonl(ytmp);
    std::memcpy(&packet.y, &ytmp, sizeof(ytmp));

    std::scoped_lock lock(m_mutex);

    // Send the point to connected clients
    for (auto& conn : m_connList) {
        for (const auto& datasetID : conn->dataSets) {
            if (datasetID == i->second) {
                // Send the value off
                conn->queueWrite(packet);
            }
        }
    }
}

void LiveGrapher::ThreadMain() {
    int maxfd;
    uint8_t ipccmd = 0;

    fd_set readfds;
    fd_set writefds;
    fd_set errorfds;

    while (ipccmd != 'x') {
        // Clear the fdsets
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&errorfds);

        // Reset the maxfd
        maxfd = std::max(m_listenFd, m_ipcFdReader);

        {
            std::scoped_lock lock(m_mutex);

            // Add the file descriptors to the list
            for (auto& conn : m_connList) {
                if (maxfd < conn->fd) {
                    maxfd = conn->fd;
                }
                if (conn->selectFlags & SocketConnection::Select::Read) {
                    FD_SET(conn->fd, &readfds);
                }
                if (conn->selectFlags & SocketConnection::Select::Write) {
                    FD_SET(conn->fd, &writefds);
                }
                if (conn->selectFlags & SocketConnection::Select::Error) {
                    FD_SET(conn->fd, &errorfds);
                }
            }
        }

        // Select on the listener fd
        FD_SET(m_listenFd, &readfds);

        // ipcfd will receive data when the thread needs to exit
        FD_SET(m_ipcFdReader, &readfds);

        // Select on the file descriptors
        select(maxfd + 1, &readfds, &writefds, &errorfds, nullptr);

        {
            std::scoped_lock lock(m_mutex);

            auto conn = m_connList.begin();
            while (conn != m_connList.end()) {
                if (FD_ISSET((*conn)->fd, &readfds)) {
                    // Handle reading
                    if (ReadPackets(conn->get()) == -1) {
                        conn = m_connList.erase(conn);
                        continue;
                    }
                }
                if (FD_ISSET((*conn)->fd, &writefds)) {
                    // Handle writing
                    (*conn)->writePackets();
                }
                if (FD_ISSET((*conn)->fd, &errorfds)) {
                    // Handle errors
                    conn = m_connList.erase(conn);
                    continue;
                }

                conn++;
            }
        }

        // Check for listener condition
        if (FD_ISSET(m_listenFd, &readfds)) {
            // Accept connections
            int fd = Accept(m_listenFd);

            if (fd != -1) {
                // Disable Nagle's algorithm
                int yes = 1;
                setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                           reinterpret_cast<char*>(&yes), sizeof(yes));

                std::scoped_lock lock(m_mutex);
                // Add it to the list, this makes it a bit non-thread-safe
                m_connList.emplace_back(
                    std::make_unique<SocketConnection>(fd, m_ipcFdWriter));
            }
        }

        // Handle IPC commands
        if (FD_ISSET(m_ipcFdReader, &readfds)) {
            read(m_ipcFdReader, reinterpret_cast<char*>(&ipccmd), 1);
        }
    }

    // Close the listener file descriptor
    close(m_listenFd);
}

int LiveGrapher::Listen(int port, uint32_t sourceAddr) {
    sockaddr_in servAddr;
    int sd = -1;

    try {
        // Create a TCP socket
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if (sd == -1) {
            throw -1;
        }

        // Allow rebinding to the socket later if the connection is interrupted
        int optval = 1;
        setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        // Zero out the serv_addr struct
        std::memset(&servAddr, 0, sizeof(sockaddr_in));

        // Set up the listener sockaddr_in struct
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = sourceAddr;
        servAddr.sin_port = htons(port);

        // Bind the socket to the listener sockaddr_in
        if (bind(sd, reinterpret_cast<sockaddr*>(&servAddr),
                 sizeof(sockaddr_in)) != 0) {
            throw -1;
        }

        // Listen on the socket for incoming connections
        if (listen(sd, 5) != 0) {
            throw -1;
        }
    } catch (int e) {
        std::perror("");
        if (sd != -1) {
            close(sd);
        }
        return -1;
    }

    // Make sure we aren't killed by SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    return sd;
}

int LiveGrapher::Accept(int listenFd) {
    unsigned int clilen;
    sockaddr_in cli_addr;

    clilen = sizeof(cli_addr);

    int newFd = -1;

    try {
        // Accept a new connection
        newFd =
            accept(listenFd, reinterpret_cast<sockaddr*>(&cli_addr), &clilen);

        // Make sure that the file descriptor is valid
        if (newFd == -1) {
            throw -1;
        }

        // Set the socket non-blocking
        int flags = fcntl(newFd, F_GETFL, 0);
        if (flags == -1) {
            throw -1;
        }

        if (fcntl(newFd, F_SETFL, flags | O_NONBLOCK) == -1) {
            throw -1;
        }
    } catch (int e) {
        std::perror("");
        if (newFd != -1) {
            close(newFd);
        }
        return -1;
    }

    return newFd;
}

int LiveGrapher::ReadPackets(SocketConnection* conn) {
    int error;
    uint8_t id;

    error = conn->recvData(reinterpret_cast<char*>(&id), 1);
    if (error == 0 || (error == -1 && errno != EAGAIN)) {
        return -1;
    }

    switch (packetID(id)) {
        case kHostConnectPacket:
            // Start sending data for the graph specified by the ID
            if (std::find(conn->dataSets.begin(), conn->dataSets.end(),
                          graphID(id)) == conn->dataSets.end()) {
                conn->dataSets.push_back(graphID(id));
            }
            break;
        case kHostDisconnectPacket:
            // Stop sending data for the graph specified by the ID
            conn->dataSets.erase(std::remove(conn->dataSets.begin(),
                                             conn->dataSets.end(), graphID(id)),
                                 conn->dataSets.end());
            break;
        case kHostListPacket:
            /* A graph count is compared against instead of the graph ID for
             * terminating list traversal because the std::map is sorted by
             * graph name instead of the ID. Since, the IDs are not necessarily
             * in order, early traversal termination could occur.
             */
            size_t graphCount = 0;
            for (auto& graph : m_graphList) {
                if (m_buf.length() < 1 + 1 + graph.first.length() + 1) {
                    m_buf.resize(1 + 1 + graph.first.length() + 1);
                }

                m_buf[0] = kClientListPacket | graph.second;
                m_buf[1] = graph.first.length();
                std::strncpy(&m_buf[2], graph.first.c_str(),
                             graph.first.length());

                // Is this the last element in the list?
                if (graphCount + 1 == m_graphList.size()) {
                    m_buf[2 + m_buf[1]] = 1;
                } else {
                    m_buf[2 + m_buf[1]] = 0;
                }

                // Queue the datagram for writing
                conn->queueWrite(m_buf.c_str(),
                                 1 + 1 + graph.first.length() + 1);

                graphCount++;
            }
            break;
    }

    return 0;
}
