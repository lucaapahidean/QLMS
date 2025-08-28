#include "server.h"
#include "clienthandler.h"
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QThread>

Server::Server(QObject *parent)
    : QObject(parent)
    , m_tcpServer(new QSslServer(this))
{
    // Load SSL certificate and key
    QFile certFile("server.crt");
    QFile keyFile("server.key");

    if (!certFile.exists() || !keyFile.exists()) {
        qWarning() << "Certificate files not found in current directory, trying parent directory";
        certFile.setFileName("../server.crt");
        keyFile.setFileName("../server.key");
    }

    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open SSL certificate or key file";
        qCritical() << "Certificate path:" << certFile.fileName() << "exists:" << certFile.exists();
        qCritical() << "Key path:" << keyFile.fileName() << "exists:" << keyFile.exists();
        return;
    }

    QSslCertificate certificate(&certFile, QSsl::Pem);
    QSslKey privateKey(&keyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    certFile.close();
    keyFile.close();

    if (certificate.isNull()) {
        qCritical() << "SSL certificate is null/invalid";
        return;
    }
    if (privateKey.isNull()) {
        qCritical() << "SSL private key is null/invalid";
        return;
    }

    // Configure SSL for the server
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setLocalCertificate(certificate);
    sslConfig.setPrivateKey(privateKey);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // Accept any client
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);

    m_tcpServer->setSslConfiguration(sslConfig);

    qInfo() << "SSL certificate and key loaded successfully";

    // Connect to handle SSL errors from the server
    connect(m_tcpServer,
            &QSslServer::errorOccurred,
            this,
            [this](QSslSocket *socket, QAbstractSocket::SocketError error) {
                if (socket) {
                    qWarning() << "QSslServer error for socket from"
                               << socket->peerAddress().toString() << ":" << error << "-"
                               << socket->errorString();
                } else {
                    qWarning() << "QSslServer error:" << error;
                }
            });

    // Connect to handle SSL errors during handshake
    connect(m_tcpServer,
            &QSslServer::sslErrors,
            this,
            [this](QSslSocket *socket, const QList<QSslError> &errors) {
                qWarning() << "SSL errors on incoming connection:";
                for (const QSslError &error : errors) {
                    qWarning() << "  -" << error.errorString();
                }
                // Accept self-signed certificates for development
                if (socket) {
                    socket->ignoreSslErrors();
                }
            });

    // Connect to handle new connections - use pendingConnectionAvailable like in the Qt example
    connect(m_tcpServer, &QSslServer::pendingConnectionAvailable, this, &Server::onNewConnection);
}

Server::~Server()
{
    stop();
}

bool Server::start(quint16 port)
{
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to start server:" << m_tcpServer->errorString();
        return false;
    }

    qInfo() << "Server started on port" << port << "(SSL enabled)";
    qInfo() << "Server listening on" << m_tcpServer->serverAddress().toString() << ":"
            << m_tcpServer->serverPort();
    return true;
}

void Server::stop()
{
    m_tcpServer->close();
    m_clients.clear();
    qInfo() << "Server stopped";
}

void Server::onNewConnection()
{
    qDebug() << "New connection available";

    // Get the pending connection as QSslSocket
    QSslSocket *socket = qobject_cast<QSslSocket *>(m_tcpServer->nextPendingConnection());

    if (!socket) {
        qWarning() << "Failed to get SSL socket from pending connection";
        return;
    }

    qInfo() << "New SSL connection from" << socket->peerAddress().toString() << ":"
            << socket->peerPort() << "(encrypted)";

    // Connect error handler for this specific socket
    connect(socket,
            &QSslSocket::errorOccurred,
            this,
            [this, socket](QAbstractSocket::SocketError error) {
                // Only log non-standard disconnection errors
                if (error != QAbstractSocket::RemoteHostClosedError) {
                    qWarning() << "Socket error from" << socket->peerAddress().toString() << ":"
                               << error << "-" << socket->errorString();
                }
            });

    // With QSslServer, the socket is already encrypted, so we can directly set up the handler
    handleEncryptedSocket(socket);
}

void Server::handleEncryptedSocket(QSslSocket *socket)
{
    qDebug() << "Setting up ClientHandler for encrypted socket from"
             << socket->peerAddress().toString();

    // Create thread and handler
    QThread *thread = new QThread();
    ClientHandler *handler = new ClientHandler(socket);

    // Move the worker to the new thread
    handler->moveToThread(thread);

    // The socket must also live in the new thread
    socket->setParent(nullptr);
    socket->moveToThread(thread);

    // When the thread starts, tell the worker to start processing
    connect(thread, &QThread::started, handler, &ClientHandler::startProcessing);

    // Forward signals from the worker back to the server in the main thread
    connect(handler, &ClientHandler::logMessage, this, &Server::onLogMessage);
    connect(handler, &ClientHandler::clientDisconnected, this, &Server::onClientDisconnected);

    // When the thread finishes, clean up everything
    connect(thread, &QThread::finished, handler, &QObject::deleteLater);
    connect(thread, &QThread::finished, socket, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    m_clients.append(handler);
    thread->start();
}

void Server::onClientDisconnected(ClientHandler *handler)
{
    m_clients.removeAll(handler);
    qInfo() << "Client handler removed, active clients:" << m_clients.size();
}

void Server::onLogMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    qInfo() << QString("[%1] %2").arg(timestamp, message);
}
