//=============================================================================
//File Name: Graph.cpp
//Description: Draws a graph in WinGDI
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#include "Graph.hpp"
#include "UIFont.hpp"
#include <cmath>
#include <cstring>
#include <iostream> // TODO Remove me
#include <fstream>
#include <sys/time.h>

#include "../SFML/Network/SocketSelector.hpp"
#include "../SFML/Network/TcpSocket.hpp"
#include "../SFML/Network/IpAddress.hpp"
#include "../SFML/Network/Packet.hpp"
#include "../SFML/System/Lock.hpp"

#include <wingdi.h>

std::map<HWND , Graph*> Graph::m_map;

/* Graph Packet Structure
 *
 * char* graphName (16 bytes)
 * float32 x
 * float32 y
 * ... <other graphs>
 */
typedef struct packet_t {
    char graphName[16];
    float x;
    float y;
} packet_t;

DataSet::DataSet( std::list<std::pair<float , float>> n_data , COLORREF n_color ) {
    data = n_data;
    color = n_color;
    startingPoint = data.begin();
}

class GraphClassInit {
public:
    GraphClassInit();
    ~GraphClassInit();

private:
    WNDCLASSEX m_windowClass;
};

GraphClassInit windowClass;

GraphClassInit::GraphClassInit() {
    m_windowClass.cbSize        = sizeof(WNDCLASSEX);
    m_windowClass.style         = 0;
    m_windowClass.lpfnWndProc   = &Graph::WindowProc;
    m_windowClass.cbClsExtra    = 0;
    m_windowClass.cbWndExtra    = 0;
    m_windowClass.hInstance     = GetModuleHandle( NULL );
    m_windowClass.hIcon         = NULL;
    m_windowClass.hCursor       = LoadCursor( NULL , IDC_ARROW );
    m_windowClass.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
    m_windowClass.lpszMenuName  = NULL;
    m_windowClass.lpszClassName = "Graph";
    m_windowClass.hIconSm       = NULL;

    RegisterClassEx( &m_windowClass );
}

GraphClassInit::~GraphClassInit() {
    UnregisterClass( "Graph" , GetModuleHandle( NULL ) );
}

Graph::Graph( HWND parentWindow , const Vector2i& position , const Vector2i& size ) :
            Drawable( parentWindow , position , size , RGB( 255 , 255 , 255 ) , RGB( 0 , 0 , 0 ) , 1 ) ,
            m_graphThread( &Graph::graphThreadFunc , this ) ,
            m_isRunning( true ) {
    m_window = CreateWindowEx( 0 ,
            "Graph" ,
            "" ,
            WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE ,
            position.X ,
            position.Y ,
            size.X , // returns image width (resized as window)
            size.Y , // returns image height (resized as window)
            parentWindow ,
            NULL ,
            GetModuleHandle( NULL ) ,
            NULL );

    m_gridPen = CreatePen( PS_SOLID , 1 , RGB( 0 , 0 , 0 ) );

    m_xMin = 0.f;
    m_yMin = 0.f;
    m_xMax = size.X;
    m_yMax = size.Y;

    m_xScale = 100.f;
    m_yScale = 200.f;

    m_xHistory = size.X;

    // Add window to global map
    std::map<HWND , Graph*>::iterator it = m_map.begin();
    m_map.insert( it , std::pair<HWND , Graph*>( m_window , this ) );
}

Graph::~Graph() {
    // Wait for graph data reception thread to exit
    m_isRunning = false;
    m_graphThread.wait();

    DeleteObject( m_gridPen );
    DestroyWindow( m_window );

    // Remove window from global map
    m_map.erase( m_window );
}

void Graph::reconnect() {
    m_isRunning = false;
    m_graphThread.wait();

    m_isRunning = true;
    m_graphThread.launch();
}

void Graph::redraw() {
    InvalidateRect( m_window , NULL , FALSE );
}

void Graph::draw( PAINTSTRUCT* ps ) {
    // Create DC to which to draw
    HDC hdc = ps->hdc;

    /* ===== Create buffer DC ===== */
    RECT clientRect;
    GetClientRect( m_window , &clientRect );

    HDC bufferDC = CreateCompatibleDC( hdc );
    // The bitmap must be used to make drawing to the DC succeed
    HBITMAP bufferBmp = CreateCompatibleBitmap( hdc , clientRect.right , clientRect.bottom );

    HBITMAP oldBmp = static_cast<HBITMAP>( SelectObject( bufferDC , bufferBmp ) );
    /* ============================ */

    // Set pixels in DC to known state
    HBRUSH hbrBkGnd = CreateSolidBrush( RGB( 255 , 255 , 255 ) );
    FillRect( bufferDC , &clientRect , hbrBkGnd );
    DeleteObject( hbrBkGnd );

    // Set up the device context for drawing this shape
    HPEN oldPen = (HPEN)SelectObject( bufferDC , m_gridPen );
    HBRUSH oldBrush = (HBRUSH)SelectObject( bufferDC , CreateSolidBrush( Drawable::getFillColor() ) );

    // Draw border
    Rectangle( bufferDC ,
            clientRect.left ,
            clientRect.top ,
            clientRect.right ,
            clientRect.bottom
            );

    std::pair<float , float> gridPoint;

    // Draw horizontal grid lines
    unsigned int startI = floor( static_cast<float>(getYMin()) / static_cast<float>(getYScale()) );
    unsigned int endI = ceil( static_cast<float>(getYMax()) / static_cast<float>(getYScale()) );
    for ( unsigned int i = startI ; i < endI ; i++ ) {
        gridPoint.first = 0;
        gridPoint.second = i * getYScale();
        gridPoint = transformToGraph( gridPoint );

        MoveToEx( bufferDC , clientRect.left , gridPoint.second , NULL );
        LineTo( bufferDC , clientRect.right , gridPoint.second );
    }

    // Draw vertical grid lines
    startI = floor( static_cast<float>(getXMin()) / static_cast<float>(getXScale()) );
    endI = ceil( static_cast<float>(getXMax()) / static_cast<float>(getXScale()) );
    for ( unsigned int i = startI ; i < endI ; i++ ) {
        gridPoint.first = i * getXScale();
        gridPoint.second = 0;
        gridPoint = transformToGraph( gridPoint );

        MoveToEx( bufferDC , gridPoint.first , clientRect.top , NULL );
        LineTo( bufferDC , gridPoint.first ,  clientRect.bottom );
    }

    m_dataMutex.lock();

    // Draw each data set from their respective starting points

    // Loop over each data set
    for ( std::list<DataSet>::iterator set = m_dataSets.begin() ; set != m_dataSets.end() ; set++ ) {
        if ( set->data.size() > 0 ) {
            // Switch current pen with one needed for current data set
            HPEN linePen = CreatePen( PS_SOLID , 2 , set->color );
            HPEN lastPen = (HPEN)SelectObject( bufferDC , linePen );
            std::pair<float , float> tempPoint;

            tempPoint = transformToGraph( *(set->startingPoint) );

            // While starting point isn't on screen anymore, advance to next point
            while ( tempPoint.first < 0 && set->startingPoint != set->data.end() ) {
                set->startingPoint++;

                // Get new transformed starting point
                tempPoint = transformToGraph( *(set->startingPoint) );
            }

            if ( set->startingPoint == set->data.end() && set->data.begin() != set->data.end() ) {
                set->startingPoint--;
            }

            // Move starting point back to just off left of screen
            if ( set->startingPoint != set->data.begin() ) {
                set->startingPoint--;
            }

            // Move DC pen to starting point
            tempPoint = transformToGraph(*(set->startingPoint));
            MoveToEx( bufferDC , tempPoint.first , clientRect.bottom - tempPoint.second , NULL );
            set->startingPoint++;

            for ( std::list<std::pair<float , float>>::iterator pt = set->startingPoint ; pt != set->data.end() ; pt++ ) {
                // Normalize point before we draw it
                tempPoint = transformToGraph(*pt);

                LineTo( bufferDC , tempPoint.first , clientRect.bottom - tempPoint.second );
            }

            // Free line pen for this data set
            SelectObject( bufferDC , lastPen );
            DeleteObject( linePen );
        }
    }

    m_dataMutex.unlock();

    // Replace old pen
    SelectObject( bufferDC , oldPen );

    // Replace old brush
    oldBrush = (HBRUSH)SelectObject( bufferDC , oldBrush );

    // Free newly created objects
    DeleteObject( oldPen );
    DeleteObject( oldBrush );

    // Blit buffer DC to real DC
    BitBlt( hdc , clientRect.left - getOutlineThickness() , clientRect.top - getOutlineThickness() , clientRect.right + 2 * getOutlineThickness() , clientRect.bottom + 2 * getOutlineThickness() , bufferDC , clientRect.left - getOutlineThickness() , clientRect.top - getOutlineThickness() , SRCCOPY );

    // Replace the old bitmap and delete the created one
    DeleteObject( SelectObject( bufferDC , oldBmp ) );

    // Free buffer DC
    DeleteDC( bufferDC );
}

void Graph::addData( unsigned int index , const std::pair<float , float>& data ) {
    m_dataMutex.lock();

    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index ; i++ ) {
        set++;
    }

    bool noPoints = set->startingPoint == set->data.end();

    set->data.push_back( data );

    if ( noPoints ) {
        set->startingPoint = set->data.begin();
    }

    m_dataMutex.unlock();
}

void Graph::removeData( unsigned int index ) {
    m_dataMutex.lock();

    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index ; i++ ) {
        set++;
    }

    /* Advance iterator pointing to value in data set if it's about to be
     * invalidated
     */
    if ( set->startingPoint == set->data.begin() ) {
        set->startingPoint++;
    }

    set->data.pop_front();

    m_dataMutex.unlock();
}

void Graph::clearAllData() {
    m_dataMutex.lock();

    for ( std::list<DataSet>::iterator set = m_dataSets.begin() ; set != m_dataSets.end() ; set++ ) {
        set->data.clear();

        // Reset iterator to beginning since no other data can be pointed to
        set->startingPoint = set->data.begin();
    }

    m_dataMutex.unlock();
}

void Graph::setGraphColor( unsigned int index , COLORREF color ) {
    m_dataMutex.lock();

    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index ; i++ ) {
        set++;
    }

    (*set).color = color;

    m_dataMutex.unlock();
}

COLORREF Graph::getGraphColor( unsigned int index ) {
    sf::Lock dataLock( m_dataMutex );

    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index ; i++ ) {
        set++;
    }

    return (*set).color;
}

void Graph::createGraph( COLORREF color ) {
    m_dataMutex.lock();

    // TODO Make list with no points
    std::list<std::pair<float , float>> data = { std::make_pair( 0.f , 0.f ) };
    m_dataSets.push_back( DataSet( data , color ) );

    m_dataMutex.unlock();
}

void Graph::removeGraph( unsigned int index ) {
    m_dataMutex.lock();

    // Remove data set
    std::list<DataSet>::iterator set = m_dataSets.begin();
    for ( unsigned int i = 0 ; i < index ; i++ ) {
        set++;
    }

    m_dataSets.erase( set );

    m_dataMutex.unlock();
}

void Graph::setYMin( int yMin ) {
    m_dataMutex.lock();
    m_yMin = yMin;
    m_dataMutex.unlock();
}

void Graph::setYMax( int yMax ) {
    m_dataMutex.lock();
    m_yMax = yMax;
    m_dataMutex.unlock();
}

void Graph::setXScale( int xScale ) {
    m_dataMutex.lock();
    m_xScale = xScale;
    m_dataMutex.unlock();
}

void Graph::setYScale( int yScale ) {
    m_dataMutex.lock();
    m_yScale = yScale;
    m_dataMutex.unlock();
}

void Graph::setHistoryLength( unsigned int length ) {
    m_dataMutex.lock();
    m_xHistory = length;
    m_dataMutex.unlock();
}

unsigned int Graph::getHistoryLength() {
    sf::Lock dataLock( m_dataMutex );

    return m_xHistory;
}

bool Graph::saveToCSV( const std::string& fileName ) {
    // There isn't any point in creating a file with no data in it
    if ( m_dataSets.size() == 0 ) {
        return false;
    }

    std::vector<std::list<std::pair<float , float>>::iterator> points( m_dataSets.size() );
    std::vector<std::list<std::pair<float , float>>::iterator> ptEnds( m_dataSets.size() );

    size_t i = 0;
    for ( std::list<DataSet>::iterator sets = m_dataSets.begin() ; sets != m_dataSets.end() ; sets++ ) {
        points[i] = sets->data.begin();
        ptEnds[i] = sets->data.end();
        i++;
    }

    /* ===== Create unique name for file ===== */
    SYSTEMTIME currentTime;
    ZeroMemory( &currentTime , sizeof(currentTime) );
    GetSystemTime( &currentTime );

    char buf[128];
    snprintf( buf , 128 , "Graph-%u.%u-%u.%u.%u.csv" ,
            currentTime.wMonth ,
            currentTime.wDay ,
            currentTime.wHour ,
            currentTime.wMinute ,
            currentTime.wSecond );
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

        return true;
    }
    else {
        return false;
    }
}

void Graph::setXMin( int xMin ) {
    m_dataMutex.lock();
    m_xMin = xMin;
    m_dataMutex.unlock();
}

void Graph::setXMax( int xMax ) {
    m_dataMutex.lock();
    m_xMax = xMax;
    m_dataMutex.unlock();
}

int Graph::getXMin() {
    sf::Lock dataLock( m_dataMutex );

    return m_xMin;
}

int Graph::getXMax() {
    sf::Lock dataLock( m_dataMutex );

    return m_xMax;
}

int Graph::getYMin() {
    sf::Lock dataLock( m_dataMutex );

    return m_yMin;
}

int Graph::getYMax() {
    sf::Lock dataLock( m_dataMutex );

    return m_yMax;
}

int Graph::getXScale() {
    sf::Lock dataLock( m_dataMutex );

    return m_xScale;
}

int Graph::getYScale() {
    sf::Lock dataLock( m_dataMutex );

    return m_yScale;
}

std::pair<float , float> Graph::transformToGraph( std::pair<float , float> point ) {
    RECT windowRect;
    GetClientRect( m_window , &windowRect );

    point.first = ( point.first - getXMin() ) * ( windowRect.right - windowRect.left ) / ( getXMax() - getXMin() );
    point.second = ( point.second - getYMin() ) * ( windowRect.bottom - windowRect.top ) / ( getYMax() - getYMin() );

    return point;
}

LRESULT CALLBACK Graph::WindowProc( HWND handle , UINT message , WPARAM wParam , LPARAM lParam ) {
    switch ( message ) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint( handle , &ps );

        m_map[handle]->draw( &ps );

        EndPaint( handle , &ps );

        break;
    }

    default: {
        return DefWindowProc( handle , message , wParam , lParam );
    }
    }

    return 0;
}

void Graph::graphThreadFunc() {
    sf::SocketSelector threadSelector;
    sf::TcpSocket dataSocket;
    unsigned short dataPort = 3513;
    //sf::IpAddress remoteIP( 127 , 0 , 0 , 1 );
    sf::IpAddress remoteIP( 10 , 35 , 12 , 2 );
    sf::Socket::Status status = sf::Socket::Disconnected;

    while ( m_isRunning && status != sf::Socket::Done ) {
        status = dataSocket.connect( remoteIP , dataPort , sf::milliseconds( 1000 ) );
        std::cout << "connect status=" << status << "\n";
    }

    // Request data sets from host
    if ( status == sf::Socket::Done ) {
        char dataBuffer[16] = "PID0";
        dataSocket.send( dataBuffer , 16 );
        std::cout << "Sent PID0 request\n";

        char name[5] = "PID1";
        std::strcpy( dataBuffer , name );
        dataSocket.send( dataBuffer , 16 );
        std::cout << "Sent PID1 request\n";
    }

    threadSelector.add( dataSocket );

    if ( status == sf::Socket::Done ) {
        createGraph( RGB( 0 , 120 , 0 ) );
        createGraph( RGB( 255 , 0 , 0 ) );

        packet_t recvData;
        int tmp;
        unsigned int sizeRecv = 0;
        sf::Socket::Status status;

        while ( m_isRunning ) {
            if ( threadSelector.wait( sf::milliseconds( 50 ) ) ) {
                if ( threadSelector.isReady( dataSocket ) ) {
                    status = dataSocket.receive( &recvData , 24 , sizeRecv );

                    // Add all points sent to local graph
                    if ( status == sf::Socket::Done ) {
                        tmp = ntohl( *( (int*)&recvData.x ) );
                        recvData.x = *( (float*)&tmp );

                        tmp = ntohl( *( (int*)&recvData.y ) );
                        recvData.y = *( (float*)&tmp );

                        /* If new data is off right of plot, move the plot so
                         * the new data is displayed
                         */
                        if ( recvData.x > getXMax() ) {
                            setXMin( recvData.x - getHistoryLength() );
                            setXMax( recvData.x );
                        }

                        std::cout << recvData.graphName << ": " << recvData.x << ", " << recvData.y << "\n";

                        if ( std::strcmp( recvData.graphName , "PID0" ) == 0 ) {
                            addData( 0 , std::make_pair( recvData.x , recvData.y ) );
                        }
                        else if ( std::strcmp( recvData.graphName , "PID1" ) == 0 ) {
                            addData( 1 , std::make_pair( recvData.x , recvData.y ) );
                        }
                    }

                    redraw();
                }
            }

            Sleep( 5 );
        }
    }

    dataSocket.disconnect();
}
