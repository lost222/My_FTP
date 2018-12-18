#include "fake_tcp.h"
#include "cstring"
#include <QDebug>


fake_tcp::fake_tcp(QObject *parent) : QObject(parent)
{
    ackNum = 0;
    sendNum = 0;
    //mode = 0;
    // reSend timer
    p_timer = new QTimer(this);
    curpath = QString("C:/server");
    connect(p_timer,SIGNAL(timeout()),this,SLOT(sendDataTimeout()));

    fromCount = 0;
    serverProPath = QString("C:\\release\\fake_ftp_server.exe");
    env = QProcessEnvironment::systemEnvironment();
}

void fake_tcp::initSocket(QString IP, int port)
{
    this->udpSocket = new QUdpSocket(this);
    udpSocket->bind(QHostAddress(IP), port);
    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(readPendingDatagrams()));
}

void fake_tcp::readPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
//           QNetworkDatagram datagram = udpSocket->receiveDatagram();
             struct fake_tcp_Datagram datagram;
             QHostAddress source;
             quint16 sourcePort;
             qint64 size_re;
             size_re = udpSocket->readDatagram((char*)&datagram, DEFAULTSIZE + 10000, &source, &sourcePort);
             qDebug()<<"send ID"<<datagram.sendId<<"now server want"<<this->ackNum<<"\n"
                    <<"ack ID"<<datagram.ackId<<"\n";

             // only condition we ac
             if (strncasecmp(datagram.realdata,"request",7) == 0) {
                 processTheDatagram_listen_server_mode(&datagram, size_re);
             }

       }
}

void fake_tcp::send_code(qint16 toPort, qint16 fromPort)
{
    // 网络序问题
    to_port = toPort;

    struct fake_tcp_Datagram datagram;
    datagram.sendId = sendNum;
    datagram.ackId = ackNum;
    datagram.sourcePort = fromPort;
    datagram.desPort = toPort;
    memcpy(datagram.realdata, sendbuffer.toStdString().c_str(),sendbuffer.size() + 1);
    udpSocket->writeDatagram((const char*)&datagram,sendbuffer.size()+96, to_ip, to_port);
}




void fake_tcp::processTheDatagram_listen_server_mode(fake_tcp_Datagram *datagram, int size)
{
    ackNum = datagram->sendId + size;
    sendNum = datagram->ackId;
    QStringList arguments;
    QProcess *myProcess = new QProcess();
    Pros.append(myProcess);
    fromCount++;
    int sendPort = datagram->sourcePort;
    int sevePort = 8080 + fromCount;
    arguments << QString("%0").arg(sevePort);
    arguments << QString("%0").arg(sendPort);
    connect(myProcess,SIGNAL(started()), this, SLOT(new_pro_star()));
    myProcess->setProcessEnvironment(env);
    myProcess->start(serverProPath, arguments);

    send_code(sendPort, sevePort);
    emit send_port(sendPort,sevePort);
}







