// Copyright (c) FRC Team 3512, Spartatroniks 2013-2016. All Rights Reserved.

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

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

public slots:
    void infoDialog(const QString& title, const QString& text);
    void criticalDialog(const QString& title, const QString& text);

private slots:
    void about();
    void reconnect();
    void clearAllData();
    void realtimeDataSlot(int graphId, float x, float y);

private:
    std::unique_ptr<Ui::MainWindow> m_ui;

    Graph m_graph;

    Settings m_settings;
    double m_xHistory;

    // Used to limit rate of recalculation of graph range
    std::chrono::steady_clock::time_point m_lastTime;

    friend class Graph;
};

#endif  // MAIN_WINDOW_HPP
