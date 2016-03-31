#include "MainWindow.hpp"

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    m_graph(this),
    m_settings("IPSettings.txt") {
    m_ui = std::make_unique<Ui::MainWindow>();
    m_ui->setupUi(this);

    connect(m_ui->actionSave_As_CSV, SIGNAL(triggered()),
            &m_graph, SLOT(saveAsCSV()));
    connect(m_ui->actionAbout, SIGNAL(triggered()),
            this, SLOT(about()));
    connect(m_ui->connectButton, SIGNAL(released()),
            this, SLOT(reconnect()));
    connect(m_ui->clearDataButton, SIGNAL(released()),
            this, SLOT(clearAllData()));
    connect(m_ui->saveButton, SIGNAL(released()),
            &m_graph, SLOT(saveAsCSV()));

    QCustomPlot* customPlot = m_ui->plot;

    customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot->xAxis->setDateTimeFormat("mm:ss");
    customPlot->xAxis->setAutoTickStep(false);
    customPlot->xAxis->setTickStep(1);
    customPlot->xAxis->setLabel("Time (s)");
    customPlot->yAxis->setLabel("Data");
    customPlot->axisRect()->setupFullAxesBox();
    customPlot->legend->setVisible(true);

    // make left and bottom axes transfer their ranges to right and top axes:
    qRegisterMetaType<QCPRange>("QCPRange");
    connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)),
            customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)),
            customPlot->yAxis2, SLOT(setRange(QCPRange)));

    /* Bind signals and slots for communication between the graph TCP client
     * thread and the UI thread
     */
    connect(&m_graph, SIGNAL(realtimeDataSignal(int, float, float)),
            this, SLOT(realtimeDataSlot(int, float, float)));

    m_xHistory = m_settings.getDouble("xHistory");
    m_lastTime = std::chrono::steady_clock::now();
}

void MainWindow::infoDialog(const QString& title, const QString& text) {
    QMessageBox::information(this, title, text);
}

void MainWindow::criticalDialog(const QString& title, const QString& text) {
    QMessageBox::critical(this, title, text);
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About LiveGrapher"),
                       tr("<br>LiveGrapher 3.0<br>"
                          "Copyright &copy;2013-2016 FRC Team 3512<br>"
                          "FRC Team 3512<br>"
                          "All Rights Reserved"));
}

void MainWindow::reconnect() {
    m_graph.reconnect();
}

void MainWindow::clearAllData() {
    m_graph.clearAllData();
}

void MainWindow::realtimeDataSlot(int graphId, float x, float y) {
    QCustomPlot* plot = m_ui->plot;

    // Don't draw anything if there are no graphs
    if (plot->graphCount() == 0) {
        return;
    }

    // Add data to lines
    plot->graph(graphId)->addData(x, y);

    if (std::chrono::steady_clock::now() - m_lastTime > 250ms) {
        for (int i = 0; i < plot->graphCount(); i++) {
            // Rescale value (vertical) axis to fit the current data
            if (i == 0) {
                plot->graph(i)->rescaleValueAxis();
            }
            else {
                plot->graph(i)->rescaleValueAxis(true);
            }

            // Remove data of lines that are outside visible range
            // plot->graph(i)->removeDataBefore(x - m_xHistory);
        }

        m_lastTime = std::chrono::steady_clock::now();
    }

    // Make key axis range scroll with the data (at a constant range size)
    plot->xAxis->setRange(x, m_xHistory, Qt::AlignRight);

    static uint64_t lastTime = 0;
    static uint64_t currentTime;
    currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (currentTime - lastTime > 1000 / 30) {
        plot->replot();
        lastTime = currentTime;
    }
}
