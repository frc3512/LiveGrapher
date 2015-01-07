#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <mutex>
#include "Graph.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow( QWidget* parent = nullptr );
    ~MainWindow();

private slots:
    void saveAsCSV();
    void about();
    void reconnect();
    void clearAllData();
    void realtimeDataSlot( int graphId , float x , float y );

private:
    Ui::MainWindow* m_ui;
    std::mutex m_uiMutex;

    Graph m_graph;

    friend class Graph;
};

#endif // MAIN_WINDOW_HPP
