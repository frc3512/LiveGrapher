// Copyright (c) 2013-2018 FRC Team 3512. All Rights Reserved.

#include "SelectDialog.hpp"

#include <QCheckBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalMapper>
#include <QVBoxLayout>

#include "Graph.hpp"

SelectDialog::SelectDialog(std::map<uint8_t, std::string>& graphNames,
                           Graph* graphData, QWidget* parent)
    : QDialog(parent) {
    m_okButton = new QPushButton(tr("&Ok"));
    connect(m_okButton, SIGNAL(released()), this, SLOT(close()));

    m_signalMapper = new QSignalMapper(this);

    m_scrollArea = new QScrollArea;
    QWidget* container = new QWidget(m_scrollArea);
    m_scrollArea->setWidget(container);

    m_graph = graphData;

    QVBoxLayout* checkList = new QVBoxLayout(container);
    QCheckBox* checkBox;
    for (const auto& name : graphNames) {
        checkBox = new QCheckBox(
            QString::fromUtf8(name.second.c_str(), name.second.size()));
        connect(checkBox, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
        m_signalMapper->setMapping(checkBox, name.first);
        checkList->addWidget(checkBox);
    }

    connect(m_signalMapper, SIGNAL(mapped(int)), this, SLOT(selectGraph(int)));

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_okButton);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(container);
    mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Select Graphs"));
}

void SelectDialog::selectGraph(int val) { m_graph->m_curSelect ^= (1 << val); }
