#include "screenprovider.h"
#include "QtDBus/qdbusconnection.h"
#include "QtDBus/qdbusinterface.h"
#include "qstandardpaths.h"
#include "qfile.h"
#include "qnetworkinterface.h"
#include "qlist.h"
#include "QtConcurrent/QtConcurrent"
#include "QFuture"
#include "qdebug.h"

ScreenProvider::ScreenProvider(QObject* parent): QTcpServer(parent)
{
    streaming_ = false;
    timer_ = new QTimer(this);
    timer_->setTimerType(Qt::PreciseTimer);
    interval_ = 140;
}

ScreenProvider::~ScreenProvider()
{
    delete clientConnection_;
    clientConnection_ = 0;
    delete timer_;
    timer_ = 0;
}

void ScreenProvider::start()
{

    if (!this->listen()) {
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }

    if (ipAddress.isEmpty()) {
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }

    ipAddress_ = ipAddress;
    port_ = this->serverPort();
    this->setMaxPendingConnections(1);
    emit serverChanged(port_, ipAddress_.toString());
}

void ScreenProvider::incomingConnection(qintptr handle) {

    if(!streaming_) {
        emit clientConnected();
        streaming_ = true;
        takeScreenshot();
        connect(timer_, SIGNAL(timeout()), this, SLOT(takeScreenshot()));
        timer_->start(interval_);
        watcher_ = new QFutureWatcher<void>();
        connect(watcher_, SIGNAL(finished()), this, SLOT(handleEndedStream()));
        QFuture<void> future = QtConcurrent::run(streamLoop, handle, std::ref(queue_), std::ref(streaming_));
        watcher_->setFuture(future);
    }

}

void ScreenProvider::stop()
{
    if(timer_->isActive()) {
        timer_->stop();
        disconnect(timer_, SIGNAL(timeout()), this, SLOT(takeScreenshot()));
    }
    streaming_ = false;
    this->close();
}

void ScreenProvider::setInterval(int interval)
{
    interval_ = interval;
    timer_->setInterval(interval_);
}

void ScreenProvider::imageReady(QDBusMessage message)
{
    QFile file(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QString("sailcast_latest.jpg"));
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QByteArray blob = file.readAll();
    queue_.enqueue(blob);
}

void ScreenProvider::imageError(QDBusError error, QDBusMessage message)
{
    qDebug() << "error";
    return;
}

void ScreenProvider::handleEndedStream()
{
    emit clientDisconnected();
    streaming_ = false;
    disconnect(watcher_, SIGNAL(finished()), this, SLOT(handleEndedStream()));
    delete watcher_;
    watcher_ = 0;
}

void ScreenProvider::takeScreenshot()
{
    if(streaming_) {
        QDBusInterface *iface = new QDBusInterface("org.nemomobile.lipstick",
                                                   "/org/nemomobile/lipstick/screenshot",
                                                   "org.nemomobile.lipstick",
                                                   QDBusConnection::sessionBus(),
                                                   this);

        QList<QVariant> args;
        args.append(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QString("sailcast_latest.jpg"));
        iface->callWithCallback("saveScreenshot", args, this,
                                SLOT(imageReady(QDBusMessage)), SLOT(imageError(QDBusError, QDBusMessage)));
    } else {
        if(timer_->isActive()) {
            timer_->stop();
            disconnect(timer_, SIGNAL(timeout()), this, SLOT(takeScreenshot()));
        }
    }
}

void streamLoop(qintptr socketDesc, QQueue<QByteArray> &queue, bool& streaming) {

    QTcpSocket* socket = new QTcpSocket(0);
    socket->setSocketDescriptor(socketDesc);
    socket->readAll();

    QByteArray ContentType = ("HTTP/1.1 200 OK\r\n" \
                              "Server: test\r\n" \
                              "Cache-Control: no-cache\r\n" \
                              "Cache-Control: private\r\n" \
                              "Connection: close\r\n"\
                              "Pragma: no-cache\r\n"\
                              "Content-Type: multipart/x-mixed-replace; boundary=--boundary\r\n\r\n");

    socket->write(ContentType);

    while((socket->state() != QAbstractSocket::ClosingState ||
           socket->state() != QAbstractSocket::UnconnectedState) &&
           socket->state() == QAbstractSocket::ConnectedState &&
           streaming) {

        QByteArray boundary = ("--boundary\r\n" \
                               "Content-Type: image/jpeg\r\n" \
                               "Content-Length: ");

        if(queue.empty()) {
            continue;
        }

        QByteArray img  = queue.dequeue();
        boundary.append(QString::number(img.length()));
        boundary.append("\r\n\r\n");

        socket->write(boundary);
        socket->write(img);
        socket->flush();

    }

    socket->disconnectFromHost();
    socket->deleteLater();
    streaming = false;
    return;
}
