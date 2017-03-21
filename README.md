# 基于QT网络库的服务器/客户端框架

## 依赖
1. `QT += network`
2. 主要使用了`QTcpServer`和`QTcpSocket`

## 限制与约定
1. 该框架实现了TCP短连接模型，服务器在回复TCP包后会主动close连接。
2. `srv.Start()` 以后必须进入主事件循环。
3. 框架底层在自动在每个数据包前加上一个4字节的包头，但是对于框架使用者来说是透明的。 

  ``` 
  void onMessage(Connection* conn, const QByteArray& msg);
  ```
 
  参数`msg`是一个完整的消息且没有包头，不管tcp将该消息拆成多少个`packet`发送，`msg`是底层将`packet`组装后的完整数据。

4. 消息`msg`在最大长度为4M-1，如果发送大文件，需要自行拆片。

## 协议包头格式
每一个数据包在发送前，应加上一个4字节(32位)的包头，包头有以下三个作用：
 * 校验数据包合法性。
 * 保证接收方能接收到一个完整的数据包。
 * 标识数据包采用的数据加密方式。

```
SS  XXXXXXXX  YYYYYYYYYYYYYYYYYYYYYY
┬-  ---┬----  ---------┬------------
│      │               │
│      │               └---> 包长度
│      └--------->包标识
└--->数据加密标识

```

说明：

1. 前2位(SS)为数据加密标识位，用于标识数据采用何种加密算法进行加密，最大支持4种数据加密方式。
 * 00 加密算法0, 通常不加密
 * 01 加密算法1
 * 10 加密算法2
 * 11 加密算法3

2. 中8位(X)为包头标识位，用于数据包校验。有效包头标识取值范围 0 ~ 255(0xFF)。默认值为0xE5。

3. 后22位(Y)为长度位，记录包体长度。有效数据包最大长度不能超过 4194303(0x3FFFFF)字节。默认最大值4194303(4M-1)。

```
#define TCP_PACK_LENGTH_BITS    22
#define TCP_PACK_CRYPTION_BITS  30

#define TCP_PACK_HEADER_FLAG    0xE5
#define TCP_PACK_FLAG_MASK      0xFF

#define TCP_PACK_MAX_LEN        0x3FFFFF
#define TCP_PACK_LENGTH_MASK    0x3FFFFF

// 构造包头
inline quint32 MakeHeader(int len, int encryption)
{
    return (encryption << TCP_PACK_CRYPTION_BITS) | (TCP_PACK_HEADER_FLAG << TCP_PACK_LENGTH_BITS) | len;
}

// 从包头解出长度，返回-1表示非法包头
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
```

## 使用示例

```
#include "tcpserver.hpp"

// 服务器只需要实现 onMessage 内的业务逻辑
void onMessage(Connection* conn, const QByteArray& msg)
{
//    qDebug() << QThread::currentThreadId() << " recved: " << msg.size() << "MD5: "
//             << QCryptographicHash::hash(msg, QCryptographicHash::Md5).toHex();
    conn->Write(msg);
}

// 客户端使用方法
void TestClient()
{
    TcpClient clt("127.0.0.1", 8080);
    if (clt.Connect()) {
        QByteArray ret = clt.Send("Hello world!");
        qDebug() << ret;
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TcpServer srv(8080);
    srv.Start();

    QThread::sleep(1);

    std::thread t([](){
       TestClient();
    });

    return a.exec();
}

```
