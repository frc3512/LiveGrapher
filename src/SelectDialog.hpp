// Copyright (c) 2013-2017 FRC Team 3512. All Rights Reserved.

#pragma once

#include <map>
#include <string>

#include <QDialog>

class QPushButton;
class QScrollArea;
class Graph;
class QSignalMapper;

class SelectDialog : public QDialog {
    Q_OBJECT

public:
    SelectDialog(std::map<uint8_t, std::string>& graphNames, Graph* graphData,
                 QWidget* parent = nullptr);

private slots:
    void selectGraph(int val);

private:
    QPushButton* m_okButton;
    QScrollArea* m_scrollArea;
    Graph* m_graph;
    QSignalMapper* m_signalMapper;
};
