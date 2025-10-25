#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QMainWindow>
#include <QPalette>
#include <QDebug>                       // библиотека для вывода отладочных сообщений
#include <QTextBrowser>
#include <QtSerialPort/QSerialPort>
#include <QTableWidgetItem>
#include <QColor>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setMainWindow();
    setTerminal();
    setTableImage();

    COMPORT = new QSerialPort();

    // настраиваем параметры порта
    COMPORT->setPortName("/dev/ttyUSB0");
    COMPORT->setBaudRate(QSerialPort::Baud115200);
    COMPORT->setDataBits(QSerialPort::Data8);
    COMPORT->setParity(QSerialPort::NoParity);
    COMPORT->setStopBits(QSerialPort::OneStop);
    COMPORT->setFlowControl(QSerialPort::NoFlowControl);

    // соединяем сигнал о прочитанных данных и слот Read_data()
    connect(COMPORT, &QIODevice::readyRead, this, &MainWindow::Read_data);

    // включаем порт
    if (COMPORT->open(QIODevice::ReadOnly)) {
        // qDebug() << "Serial port" << COMPORT->portName() << "is open";
        ui->terminal->append("Serial port " + COMPORT->portName() + " is open");

    }
    else {
        // qDebug() << "Serial port is not open." << COMPORT->error();
        ui->terminal->append("Serial port is not open");
    }
}

MainWindow::~MainWindow() {
    delete ui;
}

// читаем данные из com-порта
void MainWindow::Read_data() {
    if (COMPORT->isOpen() && COMPORT->bytesAvailable() > 0) {
        newData = COMPORT->readAll();
        serialBuffer.append(newData);               // добавляем в буффер байты из ком-порта

        // ищем начало кадра
        // проверяем наличие байтов 5A 5A и размер буффера
        while (serialBuffer.contains("\x5A\x5A") && serialBuffer.size() >= DATA_SIZE) {
            // назначаем начало кадра
            int frameStart = serialBuffer.indexOf("\x5A\x5A");

            // возвращаем достаточное количество данных
            if (serialBuffer.size() - frameStart >= DATA_SIZE) {

                // mid() возвращает байты между указанными индексами
                frame = serialBuffer.mid(frameStart, DATA_SIZE);
                dataProcessing(frame);

                // очищаем буффер
                serialBuffer.remove(0, frameStart + DATA_SIZE);

                // выводим кадр
                // qDebug() << "=== FRAME START ===";
                // qDebug() << frame.toHex(' ').toUpper();
                // qDebug() << "=== FRAME END ===";

                // ui->terminal->append("=== FRAME START ===\n");
                // ui->terminal->append(frame.toHex(' ').toUpper());
                // ui->terminal->append("=== FRAME END ===\n");

            } else break;
        }
    }
}

// Обработка данных из кадра
void MainWindow::dataProcessing(QByteArray frame_f) {

    // Принимаем в качестве аргумента готовый фрейм.
    // Обрабатываем:
    // - данные об объеме кадра (byte3 * 256 + byte2)
    // - температуру окружающей среды (собственную температуру датчика),
    // - констрольную сумму последних 771 байтов.

    // Вычисляем объем полезных данных
    dataAmountOfThisFrame = (frame_f.at(3) << 8) + frame_f.at(2);

    // qDebug() << "Объем данных:" << dataAmountOfThisFrame;

    ui->terminal->append("\n=== FRAME START ===\n");
    dataAmountOfThisFrame_text = QStringLiteral("Объем данных: ") + QString::number(dataAmountOfThisFrame);
    ui->terminal->insertPlainText(dataAmountOfThisFrame_text + "\n");

    // Вычисляем температуру (768 точек) и записываем в матрицу 32x24
    uint16_t i = 4;
    for (uint8_t row = 0; row < ROWS; row++) {
        for (uint8_t col = 0; col < COLS; col++) {

            // идем по столбцам справа налево,
            // а по строкам -- сверху вниз
            double temperature = ((frame_f[i + 1] << 8) + frame_f[i]) / 100.0;

            uint8_t actualCol = COLS - 1 - col;

            temp[row][actualCol] = temperature;

            i += 2;
        }
    }

    // Заполняем таблицу цветами
    editCells();

    // Вычисляем собственную температуру датчика
    ownTemperature = ((frame_f.at(1541) << 8) + frame_f.at(1540)) / 100.0;

    // Выводим в терминал
    // qDebug() << "Температура датчика:" << ownTemperature;
    ownTemperature_text = QStringLiteral("Температура датчика: ") + QString::number(ownTemperature);
    ui->terminal->insertPlainText(ownTemperature_text + "\n");

    // Вычисляем контрольную сумму
    cumulativeSum = (frame_f.at(1543) << 8) + frame_f.at(1542);

    // Выводим в терминал
    // qDebug() << "Контрольная сумма:" << (frame_f.at(1543) << 8) + frame_f.at(1542) << "\r\n";
    cumulativeSum_text = QStringLiteral("Контрольная сумма: ") + QString::number(cumulativeSum);
    ui->terminal->insertPlainText(cumulativeSum_text + "\n");
    ui->terminal->insertPlainText("=== FRAME END ===");

    QScrollBar *scrollBar = ui->terminal->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    // Вывод температуры в terminal
    // ui->terminal->append("==========================================================");
    // QString rowStr;
    // for (int row = 0; row < ROWS; ++row) {
    //     for (int col = 0; col < COLS; ++col) {
    //         rowStr += QString("%1\t").arg(temp[row][col], 1, 'f', 2);
    //     }
    //     ui->terminal->append(rowStr);
    // }
    // ui->terminal->append("==========================================================");

    // Вывод температуры в qDebug()
    // qDebug() << "==========================================================";
    // for (int row = 0; row < ROWS; ++row) {
    //     QString rowStr;
    //     for (int col = 0; col < COLS; ++col) {
    //         rowStr += QString("%1\t").arg(temp[row][col], 1, 'f', 2);
    //     }
    //     qDebug().noquote() << rowStr;
    // }
    // qDebug() << "==========================================================";

}

// меняем размер и цвет текста и фона terminal
void MainWindow::setTerminal() {
    QFont font = ui->terminal->font();
    font.setPointSize(14); // Установить размер в пунктах
    ui->terminal->setFont(font);

    // Устанавливаем фон текстового окна
    ui->terminal->setTextColor(QColor(255, 255, 255));
    ui->terminal->setPlainText(ui->terminal->toPlainText());
    ui->terminal->setStyleSheet("background-color: black;");
}

// меняем фон приложения
void MainWindow::setMainWindow() {
    QPixmap bkgnd(":/bkgnd/bkgnd.jpeg");
    QPixmap scaled_bkgnd = bkgnd.scaled(MainWindow::size().width(), MainWindow::size().height());
    QPalette palette;
    palette.setBrush(QPalette::Window, scaled_bkgnd);
    MainWindow::setPalette(palette);
}

void MainWindow::setTableImage() {
    ui->tableImage->setRowCount(24);
    ui->tableImage->setColumnCount(32);

    // устанавливаем шрифт и его размер
    QFont font("Arial", 5);
    ui->tableImage->setFont(font);

    // устанавливаем цвет границ и фона ячеек
    ui->tableImage->setStyleSheet("QTableWidget { gridline-color: black; background-color: black; }");

    // делаем невидимыми определение заголовков строк и столбцов
    ui->tableImage->horizontalHeader()->setVisible(false);
    ui->tableImage->verticalHeader()->setVisible(false);

    // устанавливаем квадратную форму ячеек
    ui->tableImage->horizontalHeader()->setCascadingSectionResizes(false);
    ui->tableImage->verticalHeader()->setCascadingSectionResizes(false);

    int height = 18;    // длина ребра ячейки
    ui->tableImage->verticalHeader()->setMinimumSectionSize(height);   // минимальный размер секции
    ui->tableImage->verticalHeader()->setDefaultSectionSize(height);   // устанавливаем дефолтный размер
    ui->tableImage->verticalHeader()->setMaximumSectionSize(height);   // максимальный размер секции

    ui->tableImage->horizontalHeader()->setMinimumSectionSize(height); // аналогично
    ui->tableImage->horizontalHeader()->setMaximumSectionSize(height);
    ui->tableImage->horizontalHeader()->setDefaultSectionSize(height);

    // растягиваем ячейки
    ui->tableImage->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableImage->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // отключаем редактирование ячеек
    ui->tableImage->setEditTriggers(QAbstractItemView::NoEditTriggers);
}


// создаем таблицу цветных ячеек
void MainWindow::editCells() {

    // Находим минимальное и максимальное значения
    double minVal = temp[0][0];
    double maxVal = temp[0][0];

    // Диапазонный цикл for
    // Сначала идем по строкам
    for (const auto &row : temp) {
        // Затем по каждому элементу строки
        for (double val : row) {
            if (val < minVal) minVal = val;
            if (val > maxVal) maxVal = val;
        }
    }

    // Заполняем таблицу
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {

            double value = temp[row][col];

            // Создаем объект ячейки таблицы и помещаем в каждый элемент значение температуры
            QTableWidgetItem *item = new QTableWidgetItem(QString::number(value));
            item->setTextAlignment(Qt::AlignCenter);    // выравниваем по центру

            // Вычисляем цвет
            double normalized = (value - minVal) / (maxVal - minVal);
            QColor color(
                static_cast<int>(normalized * 255),         // красный
                0,                                          // зеленый
                static_cast<int>((1 - normalized) * 255)    // синий
                );

            item->setBackground(QBrush(color));         // устанавливаем задний фон
            ui->tableImage->setItem(row, col, item);    // устанавливаем элементы в таблицу
        }
    }
}
