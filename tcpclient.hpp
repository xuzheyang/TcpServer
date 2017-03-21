#ifndef TCPCLIENT_HPP
#define TCPCLIENT_HPP

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QTcpSocket>

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
// 获取数据加密算法类型
inline int GetEncryption(quint32 header)
{
    return header >> TCP_PACK_CRYPTION_BITS;
}

class TcpClient {

public:
    TcpClient(const QString& srvIP, quint32 srvPort, quint32 timeout = 30000) :
        addr(srvIP), port(srvPort), timeout(timeout) { }

    bool Connect()
    {
        socket.connectToHost(addr, port);
        if (!socket.waitForConnected(timeout)) {
            return false;
        }
        return true;
    }

    QByteArray Send(const QByteArray& data)
    {
        quint32 header = MakeHeader(data.size(), 0);
        socket.write((char*)&header, 4);
        socket.write(data);
        socket.flush();

        QByteArray ret;
        do {
            if (!socket.waitForReadyRead(timeout)) {
                break;
            }

            if (socket.read((char*)&header, 4) != 4) {
                break;
            }
            int len = GetPacketLen(header);
            if (len == -1) {
                break;
            }

            do {
                ret.append(socket.readAll());
                socket.flush();
                if (ret.size() == len) {
                    break;
                }
            } while(socket.waitForReadyRead(timeout));

        } while (0);

        return ret;
    }

private:
    QTcpSocket socket;
    QString addr;
    quint32 port;
    quint32 timeout;
};

#endif // TCPCLIENT_HPP
