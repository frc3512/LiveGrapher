// Copyright (c) FRC Team 3512, Spartatroniks 2013-2017. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <QColor>
#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>

#include "../common/Protocol.hpp"
#include "Settings.hpp"

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

    // Kills receiving thread
    void disconnect();

    // Returns whether client is connected to graph host
    bool isConnected() const;

    // Add data point to graph at given index (push back)
    void addData(unsigned int index, const std::pair<float, float>&& point);

    // Removes all previous data from all graphs
    void clearAllData();

    // Create another set of data to graph
    void createGraph(const std::string& name, QColor color);

    // Remove graph at the given index
    void removeGraph(unsigned int index);

public slots:
    // Saves a screenshot of the current graph window in PNG format
    bool screenshotGraph();

    /* Saves all graph data to CSV in the executable's directory. Returns true
     * upon success.
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
    typedef std::vector<std::pair<float, float>> DataSet;
    std::vector<DataSet> m_dataSets;

    // Contains names for all graphs available on host
    std::map<uint8_t, std::string> m_graphNames;

    // Each bit holds receive state of data set (1 = recv, 0 = not recv)
    uint64_t m_curSelect = 0;

    std::unique_ptr<QTcpSocket> m_dataSocket;

    QHostAddress m_remoteIP;
    uint16_t m_dataPort;

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

    // Generates unique name for a file based on the current time
    std::string generateFileName();

    friend class SelectDialog;
};
