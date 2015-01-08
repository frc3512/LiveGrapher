//=============================================================================
//File Name: Graph.cpp
//Description: Draws a graph in WinGDI
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

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

DataSet::DataSet( std::list<Pair> n_data , QColor n_color ) {
    data = n_data;
    color = n_color;
    startingPoint = data.begin();
}

Graph::Graph( MainWindow* parentWindow ) :
        m_settings( "IPSettings.txt" ) ,
        m_graphNames( 64 ) ,
        m_graphNamesSize( 0 ) ,
        m_curSelect( 0 ) ,
        m_graphThread( nullptr ) ,
        m_isRunning( true ) {
    m_window = parentWindow;
}

Graph::~Graph() {
    // Wait for graph data reception thread to exit
    m_isRunning = false;
    if ( m_graphThread != nullptr ) {
        m_graphThread->join();
        delete m_graphThread;
    }
}

void Graph::reconnect() {
    m_isRunning = false;
    if ( m_graphThread != nullptr ) {
        m_graphThread->join();
        delete m_graphThread;
        m_graphThread = nullptr;
    }
    m_isRunning = true;

    remoteIP = sf::IpAddress( m_settings.getValueFor( "robotIP" ) );
    dataPort = std::strtoul( m_settings.getValueFor( "robotGraphPort" ).c_str() , NULL , 10 );

    status = sf::Socket::Disconnected;

    std::memset( &recvData , 0 , sizeof(recvData) );
    std::memset( &listData , 0 ,sizeof(listData) );

    sizeRecv = 0;

    // Reset list of data set recv statuses
    m_curSelect = 0;

    try {
        // Attempt connection to remote data set host
        status = dataSocket.connect( remoteIP , dataPort , sf::milliseconds( 1000 ) );

        if ( status != sf::Socket::Done ) {
            throw Error::FailConnect;
        }

        threadSelector.add( dataSocket );

        // Request list of all data sets on remote host
        recvData.id = 'l';
        do {
            status = dataSocket.send( &recvData , 16 );

            if ( status == sf::Socket::Disconnected ) {
                throw Error::Disconnected;
            }
        } while ( m_isRunning && status != sf::Socket::Done );

        // Clear m_graphNames before refilling it
        m_graphNamesSize = 0;
        m_graphNamesMap.clear();

        unsigned int i = 0;

        // Get list of available data sets from host and store them in a map
        bool stillRecvList = true;
        while ( m_isRunning && stillRecvList ) {
            if ( threadSelector.wait( sf::milliseconds( 50 ) ) ) {
                if ( threadSelector.isReady( dataSocket ) ) {
                    status = dataSocket.receive( buffer , sizeof(buffer) , sizeRecv );

                    if ( status == sf::Socket::Done ) {
                        if ( buffer[0] == 'l' ) {
                            std::memcpy( &listData , buffer , sizeof(buffer) );

                            m_graphNames[i] = listData.graphName;
                            m_graphNamesMap[listData.graphName] = i;

                            i++;

                            // If that was the last name, exit the recv loop
                            if ( listData.eof == 1 ) {
                                stillRecvList = false;
                            }
                        }
                    }
                }
            }
            else if ( status == sf::Socket::Disconnected ) {
                throw Error::Disconnected;
            }
        }

        m_graphNamesSize = i;

        // Allow user to select which data sets to receive
        SelectDialog* dialog = new SelectDialog( m_graphNames , m_graphNamesSize , this , m_window );
        dialog->exec();

        /* Send updated status on streams to which to connect based on the bit
         * array
         */
        for ( unsigned int i = 0 ; i < m_graphNamesSize ; i++ ) {
            std::memset( &recvData , 0 , sizeof(recvData) );

            // If the graph data is requested
            if ( m_curSelect & (1 << i) ) {
                recvData.id = 'c';
            }
            else {
                // Tell server to stop sending stream
                recvData.id = 'd';
            }

            // Store name of data set to buffer
            std::strcpy( recvData.graphName , m_graphNames[i].c_str() );

            dataSocket.send( &recvData , 16 );

            /* Create a graph for each data set requested.
             * V starts out at 1. H cycles through six colors. When it wraps
             * around 360 degrees, V is decremented by 0.25. S is always 1.
             * This algorithm gives 25 possible values, one being black.
             */
            createGraph( HSVtoRGB( 60 * i % 360 , 1 , 1 - 0.25 * std::floor( i / 6 ) ) );
        }
    }
    catch ( Error& exception ) {
        if ( exception == Error::FailConnect ) {
            QMessageBox::critical( nullptr , QObject::tr("Connection Error") , QObject::tr("Connection to remote host failed") );
        }
        else if ( exception == Error::Disconnected ) {
            QMessageBox::critical( nullptr , QObject::tr("Connection Error") , QObject::tr("Unexpected disconnection from remote host") );
        }
    }

    m_graphThread = new std::thread( [this] { Graph::graphThreadFunc(); } );
}

void Graph::addData( unsigned int index , const Pair& data ) {
    m_dataMutex.lock();

    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index && set != m_dataSets.end() ; i++ ) {
        set++;
    }

    bool noPoints = set->startingPoint == set->data.end();

    set->data.push_back( data );

    if ( noPoints ) {
        set->startingPoint = set->data.begin();
    }

    m_dataMutex.unlock();

    m_window->realtimeDataSlot( index , data.first , data.second );
}

void Graph::clearAllData() {
    m_dataMutex.lock();

    for ( std::list<DataSet>::iterator set = m_dataSets.begin() ; set != m_dataSets.end() ; set++ ) {
        set->data.clear();

        // Reset iterator to beginning since no other data can be pointed to
        set->startingPoint = set->data.begin();
    }

    m_dataMutex.unlock();

    m_window->m_uiMutex.lock();

    for ( int i = 0 ; i < m_window->m_ui->plot->graphCount() ; i++ ) {
        m_window->m_ui->plot->graph( i )->clearData();
    }

    m_window->m_uiMutex.unlock();
}

void Graph::createGraph( QColor color ) {
    m_dataMutex.lock();

    /* TODO Make list with no points; it's necessary now b/c start iterators
     * need a valid starting pt
     */
    std::list<Pair> data = { std::make_pair( 0.f , 0.f ) };
    m_dataSets.push_back( DataSet( data , color ) );

    m_dataMutex.unlock();

    m_window->m_uiMutex.lock();

    unsigned int i = m_dataSets.size() - 1;
    QCustomPlot* customPlot = m_window->m_ui->plot;
    customPlot->addGraph();
    customPlot->graph( i )->setPen( QPen(color) );
    customPlot->graph( i )->setAntialiasedFill( false );

    m_window->m_uiMutex.unlock();
}

void Graph::removeGraph( unsigned int index ) {
    m_dataMutex.lock();

    // Remove data set
    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index && set != m_dataSets.end() ; i++ ) {
        set++;
    }

    m_dataSets.erase( set );

    m_dataMutex.unlock();

    m_window->m_uiMutex.lock();
    m_window->m_ui->plot->removeGraph( index );
    m_window->m_uiMutex.unlock();
}

bool Graph::saveAsCSV() {
    // There isn't any point in creating a file with no data in it
    if ( m_dataSets.size() == 0 ) {
        QMessageBox::critical( m_window , QObject::tr("Save Data") , QObject::tr("No graph data to save") );
        return false;
    }

    std::vector<std::list<Pair>::iterator> points( m_dataSets.size() );
    std::vector<std::list<Pair>::iterator> ptEnds( m_dataSets.size() );

    size_t i = 0;
    for ( std::list<DataSet>::iterator sets = m_dataSets.begin() ; sets != m_dataSets.end() ; sets++ ) {
        points[i] = sets->data.begin();
        ptEnds[i] = sets->data.end();
        i++;
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

        size_t setsOpen = points.size();

        // While there is still data in at least one DataSet to add to the file
        while ( setsOpen > 0 ) {
            setsOpen = points.size();

            for ( size_t j = 0 ; j < points.size() ; j++ ) {
                // If there are still points in this DataSet to add
                if ( points[j] != ptEnds[j] ) {
                    saveFile << points[j]->first << "," << points[j]->second;

                    // Increment to next point
                    points[j]++;
                }
                else {
                    // Add a filler comma
                    saveFile << ",";
                }

                // If DataSet reached invalid point after incrementing
                if ( points[j] == ptEnds[j] ) {
                    setsOpen--;
                }

                // Only add another comma if there is more data to add
                if ( j < points.size() - 1 ) {
                    saveFile << ",";
                }
            }

            if ( setsOpen > 0 ) {
                saveFile << '\n';
            }
        }

        m_dataMutex.unlock();

        saveFile.close();

        QMessageBox::information( m_window , QObject::tr("Save Data") , QObject::tr("Successfully saved graph data to file") );
        return true;
    }
    else {
        QMessageBox::critical( m_window , QObject::tr("Save Data") , QObject::tr("Failed to save graph data to file") );
        return false;
    }
}

void Graph::graphThreadFunc() {
    try {
        if ( status == sf::Socket::Done ) {
            int tmp; // Used for endianness conversions

            while ( m_isRunning ) {
                if ( threadSelector.wait( sf::milliseconds( 50 ) ) ) {
                    if ( threadSelector.isReady( dataSocket ) ) {
                        status = dataSocket.receive( buffer , sizeof(buffer) , sizeRecv );

                        if ( status == sf::Socket::Done ) {
                            if ( buffer[0] == 'd' ) {
                                std::memcpy( &recvData , buffer , sizeof(buffer) );

                                /* ===== Add sent point to local graph ===== */
                                // This will only work if ints are the same size as floats
                                static_assert( sizeof(float) == sizeof(uint32_t) , "float isn't 32 bits long" );

                                // Convert endianness of x component
                                std::memcpy( &tmp , &recvData.x , sizeof(recvData.x) );
                                tmp = ntohl( tmp );
                                std::memcpy( &recvData.x , &tmp , sizeof(recvData.x) );

                                // Convert endianness of y component
                                std::memcpy( &tmp , &recvData.y , sizeof(recvData.y) );
                                tmp = ntohl( tmp );
                                std::memcpy( &recvData.y , &tmp , sizeof(recvData.y) );

                                /* Add data to appropriate data set; store point in
                                 * temps b/c references can't be made of packed
                                 * (unaligned) struct variables
                                 */
                                float x = recvData.x;
                                float y = recvData.y;
                                addData( m_graphNamesMap[recvData.graphName] , Pair( x , y ) );
                                /* ========================================= */
                            }
                        }
                        else {
                            throw Error::Disconnected;
                        }
                    }
                }

                std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
            }
        }
    }
    catch ( Error& exception ) {
        if ( exception == Error::FailConnect ) {
            QMessageBox::critical( m_window , QObject::tr("Connection Error") , QObject::tr("Connection to remote host failed") );
        }
        else if ( exception == Error::Disconnected ) {
            QMessageBox::critical( m_window , QObject::tr("Connection Error") , QObject::tr("Unexpected disconnection from remote host") );
        }
    }

    dataSocket.disconnect();
}
