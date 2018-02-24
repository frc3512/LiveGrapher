// Copyright (c) 2013-2018 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <map>
#include <string>

#include <QDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalMapper>

class Graph;

class SelectDialog : public QDialog {
    Q_OBJECT

public:
    SelectDialog(std::map<uint8_t, std::string>& graphNames, Graph* graphData,
                 QWidget* parent = nullptr);

private slots:
    void selectGraph(int val);

private:
    Graph& m_graph;
    QPushButton m_okButton{QObject::tr("&Ok")};
    QScrollArea m_scrollArea;
    QSignalMapper m_signalMapper{this};
};
