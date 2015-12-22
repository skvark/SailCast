#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <screenprovider.h>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("harbour-sailcast");
    QCoreApplication::setOrganizationName("harbour-sailcast");

    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();

    ScreenProvider interface;
    view->rootContext()->setContextProperty("ScreenProvider", &interface);

    view->setSource(SailfishApp::pathTo("qml/harbour-sailcast.qml"));
    view->showFullScreen();
    app->exec();
}

