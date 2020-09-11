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
 * Call GraphData() to send data over the network to a LiveGrapher client.
 *
 * The time value in each data pair is handled internally.
 *
 * Use the function HasIntervalPassed() to limit the frequency of data sending
 * in looping situations.
 *
 * Example:
 *     LiveGrapher pidGraph(3513);
 *
 *     void TeleopPeriodic() override {
 *         pidGraph.GraphData(frisbeeShooter.getRPM(), "PID0");
 *         pidGraph.GraphData(frisbeeShooter.getTargetRPM(), "PID1");
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
     * @param value   The y value.
     * @param dataset The name of the dataset to which the value belongs.
     */
    void GraphData(float value, const std::string& dataset);

private:
    std::thread m_thread;
    std::mutex m_mutex;
    int m_listenfd;
    int m_ipcfd_r;
    int m_ipcfd_w;

    /* Sorted by graph name instead of ID because the user passes in a string.
     * (They don't know the ID.) This makes graph ID lookups take O(log n).
     */
    std::map<std::string, uint8_t> m_graphList;

    std::vector<std::unique_ptr<SocketConnection>> m_connList;

    // Temporary buffer used in ReadPackets()
    std::string m_buf;

    static uint8_t packetID(uint8_t id);
    static uint8_t graphID(uint8_t id);

    void socket_threadmain();

    static int socket_listen(int port, uint32_t s_addr);
    static int socket_accept(int listenfd);

    int ReadPackets(SocketConnection* conn);
};
