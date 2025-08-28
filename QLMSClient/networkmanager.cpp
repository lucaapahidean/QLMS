#include "networkmanager.h"
#include <QDebug>
#include <QJsonDocument>

NetworkManager &NetworkManager::instance()
{
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager()
    : m_socket(new QSslSocket(this))
{
    m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);

    connect(m_socket, &QSslSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(m_socket, &QSslSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QSslSocket::sslErrors, this, &NetworkManager::onSslErrors);
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this,
            &NetworkManager::onSocketError);
}

NetworkManager::~NetworkManager()
{
    disconnectFromServer();
}

bool NetworkManager::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        return false;
    }

    m_socket->connectToHostEncrypted(host, port);
    return m_socket->waitForEncrypted(5000);
}

void NetworkManager::disconnectFromServer()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_buffer.clear();
    while (!m_callbacks.isEmpty()) {
        m_callbacks.dequeue();
    }
}

bool NetworkManager::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendCommand(const QString &command,
                                 const QJsonObject &data,
                                 std::function<void(const QJsonObject &)> callback)
{
    QJsonObject message;
    message["command"] = command;
    message["data"] = data;

    if (callback) {
        m_callbacks.enqueue(callback);
    }

    sendMessage(message);
}

void NetworkManager::onConnected()
{
    qDebug() << "Connected to server";
    emit connected();
}

void NetworkManager::onDisconnected()
{
    qDebug() << "Disconnected from server";
    emit disconnected();
}

void NetworkManager::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    int pos;
    while ((pos = m_buffer.indexOf('\n')) != -1) {
        QByteArray messageData = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);

        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        if (!doc.isNull() && doc.isObject()) {
            processMessage(doc.object());
        }
    }
}

void NetworkManager::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &error : errors) {
        qWarning() << "SSL Error:" << error.errorString();
    }
    // Accept self-signed certificates for development
    m_socket->ignoreSslErrors();
}

void NetworkManager::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    qCritical() << "Socket error:" << error << errorString;
    emit errorOccurred(errorString);
}

void NetworkManager::processMessage(const QJsonObject &message)
{
    emit messageReceived(message);

    // Call registered callback if available
    if (!m_callbacks.isEmpty()) {
        auto callback = m_callbacks.dequeue();
        if (callback) {
            callback(message);
        }
    }
}

void NetworkManager::sendMessage(const QJsonObject &message)
{
    if (!isConnected()) {
        qWarning() << "Cannot send message: not connected";
        return;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    m_socket->write(data);
    m_socket->flush();
}
