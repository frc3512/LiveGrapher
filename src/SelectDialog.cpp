#include "SelectDialog.hpp"

#include <QPushButton>
#include <QScrollArea>
#include <QSignalMapper>
#include <QVBoxLayout>
#include <QCheckBox>
#include "Graph.hpp"

SelectDialog::SelectDialog(std::vector<std::string>& graphNames,
                           Graph* graphData,
                           QWidget* parent) :
    QDialog(parent) {
    m_okButton = new QPushButton(tr("&Ok"));
    connect(m_okButton, SIGNAL(released()), this, SLOT(close()));

    m_signalMapper = new QSignalMapper(this);

    m_scrollArea = new QScrollArea;
    QWidget* container = new QWidget(m_scrollArea);
    m_scrollArea->setWidget(container);

    m_graph = graphData;

    QVBoxLayout* checkList = new QVBoxLayout(container);
    QCheckBox* checkBox;
    for (unsigned int i = 0; i < graphNames.size(); i++) {
        checkBox = new QCheckBox(QString::fromUtf8(graphNames[i].c_str(),
                                                   graphNames[i].size()));
        connect(checkBox, SIGNAL(clicked()),
                m_signalMapper, SLOT(map()));
        m_signalMapper->setMapping(checkBox, i);
        checkList->addWidget(checkBox);
    }

    connect(m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(selectGraph(int)));

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_okButton);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(container);
    mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Select Graphs"));
}

void SelectDialog::selectGraph(int val) {
    m_graph->m_curSelect ^= (1 << val);
}
