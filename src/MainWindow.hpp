#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <mutex>
#include <memory>
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

    friend class Graph;
};

#endif // MAIN_WINDOW_HPP
