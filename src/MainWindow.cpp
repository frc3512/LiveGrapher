// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "MainWindow.hpp"

#include <QMessageBox>
#include <QtWidgets/QAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("LiveGrapher");
    resize(640, 480);

    auto centralWidget = new QWidget(this);

    plot = new QCustomPlot(centralWidget);
    plot->setInteraction(QCP::iRangeDrag, true);
    plot->setInteraction(QCP::iRangeZoom, true);
    plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    plot->xAxis->setDateTimeFormat("mm:ss");
    plot->xAxis->setAutoTickStep(false);
    plot->xAxis->setTickStep(1);
    plot->xAxis->setLabel("Time (s)");
    plot->yAxis->setLabel("Data");
    plot->axisRect()->setupFullAxesBox();
    plot->legend->setVisible(true);

    // Make left and bottom axes transfer their ranges to right and top axes
    qRegisterMetaType<QCPRange>("QCPRange");
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), plot->xAxis2,
            SLOT(setRange(QCPRange)));
    connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), plot->yAxis2,
            SLOT(setRange(QCPRange)));

    auto buttonLayout = new QHBoxLayout;

    // Pauses graphing so user can inspect it (disables auto-scrolling but data
    // is still appended to any graphs)
    auto pauseButton = new QPushButton("Play", centralWidget);
    connect(pauseButton, &QPushButton::released, [=] {
        if (m_isPlaying) {
            pauseButton->setText("Play");
            m_isPlaying = false;
        } else if (m_graph.IsConnected()) {
            pauseButton->setText("Pause");
            m_isPlaying = true;
        }
    });

    auto connectButton = new QPushButton("Connect", centralWidget);
    connect(connectButton, &QPushButton::released, [=] {
        if (m_graph.IsConnected()) {
            connectButton->setText("Connect");
            m_graph.Disconnect();
        } else {
            m_graph.Reconnect();

            if (m_graph.IsConnected()) {
                connectButton->setText("Disconnect");

                m_isPlaying = true;
                pauseButton->setText("Pause");
            }
        }
    });

    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(pauseButton);

    buttonLayout->addItem(
        new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    auto clearButton = new QPushButton("Clear", centralWidget);
    connect(clearButton, &QPushButton::released,
            [this] { m_graph.ClearAllData(); });
    buttonLayout->addWidget(clearButton);

    auto screenshotButton = new QPushButton("Screenshot", centralWidget);
    connect(screenshotButton, SIGNAL(released()), &m_graph,
            SLOT(ScreenshotGraph()));
    buttonLayout->addWidget(screenshotButton);

    auto saveButton = new QPushButton("CSV", centralWidget);
    connect(saveButton, SIGNAL(released()), &m_graph, SLOT(SaveAsCSV()));
    buttonLayout->addWidget(saveButton);

    auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(plot);
    mainLayout->addLayout(buttonLayout);
    centralWidget->setLayout(mainLayout);

    setCentralWidget(centralWidget);

    auto menuFile = menuBar()->addMenu("File");

    auto actionScreenshot_Graph = new QAction("Screenshot Graph", this);
    connect(actionScreenshot_Graph, SIGNAL(triggered()), &m_graph,
            SLOT(ScreenshotGraph()));
    menuFile->addAction(actionScreenshot_Graph);

    auto actionSave_As_CSV = new QAction("Save As CSV", this);
    connect(actionSave_As_CSV, SIGNAL(triggered()), &m_graph,
            SLOT(SaveAsCSV()));
    menuFile->addAction(actionSave_As_CSV);

    auto menuAbout = menuBar()->addMenu("Help");

    auto actionAbout = new QAction("About LiveGrapher", this);
    connect(actionAbout, &QAction::triggered, [this] {
        QMessageBox::about(this, tr("About LiveGrapher"),
                           tr("<br>LiveGrapher 3.1<br>"
                              "Copyright &copy;2013-2020 FRC Team 3512<br>"
                              "FRC Team 3512<br>"
                              "All Rights Reserved"));
    });
    menuAbout->addAction(actionAbout);
}

void MainWindow::AddData(int graphId, float x, float y) {
    // Don't draw anything if there are no graphs
    if (plot->graphCount() == 0) {
        return;
    }

    // Add data to lines
    plot->graph(graphId)->addData(x, y);

    if (std::chrono::steady_clock::now() - m_lastTime > 250ms) {
        // Don't rescale X axis when paused
        if (m_isPlaying) {
            for (int i = 0; i < plot->graphCount(); ++i) {
                // Rescale value (vertical) axis to fit the current data. If
                // it's the first plot, shrink the window to fit. Otherwise,
                // expand the window to fit.
                plot->graph(i)->rescaleValueAxis(i != 0);
            }
        }

        m_lastTime = std::chrono::steady_clock::now();
    }

    // Don't rescale Y axis when paused
    if (m_isPlaying) {
        // Make key axis range scroll with the data (at a constant range size)
        plot->xAxis->setRange(x, m_xHistory, Qt::AlignRight);
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
        plot->replot();
        lastTime = currentTime;
    }
}
