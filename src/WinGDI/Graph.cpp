//=============================================================================
//File Name: Graph.cpp
//Description: Draws a graph in WinGDI
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#include <fstream>

#include <cmath>
#include <cstring>
#include <cassert>
#include <sys/time.h>

#include "Graph.hpp"
#include "UIFont.hpp"
#include "../Resource.h"

#include "../SFML/Network/SocketSelector.hpp"
#include "../SFML/Network/TcpSocket.hpp"
#include "../SFML/Network/IpAddress.hpp"
#include "../SFML/Network/Packet.hpp"
#include "../SFML/System/Lock.hpp"

#include <wingdi.h>

std::map<HWND , Graph*> Graph::m_map;

COLORREF HSVtoRGB( float h , float s , float v ) {
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

    return RGB( r * 255 , g * 255 , b * 255 );
}

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
struct packet_t {
    char id;
    char graphName[15];
    float x;
    float y;
} __attribute__((packed));

struct packet_list_t {
    char id;
    char graphName[15];
    char eof;
    char padding[7];
} __attribute__((packed));

DataSet::DataSet( std::list<Pair> n_data , COLORREF n_color ) {
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
            m_settings( "IPSettings.txt" ) ,
            m_graphNames( 64 ) ,
            m_graphNamesSize( 0 ) ,
            m_curSelect( 0 ) ,
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

    m_xMin = std::atoi( m_settings.getValueFor( "xMin" ).c_str() );
    m_yMin = std::atoi( m_settings.getValueFor( "yMin" ).c_str() );
    m_xMax = std::atoi( m_settings.getValueFor( "xMax" ).c_str() );
    m_yMax = std::atoi( m_settings.getValueFor( "yMax" ).c_str() );

    m_xScale = std::atoi( m_settings.getValueFor( "xScale" ).c_str() );
    m_yScale = std::atoi( m_settings.getValueFor( "yScale" ).c_str() );

    m_xHistory = std::atoi( m_settings.getValueFor( "xHistory" ).c_str() );

    // Add window to global map
    m_map.insert( m_map.begin() , std::pair<HWND , Graph*>( m_window , this ) );
}

Graph::~Graph() {
    // Wait for graph data reception thread to exit
    m_isRunning.store( false );
    m_graphThread.wait();

    DeleteObject( m_gridPen );
    DestroyWindow( m_window );

    // Remove window from global map
    m_map.erase( m_window );
}

void Graph::reconnect() {
    m_isRunning.store( false );
    m_graphThread.wait();

    m_isRunning.store( true );
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

    Pair gridPoint;

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
            Pair tempPoint;

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

            for ( std::list<Pair>::iterator pt = set->startingPoint ; pt != set->data.end() ; pt++ ) {
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

void Graph::addData( unsigned int index , const Pair& data ) {
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

    return set->color;
}

void Graph::createGraph( COLORREF color ) {
    m_dataMutex.lock();

    /* TODO Make list with no points; it's necessary now b/c start iterators
     * need a valid starting pt
     */
    std::list<Pair> data = { std::make_pair( 0.f , 0.f ) };
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

    std::vector<std::list<Pair>::iterator> points( m_dataSets.size() );
    std::vector<std::list<Pair>::iterator> ptEnds( m_dataSets.size() );

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

Pair Graph::transformToGraph( Pair point ) {
    RECT windowRect;
    GetClientRect( m_window , &windowRect );

    point.first = ( point.first - getXMin() ) * ( windowRect.right - windowRect.left ) / ( getXMax() - getXMin() );
    point.second = ( point.second - getYMin() ) * ( windowRect.bottom - windowRect.top ) / ( getYMax() - getYMin() );

    return point;
}

// Message handler for "Select Graphs" box
BOOL CALLBACK Graph::GraphDlgCbk( HWND hDlg , UINT message , WPARAM wParam , LPARAM lParam ) {
    switch ( message ) {
    case WM_INITDIALOG: {
        /* Add dialog to map since its real parent can't be retrieved with
         * GetParent() for some reason
         */
        m_map.insert( m_map.begin() , std::pair<HWND , Graph*>( hDlg , m_map[(HWND)lParam] ) );

        RECT windowSize;
        GetClientRect( hDlg , &windowSize );

        // Create check box for each data set
        unsigned int idIndex = 0;
        HGDIOBJ hfDefault = GetStockObject( DEFAULT_GUI_FONT );
        for ( unsigned int i = 0 ; i != m_map[(HWND)lParam]->m_graphNamesSize ; i++ ) {
            HWND checkBox = CreateWindowEx( 0,
                "BUTTON",
                m_map[hDlg]->m_graphNames[i].c_str(),
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
                13,
                idIndex * 20 + 13,
                100,
                13,
                hDlg,
                reinterpret_cast<HMENU>( IDC_GRAPHSTART  + idIndex ),
                GetModuleHandle(NULL),
                NULL);

            SendMessage(checkBox,
                WM_SETFONT,
                reinterpret_cast<WPARAM>( hfDefault ),
                MAKELPARAM( FALSE , 0 ) );

            CheckDlgButton( hDlg , IDC_GRAPHSTART + idIndex , m_map[hDlg]->m_curSelect & (1 << idIndex) );

            idIndex++;
        }

        return TRUE;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);

        if ( wmId == IDOK ) {
            // Remove dialog window from map since it's about to be destroyed
            m_map.erase( hDlg );

            EndDialog( hDlg , LOWORD(wParam) );
        }
        else if ( wmId >= IDC_GRAPHSTART ) {
            // Store pointer to selection bits
            unsigned long long* curSelect = &(m_map[hDlg]->m_curSelect);

            // Toggle state of check box
            CheckDlgButton( hDlg , wmId , !(*curSelect & (1 << (wmId - IDC_GRAPHSTART) )) );

            // Toggle selection bit in m_curSelect
            *curSelect ^= (1 << (wmId - IDC_GRAPHSTART) );
        }

        break;
    }

    case WM_CLOSE: {
        // Remove dialog window from map since it's about to be destroyed
        m_map.erase( hDlg );

        EndDialog( hDlg , 0 );

        break;
    }

    default: {
        return FALSE;
    }
    }

    return FALSE;
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
    enum Error {
        FailConnect = 0,
        Disconnected
    };

    sf::SocketSelector threadSelector;

    sf::TcpSocket dataSocket;

    sf::IpAddress remoteIP( m_settings.getValueFor( "robotIP" ) );
    unsigned short dataPort = std::strtoul( m_settings.getValueFor( "robotGraphPort" ).c_str() , NULL , 10 );

    sf::Socket::Status status = sf::Socket::Disconnected;

    char buffer[24];
    struct packet_t recvData;
    struct packet_list_t listData;
    size_t sizeRecv = 0;

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
                    status = dataSocket.receive( &buffer , sizeof(buffer) , sizeRecv );

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

        /* Allow user to select which data sets to receive
         * lParam argument is the parent window of the dialog box
         */
        DialogBoxParam( GetModuleHandle(NULL) , MAKEINTRESOURCE(IDD_GRAPHSELECTBOX) , m_window , GraphDlgCbk , (LPARAM)m_window );

        // Send updated status on all streams based on the bit array
        for ( unsigned int i = 0 ; i < m_graphNamesSize ; i++ ) {
            // If the graph data is requested
            if ( m_curSelect & (1 << i) ) {
                recvData.id = 'c';

                // Store name of data set to buffer
                std::strcpy( recvData.graphName , m_graphNames[i].c_str() );

                // Send request
                dataSocket.send( &recvData , 16 );
            }
            else {
                recvData.id = 'd';
            }

            /* Create a graph for each data set requested.
             * V starts out at 1. H cycles through six colors. When it wraps
             * around 360 degrees, V is decremented by 0.25. S is always 1.
             * This algorithm gives 25 possible values, one being black.
             */
            createGraph( HSVtoRGB( 60 * i % 360 , 1 , 1 - 0.25 * std::floor( i / 6 ) ) );
        }

        if ( status == sf::Socket::Done ) {
            int tmp; // Used for endianness conversions

            while ( m_isRunning ) {
                if ( threadSelector.wait( sf::milliseconds( 50 ) ) ) {
                    if ( threadSelector.isReady( dataSocket ) ) {
                        status = dataSocket.receive( &buffer , sizeof(buffer) , sizeRecv );

                        if ( status == sf::Socket::Done ) {
                            if ( buffer[0] == 'd' ) {
                                std::memcpy( &recvData , &buffer , sizeof(buffer) );

                                /* ===== Add sent point to local graph ===== */
                                // This will only work if ints are the same size as floats
                                assert( sizeof(int) == sizeof(float) );

                                // Convert endianness of x component
                                std::memcpy( &tmp , &recvData.x , sizeof(recvData.x) );
                                tmp = ntohl( tmp );
                                std::memcpy( &recvData.x , &tmp , sizeof(recvData.x) );

                                // Convert endianness of y component
                                std::memcpy( &tmp , &recvData.y , sizeof(recvData.y) );
                                tmp = ntohl( tmp );
                                std::memcpy( &recvData.y , &tmp , sizeof(recvData.y) );

                                /* If new data is off right of plot, move the plot so
                                 * the new data is displayed
                                 */
                                if ( recvData.x > getXMax() ) {
                                    setXMin( recvData.x - getHistoryLength() );
                                    setXMax( recvData.x );
                                }

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

                        redraw();
                    }
                }

                Sleep( 5 );
            }
        }
    }
    catch ( Error& exception ) {
        if ( exception == Error::FailConnect ) {
            MessageBox( m_window , "Connection to remote host failed" , "Connection Error" , MB_ICONERROR | MB_OK );
        }
        else if ( exception == Error::Disconnected ) {
            MessageBox( m_window , "Unexpected disconnection from remote host" , "Connection Error" , MB_ICONERROR | MB_OK );
        }
    }

    dataSocket.disconnect();
}
