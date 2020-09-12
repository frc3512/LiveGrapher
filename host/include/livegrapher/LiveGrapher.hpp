// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "livegrapher/Protocol.hpp"
#include "livegrapher/SocketConnection.hpp"

/**
 * The host for the LiveGrapher real-time graphing application.
 *
 * Usage:
 *
 * The LiveGrapher interface is started upon object initialization.
 *
 * Call AddData() to send data over the network to a LiveGrapher client.
 *
 * The time value in each data pair is handled internally.
 *
 * Example:
 *     LiveGrapher grapher{3513};
 *
 *     void TeleopPeriodic() override {
 *         grapher.GraphData("PID0", frisbeeShooter.getRPM());
 *         grapher.GraphData("PID1", frisbeeShooter.getTargetRPM());
 *     }
 */
class LiveGrapher {
public:
    explicit LiveGrapher(int port);
    ~LiveGrapher();

    /**
     * Send data (y value) for a given dataset to remote client.
     *
     * The current time is sent as the x value.
     *
     * @param dataset The name of the dataset to which the value belongs.
     * @param value   The y value.
     */
    void AddData(const std::string& dataset, float value);

private:
    std::thread m_thread;
    std::mutex m_mutex;
    int m_listenFd;
    int m_ipcFdReader;
    int m_ipcFdWriter;

    /* Sorted by graph name instead of ID because the user passes in a string.
     * (They don't know the ID.) This makes graph ID lookups take O(log n).
     */
    std::map<std::string, uint8_t> m_graphList;

    std::vector<std::unique_ptr<SocketConnection>> m_connList;

    // Temporary buffer used in ReadPackets()
    std::string m_buf;

    static constexpr uint8_t packetID(uint8_t id) {
        // Masks two high-order bits
        return id & 0xC0;
    }

    static constexpr uint8_t graphID(uint8_t id) {
        // Masks six low-order bits
        return id & 0x2F;
    }

    void ThreadMain();

    /**
     * Listen for new connection on the given port and return the file
     * descriptor of the listening socket..
     *
     * @param port       Port on which to listen
     * @param sourceAddr Source address on which to listen
     */
    static int Listen(int port, uint32_t sourceAddr);

    /**
     * Accept new connection from the given listener socket file descriptor.
     *
     * @param listenFd The listener socket file descriptor.
     * @return The new connection's file descriptor.
     */
    static int Accept(int listenFd);

    int ReadPackets(SocketConnection* conn);
};
