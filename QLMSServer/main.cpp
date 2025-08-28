#include "databasemanager.h"
#include "server.h"
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("QLMS Server");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Learning Management System Server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList() << "p" << "port",
                                  "Server port (default: 8080)",
                                  "port",
                                  "8080");
    parser.addOption(portOption);

    QCommandLineOption dbHostOption(QStringList() << "H" << "host",
                                    "Database host (default: localhost)",
                                    "host",
                                    "localhost");
    parser.addOption(dbHostOption);

    QCommandLineOption dbPortOption("dbport", "Database port (default: 5432)", "dbport", "5432");
    parser.addOption(dbPortOption);

    QCommandLineOption dbNameOption(QStringList() << "d" << "database",
                                    "Database name (default: qlms)",
                                    "database",
                                    "qlms");
    parser.addOption(dbNameOption);

    QCommandLineOption dbUserOption(QStringList() << "u" << "user",
                                    "Database user (default: postgres)",
                                    "user",
                                    "postgres");
    parser.addOption(dbUserOption);

    QCommandLineOption dbPassOption(QStringList() << "P" << "password",
                                    "Database password (default: postgres)",
                                    "password",
                                    "postgres");
    parser.addOption(dbPassOption);

    parser.process(app);

    // Initialize database
    if (!DatabaseManager::instance().initialize(parser.value(dbHostOption),
                                                parser.value(dbPortOption).toInt(),
                                                parser.value(dbNameOption),
                                                parser.value(dbUserOption),
                                                parser.value(dbPassOption))) {
        qCritical() << "Failed to initialize database connection";
        return 1;
    }

    // Start server
    Server server;
    if (!server.start(parser.value(portOption).toUShort())) {
        return 1;
    }

    qInfo() << "QLMS Server is running. Press Ctrl+C to stop.";

    return app.exec();
}
