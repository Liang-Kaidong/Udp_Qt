#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUdpSocket>
#include <QHostAddress>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    QUdpSocket *udpSocket;

private slots:
    void on_OpenButton_clicked();

    void on_CloseButton_clicked();

    void on_SendButton_clicked();

    void readyRead_Slot();

private:
    Ui::Widget *ui;
};

#endif // WIDGET_H
