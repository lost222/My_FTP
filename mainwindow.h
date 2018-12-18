#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "fake_tcp.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    //void show_port(int toPort, int servePort);
    ~MainWindow();
    fake_tcp * fake_ftp_server;


public slots:
    void show_port(int toPort, int servePort);

private slots:

    void fake_tcp_recv(const QString &realdata);

    void deal_with_fake_tcp_err(int errCode);



private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
