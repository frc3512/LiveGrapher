#include <QApplication>
#include <QIcon>

#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    Q_INIT_RESOURCE(LiveGrapher);

    QApplication app(argc, argv);
    app.setOrganizationName("FRC Team 3512");
    app.setApplicationName("LiveGrapher");

    MainWindow mainWin;
    mainWin.setWindowIcon(QIcon(":/images/Spartatroniks.ico"));
    mainWin.show();

    return app.exec();
}
