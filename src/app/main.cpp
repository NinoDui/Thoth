#include <QApplication>

#include "Q_AppMainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    Q_AppMainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}