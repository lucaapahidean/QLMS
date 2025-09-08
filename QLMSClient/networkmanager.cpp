#include "networkmanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QSslConfiguration>

NetworkManager &NetworkManager::instance()
{
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager()
    : m_socket(new QSslSocket(this))
{
    // Configure SSL settings before any connection
    QSslConfiguration sslConfig = m_socket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // Accept self-signed certificates
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    m_socket->setSslConfiguration(sslConfig);

    connect(m_socket, &QSslSocket::connected, this, &NetworkManager::onConnected);
    connect(m_socket, &QSslSocket::encrypted, this, &NetworkManager::onEncrypted);
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
        qWarning() << "Socket is already connected or connecting";
        return false;
    }

    qDebug() << "Attempting to connect to" << host << ":" << port;

    // Clear any previous errors
    m_buffer.clear();

    // Start encrypted connection
    m_socket->connectToHostEncrypted(host, port);

    // Wait for the connection with a longer timeout for SSL handshake
    if (!m_socket->waitForConnected(5000)) {
        qWarning() << "Connection timeout or error:" << m_socket->errorString();
        return false;
    }

    qDebug() << "TCP connection established, waiting for SSL handshake...";

    // Wait for encryption
    if (!m_socket->waitForEncrypted(10000)) { // Increased timeout for SSL
        qWarning() << "SSL handshake failed:" << m_socket->errorString();
        m_socket->disconnectFromHost();
        return false;
    }

    qDebug() << "SSL handshake completed successfully";
    return true;
}

void NetworkManager::disconnectFromServer()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(5000);
        }
    }
    m_buffer.clear();
    while (!m_callbacks.isEmpty()) {
        m_callbacks.dequeue();
    }
}

bool NetworkManager::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState && m_socket->isEncrypted();
}

void NetworkManager::sendCommand(const QString &command,
                                 const QJsonObject &data,
                                 std::function<void(const QJsonObject &)> callback)
{
    if (!isConnected()) {
        qWarning() << "Cannot send command: not connected or not encrypted";
        if (callback) {
            QJsonObject error;
            error["type"] = "ERROR";
            error["message"] = "Not connected to server";
            callback(error);
        }
        return;
    }

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
    qDebug() << "TCP connection established";
    // Don't emit connected yet - wait for encryption
}

void NetworkManager::onEncrypted()
{
    qDebug() << "SSL connection encrypted";
    emit connected();
}

void NetworkManager::onDisconnected()
{
    qDebug() << "Disconnected from server";
    m_buffer.clear();
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
        } else {
            qWarning() << "Received invalid JSON message:" << messageData;
        }
    }
}

void NetworkManager::onSslErrors(const QList<QSslError> &errors)
{
    qWarning() << "SSL Errors occurred:";
    for (const QSslError &error : errors) {
        qWarning() << "  -" << error.errorString();
    }
    // Accept self-signed certificates for development
    m_socket->ignoreSslErrors();
}

void NetworkManager::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    qCritical() << "Socket error:" << error << "-" << errorString;

    // Provide more specific error messages
    QString userMessage;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        userMessage = "Connection refused. Is the server running?";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        userMessage = "Server closed the connection";
        break;
    case QAbstractSocket::HostNotFoundError:
        userMessage = "Server host not found";
        break;
    case QAbstractSocket::SocketTimeoutError:
        userMessage = "Connection timed out";
        break;
    case QAbstractSocket::SslHandshakeFailedError:
        userMessage = "SSL handshake failed. Certificate issue?";
        break;
    default:
        userMessage = errorString;
    }

    emit errorOccurred(userMessage);
}

void NetworkManager::processMessage(const QJsonObject &message)
{
    qDebug() << "Received message:"
             << QJsonDocument(message).toJson(QJsonDocument::Indented).constData();

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
        qWarning() << "Cannot send message: not connected or not encrypted";
        return;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    qDebug() << "Sending message:" << doc.toJson(QJsonDocument::Indented).constData();

    qint64 written = m_socket->write(data);
    if (written == -1) {
        qWarning() << "Failed to write to socket:" << m_socket->errorString();
    } else {
        m_socket->flush();
    }
}
