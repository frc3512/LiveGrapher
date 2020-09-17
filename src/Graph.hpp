// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <QColor>
#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>

#include "Protocol.hpp"
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

    /**
     * Kills receiving thread and restarts it.
     *
     * This function will block.
     */
    void Reconnect();

    /**
     * Kills receiving thread.
     */
    void Disconnect();

    /**
     * Returns true if a client is connected.
     */
    bool IsConnected() const;

    /**
     * Append data point to the graph located at the given index.
     *
     * @param index The index of the dataset.
     * @param x     The x value.
     * @param y     The y value.
     */
    void AddData(uint32_t index, float x, float y);

    /**
     * Removes all previous data from all graphs.
     */
    void ClearAllData();

    /**
     * Create another set of data to graph.
     *
     * @param name Name of new dataset.
     * @param color Color of dataset's graph line.
     */
    void CreateGraph(const std::string& name, QColor color);

    /**
     * Remove graph at the given index.
     *
     * @param index The index of the graph to remove.
     */
    void RemoveGraph(uint32_t index);

    /**
     * Removes all graphs.
     */
    void RemoveAllGraphs();

public slots:
    /**
     * Saves a screenshot of the current graph window in PNG format.
     *
     * @return True on success.
     */
    bool ScreenshotGraph();

    /**
     * Saves all graph data to CSV in the executable's directory.
     *
     * @return True on success.
     */
    bool SaveAsCSV();

private slots:
    void HandleSocketData();
    void SendGraphChoices();

private:
    MainWindow& m_window;

    Settings m_settings{"IPSettings.txt"};

    // Contains graph data to plot
    std::vector<std::map<float, float>> m_datasets;

    // Contains names for all graphs available on host
    std::map<uint8_t, std::string> m_graphNames;
    std::map<uint8_t, std::string> m_oldGraphNames;

    // Each bit holds receive state of data set (1 = recv, 0 = not recv)
    uint64_t m_curSelect = 0;

    QTcpSocket m_dataSocket{this};

    QHostAddress m_remoteIP{
        QHostAddress(QString::fromStdString(m_settings.getString("robotIP")))};
    uint16_t m_dataPort = m_settings.getInt("robotGraphPort");

    uint64_t m_startTime = 0;

    HostPacket m_hostPacket;
    ClientDataPacket m_clientDataPacket;
    ClientListPacket m_clientListPacket;
    ReceiveState m_state = ReceiveState::ID;

    /**
     * Sends block of data to host.
     *
     * @param buf Buffer containing data to send.
     * @return True on success.
     */
    bool SendData(std::string_view buf);

    /**
     * Receives block of data from host.
     *
     * This will block if not enough data is available to read.
     *
     * @param data   Buffer in which to store received data.
     * @param length Length of buffer.
     */
    bool RecvData(void* data, size_t length);

    /**
     * Extract the packet type from the ID field of a received client packet.
     *
     * @param id The client packet ID field.
     */
    static constexpr uint8_t PacketType(uint8_t id) {
        // Masks two high-order bits
        return id & 0xC0;
    }

    /**
     * Extract the graph ID from a host connect or disconnect packet's ID field.
     *
     * @param id The client packet ID field.
     */
    static constexpr uint8_t GraphID(uint8_t id) {
        // Masks six low-order bits
        return id & 0x2F;
    }

    /**
     * Generates unique name for a file based on the current time.
     */
    std::string GenerateFilename();

    friend class SelectDialog;
};
