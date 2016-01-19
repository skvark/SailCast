#include "screenprovider.h"
#include "QtDBus/qdbusconnection.h"
#include "QtDBus/qdbusinterface.h"
#include "qstandardpaths.h"
#include "qnetworkinterface.h"
#include "qlist.h"
#include "QtConcurrent/QtConcurrent"
#include "QFuture"
#include "qdebug.h"
#include <QBuffer>
#include <QThread>
#include <QImage>
#include <QElapsedTimer>
#include <QOrientationReading>

ScreenProvider::ScreenProvider(QObject* parent): QTcpServer(parent)
{
    streaming_ = false;
    orientation_ = QOrientationReading::TopUp;
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;

    framebufferinfo_.fbmmap = (char*)MAP_FAILED;
    framebufferinfo_.scrinfo = var;
    framebufferinfo_.fix_scrinfo = fix;
    framebufferinfo_.fps = 0.0;
    framebufferinfo_.compression = 50;

    fpstimer_.setInterval(1000);
    connect(&fpstimer_, SIGNAL(timeout()), this, SLOT(updateFps()));
    connect(this, SIGNAL(clientConnected()), this, SLOT(onClientConnected()));

    if ((fbDevice_ = open("/dev/fb0", O_RDONLY)) == -1)
    {
        return;
    }

    if (ioctl(fbDevice_, FBIOGET_FSCREENINFO, &framebufferinfo_.fix_scrinfo) != 0){
        return;
    }

    if (ioctl(fbDevice_, FBIOGET_VSCREENINFO, &framebufferinfo_.scrinfo) != 0){
        return;
    }

    framebufferinfo_.fbmmap = (unsigned int*)mmap(NULL,
                                                  framebufferinfo_.fix_scrinfo.smem_len,
                                                  PROT_READ,
                                                  MAP_SHARED,
                                                  fbDevice_,
                                                  0);

    if ((char*)framebufferinfo_.fbmmap == MAP_FAILED){
        return;
    }
}

ScreenProvider::~ScreenProvider()
{
    if(streaming_) {
        stopStreaming();
    }

    while(future_.isRunning() || future2_.isRunning()) {} // wait threads to exit

    if(orientationSensor_) {
        orientationSensor_->stop();
        delete orientationSensor_;
    }

    munmap(framebufferinfo_.fbmmap, framebufferinfo_.fix_scrinfo.smem_len);

    if(fbDevice_ != -1){
        ::close(fbDevice_);
    }
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

        streaming_ = true;
        emit clientConnected();

        watcher_ = new QFutureWatcher<void>();
        connect(watcher_, SIGNAL(finished()), this, SLOT(handleEndedStream()));
        future2_ = QtConcurrent::run(streamLoop,
                                     handle,
                                     std::ref(queue_),
                                     std::ref(streaming_));
        watcher_->setFuture(future2_);
    }
}

void ScreenProvider::stopStreaming()
{
    streaming_ = false;
    this->close();
}

void ScreenProvider::setCompression(int value)
{
    framebufferinfo_.compression = value;
}

void ScreenProvider::setRotate(bool value)
{
    if(value) {
        orientationSensor_ = new QOrientationSensor(this);
        connect(orientationSensor_, SIGNAL(readingChanged()), this, SLOT(orientationChanged()));
        orientationSensor_->start();
    } else {
        orientationSensor_->stop();
        disconnect(orientationSensor_, SIGNAL(readingChanged()), this, SLOT(orientationChanged()));
        delete orientationSensor_;
        orientationSensor_ = 0;
        orientation_ = QOrientationReading::TopUp;
    }
}

void ScreenProvider::handleEndedStream()
{
    emit clientDisconnected();
    streaming_ = false;
    fpstimer_.stop();
    disconnect(watcher_, SIGNAL(finished()), this, SLOT(handleEndedStream()));
    delete watcher_;
    watcher_ = 0;
    framebufferinfo_.fps = 0;
    framebufferinfo_.frametime = 0;
}

void ScreenProvider::updateFps()
{
    emit fpsChanged(framebufferinfo_.fps);
    emit frameTime(framebufferinfo_.frametime);
}

void ScreenProvider::onClientConnected()
{
    future_ = QtConcurrent::run(grabFramesLoop,
                                std::ref(streaming_),
                                std::ref(framebufferinfo_),
                                std::ref(queue_),
                                std::ref(orientation_));
    fpstimer_.start();
}

void ScreenProvider::orientationChanged()
{
    orientation_ = orientationSensor_->reading()->orientation();
}

void grabFramesLoop(bool &streaming, framebufferinfo &info, QQueue<QByteArray> &queue, QOrientationReading::Orientation &orientation)
{
    QImage img(info.scrinfo.xres, info.scrinfo.yres, QImage::Format_RGBA8888);
    QBuffer imgBuffer;

    QElapsedTimer timer;
    timer.start();

    int frames = 0;
    int old = 0;
    int now = 0;
    int line = info.fix_scrinfo.line_length / 4;
    QByteArray ba;
    QTransform transform;

    while(streaming) {

        for (unsigned int y = 0; y < info.scrinfo.yres; ++y)
        {
            QRgb *rowData = (QRgb*)img.scanLine(y);
            for (unsigned int x = 0; x < info.scrinfo.xres; ++x)
            {
                rowData[x] = *((unsigned int *)info.fbmmap + ((x + info.scrinfo.xoffset) + (y + info.scrinfo.yoffset) * line));
            }
        }

        imgBuffer.setBuffer(&ba);
        imgBuffer.open(QIODevice::WriteOnly);

        if(orientation != QOrientationReading::TopUp) {

            switch (orientation) {
                case QOrientationReading::TopDown:
                    img = img.transformed(transform.rotate(180));
                    break;
                case QOrientationReading::LeftUp:
                    img = img.transformed(transform.rotate(90));
                    break;
                case QOrientationReading::RightUp:
                    img = img.transformed(transform.rotate(-90));
                    break;
                default:
                    break;
            }

            img.save(&imgBuffer, "JPG", info.compression);
            transform.reset();
            // reset to original (correct width x height)
            img = QImage(info.scrinfo.xres, info.scrinfo.yres, QImage::Format_RGBA8888);

        } else {
            img.save(&imgBuffer, "JPG", info.compression);
        }

        queue.enqueue(ba);
        imgBuffer.close();

        ++frames;
        now = timer.elapsed();

        if(now > old) {
            info.frametime = now - old;
        }

        old = now;

        // get average fps over the last 50 frames
        if(frames == 50) {
            info.fps = round(50.0 / (timer.restart() / 1000.0));
            frames = 0;
        }
    }
    return;
}

void streamLoop(qintptr socketDesc, QQueue<QByteArray> &queue, bool& streaming) {

    QTcpSocket* socket = new QTcpSocket();
    // TCP_NODELAY + disable Nagle's algorithm
    socket->setSocketOption(QAbstractSocket::LowDelayOption, QVariant::fromValue(1));
    // Internetwork control
    socket->setSocketOption(QAbstractSocket::TypeOfServiceOption, QVariant::fromValue(192));
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

        if(queue.empty()) { // no new frame available
            continue;
        }

        // make sure that the queue doesn't grow too big or
        // the OOM killer will kick in
        if(queue.length() > 20) {
            queue.clear();
            continue;
        }

        QByteArray boundary = ("--boundary\r\n" \
                               "Content-Type: image/jpeg\r\n" \
                               "Content-Length: ");

        QByteArray img  = queue.dequeue();
        boundary.append(QString::number(img.length()));
        boundary.append("\r\n\r\n");

        socket->write(boundary);
        socket->waitForBytesWritten();
        boundary.clear();
        socket->write(img);
        socket->waitForBytesWritten();
        img.clear();
    }

    socket->flush();
    socket->abort();
    socket->deleteLater();
    streaming = false;
    queue.clear();
    return;
}
