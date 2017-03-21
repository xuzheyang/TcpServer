#include <QDebug>
#include "tcpclient.hpp"

void TestClient()
{
    TcpClient clt("127.0.0.1", 8080);
    if (clt.Connect()) {
        QByteArray ret = clt.Send("Hello world!");
        qDebug() << ret;
    }
}
