// Copyright (c) 2013-2020 FRC Team 3512. All Rights Reserved.

#include "SelectDialog.hpp"

#include <QCheckBox>
#include <QVBoxLayout>

#include "Graph.hpp"

SelectDialog::SelectDialog(const std::map<uint8_t, std::string>& graphNames,
                           Graph* graph, QWidget* parent)
    : QDialog(parent), m_graph(*graph) {
    setGeometry(100, 100, 260, 260);
    connect(&m_okButton, SIGNAL(released()), this, SLOT(close()));
    connect(&m_signalMapper, SIGNAL(mapped(int)), this, SLOT(selectGraph(int)));

    m_scrollArea.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_scrollArea.setWidgetResizable(true);
    m_scrollArea.setGeometry(10, 10, 200, 200);

    auto widget = new QWidget;
    m_scrollArea.setWidget(widget);

    auto checkList = new QVBoxLayout;
    for (const auto& [id, name] : graphNames) {
        auto checkBox = new QCheckBox(QString::fromStdString(name));
        connect(checkBox, SIGNAL(clicked()), &m_signalMapper, SLOT(map()));
        m_signalMapper.setMapping(checkBox, id);

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
