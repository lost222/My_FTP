#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fake_ftp_server = new fake_tcp;
    fake_ftp_server->initSocket("127.0.0.1", 8080);
    fake_ftp_server->to_ip = QHostAddress("127.0.0.1");
    fake_ftp_server->to_port = 1234;
    connect(fake_ftp_server,SIGNAL(err_state(int)), this, SLOT(deal_with_fake_tcp_err(int)));
    connect(fake_ftp_server,SIGNAL(recv_code(const QString &)), this,SLOT(fake_tcp_recv(const QString &)));
    connect(fake_ftp_server,SIGNAL(send_port(int,int)),this,SLOT(show_port(int,int)));
}



MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::fake_tcp_recv(const QString &realdata)
{
    ui->Proshow->append(realdata);
}

void MainWindow::deal_with_fake_tcp_err(int errCode)
{
    ui->Proshow->append(errList[errCode]);
}

void MainWindow::show_port(int toPort, int servePort)
{
    QString to_show = QString("serving in %0, send to %1").arg(servePort).arg(toPort);
    ui->Proshow->append(to_show);
}
