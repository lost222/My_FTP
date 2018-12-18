#ifndef FAKE_TCP_H
#define FAKE_TCP_H

#include <QObject>
#include  <QThread>
#include <QTimer>
#include <QUdpSocket>
#include <QString>
#include <QHostAddress>
#include <QDir>
#include <QProcess>
#include <QVector>
#include <QDebug>

#define DEFAULTSIZE 4096
#define DEFALTWAITTIME 5000000
#define FAKETCPHEADLEN 96
#define FAKETCPDATALEN DEFAULTSIZE - FAKETCPHEADLEN

class fake_tcp : public QObject
{
    Q_OBJECT
public:
    explicit fake_tcp(QObject *parent = 0);
    unsigned int ackNum;
    unsigned int sendNum;
    QHostAddress to_ip;
    unsigned short to_port;
    QTimer * p_timer;
    QUdpSocket* udpSocket;
    QString sendbuffer;
    QString curpath;
    void send_code(qint16 toPort, qint16 fromPort);
    void initSocket(QString IP, int port);
    QVector<QProcess *> Pros;
    int fromCount;
    QProcessEnvironment env;
    QString serverProPath;

private:
    void processTheDatagram_listen_server_mode(struct fake_tcp_Datagram* datagram, int size);

signals:
    void recv_code(const QString &data);
    void err_state(int err_code);
    void send_port(int toPort, int servePort);

public slots:

    void readPendingDatagrams();

};

struct fake_tcp_Datagram
{
    unsigned short  sourcePort;
    unsigned short  desPort;
    unsigned int sendId;
    unsigned int ackId;
    char realdata[DEFAULTSIZE - 96];
};

static const char* errList[50] = {
    "re send",
    "can't save file"
    "unkown err"
};


#endif // FAKE_TCP_H
