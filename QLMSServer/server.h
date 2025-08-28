#ifndef SERVER_H
#define SERVER_H

#include <QList>
#include <QObject>
#include <QSslServer>

class ClientHandler;
class QSslSocket;

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

    bool start(quint16 port);
    void stop();

private slots:
    void onNewConnection();
    void onClientDisconnected(ClientHandler *handler);
    void onLogMessage(const QString &message);

private:
    void handleEncryptedSocket(QSslSocket *socket);

private:
    QSslServer *m_tcpServer;
    QList<ClientHandler *> m_clients;
};

#endif // SERVER_H
