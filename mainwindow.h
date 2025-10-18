#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPalette>
#include <QDebug>                       // библиотека для вывода отладочных сообщений
#include <QString>
#include <QTextBrowser>
#include <QTableWidget>
#include <QtSerialPort/QSerialPort>

constexpr uint16_t DATA_SIZE{1544};
constexpr uint8_t ROWS{24};
constexpr uint8_t COLS{32};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setTerminal();     // конфигурируем текстовое окно для отладочной информации
    void setMainWindow();   // настраиваем главное окно
    void setTableImage();   // конфигурируем таблицу с цветовыми значениями
    void editCells();       //

    void dataProcessing(QByteArray frame_f);

private slots:
    void Read_data();   // создаем слот для чтения данных из порта

private:
    Ui::MainWindow * ui;        // графический интерфейс

    // QTableWidget tableWidget;   // объект таблицы
    // QTextBrowser *textBrowser;

    QSerialPort *COMPORT;       // объект порта

    QByteArray serialBuffer;
    QByteArray frame;           // преобразованный кадр с датчика
    QByteArray newData;

    double temp[ROWS][COLS];    // матрица температур 24 строки на 32 столбца

    int dataAmountOfThisFrame;  // данные о кадре
    QString dataAmountOfThisFrame_text;  // преобразованные в строку данные о кадре

    double ownTemperature;      // собственная температура датчика
    QString ownTemperature_text; // строка "собственная температура датчика"

    int cumulativeSum;          // контрольная сумма кадра
    QString cumulativeSum_text; // строка "контрольная сумма кадра"

};
#endif // MAINWINDOW_H
