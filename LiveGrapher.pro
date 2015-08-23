#-------------------------------------------------
#
# Project created by QtCreator 2015-01-01T21:16:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network printsupport

TARGET = LiveGrapher
TEMPLATE = app
CONFIG += c++14

SOURCES +=\
    src/Graph.cpp \
    src/Main.cpp \
    src/MainWindow.cpp \
    src/qcustomplot.cpp \
    src/Settings.cpp \
    src/SelectDialog.cpp

HEADERS  += \
    src/Graph.hpp \
    src/MainWindow.hpp \
    src/qcustomplot.h \
    src/Settings.hpp \
    src/SelectDialog.hpp

FORMS    += MainWindow.ui

RESOURCES += \
    LiveGrapher.qrc

DISTFILES += \
    Resources.rc
