// Copyright (c) FRC Team 3512, Spartatroniks 2013-2016. All Rights Reserved.

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <QDialog>

class QPushButton;
class QScrollArea;
class Graph;
class QSignalMapper;

class SelectDialog : public QDialog {
    Q_OBJECT

public:
    SelectDialog(std::vector<std::pair<uint8_t, std::string>>& graphNames,
                 Graph* graphData, QWidget* parent = nullptr);

private slots:
    void selectGraph(int val);

private:
    QPushButton* m_okButton;
    QScrollArea* m_scrollArea;
    Graph* m_graph;
    QSignalMapper* m_signalMapper;
};
