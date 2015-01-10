#include "MainWindow.hpp"
#include "ui_MainWindow.h"

MainWindow::MainWindow( QWidget* parent ) :
    QMainWindow( parent ) ,
    m_ui( new Ui::MainWindow ) ,
    m_graph( this ) ,
    m_settings( "IPSettings.txt" )
{
    m_ui->setupUi( this );

    connect( m_ui->actionSave_As_CSV , SIGNAL(triggered()) , &m_graph , SLOT(saveAsCSV()) );
    connect( m_ui->actionAbout , SIGNAL(triggered()) , this , SLOT(about()) );
    connect( m_ui->connectButton , SIGNAL(released()) , this , SLOT(reconnect()) );
    connect( m_ui->clearDataButton , SIGNAL(released()) , this , SLOT(clearAllData()) );
    connect( m_ui->saveButton , SIGNAL(released()) , &m_graph , SLOT(saveAsCSV()) );

    QCustomPlot* customPlot = m_ui->plot;

    customPlot->xAxis->setTickLabelType( QCPAxis::ltDateTime );
    customPlot->xAxis->setDateTimeFormat( "mm:ss" );
    customPlot->xAxis->setAutoTickStep( false );
    customPlot->xAxis->setTickStep( 1 );
    customPlot->axisRect()->setupFullAxesBox();
    customPlot->legend->setVisible( true );

    // make left and bottom axes transfer their ranges to right and top axes:
    qRegisterMetaType<QCPRange>( "QCPRange" );
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

    /* Bind signals and slots for communication between the graph TCP client thread
     * and the UI thread */
    connect(&m_graph, SIGNAL(realtimeDataSignal(int,float,float)), this, SLOT(realtimeDataSlot(int,float,float)),
            Qt::BlockingQueuedConnection);
    connect(&m_graph, SIGNAL(infoDialogSignal(const QString&,const QString&)), this, SLOT(infoDialog(const QString&,const QString&)),
            Qt::QueuedConnection);
    connect(&m_graph, SIGNAL(criticalDialogSignal(const QString&,const QString&)), this, SLOT(criticalDialog(const QString&,const QString&)),
            Qt::QueuedConnection);

    m_xHistory = m_settings.getFloat( "xHistory" );
}

MainWindow::~MainWindow() {
    delete m_ui;
}

void MainWindow::infoDialog( const QString& title , const QString& text ) {
    QMessageBox::information( this , title , text );
}

void MainWindow::criticalDialog( const QString& title , const QString& text ) {
    QMessageBox::critical( this , title , text );
}

void MainWindow::about() {
    QMessageBox::about( this , tr("About LiveGrapher") ,
            tr("<br>LiveGrapher, Version 1.0<br>"
               "Copyright &copy;2013-2015 FRC Team 3512<br>"
               "FRC Team 3512<br>"
               "All Rights Reserved") );
}

void MainWindow::reconnect() {
    m_graph.reconnect();
}

void MainWindow::clearAllData() {
    m_graph.clearAllData();
}

void MainWindow::realtimeDataSlot( int graphId , float x , float y ) {
    m_uiMutex.lock();

    // Add data to lines
    m_ui->plot->graph( graphId )->addData( x , y );

    // Remove data of lines that are outside visible range
    for ( int i = 0 ; i < m_ui->plot->graphCount() ; i++ ) {
        m_ui->plot->graph( i )->removeDataBefore( x - m_xHistory );
    }

    // Rescale value (vertical) axis to fit the current data
    if ( m_ui->plot->graphCount() > 0 ) {
        m_ui->plot->graph( 0 )->rescaleValueAxis();

        for ( int i = 1 ; i < m_ui->plot->graphCount() ; i++ ) {
            m_ui->plot->graph( i )->rescaleValueAxis( true );
        }
    }

    // Make key axis range scroll with the data (at a constant range size)
    m_ui->plot->xAxis->setRange( x , m_xHistory , Qt::AlignRight );
    m_ui->plot->replot();

    m_uiMutex.unlock();
}
