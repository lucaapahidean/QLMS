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
    QFile certFile("../../server.crt");
    QFile keyFile("../../server.key");
    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open SSL certificate or key file for QSslServer";
        return;
    }

    QSslCertificate certificate(&certFile);
    QSslKey privateKey(&keyFile, QSsl::Rsa, QSsl::Pem);
    certFile.close();
    keyFile.close();

    QSslConfiguration sslConfig;
    sslConfig.setLocalCertificate(certificate);
    sslConfig.setPrivateKey(privateKey);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

    m_tcpServer->setSslConfiguration(sslConfig);

    connect(m_tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);
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
    qInfo() << "Server started on port" << port;
    return true;
}

void Server::stop()
{
    m_tcpServer->close();
    // Since handlers are managed by their threads, we just need to wait for them to finish.
    // A more advanced server might explicitly tell threads to quit here.
    m_clients.clear();
    qInfo() << "Server stopped";
}

void Server::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        QSslSocket *socket = qobject_cast<QSslSocket *>(m_tcpServer->nextPendingConnection());
        if (socket) {
            // Wait for the handshake to finish before doing anything.
            connect(socket, &QSslSocket::encrypted, this, [this, socket]() {
                // Handshake OK! Now create the thread and worker.
                QThread *thread = new QThread();
                ClientHandler *handler = new ClientHandler(socket);

                // Move the worker to the new thread.
                handler->moveToThread(thread);

                // The socket must also live in the new thread.
                socket->setParent(nullptr);
                socket->moveToThread(thread);

                // When the thread starts, tell the worker to start processing.
                connect(thread, &QThread::started, handler, &ClientHandler::startProcessing);

                // Forward signals from the worker back to the server in the main thread.
                connect(handler, &ClientHandler::logMessage, this, &Server::onLogMessage);
                connect(handler,
                        &ClientHandler::clientDisconnected,
                        this,
                        &Server::onClientDisconnected);

                // When the thread finishes, clean up everything.
                connect(thread, &QThread::finished, handler, &QObject::deleteLater);
                connect(thread, &QThread::finished, socket, &QObject::deleteLater);
                connect(thread, &QThread::finished, thread, &QObject::deleteLater);

                m_clients.append(handler);
                thread->start();
            });

            // Clean up the socket if the client disconnects before handshake finishes.
            connect(socket, &QSslSocket::disconnected, socket, &QObject::deleteLater);
        }
    }
}

void Server::onClientDisconnected(ClientHandler *handler)
{
    m_clients.removeAll(handler);
}

void Server::onLogMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    qInfo() << QString("[%1] %2").arg(timestamp, message);
}
