#include <QCoreApplication>
#include <QCryptographicHash>
#include <QThread>

#include "tcpserver.hpp"


void onMessage(Connection* conn, const QByteArray& msg)
{
//    qDebug() << QThread::currentThreadId() << " recved: " << msg.size() << "MD5: "
//             << QCryptographicHash::hash(msg, QCryptographicHash::Md5).toHex();
    conn->Write(msg);
}

extern void TestClient();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TcpServer srv(8080);
    srv.handler.onMessage = onMessage;
    srv.Start();

//    QThread::sleep(1);

//    std::thread t([](){
//       TestClient();
//    });

    return a.exec();
}
