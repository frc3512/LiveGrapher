//=============================================================================
//File Name: Graph.hpp
//Description: Manages a graph in Qt
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "Settings.hpp"
#include <list>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>

#include <QObject>
#include <QColor>
#include <QHostAddress>

class QTcpSocket;

typedef std::pair<float , float> Pair;
typedef std::list<Pair> DataSet;

/* Sending packets:
 * 'c': Asks host to start sending data set of given name
 * 'd': Asks host to stop sending data set of given name
 *
 * Note: Only the first 16 bytes of the packet are used for 'c' and 'd' packets.
 *       The rest are padding.
 *
 * 'l': Asks host to send list of names of available data sets
 *
 * Note: Only the first byte is used by remote host in 'l' packet. There are 15
 *       more bytes sent in that packet type, which are padding.
 *       padding.
 *
 * Receiving packets:
 * 'd': Contains point of data from given data set
 */
struct [[gnu::packed]] packet_t {
    char id;
    char graphName[15];
    uint64_t x;
    float y;
};

/* Receiving packets:
 * 'l': Contains name of data set on host
 */
struct [[gnu::packed]] packet_list_t {
    char id;
    char graphName[15];
    char eof;
    char padding[11];
};

static_assert( sizeof(struct packet_t) == sizeof(struct packet_list_t) ,
               "packet structs are not same size" );

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
    void createGraph( const std::string& name , QColor color );

    // Remove graph at the given index
    void removeGraph( unsigned int index );

public slots:
    /* Saves all graph data to CSV in the executable's directory
     * returns true upon success
     */
    bool saveAsCSV();

signals:
    void realtimeDataSignal( int graphId , float x , float y );

private slots:
    void handleSocketData();
    void sendGraphChoices();

private:
    MainWindow* m_window;

    Settings m_settings;

    // Contains graph data to plot
    std::vector<DataSet> m_dataSets;

    // Contains names for all graphs available on host
    std::vector<std::string> m_graphNames;

    // Each bit holds receive state of data set (1 = recv, 0 = not recv)
    uint64_t m_curSelect;

    // Provides way to get a data set's index given the name
    std::map<std::string , unsigned char> m_graphNamesMap;

    QTcpSocket* m_dataSocket;

    QHostAddress m_remoteIP;
    unsigned short m_dataPort;

    uint64_t m_startTime;

    char m_buffer[sizeof(struct packet_t)];
    struct packet_t m_recvData;
    struct packet_list_t m_listData;

    friend class SelectDialog;
};

#endif // GRAPH_HPP
