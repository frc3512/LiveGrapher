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

/* '0b00' type outbound packet
 *   > uint8_t packetID : 2
 *    '0b00': Asks host to start sending data set of given name
 *   > uint8_t graphID : 6
 *     Contains ID of graph
 *
 * '0b01' type outbound packet
 *   > uint8_t packetID : 2
 *     '0b01': Asks host to stop sending data set of given name
 *   > uint8_t graphID : 6
 *     Contains ID of graph
 *
 * '0b10' type outbound packet
 *   > uint8_t packetID : 2
 *     '0b10': Asks host to send list of names of available data sets
 *   > uint8_t graphID : 6
 *     Not used. Should be set to 0
 */
struct [[gnu::packed]] InboundPacket {
    uint8_t ID;
};

constexpr uint8_t k_inConnectPacket = 0b00 << 6;
constexpr uint8_t k_inDisconnectPacket = 0b01 << 6;
constexpr uint8_t k_inListPacket = 0b10 << 6;

/* '0b00' type inbound packet
 *   > uint8_t packetID : 2
 *     '0b00': Contains point of data from given data set
 *   > uint8_t graphID : 6
 *     Contains ID of graph
 *   > uint64_t x
 *     X coordinate of graph point
 *   > float y
 *     Y coordinate of graph point
 *
 * '0b01' type inbound packet
 *   > uint8_t packetID : 2
 *     '0b01': Contains name of data set on host
 *   > uint8_t graphID : 6
 *     Contains ID of graph
 *   > uint8_t length
 *     Contains length of name. Max length is 255.
 *   > uint8_t name[]
 *     Contains name which is 'length' bytes long (not NULL terminated)
 *   > uint8_t eof
 *     1 if this graph name is the last; 0 otherwise
 */
struct [[gnu::packed]] OutboundDataPacket {
    uint8_t ID;
    uint64_t x;
    float y;
};

struct OutboundListPacket {
    uint8_t ID;
    uint8_t length;
    std::string name;
    char eof;
};

constexpr uint8_t k_outDataPacket = 0b00 << 6;
constexpr uint8_t k_outListPacket = 0b01 << 6;

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
