//=============================================================================
//File Name: Graph.hpp
//Description: Draws a graph in WinGDI
//Author: FRC Team 3512, Spartatroniks
//=============================================================================

#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "Drawable.hpp"
#include <list>
#include <utility>
#include <map>
#include <string>
#include <atomic>

#include "../SFML/System/Mutex.hpp"
#include "../SFML/System/Thread.hpp"

typedef std::pair<float , float> Pair;

struct DataSet {
    DataSet( std::list<std::pair<float , float>> n_data , COLORREF n_color );

    std::list<std::pair<float , float>> data;
    std::list<std::pair<float , float>>::iterator startingPoint;
    COLORREF color;
};

class GraphClassInit;

class Graph : public Drawable {
public:
    Graph( HWND parentWindow , const Vector2i& position , const Vector2i& size );
    virtual ~Graph();

    // Kills receiving thread and restarts it; this function will block
    void reconnect();

    // Add data point to graph at given index (push back)
    void addData( unsigned int index , const std::pair<float , float>& point );

    // Remove oldest data point from graph at given index (pop front)
    void removeData( unsigned int index );

    // Removes all previous data from all graphs
    void clearAllData();

    // Set color of graph at given index
    void setGraphColor( unsigned int index , COLORREF color );

    // Get color of graph at given index
    COLORREF getGraphColor( unsigned int index );

    // Create another set of data to graph
    void createGraph( COLORREF color );

    // Remove graph at the given index
    void removeGraph( unsigned int index );

    // Set yMin of plot
    void setYMin( int yMin );

    // Set yMax of plot
    void setYMax( int yMax );

    // Set number of points between vertical grid line
    void setXScale( int xScale );

    // Set number of points between horizontal grid line
    void setYScale( int yScale );

    // Set length of history
    void setHistoryLength( unsigned int length );

    // Get length of history
    unsigned int getHistoryLength();

    /* Saves all graph data to CSV in the executable's directory
     * returns true upon success
     */
    bool saveToCSV( const std::string& fileName );

    void redraw();

    // Transforms/normalizes real coordinates to coordinates used by graph
    std::pair<float , float> transformToGraph( std::pair<float , float> point );

private:
    // Set xMin of plot
    void setXMin( int xMin );

    // Set xMax of plot
    void setXMax( int xMax );

    // Get xMin of plot
    int getXMin();

    // Get xMax of plot
    int getXMax();

    // Get yMin of plot
    int getYMin();

    // Get yMax of plot
    int getYMax();

    // Get xScale of plot
    int getXScale();

    // Get yScale of plot
    int getYScale();

    void draw( PAINTSTRUCT* ps );

    HWND m_window;

    HPEN m_gridPen;

    // Contains graph data to plot
    std::list<DataSet> m_dataSets;

    int m_xMin;
    int m_yMin;
    int m_xMax;
    int m_yMax;

    int m_xScale;
    int m_yScale;

    // Tells how far back xMin is from xMax when xMax is shifted
    int m_xHistory;

    sf::Mutex m_dataMutex;

    sf::Thread m_graphThread;
    volatile std::atomic<bool> m_isRunning;
    void graphThreadFunc();

    static std::map<HWND , Graph*> m_map;
    static LRESULT CALLBACK WindowProc( HWND handle , UINT message , WPARAM wParam , LPARAM lParam );

    friend GraphClassInit;
};

#endif // GRAPH_HPP
