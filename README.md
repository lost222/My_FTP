# My_FTP

## 摘要

在本次作业中, 我设计了一个基于UDP的文件传输协议, 协议使用了停等和超时重传来确保可靠数据传输, 通过标识源端口和目的端口区分UDP报文, 模仿TCP连接,进而实现多用户. 协议采取了客户端-服务器模式, 客户端通过输入命令来和服务器交互.协议的内容和实现细节很多(可靠性和多用户)是从模仿TCP行为而来.

## 典型交互程序截图

send命令

![](https://github.com/lost222/My_FTP/blob/master/image/sendShow.png)

左边客户端, 右边服务器, 各自显示了自己所处文件夹下的文件(文件左边的是文件大小,文件大小为0的是文件夹),右边那一排是停等协议的ACK. 具体协议的交互流程请看后文**协议时序**

[TOC]

## 协议语法

我在UDP的数据部分又加上了一个头(包括来源端口, 目的端口, 顺序号, 确认号). 如图

![报文格式](https://github.com/lost222/My_FTP/blob/master/image/datagram_form.png)

```c++
struct fake_tcp_Datagram
{
    unsigned short  sourcePort;
    unsigned short  desPort;
    unsigned int sendId;
    unsigned int ackId;
    char realdata[FAKETCPDATALEN];
};
```

加上这几个变量是为了后面的**可靠数据传输**和**多用户支持**功能. 值得一提的是源端口和目的端口是UDP头部自带的,这里再写一次,一方面是考虑到**不一定使用**获取报文的源端口和目的端口的**系统调用**(虽然正常情况下是有API的), 另一方面在多用户支持的时候,我们通过填一个假的源端口,欺骗客户机下次发信息找新的端口来方便的实现了多进程级别的并行的多用户支持.

## 可靠数据传输

### 基于停等协议的可靠数据传输

基本思路： 半双工， 发送方每发一个包开始计时， 接收方每收一个包回一个ACK。

### 半双工

基于停等协议的可靠数据传输必然只支持半双工。 即虽然双方都可以发和收， 但是一个时间点内只有一方可以发。

### 类似TCP的ACK机制

#### 发送端行为

1. 发送数据
2. 启动计时器
3. 等待ACK

这里有两个情况：

- 超时之前， 受到了回复。

  - 验证回复正确性 

    ```c++
    // 伪代码
    // 接收条件, 不满足条件的包干脆不会收
    if (datagram->sendId == fake_tcp.ackNum) {
        // 正确ACK条件， 不满足的包是重复ACK
        if (fake_tcp.sendNum < datagram->ackId) {
            timer->stop();
       		//处理数据包
            process（datagram）；
        }
    }
    ```

    任何不满足条件的数据包都不会被接受。

- 超时重传

  - 重传什么依据当前状态

    ```c++
    // 超时重传函数
    void fake_tcp::sendDataTimeout()
    {
        //计时器停止
        p_timer->stop();
        // 根据发送端所处的模式重传
        if (mode == 2) {
            send_data(sendNum - startByte - sendCount*FAKETCPHEADLEN);
        } else if (mode == 3 || mode == 1 || mode == -1) {
            send_code();
        }
        emit err_state(0);
        //计时器重启
        p_timer->start(DEFALTWAITTIME);
    }
    ```

#### 接收端行为

1. 接收数据包
2. 依据数据包要求和接收端所处状态做出响应
3. 发出响应报文（ACK捎带在响应报文里）

捎带ACK可以减少短包， 提高传输效率



### 可靠性论证

这是一个基础的停等协议， 可靠性是由发送端超时重传保证的。 

- 数据报到达保证

  传输过程中的任何错误最后都会变成发送端的超时重传

  - 发送端数据报丢失   $$\rightarrow$$  接收端什么也不知道  $$\rightarrow$$  发送端超时重传
  - 接收端发出ack丢失 $$\rightarrow$$  发送端没有收到ack $$\rightarrow$$  发送端超时重传

- 数据报有序性保证

  停等协议本来是不存在有序性问题的 , 因为第 n 个包的ack 回来之前,  第 n + 1 个包是不会发出的. 

   本不需要复杂的使用类似tcp的顺序号和确认号的机制. 之所以这么做是因为一开始就考虑到使用流水线机制的可能性.  目前框架的确可以做tcp的流水线机制, 无奈时间不足, 作罢.

- 数据报bit错误恢复

  这是我没有做的点.  我之所以没有做主要考虑两点

  - 真实网络环境中,丢包的可能性比数据报传输到达, 但是传错一两个bit的可能性要远高. 目前我们学习的重心也是丢包的应对措施, 检测bit错误主要是编码问题, 相对不是这个学期需要掌握的核心
  - 这样的校验是非常耗时的. 出于这一点考虑, tcp和udp做的校验都只针对头, 而且甚至校验部分放到网卡硬件上做.



## 协议语义

协议语义的部分主要体现在这里, 协议的时序主要在下一节.

### 目前支持的命令

####ls命令

用户可以通过`ls`命令查看客户端**当前目录**以及**当前目录下的文件** (相当于集成`pwd`和`ls`) 

#### cd命令

用户可以通过`cd`命令在客户机上移动. cd命令支持平时常用的

```shell
cd ../
```

和

```shell
cd ./xx/
```

命令, 几乎可以像`shell` 的`cd ` 命令一样使用, 每次使用后回显当前目录. 用户可以使用`cd`命令在server上随意移动, 在根目录调用`cd ../`会保持在根目录

#### send 命令

用户可以通过`send`命令,将用户机上的文件发送到服务器的当前目录上. 目前只支持单个文件.

使用例子

```shell
send main.cpp
```

可以将用户机上的文件传输到服务器当前目录下, 最后服务器回显

```shell
goodSave
```

表示自己已经成功保存

#### get命令

用户可以通过`get`命令获得服务器当前目录下的某个文件,  目前只支持单个文件

使用例子

```shell
get hello.py
```

可以将服务器当前目录下的 `hello.py` 拿到客户机, 最后服务器回显

```
sendEnd
```

表示发送完成

### 努努力马上就能支持的命令

这些命令我设计出来希望作为协议的一部分, 但是大部分因为是重复性的工作, 不涉及这次作业的核心知识, 时间紧就没有做.

#### login命令

因为多用户支持我是实现了的, 所以可以轻松加入`login`命令, 我设计交互流程如下

客户机

```
login Bob
```

服务器

```
password
```

客户机

```
xxxxxx
```

服务器

```
hello, Bob
```

或者

```
bad login
```

之所以不做是因为前一个大作业已经做过类似的东西, 主要涉及字符串匹配(因为没有数据库,也没法真的加密). 并不复杂, 

#### sendn命令和getn命令

ftp有命令支持多个文件传输. 做起来也并不复杂

#### 通配符支持

希望能够使用通配符扩展`send`命令和`get`命令. 我设计的使用例子如下

```shell
sendn *.cpp
```

可以发送用户目录下所有cpp结尾的文件



## 协议时序

### 有限自动机

发送端和接收端的行为依据

1. 客户端的`mode`
2. 报文传输的数据或者指令

```c++
//  -1:login   多用户支持中"握手"时候的状态, 
//  0:server   // 服务器进程状态, 主要行为就是根据命令返回结果 比如cd, 客户端不使用
//  1:recv   // 客户端调用了 get 命令后会切入这个状态, 主要功能就是接受保存data, 返回ack
//  2:send    // 客户端调用了 send 命令后切入, 主要功能是把大文件分包发送, 计时重传
//  3:code     //客户端大部分情况下处于的状态, 发送命令, 计时重传
int mode; 
```

之所以需要这么多状态主要是因为停等协议半双工的关系. `send`和`code`模式下一定是需要计时重传的, `recv` 模式返回`ack`,不能计时.

### 典型交互过程示意图

从上到下, 划分方格表示模式切换.

左右互相是发送数据报.

#### ls 和 cd 命令

![ls](https://github.com/lost222/My_FTP/blob/master/image/ls.png)

#### send命令

![](https://github.com/lost222/My_FTP/blob/master/image/send.png)

send命令运行完成之后, client回到`code`模式, server 回到`server`模式, 可以正常进行下一条命令的发送和响应

#### get命令

![](https://github.com/lost222/My_FTP/blob/master/image/get.png)



## 多用户支持

我实现了**完全并行**的多用户支持, 每一个用户都有一个专门的`server进程`, 在进程和端口数量足够的情况下可以支持任意多的用户.

相比TCP, UDP对于多用户的支持不算非常友好. TCP以四元组(源IP, 目的IP, 源端口, 目的端口) 标识TCP连接. 不同用户对FTP服务器的连接自然能分配到不同的`TCP SOCKET`里.UDP 只以二元组(目的IP, 目的端口) 标识`UDP SOCKET`. 多用户支持需要应用层自己实现. 



### 解决UDP无状态问题

让UDP适合多用户有两个难点:

- 二元组标识`UDP SOCKET`

  因为`UDP SOCKET`只是由二元组支持的, 假设服务器在8080端口上响应A客户端, 这个时候B客户端也给服务器的8080端口发信息, UDP是不会因为来源的IP和端口不同就对A和B的报文做区分, 他们都被平等的投递到服务器的8080端口, 服务器需要别的手段保证A和B不相互干扰

- 无状态

  UDP是无状态的. 只能指定UDP`bind`的端口和IP,不能指望UDP发现多个用户连接同一个客户端端口.这么说不太恰当.UDP干脆是没有连接这个概念的.



为此我的解决方案是:

#### 报文标识

还是类似TCP, 给报文添加上`源端口` , `目的端口`, 头部, 以此标识报文来源.

#### 类似TCP状态追踪

每个server进程都保存一份自己的`sendNum` 和 `ackNum`, 这一定程度上解决了UDP不能分辩报文来源问题, 假设有一个别人发错的报文发到服务器`server进程`监听的端口上, 也会因为报文里的`sendId`和`server进程`的`ackNum` 对不上, `server进程`直接**丢弃**报文.

#### 类似TCP三次握手

服务器上一开始只运行一个`listenServer`进程, 这个进程监听8080端口, 接收连接请求.

1. 每当接收到一个合法请求, 他创建一个新的`server进程`, server监听另一个端口n, 填好报文(把源端口填为n, 目的端口填为合法请求的发送端口) ,
2. 发送一条特殊的报文,  告诉客户机去找这个新的端口. 
3. 客户机收到报文, 同步`ackNum`和`sendNum`(这样才能和新服务器通信) , 更改目的端口, 之后所有的命令发送到新端口 

可以看到, 我的设计和TCP三次握手过程是很像的. 

### 多用户解决方案示意图

蓝色和橙色是两个客户端, 他们各自得到了一个单独的`server进程` , 互不干扰地得到了服务.

![多用户](https://github.com/lost222/My_FTP/blob/master/image/mulUsers.png)

多用户实际程序运行截图

![mulUserShow](https://github.com/lost222/My_FTP/blob/master/image/mulUserShow.png)

左边是两个客户端, 右边是两个独立的`server进程` 中间是负责分配服务资源的的 `listen_server`进程

## 协议实现

### 程序总体框架

循环监听端口, 根据收到的数据报和当前所处的状态决定怎么怎么返回.

### fake_tcp类

维护保持连接需要的各种变量, 努力让发送命令和数据的时候看起来像是底下用了一个TCP

```c++
class fake_tcp : public QObject
{
    Q_OBJECT
public:
    explicit fake_tcp(QObject *parent = 0);
    // ack and send
    unsigned int ackNum;
    unsigned int sendNum;
    // 去向的IP地址和端口 想办法模拟一个TCP连接
    QHostAddress to_ip;
    unsigned short to_port;
    // 计时器
    QTimer * p_timer;
    QString sendbuffer;
    int mode; //  -1 : login  0:server   1 : recv   2: send   3:code
    QDir clientDir;
    // send_code发指令, send_data发数据, 分开实现 
    void send_code(void);
    void send_data(int fromByte);
    // 绑定一个UDP 端口
    void initSocket(QString IP, int port);
    // 依据不同模式处理数据报
    void processTheDatagram_recv_mode(struct fake_tcp_Datagram* datagram, qint64 recv_size);
    void processTheDatagram_send_mode(struct fake_tcp_Datagram* datagram, qint64 recv_size);
    void processTheDatagram_code_mode(struct fake_tcp_Datagram* datagram, qint64 recv_size);
    void processTheDatagram_request_mode(struct fake_tcp_Datagram* datagram, qint64 recv_size);
    void send_ex(qint16 toPort, qint16 fromPort);

private:
    QString saveFileName;
    // 大文件切片需要维护的变量
    int startByte;  
    int sendCount;
    QByteArray file_data;
    QByteArray saveData;
    void init_data_send(QString fileName);
signals:
	// 显示交互流程
    void recvdata(const QString &data);
    // 显示错误
    void err_state(int err_code);

public slots:
	// 超时处理函数
    void sendDataTimeout();
    // UDP收到数据报处理函数
    void readPendingDatagrams();


```



更多实现就在`fake_tcp.cpp`中. 客户端和服务器的具体实现可能有细微不同(多几个少几个函数).具体请看`client` 文件夹和 `server` 文件夹.  `listen_server`文件夹里的是那个负责监听8080端口, 给每一个用户"握手" 分配server的进程. 
