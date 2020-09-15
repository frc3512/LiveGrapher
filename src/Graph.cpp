// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "Graph.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <QMessageBox>
#include <QtEndian>

#include "MainWindow.hpp"
#include "SelectDialog.hpp"
#include "ui_MainWindow.h"

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
    m_datasets.reserve(64);

    connect(&m_dataSocket, SIGNAL(readyRead()), this, SLOT(HandleSocketData()));
}

void Graph::Reconnect() {
    // Reset list of dataset recv statuses
    m_curSelect = 0;

    // Attempt connection to remote dataset host
    if (m_dataSocket.state() != QAbstractSocket::ConnectedState) {
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
        m_dataSocket.disconnect();
        m_startTime = 0;
        return;
    }
}

void Graph::Disconnect() { m_dataSocket.disconnectFromHost(); }

bool Graph::IsConnected() const {
    return m_dataSocket.state() == QAbstractSocket::ConnectedState;
}

void Graph::AddData(uint32_t index, const std::pair<float, float>&& data) {
    m_datasets[index].push_back(data);
    m_window.AddData(index, data.first, data.second);
}

void Graph::ClearAllData() {
    for (auto set : m_datasets) {
        set.clear();
    }

    for (int i = 0; i < m_window.m_ui.plot->graphCount(); i++) {
        m_window.m_ui.plot->graph(i)->clearData();
    }
}

void Graph::CreateGraph(const std::string& name, QColor color) {
    if (name.length() > 255) {
        std::cerr << "LiveGrapher: Graph::CreateGraph(): "
                  << "name exceeds 255 characters\n";
        return;
    }
    m_datasets.emplace_back();

    QCustomPlot* customPlot = m_window.m_ui.plot;
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

    m_window.m_ui.plot->removeGraph(index);
    m_window.m_ui.plot->legend->removeItem(index);
}

bool Graph::ScreenshotGraph() {
    // There isn't any point in creating a file with no data in it
    if (m_datasets.size() == 0) {
        QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                              QObject::tr("No graphs exist"));
        return false;
    } else {
        bool success = m_window.m_ui.plot->savePng(
            QString::fromStdString("./" + GenerateFileName() + ".png"));

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
    // There isn't any point in creating a file with no data in it
    if (m_datasets.size() == 0) {
        QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                              QObject::tr("No graphs exist"));
        return false;
    }

    std::ofstream saveFile(GenerateFileName() + ".csv", std::ofstream::trunc);

    if (!saveFile.is_open()) {
        QMessageBox::critical(&m_window, QObject::tr("Save Data"),
                              QObject::tr("Open CSV file failed"));
        return false;
    }

    // Tracks positions in each dataset; start at 0 for each
    std::vector<size_t> rowIdx{m_datasets.size(), 0};

    // X axis label
    saveFile << "Time (s),";

    // Write axis and data labels to file
    for (size_t i = 0; i < m_datasets.size(); ++i) {
        saveFile << m_graphNames[i];

        // If last Y axis label hasn't been written yet
        if (i + 1 < m_datasets.size()) {
            saveFile << ',';
        }
    }
    saveFile << '\n';

    // While there is still data in at least one dataset to add to the file
    size_t row = 0;
    bool haveData = true;
    while (haveData) {
        haveData = false;

        // Only write X values of first dataset since X values of all
        // datasets are identical
        saveFile << m_datasets[0][row].first << ',';

        for (size_t col = 0; col < m_datasets.size(); ++col) {
            // If there are still points in this dataset to add
            if (row < m_datasets[col].size()) {
                saveFile << m_datasets[col][row].second;
                haveData = true;
            }

            // Only add another comma if there is more data to add
            if (col < m_datasets.size() - 1) {
                saveFile << ',';
            }
        }

        saveFile << '\n';

        ++row;
    }

    saveFile.close();

    QMessageBox::information(&m_window, QObject::tr("Save Data"),
                             QObject::tr("Export to CSV successful"));
    return true;
}

void Graph::HandleSocketData() {
    bool recvSuccess = true;
    bool continueReceive = true;

    while (continueReceive) {
        if (m_state == ReceiveState::ID) {
            if (m_dataSocket.bytesAvailable() >= 1) {
                char id;
                recvSuccess = RecvData(&id, 1);
                if (recvSuccess) {
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
                }
            } else {
                continueReceive = false;
            }
        } else if (m_state == ReceiveState::Data) {
            if (static_cast<quint64>(m_dataSocket.bytesAvailable()) >=
                sizeof(m_clientDataPacket.x) + sizeof(m_clientDataPacket.y)) {
                recvSuccess = RecvData(&m_clientDataPacket.x,
                                       sizeof(m_clientDataPacket.x));

                if (recvSuccess) {
                    recvSuccess = RecvData(&m_clientDataPacket.y,
                                           sizeof(m_clientDataPacket.y));
                    if (recvSuccess) {
                        m_state = ReceiveState::DataComplete;
                    }
                }
            } else {
                continueReceive = false;
            }
        } else if (m_state == ReceiveState::NameLength) {
            if (m_dataSocket.bytesAvailable() >= 1) {
                recvSuccess = RecvData(&m_clientListPacket.length,
                                       sizeof(m_clientListPacket.length));
                if (recvSuccess) {
                    m_clientListPacket.name.resize(m_clientListPacket.length);
                    m_state = ReceiveState::Name;
                }
            } else {
                continueReceive = false;
            }
        } else if (m_state == ReceiveState::Name) {
            if (m_dataSocket.bytesAvailable() >= m_clientListPacket.length) {
                recvSuccess = RecvData(&m_clientListPacket.name[0],
                                       m_clientListPacket.length);
                if (recvSuccess) {
                    m_state = ReceiveState::EndOfFile;
                }
            } else {
                continueReceive = false;
            }
        } else if (m_state == ReceiveState::EndOfFile) {
            if (m_dataSocket.bytesAvailable() >= 1) {
                recvSuccess = RecvData(&m_clientListPacket.eof, 1);
                if (recvSuccess) {
                    m_state = ReceiveState::ListComplete;
                }
            } else {
                continueReceive = false;
            }
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
            AddData(GraphID(m_clientDataPacket.ID), std::pair{x / 1000.f, y});
            /* ========================================= */

            m_state = ReceiveState::ID;
        } else if (m_state == ReceiveState::ListComplete) {
            m_graphNames[GraphID(m_clientListPacket.ID)] =
                m_clientListPacket.name;

            // If that was the last name, exit the recv loop
            if (m_clientListPacket.eof == 1) {
                // Allow user to select which datasets to receive
                auto dialog = new SelectDialog(m_graphNames, this, &m_window);
                connect(dialog, SIGNAL(finished(int)), this,
                        SLOT(SendGraphChoices()));
                dialog->open();
            }

            m_state = ReceiveState::ID;
        }
    }

    if (!recvSuccess) {
        QMessageBox::critical(
            &m_window, QObject::tr("Connection Error"),
            QObject::tr("Receiving data from remote host failed"));
        m_dataSocket.disconnect();
        m_startTime = 0;
        m_state = ReceiveState::ID;
    }
}

void Graph::SendGraphChoices() {
    // If true, graphs aren't created yet
    bool makeGraphs = m_window.m_ui.plot->graphCount() == 0;

    /* Send updated status on streams to which to connect based on the bit
     * array
     */
    for (uint32_t i = 0; i < m_graphNames.size(); i++) {
        // If the graph data is requested
        if (m_curSelect & (1 << i)) {
            m_hostPacket.ID = k_hostConnectPacket | i;
        } else {
            // Tell server to stop sending stream
            m_hostPacket.ID = k_hostDisconnectPacket | i;
        }

        if (!SendData({reinterpret_cast<char*>(&m_hostPacket),
                       sizeof(m_hostPacket)})) {
            QMessageBox::critical(
                &m_window, QObject::tr("Connection Error"),
                QObject::tr("Sending graph choices to remote host failed"));
            m_dataSocket.disconnect();
            m_startTime = 0;
        }

        /* Create a graph for each dataset requested.
         *
         * V starts out at 1. H cycles through 12 colors (roughly the rainbow).
         * When it wraps around 360 degrees, V is decremented by 0.5. S is
         * always 1. This algorithm gives 25 possible values, one being black.
         */
        if (makeGraphs) {
            constexpr uint32_t parts = 12;
            CreateGraph(m_graphNames[i],
                        HSVtoRGB(360 / parts * i % 360, 1,
                                 1 - 0.5 * std::floor(i / parts)));
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

std::string Graph::GenerateFileName() {
    using days = std::chrono::duration<
        int,
        std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type>;
    auto tp = std::chrono::system_clock::now().time_since_epoch();
    auto d = std::chrono::duration_cast<days>(tp);
    tp -= d;
    auto h = std::chrono::duration_cast<std::chrono::hours>(tp);
    tp -= h;
    auto m = std::chrono::duration_cast<std::chrono::minutes>(tp);
    tp -= m;
    auto s = std::chrono::duration_cast<std::chrono::seconds>(tp);
    tp -= s;

    std::stringstream ss;
    ss << "Graph-" << d.count() << "-T" << h.count() << "." << m.count() << "."
       << s.count();

    return ss.str();
}
