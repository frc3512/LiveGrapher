// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <chrono>

#include <QMainWindow>

#include "Graph.hpp"
#include "qcustomplot.h"

class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QCustomPlot* plot;

    Graph m_graph{this};
    bool m_isPlaying = false;

    Settings m_settings{"IPSettings.txt"};
    double m_xHistory = m_settings.getDouble("xHistory");

    // Used to limit rate of recalculation of graph range
    std::chrono::steady_clock::time_point m_lastTime =
        std::chrono::steady_clock::now();

    void AddData(int graphId, float x, float y);

    friend class Graph;
};
