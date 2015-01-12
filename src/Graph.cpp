//=============================================================================
//File Name: Graph.cpp
//Description: Draws a graph in WinGDI
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#include <iostream>
#include <fstream>

#include <cmath>
#include <cstring>

#include "Graph.hpp"
#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "SelectDialog.hpp"

#include <QMessageBox>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

QColor HSVtoRGB( float h , float s , float v ) {
    float r;
    float g;
    float b;

    float chroma = v * s;
    float hPrime = h / 60.f;
    float m = v - chroma;

    float modTemp = hPrime - static_cast<int>(hPrime);
    float x = chroma * ( 1.f - std::fabs( static_cast<int>(hPrime) % 2 + modTemp - 1 ) );

    if ( 0 <= hPrime && hPrime < 1 ) {
        r = chroma;
        g = x;
        b = 0;
    }
    else if ( 1 <= hPrime && hPrime < 2 ) {
        r = x;
        g = chroma;
        b = 0;
    }
    else if ( 2 <= hPrime && hPrime < 3 ) {
        r = 0;
        g = chroma;
        b = x;
    }
    else if ( 3 <= hPrime && hPrime < 4 ) {
        r = 0;
        g = x;
        b = chroma;
    }
    else if ( 4 <= hPrime && hPrime < 5 ) {
        r = x;
        g = 0;
        b = chroma;
    }
    else if ( 5 <= hPrime && hPrime < 6 ) {
        r = chroma;
        g = 0;
        b = x;
    }
    else {
        r = 0;
        g = 0;
        b = 0;
    }

    r += m;
    g += m;
    b += m;

    return QColor( r * 255 , g * 255 , b * 255 );
}

Graph::Graph( MainWindow* parentWindow ) :
        QObject(parentWindow),
        m_settings( "IPSettings.txt" ) ,
        m_curSelect( 0 ) ,
        m_dataSocket( this ) {
    m_window = parentWindow;
    m_dataSets.reserve( 64 );
    m_graphNames.reserve( 64 );

    connect( &m_dataSocket , SIGNAL(readyRead()) , this , SLOT(handleSocketData()) ,
            Qt::QueuedConnection );
    connect( &m_dataSocket , SIGNAL(stateChanged(QAbstractSocket::SocketState)) ,
            this , SLOT(handleStateChange(QAbstractSocket::SocketState)) );
}

Graph::~Graph() {
    m_dataSocket.disconnect();
}

void Graph::reconnect() {
    m_dataSocket.disconnect();

    while ( m_dataSets.size() > 0 ) {
        removeGraph( m_dataSets.size() - 1 );
    }

    m_remoteIP = QString::fromUtf8( m_settings.getString( "robotIP" ).c_str() );
    m_dataPort = m_settings.getInt( "robotGraphPort" );

    std::memset( &m_recvData , 0 , sizeof(m_recvData) );
    std::memset( &m_listData , 0 ,sizeof(m_listData) );

    // Reset list of data set recv statuses
    m_curSelect = 0;

    try {
        // Attempt connection to remote data set host
        m_dataSocket.connectToHost( m_remoteIP , m_dataPort );

        if ( !m_dataSocket.isValid() ) {
            throw Error::FailConnect;
        }

        // Request list of all data sets on remote host
        m_recvData.id = 'l';
        int64_t count = 0;
        int64_t sent = 0;
        while ( count < 16 ) {
            sent = m_dataSocket.write( reinterpret_cast<char*>(&m_recvData) , 16 );
            if ( !m_dataSocket.isValid() || sent < 0 ) {
                throw Error::Disconnected;
            }
            else {
                count += sent;
            }
        }

        // Clear m_graphNames before refilling it
        m_graphNames.clear();
        m_graphNamesMap.clear();
    }
    catch ( Error& exception ) {
        if ( exception == Error::FailConnect ) {
            emit criticalDialogSignal( QObject::tr("Connection Error") , QObject::tr("Connection to remote host failed") );
        }
        else if ( exception == Error::Disconnected ) {
            emit criticalDialogSignal( QObject::tr("Connection Error") , QObject::tr("Unexpected disconnection from remote host") );
        }
    }
}

void Graph::addData( unsigned int index , const Pair& data ) {
    m_dataMutex.lock();
    m_dataSets[index].push_back( data );
    m_dataMutex.unlock();

    emit realtimeDataSignal( index , data.first , data.second );
}

void Graph::clearAllData() {
    m_dataMutex.lock();

    for ( auto set : m_dataSets ) {
        set.clear();
    }

    m_dataMutex.unlock();

    m_window->m_uiMutex.lock();

    for ( int i = 0 ; i < m_window->m_ui->plot->graphCount() ; i++ ) {
        m_window->m_ui->plot->graph( i )->clearData();
    }

    m_window->m_uiMutex.unlock();
}

void Graph::createGraph( const std::string& name , QColor color ) {
    m_dataMutex.lock();
    m_dataSets.push_back( DataSet() );
    m_dataMutex.unlock();

    m_window->m_uiMutex.lock();

    QCustomPlot* customPlot = m_window->m_ui->plot;
    customPlot->addGraph();
    customPlot->graph()->setName( QString::fromUtf8(name.c_str()) );
    customPlot->graph()->setPen( QPen(color) );
    customPlot->graph()->setAntialiasedFill( false );

    m_window->m_uiMutex.unlock();
}

void Graph::removeGraph( unsigned int index ) {
    m_dataMutex.lock();

    // Remove data set
    m_dataSets.erase( m_dataSets.begin() + index );

    m_dataMutex.unlock();

    m_window->m_uiMutex.lock();
    m_window->m_ui->plot->removeGraph( index );
    m_window->m_ui->plot->legend->removeItem( index );
    m_window->m_uiMutex.unlock();
}

bool Graph::saveAsCSV() {
    // There isn't any point in creating a file with no data in it
    if ( m_dataSets.size() == 0 ) {
        emit criticalDialogSignal( QObject::tr("Save Data") , QObject::tr("No graph data to save") );
        return false;
    }

    /* ===== Create unique name for file ===== */
    typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type> days;
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    days d = std::chrono::duration_cast<days>(tp);
    tp -= d;
    std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp);
    tp -= h;
    std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(tp);
    tp -= m;
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(tp);
    tp -= s;

    char buf[128];
    snprintf( buf , 128 , "Graph-%d-T%ld.%ld.%ld.csv" ,
            d.count() ,
            h.count() ,
            m.count() ,
            s.count() );
    /* ======================================= */

    std::ofstream saveFile( buf , std::ios_base::trunc );

    if ( saveFile.is_open() ) {
        m_dataMutex.lock();

        std::vector<std::list<Pair>::iterator> sets( m_dataSets.size() );

        for ( unsigned int i = 0 ; i < m_dataSets.size() ; i++ ) {
            sets[i] = m_dataSets[i].begin();
        }

        size_t setsOpen = m_dataSets.size();

        // While there is still data in at least one data set to add to the file
        while ( setsOpen > 0 ) {
            setsOpen = m_dataSets.size();

            for ( size_t j = 0 ; j < m_dataSets.size() ; j++ ) {
                // If there are still points in this data set to add
                if ( sets[j] != m_dataSets[j].end() ) {
                    saveFile << sets[j]->first << "," << sets[j]->second;

                    // Increment to next point in data set
                    sets[j]++;
                }
                // Data set has reached invalid point after incrementing
                else {
                    // Add a filler comma
                    saveFile << ",";

                    setsOpen--;
                }

                // Only add another comma if there is more data to add
                if ( j < m_dataSets.size() - 1 ) {
                    saveFile << ",";
                }
            }

            if ( setsOpen > 0 ) {
                saveFile << '\n';
            }
        }

        m_dataMutex.unlock();

        saveFile.close();

        emit infoDialogSignal( QObject::tr("Save Data") , QObject::tr("Successfully saved graph data to file") );
        return true;
    }
    else {
        emit criticalDialogSignal( QObject::tr("Save Data") , QObject::tr("Failed to save graph data to file") );
        return false;
    }
}

void Graph::handleSocketData() {
    printf( "socket data\n" );

    if ( m_dataSocket.bytesAvailable() < static_cast<int>(sizeof(m_buffer)) ) {
        return;
    }

    uint64_t count = 0;
    int64_t received = 0;
    while ( count < sizeof(m_buffer) ) {
        received = m_dataSocket.read( m_buffer , sizeof(m_buffer) );
        if ( !m_dataSocket.isValid() || received < 0 ) {
            emit criticalDialogSignal( QObject::tr("Connection Error") , QObject::tr("Unexpected disconnection from remote host") );
            m_dataSocket.disconnect();
            return;
        }
        else {
            count += received;
        }
    }

    if ( m_buffer[0] == 'd' ) {
        std::memcpy( &m_recvData , m_buffer , sizeof(m_buffer) );

        /* ===== Add sent point to local graph ===== */
        // This will only work if ints are the same size as floats
        static_assert( sizeof(float) == sizeof(uint32_t) , "float isn't 32 bits long" );

        // Used for endianness conversions
        uint32_t tmp;

        // Convert endianness of x component
        std::memcpy( &tmp , &m_recvData.x , sizeof(m_recvData.x) );
        tmp = ntohl( tmp );
        std::memcpy( &m_recvData.x , &tmp , sizeof(m_recvData.x) );

        // Convert endianness of y component
        std::memcpy( &tmp , &m_recvData.y , sizeof(m_recvData.y) );
        tmp = ntohl( tmp );
        std::memcpy( &m_recvData.y , &tmp , sizeof(m_recvData.y) );

        /* Add data to appropriate data set; store point in
         * temps b/c references can't be made of packed
         * (unaligned) struct variables
         */
        float x = m_recvData.x;
        float y = m_recvData.y;
        addData( m_graphNamesMap[m_recvData.graphName] , Pair( x , y ) );
        /* ========================================= */
    }
    else if ( m_buffer[0] == 'l' ) {
        std::memcpy( &m_listData , m_buffer , sizeof(m_buffer) );

        m_graphNames.push_back( m_listData.graphName );
        m_graphNamesMap[m_listData.graphName] = m_graphNames.size() - 1;

        // If that was the last name, exit the recv loop
        if ( m_listData.eof == 1 ) {
            // Allow user to select which data sets to receive
            SelectDialog* dialog = new SelectDialog( m_graphNames , this , m_window );
            connect( dialog , SIGNAL(finished(int)) , this , SLOT(sendGraphChoices()) );
            dialog->exec();
        }
    }
}

void Graph::sendGraphChoices() {
    /* Send updated status on streams to which to connect based on the bit
     * array
     */
    for ( unsigned int i = 0 ; i < m_graphNames.size() ; i++ ) {
        std::memset( &m_recvData , 0 , sizeof(m_recvData) );

        // If the graph data is requested
        if ( m_curSelect & (1 << i) ) {
            m_recvData.id = 'c';
        }
        else {
            // Tell server to stop sending stream
            m_recvData.id = 'd';
        }

        // Store name of data set to buffer
        std::strcpy( m_recvData.graphName , m_graphNames[i].c_str() );

        uint64_t count = 0;
        int64_t sent = 0;
        while ( count < 16 ) {
            sent = m_dataSocket.write( reinterpret_cast<char*>(&m_recvData) , 16 );
            if ( !m_dataSocket.isValid() || sent < 0 ) {
                emit criticalDialogSignal( QObject::tr("Connection Error") , QObject::tr("Unexpected disconnection from remote host") );
                m_dataSocket.disconnect();
                return;
            }
            else {
                count += sent;
            }
        }

        /* Create a graph for each data set requested.
         * V starts out at 1. H cycles through six colors. When it wraps
         * around 360 degrees, V is decremented by 0.25. S is always 1.
         * This algorithm gives 25 possible values, one being black.
         */
        constexpr unsigned int parts = 3;
        createGraph( m_graphNames[i] , HSVtoRGB( 360 / parts * i % 360 , 1 , 1 - 0.25 * std::floor( i / parts ) ) );
    }
}

void Graph::handleStateChange( QAbstractSocket::SocketState state ) {
    if ( state == QAbstractSocket::SocketState::UnconnectedState ) {
        emit criticalDialogSignal( QObject::tr("Connection Error") , QObject::tr("Connection to remote host failed") );
    }
    else if ( state == QAbstractSocket::SocketState::ConnectingState ) {
        printf( "Connecting...\n" );
    }
    else if ( state == QAbstractSocket::SocketState::ConnectedState ) {
        printf( "Connected\n" );
    }
    else if ( state == QAbstractSocket::SocketState::ClosingState ) {
        printf( "Closing...\n" );
    }
}
