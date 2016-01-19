import QtQuick 2.0
import Sailfish.Silica 1.0


Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height

        PullDownMenu {
            id: menu
            MenuItem {
                text: "About"
                onClicked: {
                    pageStack.push("About.qml");
                }
            }
        }

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
                    ScreenProvider.stopStreaming();
                    enabled = false;
                    startbutton.enabled = true;
                    fpstext.text = "";
                    frametime.text = "";
                    addresstext.text = "";
                }
            }

            Label {
                width: parent.width
                height: 140
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

            Slider {
                width: parent.width
                height: 100
                minimumValue: 0
                maximumValue: 100
                value: 50
                label: "JPEG Compression Level"
                valueText: value
                stepSize: 1;
                onReleased:  {
                    ScreenProvider.setCompression(value);
                }
            }

            TextSwitch {
                anchors.leftMargin: Theme.paddingLarge;
                anchors.rightMargin: Theme.paddingLarge;
                anchors.left: parent.left;
                anchors.right: parent.right;
                text: "Rotate stream when device is rotated"
                height: 90
                checked: false;
                onCheckedChanged: {
                    ScreenProvider.setRotate(checked)
                }
            }

            Label {
                width: parent.width
                height: 70
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                id: clienttext
                text: ""
            }

            Label {
                width: parent.width
                height: 30
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                id: fpstext
                text: ""
            }

            Label {
                width: parent.width
                height: 30
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                id: frametime
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
            fpstext.text = fps + " frames per second";
        }

        onFrameTime: {
            frametime.text = "Frame grab time: " + ms + " ms";
        }

    }

}


