// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "Graph.hpp"

#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>

#include <QMessageBox>
#include <QtEndian>

#include "MainWindow.hpp"
#include "SelectDialog.hpp"

constexpr QRgb HSVtoRGB(float h, float s, float v) {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;

    float chroma = v * s;
    float hPrime = h / 60.f;
    float m = v - chroma;

    float modTemp = hPrime - static_cast<int>(hPrime);
    float x =
        chroma * (1.f - std::abs(static_cast<int>(hPrime) % 2 + modTemp - 1));

    if (0 <= hPrime && hPrime < 1) {
        r = chroma;
        g = x;
        b = 0;
    } else if (1 <= hPrime && hPrime < 2) {
        r = x;
        g = chroma;
        b = 0;
    } else if (2 <= hPrime && hPrime < 3) {
        r = 0;
        g = chroma;
        b = x;
    } else if (3 <= hPrime && hPrime < 4) {
        r = 0;
        g = x;
        b = chroma;
    } else if (4 <= hPrime && hPrime < 5) {
        r = x;
        g = 0;
        b = chroma;
    } else if (5 <= hPrime && hPrime < 6) {
        r = chroma;
        g = 0;
        b = x;
    } else {
        r = 0;
        g = 0;
        b = 0;
    }

    r += m;
    g += m;
    b += m;

    return qRgb(r * 255, g * 255, b * 255);
}

Graph::Graph(MainWindow* parentWindow)
    : QObject(parentWindow), m_window(*parentWindow) {
    connect(&m_dataSocket, SIGNAL(readyRead()), this, SLOT(HandleSocketData()));
}

void Graph::Reconnect() {
    // Clear the old list of graph names because a new set will be received
    m_graphNames.clear();

    // Attempt connection to remote dataset host
    if (m_dataSocket.state() != QAbstractSocket::ConnectedState) {
        std::cout << "Connecting to " << m_remoteIP.toString().toStdString()
                  << ":" << m_dataPort << std::endl;
        m_dataSocket.connectToHost(m_remoteIP, m_dataPort);

        if (!m_dataSocket.waitForConnected(500)) {
            QMessageBox::critical(
                &m_window, QObject::tr("Connection Error"),
                QObject::tr("Connection to remote host failed"));
            return;
        }
    }

    // Request list of all datasets on remote host

    // Populate outbound list packet
    m_hostPacket.ID = k_hostListPacket;

    if (!SendData({reinterpret_cast<char*>(&m_hostPacket.ID),
                   sizeof(m_hostPacket.ID)})) {
        QMessageBox::critical(
            &m_window, QObject::tr("Connection Error"),
            QObject::tr("Asking remote host for graph list failed"));
        m_dataSocket.disconnectFromHost();
        m_startTime = 0;
        return;
    }
}

void Graph::Disconnect() { m_dataSocket.disconnectFromHost(); }

bool Graph::IsConnected() const {
    return m_dataSocket.state() == QAbstractSocket::ConnectedState;
}

void Graph::AddData(uint32_t index, float x, float y) {
    auto& dataset = m_datasets[index];
    dataset.emplace_hint(dataset.end(), x, y);
    m_window.AddData(index, x, y);
}

void Graph::ClearAllData() {
    for (auto& dataset : m_datasets) {
        dataset.clear();
    }

    for (int i = 0; i < m_window.plot->graphCount(); ++i) {
        m_window.plot->graph(i)->clearData();
    }
}

void Graph::CreateGraph(const std::string& name, QColor color) {
    if (name.length() > 255) {
        std::cerr << "LiveGrapher: Graph::CreateGraph(): "
                  << "name exceeds 255 characters\n";
        return;
    }
    m_datasets.emplace_back();

    QCustomPlot* customPlot = m_window.plot;
    customPlot->addGraph();
    customPlot->graph()->setName(QString::fromUtf8(name.c_str()));
    customPlot->graph()->setAntialiasedFill(false);
    customPlot->setNoAntialiasingOnDrag(true);

    QPen pen(color);
    customPlot->graph()->setPen(pen);
}

void Graph::RemoveGraph(uint32_t index) {
    // Remove dataset
    m_datasets.erase(m_datasets.begin() + index);

    m_window.plot->removeGraph(index);
    m_window.plot->legend->removeItem(index);
}

void Graph::RemoveAllGraphs() {
    // Clear all graphs and the local dataset storage it represents
    m_window.plot->clearGraphs();
    m_datasets.clear();

    m_startTime = 0;
}

bool Graph::ScreenshotGraph() {
    // There isn't any point in creating a file with no data in it
    if (m_datasets.size() == 0) {
        QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                              QObject::tr("No graphs exist"));
        return false;
    } else {
        bool success = m_window.plot->savePng(
            QString::fromStdString(GenerateFilename() + ".png"));

        if (success) {
            QMessageBox::information(&m_window, QObject::tr("Save Data"),
                                     QObject::tr("Screenshot successful"));
            return true;
        } else {
            QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                                  QObject::tr("Screenshot failed"));
            return false;
        }
    }
}

bool Graph::SaveAsCSV() {
    // Make list of datasets that have data in them to export
    std::vector<size_t> plottedIdxs;
    for (size_t i = 0; i < m_datasets.size(); ++i) {
        if (m_datasets[i].size() > 0) {
            plottedIdxs.emplace_back(i);
        }
    }

    // There isn't any point in creating a file with no data in it
    if (plottedIdxs.size() == 0) {
        QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                              QObject::tr("No graphs exist"));
        return false;
    }

    std::ofstream saveFile{GenerateFilename() + ".csv", std::ofstream::trunc};

    if (!saveFile.is_open()) {
        QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                              QObject::tr("Failed to open CSV file"));
        return false;
    }

    // Write X axis label, then data labels
    saveFile << "Time (s)";
    for (size_t idx : plottedIdxs) {
        saveFile << ',' << m_graphNames[idx];
    }
    saveFile << '\n';

    // Collate all data in a format easier written to CSV. This converts a list
    // of datasets with x-y pairs into a list of x values which each have a list
    // of associated y values (one entry for each dataset that has an entry for
    // that x value).
    std::map<float, std::vector<std::optional<float>>> csvData;
    for (size_t i = 0; i < plottedIdxs.size(); ++i) {
        for (const auto& [time, value] : m_datasets[plottedIdxs[i]]) {
            auto& values = csvData[time];

            // If the list of values was just created because the time entry is
            // new, make room for all pending columns
            while (plottedIdxs.size() > values.size()) {
                values.emplace_back();
            }

            // Place value at approriate spot in list of values. An optional is
            // used so that empty cells are retained when generating the CSV.
            values[i] = std::optional{value};
        }
    }

    // Write out CSV data
    for (const auto& [time, values] : csvData) {
        saveFile << time;
        for (const auto& value : values) {
            saveFile << ',';
            if (value.has_value()) {
                saveFile << value.value();
            }
        }
        saveFile << '\n';
    }

    QMessageBox::information(&m_window, QObject::tr("Save Data"),
                             QObject::tr("Export to CSV successful"));
    return true;
}

void Graph::HandleSocketData() {
    auto reportFailure = [&] {
        QMessageBox::critical(
            &m_window, QObject::tr("Connection Error"),
            QObject::tr("Receiving data from remote host failed"));
        m_dataSocket.disconnectFromHost();
        m_startTime = 0;
        m_state = ReceiveState::ID;
    };

    while (1) {
        if (m_state == ReceiveState::ID) {
            if (m_dataSocket.bytesAvailable() == 0) {
                return;
            }

            char id;
            if (!RecvData(&id, 1)) {
                reportFailure();
                return;
            }

            // Check packet type
            switch (PacketType(id)) {
                case k_clientDataPacket:
                    m_clientDataPacket.ID = id;
                    m_state = ReceiveState::Data;
                    break;
                case k_clientListPacket:
                    m_clientListPacket.ID = id;
                    m_state = ReceiveState::NameLength;
                    break;
            }
        } else if (m_state == ReceiveState::Data) {
            if (static_cast<quint64>(m_dataSocket.bytesAvailable()) <
                sizeof(m_clientDataPacket.x) + sizeof(m_clientDataPacket.y)) {
                return;
            }

            if (!RecvData(&m_clientDataPacket.x,
                          sizeof(m_clientDataPacket.x))) {
                reportFailure();
                return;
            }

            if (!RecvData(&m_clientDataPacket.y,
                          sizeof(m_clientDataPacket.y))) {
                reportFailure();
                return;
            }

            m_state = ReceiveState::DataComplete;
        } else if (m_state == ReceiveState::NameLength) {
            if (m_dataSocket.bytesAvailable() == 0) {
                return;
            }

            if (!RecvData(&m_clientListPacket.length,
                          sizeof(m_clientListPacket.length))) {
                reportFailure();
                return;
            }

            m_clientListPacket.name.resize(m_clientListPacket.length);
            m_state = ReceiveState::Name;
        } else if (m_state == ReceiveState::Name) {
            if (m_dataSocket.bytesAvailable() < m_clientListPacket.length) {
                return;
            }

            if (!RecvData(&m_clientListPacket.name[0],
                          m_clientListPacket.length)) {
                reportFailure();
                return;
            }

            m_state = ReceiveState::EndOfFile;
        } else if (m_state == ReceiveState::EndOfFile) {
            if (m_dataSocket.bytesAvailable() == 0) {
                return;
            }

            if (!RecvData(&m_clientListPacket.eof, 1)) {
                reportFailure();
                return;
            }

            m_state = ReceiveState::ListComplete;
        } else if (m_state == ReceiveState::DataComplete) {
            /* ===== Add sent point to local graph ===== */
            // This will only work if ints are the same size as floats
            static_assert(sizeof(float) == sizeof(uint32_t),
                          "float isn't 32 bits long");

            // Convert endianness of x component
            uint64_t xtmp;
            std::memcpy(&xtmp, &m_clientDataPacket.x,
                        sizeof(m_clientDataPacket.x));
            xtmp = qFromBigEndian<qint64>(xtmp);
            std::memcpy(&m_clientDataPacket.x, &xtmp,
                        sizeof(m_clientDataPacket.x));

            // Convert endianness of y component
            uint64_t ytmp;
            std::memcpy(&ytmp, &m_clientDataPacket.y,
                        sizeof(m_clientDataPacket.y));
            ytmp = qFromBigEndian<qint32>(ytmp);
            std::memcpy(&m_clientDataPacket.y, &ytmp,
                        sizeof(m_clientDataPacket.y));

            // Set time offset based on remote clock
            if (m_startTime == 0) {
                m_startTime = m_clientDataPacket.x;
            }

            // Add data to appropriate dataset; store point in temps because
            // references can't be made of packed (unaligned) struct variables
            uint64_t x = m_clientDataPacket.x - m_startTime;
            float y = m_clientDataPacket.y;
            AddData(GraphID(m_clientDataPacket.ID), x / 1000.f, y);
            /* ========================================= */

            m_state = ReceiveState::ID;
        } else if (m_state == ReceiveState::ListComplete) {
            m_graphNames[GraphID(m_clientListPacket.ID)] =
                m_clientListPacket.name;

            // If that was the last name, exit the recv loop
            if (m_clientListPacket.eof == 1) {
                // If list of graph names changed, clear the checkbox states.
                // Otherwise, retain the old states for user convenience.
                if (m_oldGraphNames != m_graphNames) {
                    m_curSelect = 0;
                }

                // Allow user to select which datasets to receive
                auto dialog = new SelectDialog(m_graphNames, this, &m_window);
                connect(dialog, SIGNAL(finished(int)), this,
                        SLOT(SendGraphChoices()));
                dialog->open();
            }

            m_state = ReceiveState::ID;
        }
    }
}

void Graph::SendGraphChoices() {
    // If graph names changed, remake graphs. This also works on new connections
    // because the list of old graph names will be empty.
    if (m_oldGraphNames != m_graphNames) {
        // If old and new dataset names don't all match, remove all graphs and
        // create new ones because the old data and plots are no longer valid
        RemoveAllGraphs();

        m_oldGraphNames = m_graphNames;
    }

    // If true, graphs haven't been created yet
    bool createGraphs = m_window.plot->graphCount() == 0;

    // Send updated status on streams to which to connect based on the bit array
    for (uint32_t i = 0; i < m_graphNames.size(); ++i) {
        // If there are no graphs yet, create one for each dataset
        if (createGraphs) {
            // For HSV, V starts out at 1. H cycles through 12 colors (roughly
            // the rainbow). When it wraps around 360 degrees, V is decremented
            // by 0.5. S is always 1. This algorithm gives 25 possible values,
            // one being black.
            constexpr uint32_t parts = 12;
            CreateGraph(m_graphNames[i],
                        HSVtoRGB(360 / parts * i % 360, 1,
                                 1 - 0.5 * std::floor(i / parts)));
        }

        // Remove all graphs from legend so the requested ones are properly
        // ordered after reconnects
        m_window.plot->graph(i)->removeFromLegend();

        // If the graph data is requested
        if (m_curSelect & (1 << i)) {
            m_hostPacket.ID = k_hostConnectPacket | i;

            // Add dataset back to legend
            m_window.plot->graph(i)->addToLegend();
        } else {
            // Tell server to stop sending stream
            m_hostPacket.ID = k_hostDisconnectPacket | i;
        }

        if (!SendData({reinterpret_cast<char*>(&m_hostPacket),
                       sizeof(m_hostPacket)})) {
            QMessageBox::critical(
                &m_window, QObject::tr("Connection Error"),
                QObject::tr("Sending graph choices to remote host failed"));
            m_dataSocket.disconnectFromHost();
            m_startTime = 0;
        }
    }
}

bool Graph::SendData(std::string_view buf) {
    uint64_t count = 0;

    while (count < buf.length()) {
        int64_t sent =
            m_dataSocket.write(buf.data() + count, buf.length() - count);
        if (m_dataSocket.state() != QAbstractSocket::ConnectedState ||
            sent < 0) {
            return false;
        } else {
            count += sent;
        }
    }

    return true;
}

bool Graph::RecvData(void* data, size_t length) {
    uint64_t count = 0;
    int64_t received = 0;

    while (count < length) {
        received =
            m_dataSocket.read(reinterpret_cast<char*>(data), length - count);
        if (m_dataSocket.state() != QAbstractSocket::ConnectedState ||
            received < 0) {
            return false;
        } else {
            count += received;
        }
    }

    return true;
}

std::string Graph::GenerateFilename() {
    // Get the current date/time as a string. ISO 8601 format is roughly
    // YYYY-MM-DDTHH:mm:ss.
    auto time = std::time(nullptr);
    struct tm localTime = *std::localtime(&time);
    char datetime[80];
    std::strftime(datetime, sizeof(datetime), "%Y-%m-%dT%H_%M_%S", &localTime);

    return std::string{"Graph-"} + datetime;
}
