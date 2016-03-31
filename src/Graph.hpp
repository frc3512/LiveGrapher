#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <cstdint>
#include <string>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <QColor>
#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>

#include "Settings.hpp"

typedef std::vector<std::pair<float, float>> DataSet;

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

constexpr uint8_t k_inDataPacket = 0b00 << 6;
constexpr uint8_t k_inListPacket = 0b01 << 6;

enum class ReceiveState {
    ID,
    Data,
    NameLength,
    Name,
    EndOfFile,
    DataComplete,
    ListComplete
};

class MainWindow;
class SelectDialog;

class Graph : public QObject {
    Q_OBJECT

public:
    explicit Graph(MainWindow* parentWindow);
    virtual ~Graph() = default;

    // Kills receiving thread and restarts it; this function will block
    void reconnect();

    // Add data point to graph at given index (push back)
    void addData(unsigned int index, const std::pair<float, float>&& point);

    // Removes all previous data from all graphs
    void clearAllData();

    // Create another set of data to graph
    void createGraph(const std::string& name, QColor color);

    // Remove graph at the given index
    void removeGraph(unsigned int index);

public slots:
    /* Saves all graph data to CSV in the executable's directory
     * returns true upon success
     */
    bool saveAsCSV();

signals:
    void realtimeDataSignal(int graphId, float x, float y);

private slots:
    void handleSocketData();
    void sendGraphChoices();

private:
    MainWindow* m_window;

    Settings m_settings{"IPSettings.txt"};

    // Contains graph data to plot
    std::vector<DataSet> m_dataSets;

    // Contains names for all graphs available on host
    std::vector<std::pair<uint8_t, std::string>> m_graphNames;

    // Each bit holds receive state of data set (1 = recv, 0 = not recv)
    uint64_t m_curSelect = 0;

    std::unique_ptr<QTcpSocket> m_dataSocket;

    QHostAddress m_remoteIP;
    unsigned short m_dataPort;

    uint64_t m_startTime = 0;

    HostPacket m_hostPacket;
    ClientDataPacket m_clientDataPacket;
    ClientListPacket m_clientListPacket;
    ReceiveState m_state = ReceiveState::ID;

    // Used for endianness conversions
    uint64_t xtmp;
    uint32_t ytmp;

    /* Sends block of data to host. Returns 'true' on success or 'false' on
     * failure
     */
    bool sendData(void* data, size_t length);

    /* Receives block of data from host. This will block if not enough data is
     * available to read.
     */
    bool recvData(void* data, size_t length);

    static inline uint8_t packetID(uint8_t id);
    static inline uint8_t graphID(uint8_t id);

    friend class SelectDialog;
};

#endif // GRAPH_HPP
