//=============================================================================
//File Name: Graph.cpp
//Description: Manages a graph in Qt
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#include <fstream>

#include <cmath>
#include <cstring>

#include "Graph.hpp"
#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "SelectDialog.hpp"

#include <QTcpSocket>
#include <QtEndian>

QColor HSVtoRGB( float h , float s , float v ) {
    float r;
    float g;
    float b;

    float chroma = v * s;
    float hPrime = h / 60.f;
    float m = v - chroma;

    float modTemp = hPrime - static_cast<int>(hPrime);
    float x = chroma *
            ( 1.f - std::fabs( static_cast<int>(hPrime) % 2 + modTemp - 1 ) );

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
        QObject( parentWindow ) ,
        m_settings( "IPSettings.txt" ) ,
        m_curSelect( 0 ) {
    m_window = parentWindow;
    m_dataSets.reserve( 64 );
    m_graphNames.reserve( 64 );
    m_dataSocket = new QTcpSocket( this );
    m_remoteIP = QString::fromUtf8( m_settings.getString( "robotIP" ).c_str() );
    m_dataPort = m_settings.getInt( "robotGraphPort" );

    m_startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

    connect( m_dataSocket , SIGNAL(readyRead()) ,
             this , SLOT(handleSocketData()) );
}

Graph::~Graph() {
    m_dataSocket->disconnect();
}

void Graph::reconnect() {
    std::memset( &m_recvData , 0 , sizeof(m_recvData) );
    std::memset( &m_listData , 0 ,sizeof(m_listData) );

    // Reset list of data set recv statuses
    m_curSelect = 0;

    // Attempt connection to remote data set host
    if ( !m_dataSocket->isValid() ) {
        m_dataSocket->connectToHost( m_remoteIP , m_dataPort );

        if ( !m_dataSocket->waitForConnected( 1000 ) ) {
            QMessageBox::critical( m_window , QObject::tr("Connection Error") ,
                QObject::tr("Connection to remote host failed") );
            return;
        }
    }

    // Request list of all data sets on remote host
    m_recvData.id = 'l';
    int64_t count = 0;
    int64_t sent = 0;
    while ( count < 16 ) {
        sent = m_dataSocket->write( reinterpret_cast<char*>(&m_recvData) , 16 );
        if ( !m_dataSocket->isValid() || sent < 0 ) {
            printf( "hi\n" );
            QMessageBox::critical( m_window , QObject::tr("Connection Error") ,
                QObject::tr("Unexpected disconnection from remote host") );
            m_dataSocket->disconnect();
            return;
        }
        else {
            count += sent;
        }
    }
}

void Graph::addData( unsigned int index , const Pair& data ) {
    m_dataSets[index].push_back( data );

    m_window->realtimeDataSlot( index , data.first , data.second );
    //emit realtimeDataSignal( index , data.first , data.second );
}

void Graph::clearAllData() {
    for ( auto set : m_dataSets ) {
        set.clear();
    }

    for ( int i = 0 ; i < m_window->m_ui->plot->graphCount() ; i++ ) {
        m_window->m_ui->plot->graph( i )->clearData();
    }
}

void Graph::createGraph( const std::string& name , QColor color ) {
    m_dataSets.push_back( DataSet() );

    QCustomPlot* customPlot = m_window->m_ui->plot;
    customPlot->addGraph();
    customPlot->graph()->setName( QString::fromUtf8(name.c_str()) );
    customPlot->graph()->setAntialiasedFill( false );

    QPen pen( color );
    pen.setWidth( 1 );
    customPlot->graph()->setPen( pen );
}

void Graph::removeGraph( unsigned int index ) {
    // Remove data set
    m_dataSets.erase( m_dataSets.begin() + index );

    m_window->m_ui->plot->removeGraph( index );
    m_window->m_ui->plot->legend->removeItem( index );
}

bool Graph::saveAsCSV() {
    // There isn't any point in creating a file with no data in it
    if ( m_dataSets.size() == 0 ) {
        QMessageBox::critical( m_window , QObject::tr("Save Data") ,
            QObject::tr("No graph data to save") );
        return false;
    }

    /* ===== Create unique name for file ===== */
    typedef std::chrono::duration<int,std::ratio_multiply<
            std::chrono::hours::period, std::ratio<24>>::type> days;
    std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    days d = std::chrono::duration_cast<days>(tp);
    tp -= d;
    std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp);
    tp -= h;
    std::chrono::minutes m =
            std::chrono::duration_cast<std::chrono::minutes>(tp);
    tp -= m;
    std::chrono::seconds s =
            std::chrono::duration_cast<std::chrono::seconds>(tp);
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

        saveFile.close();

        QMessageBox::information( m_window , QObject::tr("Save Data") ,
            QObject::tr("Successfully saved graph data to file") );
        return true;
    }
    else {
        QMessageBox::critical( m_window , QObject::tr("Save Data") ,
            QObject::tr("Failed to save graph data to file") );
        return false;
    }
}

void Graph::handleSocketData() {
    while ( m_dataSocket->bytesAvailable() >=
            static_cast<int>(sizeof(m_buffer)) ) {
        uint64_t count = 0;
        int64_t received = 0;
        while ( count < sizeof(m_buffer) ) {
            received = m_dataSocket->read( m_buffer , sizeof(m_buffer) );
            if ( !m_dataSocket->isValid() || received < 0 ) {
                QMessageBox::critical( m_window ,
                    QObject::tr("Connection Error") ,
                    QObject::tr("Unexpected disconnection from remote host") );
                m_dataSocket->disconnect();
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
            static_assert( sizeof(float) == sizeof(uint32_t) ,
                           "float isn't 32 bits long" );

            // Used for endianness conversions
            decltype(m_recvData.x) xtmp;
            uint32_t ytmp;

            // Convert endianness of x component
            std::memcpy( &xtmp , &m_recvData.x , sizeof(m_recvData.x) );
            xtmp = qFromBigEndian<qint64>( xtmp );
            std::memcpy( &m_recvData.x , &xtmp , sizeof(m_recvData.x) );

            // Convert endianness of y component
            std::memcpy( &ytmp , &m_recvData.y , sizeof(m_recvData.y) );
            ytmp = qFromBigEndian<qint32>( ytmp );
            std::memcpy( &m_recvData.y , &ytmp , sizeof(m_recvData.y) );

            /* Add data to appropriate data set; store point in
             * temps b/c references can't be made of packed
             * (unaligned) struct variables
             */
            decltype(m_recvData.x) x = m_recvData.x - m_startTime;
            decltype(m_recvData.y) y = m_recvData.y;
            addData( m_graphNamesMap[m_recvData.graphName] ,
                Pair( x/1000.f , y ) );
            /* ========================================= */
        }
        else if ( m_buffer[0] == 'l' ) {
            std::memcpy( &m_listData , m_buffer , sizeof(m_buffer) );

            decltype(m_graphNames)::iterator i =
                    std::find( m_graphNames.begin() , m_graphNames.end() ,
                               m_listData.graphName );
            if ( i == m_graphNames.end() ) {
                m_graphNames.push_back( m_listData.graphName );
                m_graphNamesMap[m_listData.graphName] = m_graphNames.size() - 1;
            }

            // If that was the last name, exit the recv loop
            if ( m_listData.eof == 1 ) {
                // Allow user to select which data sets to receive
                SelectDialog* dialog = new SelectDialog( m_graphNames , this ,
                                                         m_window );
                connect( dialog , SIGNAL(finished(int)) ,
                         this , SLOT(sendGraphChoices()) );
                dialog->open();
            }
        }
    }
}

void Graph::sendGraphChoices() {
    // Remove old graphs before creating new ones
    while ( m_dataSets.size() > 0 ) {
        removeGraph( m_dataSets.size() - 1 );
    }

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
            sent = m_dataSocket->write( reinterpret_cast<char*>(&m_recvData) ,
                                        16 );
            if ( !m_dataSocket->isValid() || sent < 0 ) {
                QMessageBox::critical( m_window ,
                    QObject::tr("Connection Error") ,
                    QObject::tr("Unexpected disconnection from remote host") );
                m_dataSocket->disconnect();
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
        createGraph( m_graphNames[i] , HSVtoRGB( 360 / parts * i % 360 , 1 ,
            1 - 0.25 * std::floor( i / parts ) ) );
    }
}
