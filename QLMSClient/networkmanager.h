#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <functional>
#include <QJsonObject>
#include <QObject>
#include <QQueue>
#include <QSslSocket>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    static NetworkManager& instance();

    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendCommand(const QString& command,
                     const QJsonObject& data,
                     std::function<void(const QJsonObject&)> callback = nullptr);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject& message);
    void errorOccurred(const QString& error);

private:
    NetworkManager();
    ~NetworkManager();

    void sendMessage(const QJsonObject& message);
    void processMessage(const QJsonObject& message);

private slots:
    void onConnected();
    void onEncrypted(); // Added this slot
    void onDisconnected();
    void onReadyRead();
    void onSslErrors(const QList<QSslError>& errors);
    void onSocketError(QAbstractSocket::SocketError error);

private:
    QSslSocket* m_socket;
    QByteArray m_buffer;
    QQueue<std::function<void(const QJsonObject&)>> m_callbacks;
};

#endif // NETWORKMANAGER_H
