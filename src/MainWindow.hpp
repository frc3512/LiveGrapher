// Copyright (c) 2013-2017 FRC Team 3512. All Rights Reserved.

#pragma once

#include <chrono>
#include <memory>
#include <mutex>

#include <QMainWindow>

#include "Graph.hpp"
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void reconnect();
    void realtimeDataSlot(int graphId, float x, float y);

private:
    std::unique_ptr<Ui::MainWindow> m_ui;

    Graph m_graph;
    bool m_isPlaying = false;

    Settings m_settings;
    double m_xHistory;

    // Used to limit rate of recalculation of graph range
    std::chrono::steady_clock::time_point m_lastTime;

    friend class Graph;
};
