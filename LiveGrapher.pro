QT += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = LiveGrapher
TEMPLATE = app
lessThan(QT_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -std=c++1y
greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++14

CONFIG += debug_and_release

SOURCES += \
    src/Graph.cpp \
    src/Main.cpp \
    src/MainWindow.cpp \
    src/qcustomplot.cpp \
    src/Settings.cpp \
    src/SelectDialog.cpp

HEADERS += \
    common/Protocol.hpp \
    src/Graph.hpp \
    src/MainWindow.hpp \
    src/qcustomplot.h \
    src/Settings.hpp \
    src/SelectDialog.hpp

FORMS += MainWindow.ui

RESOURCES += \
    LiveGrapher.qrc

DISTFILES += \
    Resources.rc
