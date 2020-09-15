QT += network widgets printsupport

TARGET = LiveGrapher
TEMPLATE = app

CONFIG += c++17 debug_and_release

SOURCES += \
    src/Graph.cpp \
    src/Main.cpp \
    src/MainWindow.cpp \
    src/qcustomplot.cpp \
    src/Settings.cpp \
    src/SelectDialog.cpp

HEADERS += \
    src/Protocol.hpp \
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
