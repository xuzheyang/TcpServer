#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QList>
#include <QThread>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
typedef int qintptr;
#endif


#define TCP_PACK_LENGTH_BITS    22
#define TCP_PACK_CRYPTION_BITS  30

#define TCP_PACK_HEADER_FLAG    0xE5
#define TCP_PACK_FLAG_MASK      0xFF

#define TCP_PACK_MAX_LEN        0x3FFFFF
#define TCP_PACK_LENGTH_MASK    0x3FFFFF

inline quint32 MakeHeader(int len, int encryption)
{
    return (encryption << TCP_PACK_CRYPTION_BITS) | (TCP_PACK_HEADER_FLAG << TCP_PACK_LENGTH_BITS) | len;
}

inline int GetPacketLen(quint32 header)
{
    unsigned short flag = (unsigned short)((header >> TCP_PACK_LENGTH_BITS) & TCP_PACK_FLAG_MASK);
    if (flag != TCP_PACK_HEADER_FLAG) {
        return -1;
    }
    return header & TCP_PACK_LENGTH_MASK;
}

inline int GetEncryption(quint32 header)
{
    return header >> TCP_PACK_CRYPTION_BITS;
}

class Connection;
struct Handler{
    void (*onMessage)(Connection* conn, const QByteArray& msg);
    void (*onOpen)(Connection* conn);
    void (*onClose)(Connection* conn);
    void (*onError)(Connection* conn);
    Handler() : onMessage(NULL), onOpen(NULL), onClose(NULL), onError(NULL){ }
};
class Connection : public QObject
{
    Q_OBJECT
public:
    Connection(qintptr socketDescriptor, Handler* handler, QObject* parent = 0):
        QObject(parent), handler(handler)
    {
        socket = new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);

        connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        connect(socket, SIGNAL(disconnected()), this, SLOT(close()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
    }

    void Write(const QByteArray& data)
    {
        quint32 header = MakeHeader(data.size(), 0);
        socket->write(QByteArray((const char*)&header, 4));
        socket->write(data);
        socket->flush();
    }

    QTcpSocket* Socket()
    {
        return socket;
    }

signals:
    void finished(Connection* conn);

private slots:
    void onReadyRead()
    {
        if (readBuf.size() == 0) {
            socket->read((char*)&header, 4);
            int len = GetPacketLen(header);
            if (len == -1) {
                close();
                return ;
            }
            dataLen = len;
            readBuf.reserve(dataLen);
        }
        readBuf.append(socket->readAll());

        if (readBuf.size() == dataLen) {
            if (handler->onMessage) {
                handler->onMessage(this, readBuf);
                close();
            }
        }
    }

    void onError(QAbstractSocket::SocketError err)
    {
        //qDebug() << "error:" << err;
        close();
    }

    void close()
    {
        socket->close();
        emit finished(this);
    }

private:
    QTcpSocket* socket;
    QByteArray readBuf;
    quint32 header;
    quint32 dataLen;
    Handler* handler;
};


class TcpServer : public QTcpServer
{
    Q_OBJECT

public:
    TcpServer(quint16 port, quint32 numThreads = 0, QObject* parent = 0):
        QTcpServer(parent), port(port), totalWorkers(numThreads), workerThreadID(0)
    {
        if (totalWorkers == 0) {
            /**
             * 返回当前操作系统能运行线程的数量
             * 返回-1，代表CPU的核心数量不能被探测
             */
            totalWorkers = QThread::idealThreadCount();
            if (totalWorkers == 0) {
                totalWorkers = 1;
            }
        }

        /**
         * 创建系统支持最大的线程，并且添加到threadList链表中备用
         */
        for (int i = 0; i < totalWorkers; i++) {
            QThread* t = new QThread(this);
            threadList.append(t);
        }
    }

    ~TcpServer()
    {
        for (QList<Connection*>::iterator con = conList.begin(); con != conList.end(); con++) {
            (*con)->deleteLater();
        }

        if (isListening()) {
            Stop();
        }
    }


    bool Start()
    {
        bool ok = QTcpServer::listen(QHostAddress(QHostAddress::Any), port);
        if (ok) {
            for (QList<QThread*>::iterator thread = threadList.begin(); thread != threadList.end(); thread++) {
                (*thread)->start();
            }
        }
        return ok;
    }

    void Stop()
    {
        for (QList<QThread*>::iterator thread = threadList.begin(); thread != threadList.end(); thread++) {
            (*thread)->quit();
        }
        for (QList<QThread*>::iterator thread = threadList.begin(); thread != threadList.end(); thread++) {
            (*thread)->wait();
        }
        QTcpServer::close();
    }


public:
    Handler handler;

protected:
    void incomingConnection(qintptr socketDescriptor)
    {
        Connection* con = new Connection(socketDescriptor, &handler);
        conList.append(con);
        connect(con, SIGNAL(finished(Connection*)), this, SLOT(onConnectionFinished(Connection*)));
        con->moveToThread(threadList.at(workerThreadID++));
        workerThreadID %= totalWorkers;
    }

private slots:
    void onConnectionFinished(Connection* conn)
    {
        if (conList.removeOne(conn)) {
            conn->deleteLater();
        }
    }

private:
    QList<QThread*> threadList;
    QList<Connection*> conList;
    quint64 workerThreadID;
    quint64 totalWorkers;
    quint16 port;
};

#endif // TCPSERVER_H
