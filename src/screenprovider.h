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
#include <QOrientationSensor>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <unistd.h>


struct framebufferinfo {
    fb_var_screeninfo scrinfo;
    fb_fix_screeninfo fix_scrinfo;
    void *fbmmap;
    int fps;
    int frametime;
    int compression;
};

void streamLoop(qintptr socketDesc, QQueue<QByteArray> &queue, bool& streaming);
void grabFramesLoop(bool &streaming, framebufferinfo &info, QQueue<QByteArray> &queue, QOrientationReading::Orientation &orientation);

class ScreenProvider : public QTcpServer
{
    Q_OBJECT
public:
    ScreenProvider(QObject* parent = 0);
    ~ScreenProvider();

    Q_INVOKABLE void start();
    Q_INVOKABLE void stopStreaming();
    Q_INVOKABLE void setCompression(int value);
    Q_INVOKABLE void setRotate(bool value);
    void incomingConnection(qintptr handle) Q_DECL_OVERRIDE;

signals:
    void serverChanged(int port, QString address);
    void clientConnected();
    void clientDisconnected();
    void fpsChanged(int fps);
    void frameTime(int ms);

public slots:
    void handleEndedStream();
    void updateFps();
    void onClientConnected();
    void orientationChanged();

private:

    QFutureWatcher<void> *watcher_;
    QQueue<QByteArray> queue_;
    QFuture<void> future_;
    QFuture<void> future2_;
    bool streaming_;
    QHostAddress ipAddress_;
    int port_;
    QTimer fpstimer_;
    int compression_;
    QOrientationSensor *orientationSensor_;
    QOrientationReading::Orientation orientation_;

    int fbDevice_;
    struct framebufferinfo framebufferinfo_;
};

#endif // SCREENPROVIDER_H
