import QtQuick 2.0
import Sailfish.Silica 1.0


Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        Column {
            id: column

            width: page.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("SailCast")
            }

            Button {
                id: startbutton
                text: "Start"
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    ScreenProvider.start();
                    enabled = false;
                    stopbutton.enabled = true;
                }
            }

            Button {
                id: stopbutton
                text: "Stop"
                enabled: false;
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    ScreenProvider.stop();
                    enabled = false;
                    startbutton.enabled = true;
                }
            }

            Label {
                width: parent.width
                height: 200
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                id: addresstext
                text: ""
                textFormat: Text.RichText;
                onLinkActivated: Qt.openUrlExternally(link)
            }

            Label {
                width: parent.width
                height: 100
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                id: fpstext
                text: ""
            }

            Label {
                width: parent.width
                height: 200
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                id: clienttext
                text: ""
            }
        }
    }

    Connections {
        target: ScreenProvider

        onServerChanged: {
            addresstext.text = "Server running at " + address + ":" + port;
        }

        onClientConnected: {
            clienttext.text = "Client connected, streaming."
        }

        onClientDisconnected: {
            clienttext.text = ""
        }

        onFpsChanged: {
            fpstext.text = "Current fps: " + fps;
        }

    }

}


