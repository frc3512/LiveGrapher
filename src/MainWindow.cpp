// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "MainWindow.hpp"

#include <QMessageBox>

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_ui.setupUi(this);

    m_ui.plot->setInteraction(QCP::iRangeDrag, true);
    m_ui.plot->setInteraction(QCP::iRangeZoom, true);

    connect(m_ui.actionScreenshot_Graph, SIGNAL(triggered()), &m_graph,
            SLOT(ScreenshotGraph()));
    connect(m_ui.actionSave_As_CSV, SIGNAL(triggered()), &m_graph,
            SLOT(SaveAsCSV()));
    connect(m_ui.actionAbout, &QAction::triggered, [this] {
        QMessageBox::about(this, tr("About LiveGrapher"),
                           tr("<br>LiveGrapher 3.1<br>"
                              "Copyright &copy;2013-2017 FRC Team 3512<br>"
                              "FRC Team 3512<br>"
                              "All Rights Reserved"));
    });
    connect(m_ui.connectButton, SIGNAL(released()), this, SLOT(Reconnect()));

    /* Pauses graphing so user can inspect it (disables auto-scrolling but data
     * is still appended to any graphs)
     */
    connect(m_ui.pauseButton, &QPushButton::released, [this] {
        if (m_isPlaying) {
            m_ui.pauseButton->setText("Play");
            m_isPlaying = false;
        } else if (m_graph.IsConnected()) {
            m_ui.pauseButton->setText("Pause");
            m_isPlaying = true;
        }
    });

    connect(m_ui.clearButton, &QPushButton::released,
            [this] { m_graph.ClearAllData(); });
    connect(m_ui.screenshotButton, SIGNAL(released()), &m_graph,
            SLOT(ScreenshotGraph()));
    connect(m_ui.saveButton, SIGNAL(released()), &m_graph, SLOT(SaveAsCSV()));

    QCustomPlot& customPlot = *m_ui.plot;

    customPlot.xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot.xAxis->setDateTimeFormat("mm:ss");
    customPlot.xAxis->setAutoTickStep(false);
    customPlot.xAxis->setTickStep(1);
    customPlot.xAxis->setLabel("Time (s)");
    customPlot.yAxis->setLabel("Data");
    customPlot.axisRect()->setupFullAxesBox();
    customPlot.legend->setVisible(true);

    // Make left and bottom axes transfer their ranges to right and top axes:
    qRegisterMetaType<QCPRange>("QCPRange");
    connect(customPlot.xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot.xAxis2,
            SLOT(setRange(QCPRange)));
    connect(customPlot.yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot.yAxis2,
            SLOT(setRange(QCPRange)));

    m_xHistory = m_settings.getDouble("xHistory");
    m_lastTime = std::chrono::steady_clock::now();
}

void MainWindow::Reconnect() {
    if (m_graph.IsConnected()) {
        m_ui.connectButton->setText("Connect");
        m_graph.Disconnect();
    } else {
        m_graph.Reconnect();

        if (m_graph.IsConnected()) {
            m_ui.connectButton->setText("Disconnect");

            m_isPlaying = true;
            m_ui.pauseButton->setText("Pause");
        }
    }
}

void MainWindow::AddData(int graphId, float x, float y) {
    QCustomPlot& plot = *m_ui.plot;

    // Don't draw anything if there are no graphs
    if (plot.graphCount() == 0) {
        return;
    }

    // Add data to lines
    plot.graph(graphId)->addData(x, y);

    if (std::chrono::steady_clock::now() - m_lastTime > 250ms) {
        // Don't rescale X axis when paused
        if (m_isPlaying) {
            for (int i = 0; i < plot.graphCount(); i++) {
                // Rescale value (vertical) axis to fit the current data
                if (i == 0) {
                    // Shrink window to fit first plot
                    plot.graph(i)->rescaleValueAxis(false);
                } else {
                    // Expand window to fit all subsequent plots
                    plot.graph(i)->rescaleValueAxis(true);
                }
            }
        }

        m_lastTime = std::chrono::steady_clock::now();
    }

    // Don't rescale Y axis when paused
    if (m_isPlaying) {
        // Make key axis range scroll with the data (at a constant range size)
        plot.xAxis->setRange(x, m_xHistory, Qt::AlignRight);
    }

    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    static uint64_t lastTime = 0;
    uint64_t currentTime =
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
            .count();

    // Limit refresh rate to 30 FPS
    if (currentTime - lastTime > 1000 / 30) {
        plot.replot();
        lastTime = currentTime;
    }
}
