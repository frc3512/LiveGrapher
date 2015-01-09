//=============================================================================
//File Name: Graph.hpp
//Description: Draws a graph in WinGDI
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "Settings.hpp"
#include <list>
#include <utility>
#include <map>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <cstdint>

#include <SFML/Network/SocketSelector.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>

#include <QObject>
#include <QColor>

typedef std::pair<float , float> Pair;
typedef std::list<Pair> DataSet;

/* Graph Packet Structure
 *
 * Sending packets:
 *     id can be one of three values with its respective packet structure:
 *
 *     'c': Asks host to start sending data set of given name
 *     struct packet_t {
 *         char id;
 *         char graphName[15];
 *     }
 *
 *     'd': Asks host to stop sending data set of given name
 *     struct packet_t {
 *         char id;
 *         char graphName[15];
 *     }
 *
 *     'l': Asks host to send list of names of available data sets
 *     struct packet_t {
 *         char id;
 *     }
 *
 * Receiving packets:
 *     id can be one of two values with its respective packet structure:
 *
 *     'l': Contains name of data set on host
 *     struct packet_t {
 *         char id;
 *         char graphName[15];
 *     }
 *
 *     'p': Contains point of data from given data set
 *     struct packet_t {
 *         char id;
 *         char graphName[15];
 *         float x;
 *         float y;
 *     }
 */
struct [[gnu::packed]] packet_t {
    char id;
    char graphName[15];
    float x;
    float y;
};

struct [[gnu::packed]] packet_list_t {
    char id;
    char graphName[15];
    char eof;
    char padding[7];
};

class MainWindow;
class SelectDialog;

class Graph : public QObject {
    Q_OBJECT
public:
    explicit Graph( MainWindow* parentWindow );
    virtual ~Graph();

    // Kills receiving thread and restarts it; this function will block
    void reconnect();

    // Add data point to graph at given index (push back)
    void addData( unsigned int index , const Pair& point );

    // Removes all previous data from all graphs
    void clearAllData();

    // Create another set of data to graph
    void createGraph( QColor color );

    // Remove graph at the given index
    void removeGraph( unsigned int index );

public slots:
    /* Saves all graph data to CSV in the executable's directory
     * returns true upon success
     */
    bool saveAsCSV();

signals:
    void updateUi( int graphId , float x , float y );

private:
    MainWindow* m_window;

    Settings m_settings;

    // Contains graph data to plot
    std::vector<DataSet> m_dataSets;

    std::mutex m_dataMutex;

    // Contains names for all graphs available on host
    std::vector<std::string> m_graphNames;

    // Each bit holds receive state of data set (1 = recv, 0 = not recv)
    uint64_t m_curSelect;

    // Provides way to get a data set's index given the name
    std::map<std::string , unsigned char> m_graphNamesMap;

    std::thread* m_graphThread;
    std::atomic<bool> m_isRunning;
    void graphThreadFunc();

    enum Error {
        FailConnect = 0,
        Disconnected
    };

    sf::SocketSelector dataSelector;

    sf::TcpSocket dataSocket;

    sf::IpAddress remoteIP;
    unsigned short dataPort;

    sf::Socket::Status status;

    char buffer[24];
    struct packet_t recvData;
    struct packet_list_t listData;
    size_t sizeRecv;

    friend class SelectDialog;
};

#endif // GRAPH_HPP
