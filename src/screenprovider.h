#ifndef SCREENPROVIDER_H
#define SCREENPROVIDER_H

#include <QObject>
#include <QtDBus/QDBusMessage>
#include <QTcpServer>
#include <QTcpSocket>
#include <QQueue>
#include <QByteArray>
#include <QHostAddress>
#include <QFutureWatcher>
#include <QString>
#include <QTimer>
#include <QFile>

void streamLoop(qintptr socketDesc, QQueue<QByteArray> &queue, bool& streaming);

class ScreenProvider : public QTcpServer
{
    Q_OBJECT
public:
    ScreenProvider(QObject* parent = 0);
    ~ScreenProvider();

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setInterval(int interval);
    void incomingConnection(qintptr handle) Q_DECL_OVERRIDE;

signals:
    void serverChanged(int port, QString address);
    void clientConnected();
    void clientDisconnected();

public slots:
    void imageReady(QDBusMessage message);
    void imageError(QDBusError error, QDBusMessage message);
    void handleEndedStream();
    void takeScreenshot();

private:

    QFutureWatcher<void> *watcher_;
    QTcpSocket* clientConnection_;
    QQueue<QByteArray> queue_;
    bool streaming_;
    QHostAddress ipAddress_;
    int port_;
    QTimer* timer_;
    int interval_;

};

#endif // SCREENPROVIDER_H
