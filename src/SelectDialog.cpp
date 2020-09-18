// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "SelectDialog.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalMapper>
#include <QVBoxLayout>

#include "Graph.hpp"

SelectDialog::SelectDialog(const std::map<uint8_t, std::string>& graphNames,
                           Graph* graph, QWidget* parent)
    : QDialog(parent), m_graph(*graph) {
    setWindowTitle("Select Graphs");
    setGeometry(100, 100, 260, 260);

    auto signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(int)), this,
            SLOT(toggleGraphSelect(int)));

    auto checkList = new QVBoxLayout;
    for (const auto& [id, name] : graphNames) {
        auto checkBox = new QCheckBox(QString::fromStdString(name));
        connect(checkBox, SIGNAL(clicked()), signalMapper, SLOT(map()));
        signalMapper->setMapping(checkBox, id);

        // Set the initial checkbox state to the selection choice from the
        // previous connection. The choices are all overrridden with unchecked
        // if the list of graph names changed since the last connection.
        if (m_graph.m_curSelect & (1 << id)) {
            checkBox->setCheckState(Qt::Checked);
        } else {
            checkBox->setCheckState(Qt::Unchecked);
        }

        checkList->addWidget(checkBox);
    }

    auto scrollArea = new QScrollArea(this);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setWidgetResizable(true);
    scrollArea->setGeometry(10, 10, 200, 200);
    auto widget = new QWidget;
    widget->setLayout(checkList);
    scrollArea->setWidget(widget);

    auto okButton = new QPushButton("&Ok");
    connect(okButton, SIGNAL(released()), this, SLOT(close()));

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(okButton);
    setLayout(mainLayout);
}

void SelectDialog::toggleGraphSelect(int i) { m_graph.m_curSelect ^= 1 << i; }
