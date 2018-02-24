// Copyright (c) 2013-2018 FRC Team 3512. All Rights Reserved.

#include "SelectDialog.hpp"

#include <QCheckBox>
#include <QVBoxLayout>

#include "Graph.hpp"

SelectDialog::SelectDialog(std::map<uint8_t, std::string>& graphNames,
                           Graph* graphData, QWidget* parent)
    : QDialog(parent), m_graph(*graphData) {
    setGeometry(100, 100, 260, 260);
    connect(&m_okButton, SIGNAL(released()), this, SLOT(close()));
    connect(&m_signalMapper, SIGNAL(mapped(int)), this, SLOT(selectGraph(int)));

    m_scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_scrollArea.setWidgetResizable(true);
    m_scrollArea.setGeometry(10, 10, 200, 200);

    auto widget = new QWidget;
    m_scrollArea.setWidget(widget);

    auto checkList = new QVBoxLayout;
    for (const auto& name : graphNames) {
        auto checkBox = new QCheckBox(
            QString::fromUtf8(name.second.c_str(), name.second.size()));
        connect(checkBox, SIGNAL(clicked()), &m_signalMapper, SLOT(map()));
        m_signalMapper.setMapping(checkBox, name.first);
        checkList->addWidget(checkBox);
    }
    widget->setLayout(checkList);

    auto mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    mainLayout->addWidget(&m_scrollArea);

    auto bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(&m_okButton);
    mainLayout->addLayout(bottomLayout);

    setWindowTitle(QObject::tr("Select Graphs"));
}

void SelectDialog::selectGraph(int val) { m_graph.m_curSelect ^= (1 << val); }
