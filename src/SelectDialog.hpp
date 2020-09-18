// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <map>
#include <string>

#include <QDialog>

class Graph;

class SelectDialog : public QDialog {
    Q_OBJECT

public:
    SelectDialog(const std::map<uint8_t, std::string>& graphNames, Graph* graph,
                 QWidget* parent = nullptr);

private slots:
    void toggleGraphSelect(int i);

private:
    Graph& m_graph;
};
