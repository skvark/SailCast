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

ScreenProvider::ScreenProvider(QObject* parent): QTcpServer(parent)
{
    streaming_ = false;
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;

    framebufferinfo_.fbmmap = (unsigned int*)MAP_FAILED;
    framebufferinfo_.scrinfo = var;
    framebufferinfo_.fix_scrinfo = fix;
    framebufferinfo_.fps = 0.0;

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
                                                  PROT_READ, MAP_SHARED,
                                                  fbDevice_,
                                                  0);

    if (framebufferinfo_.fbmmap == (unsigned int*)MAP_FAILED){
        return;
    }
}

ScreenProvider::~ScreenProvider()
{
    delete clientConnection_;
    clientConnection_ = 0;

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
        QFuture<void> future = QtConcurrent::run(streamLoop,
                                                 handle,
                                                 std::ref(queue_),
                                                 std::ref(streaming_));
        watcher_->setFuture(future);

    }
}

void ScreenProvider::stop()
{
    streaming_ = false;
    this->close();
}

void ScreenProvider::handleEndedStream()
{
    emit clientDisconnected();
    streaming_ = false;
    fpstimer_.stop();
    disconnect(watcher_, SIGNAL(finished()), this, SLOT(handleEndedStream()));
    delete watcher_;
    watcher_ = 0;
}

void ScreenProvider::updateFps()
{
    emit fpsChanged(framebufferinfo_.fps);
}

void ScreenProvider::onClientConnected()
{
    QtConcurrent::run(grabFramesLoop,
                      std::ref(streaming_),
                      std::ref(framebufferinfo_),
                      std::ref(queue_));
    fpstimer_.start();
}

void grabFramesLoop(bool &streaming, framebufferinfo &info, QQueue<QByteArray> &queue)
{
    QImage* img = new QImage(info.scrinfo.xres, info.scrinfo.yres, QImage::Format_RGBA8888);
    QBuffer* imgBuffer = new QBuffer();
    QElapsedTimer timer;
    timer.start();
    long long frames = 0;

    while(streaming) {

        for (unsigned int y = 0; y < info.scrinfo.yres; ++y)
        {
            QRgb *rowData = (QRgb*)img->scanLine(y);
            for (unsigned int x = 0; x < info.scrinfo.xres; ++x)
            {
                unsigned int value = *((unsigned int *)info.fbmmap + ((x + info.scrinfo.xoffset) + (y + info.scrinfo.yoffset) * (info.fix_scrinfo.line_length / 4)));
                int b = (value) & 0xFF;
                int g = (value >> 8) & 0xFF;
                int r = (value >> 16) & 0xFF;
                int a = (value >> 24)& 0xFF;
                rowData[x] = qRgba(r, g, b, a);
            }
        }

        QByteArray ba;
        imgBuffer->setBuffer(&ba);
        imgBuffer->open(QIODevice::WriteOnly);
        img->save(imgBuffer, "JPG");
        queue.enqueue(ba);
        imgBuffer->close();

        ++frames;

        info.fps = round((float)frames / (timer.elapsed() / 1000));

    }

    delete img;
    delete imgBuffer;
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

        if(queue.empty()) { // no new frame available
            QThread::usleep(30); // wait a bit, grabbing single frame takes about 60 ms
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
