// Copyright (c) 2013-2017 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Protocol.hpp"
#include "SocketConnection.hpp"

/**
 * The host for the LiveGrapher real-time graphing application.
 *
 * Usage:
 *
 * The GraphHost interface is started upon object initialization.
 *
 * Call graphData() to send data over the network to a LiveGrapher client.
 *
 * The time value in each data pair is handled internally.
 *
 * Use the function hasIntervalPassed() to limit the frequency of data sending
 * in looping situations.
 *
 * Example:
 *     GraphHost pidGraph(3513);
 *     pidGraph.SetSendInterval(5ms);
 *
 *     while (IsOperatorControl() && IsEnabled()) {
 *         if (pidGraph.HasIntervalPassed()) {
 *             pidGraph.GraphData(frisbeeShooter.getRPM(), "PID0");
 *             pidGraph.GraphData(frisbeeShooter.getTargetRPM(), "PID1");
 *
 *             pidGraph.ResetInterval();
 *         }
 *     }
 */
class GraphHost {
public:
    explicit GraphHost(int port);
    ~GraphHost();

    /* Send data (y value) for a given dataset to remote client. The current
     * time is sent as the x value. Returns true if data was sent successfully
     * and false upon failure or host isn't running.
     */
    bool GraphData(float value, std::string dataset);

    /* Sets time interval after which data is sent to graph (milliseconds per
     * sample)
     */
    template <typename Rep, typename Period>
    void SetSendInterval(const std::chrono::duration<Rep, Period>& time);

    /* Returns true if the time between the last data transmission is greater
     * than the sending interval time
     */
    bool HasIntervalPassed();

    /* Resets time interval passed since last data transmission (makes
     * hasIntervalPassed() return false)
     */
    void ResetInterval();

private:
    // Last time data was graphed
    uint64_t m_lastTime = 0;

    /* Time interval after which data is sent to graph (in milliseconds per
     * sample)
     */
    uint32_t m_sendInterval = 5;

    // Used as a temp variable in graphData()
    uint64_t m_currentTime;

    // Mark the thread as not running, this will be set to true by the thread
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    int m_ipcfd_r;
    int m_ipcfd_w;
    int m_port;

    /* Sorted by graph name instead of ID because the user passes in a string.
     * (They don't know the ID.) This makes graph ID lookups take O(log n).
     */
    std::map<std::string, uint8_t> m_graphList;

    std::vector<std::unique_ptr<SocketConnection>> m_connList;

    // Temporary buffer used in ReadPackets()
    std::string m_buf;

    static inline uint8_t packetID(uint8_t id);
    static inline uint8_t graphID(uint8_t id);

    void socket_threadmain();

    static int socket_listen(int port, uint32_t s_addr);
    static int socket_accept(int listenfd);

    int ReadPackets(SocketConnection* conn);
};

#include "GraphHost.inl"
