// main.cpp -- получение, обработка и визуализация данных с тепловизора

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.setWindowTitle("GYMCU90640 Module");

    w.show();

    return app.exec();
}
