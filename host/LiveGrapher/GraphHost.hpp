// =============================================================================
// Description: The host for the LiveGrapher real-time graphing application
// Author: FRC Team 3512, Spartatroniks
// =============================================================================

#ifndef GRAPHHOST_HPP
#define GRAPHHOST_HPP

#include <cstdint>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "SocketConnection.hpp"

/* Usage:
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
 *     if (pidGraph.HasIntervalPassed()) {
 *         pidGraph.GraphData(frisbeeShooter.getRPM(), "PID0");
 *         pidGraph.GraphData(frisbeeShooter.getTargetRPM(), "PID1");
 *
 *         pidGraph.ResetInterval();
 *     }
 */

using namespace std::chrono;
using namespace std::chrono_literals;

/* See README.md in the root directory of this project for protocol
 * documentation.
 */

struct[[gnu::packed]] HostPacket{
    uint8_t ID;
};

constexpr uint8_t k_hostConnectPacket = 0b00 << 6;
constexpr uint8_t k_hostDisconnectPacket = 0b01 << 6;
constexpr uint8_t k_hostListPacket = 0b10 << 6;

struct[[gnu::packed]] ClientDataPacket{
    uint8_t ID;
    uint64_t x;
    float y;
};

struct ClientListPacket {
    uint8_t ID;
    uint8_t length;
    std::string name;
    uint8_t eof;
};

constexpr uint8_t k_clientDataPacket = 0b00 << 6;
constexpr uint8_t k_clientListPacket = 0b01 << 6;

class GraphHost {
public:
    explicit GraphHost(int port);
    ~GraphHost();

    /* Send data (y value) for a given dataset to remote client. The current time
     * is sent as the x value. Returns true if data was sent successfully and
     * false upon failure or host isn't running.
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

    // Time interval after which data is sent to graph (in milliseconds per sample)
    uint32_t m_sendInterval = 5;

    // Used as a temp variables in graphData(2)
    uint64_t m_currentTime;

    // Mark the thread as not running, this will be set to true by the thread
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    int m_ipcfd_r;
    int m_ipcfd_w;
    int m_port;
    std::map<std::string, uint8_t> m_graphList;
    std::vector<std::unique_ptr<SocketConnection>> m_connList;

    static inline uint8_t packetID(uint8_t id);
    static inline uint8_t graphID(uint8_t id);

    void socket_threadmain();

    static int socket_listen(int port, uint32_t s_addr);
    static int socket_accept(int listenfd);

    int ReadPackets(SocketConnection* conn);
};

#include "GraphHost.inl"

#endif // GRAPHHOST_HPP
