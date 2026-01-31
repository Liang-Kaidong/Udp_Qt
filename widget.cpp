#include "widget.h"
#include "ui_widget.h"
#include <QString>
#include <QMessageBox>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("Udp");

    udpSocket = new QUdpSocket(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_OpenButton_clicked()
{
    ui->OpenButton->setEnabled(false);

    /* 1. 去除空格，校验是否为空 */
    QString localPortText = ui->LocalPort->text().trimmed();
    if (localPortText.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "本地端口号不能为空！");
        ui->OpenButton->setEnabled(true);
        return;
    }

    /* 2. 校验端口是否为有效数字，并获取端口值 */
    bool isNumber = false;
    quint32 localPort = localPortText.toUInt(&isNumber);
    if (!isNumber || localPort < 1 || localPort > 65535) {
        QMessageBox::warning(this, "输入错误", "请输入1-65535之间的有效端口号！");
        ui->OpenButton->setEnabled(true);
        return;
    }

    /* 3.校验目标IP */
    QString targetIp = ui->TargetIP->text().trimmed();
    if (targetIp.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "目标IP地址不能为空！");
        ui->OpenButton->setEnabled(true);
        return;
    }

    /* 4.校验目标端口 */
    QString targetPortText = ui->TargetPort->text().trimmed();
    if (targetPortText.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "目标端口号不能为空！");
        ui->OpenButton->setEnabled(true);
        return;
    }

    /* 5.先解绑已绑定的端口（避免多次点击导致端口占用） */
    if (udpSocket->state() != QUdpSocket::UnconnectedState) {
        udpSocket->abort();
    }

    /* 6.执行绑定，并接收绑定结果 */
    ui->OpenButton->setEnabled(false);
    bool bindSuccess = udpSocket->bind(localPort);

    if (bindSuccess == true) {
        /**
         * UDP 场景：udpSocket是全局唯一对象，重复connect不会新增绑定，因此无需防护
         * TCP 场景：tcpSocket是动态替换的临时对象，且存在Qt对象复用，可能出现 “多对象绑定同一槽” 或 “复用对象残留旧绑定”，因此必须用disconnect前置清理
         */
        connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readyRead_Slot()));
        QMessageBox::information(this, "提示", QString("端口 %1 绑定成功！").arg(localPort));
    } else {
        QMessageBox::critical(this, "失败",
            QString("端口 %1 绑定失败！原因：%2").arg(localPort).arg(udpSocket->errorString()));
        ui->OpenButton->setEnabled(true);
    }
}

void Widget::on_CloseButton_clicked()
{
    /* 1.判断打开按钮是否可启用（未绑定端口） */
    if (ui->OpenButton->isEnabled()) {
        QMessageBox::warning(this, "操作提示", "请先绑定本地端口，再执行关闭操作！");
        ui->OpenButton->setEnabled(true);
        return;
    }

    /* 2.终止UDP绑定，释放端口 */
    udpSocket->abort();

    /* 3.断开信号连接（防止残留触发） */
    disconnect(udpSocket, SIGNAL(readyRead()), this, SLOT(readyRead_Slot()));

    /* 3.弹窗提示 */
    QMessageBox::information(this, "提示", "端口已关闭！");
    ui->OpenButton->setEnabled(true);
}

void Widget::on_SendButton_clicked()
{
    /* 1.校验：未绑定端口时弹窗提示（核心异常反馈） */
    if (udpSocket->state() == QUdpSocket::UnconnectedState) {
        QMessageBox::warning(this, "操作提示", "请先绑定本地端口后再发送数据！");
        ui->OpenButton->setEnabled(true);
        return;
    }

    /* 2. 校验发送内容是否为空 */
    QString sendBuff = ui->SendEdit->text().trimmed();
    if (sendBuff.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "发送内容不能为空！");
        return;
    }

    /* 3.已绑定端口：正常发送数据 */
    quint16 port;
    QHostAddress address;

    address.setAddress(ui->TargetIP->text());
    port = ui->TargetPort->text().toUInt();
    sendBuff = ui->SendEdit->text().trimmed() + "\r\n";
    udpSocket->writeDatagram(sendBuff.toUtf8().data(), sendBuff.length(), address, port);
}

void Widget::readyRead_Slot()
{
    /**
     * 1.判断数据是否读取完
     *
     * hasPendingDatagrams()：若未读取完，返回true
     */
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray array;   /* 存储还未读取完的数据 */
        array.resize(udpSocket->pendingDatagramSize()); /* .resize(pendingDatagramSize())调整数组大小 */

        /* 读取数据 */
        udpSocket->readDatagram(array.data(), array.size());

        QString buf = QString::fromUtf8(array);
        ui->RecieveEdit->appendPlainText(buf);
    }
}
