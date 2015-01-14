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

public slots:
    void infoDialog( const QString& title , const QString& text );
    void criticalDialog( const QString& title , const QString& text );

private slots:
    void about();
    void reconnect();
    void clearAllData();
    void realtimeDataSlot( int graphId , float x , float y );

private:
    Ui::MainWindow* m_ui;

    Graph m_graph;

    Settings m_settings;
    float m_xHistory;

    friend class Graph;
};

#endif // MAIN_WINDOW_HPP
