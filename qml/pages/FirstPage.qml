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

            Slider {
                width: parent.width
                minimumValue: 90
                maximumValue: 250
                value: 140
                label: "Poll interval (ms), small values will stall the UI"
                valueText: value + " ms"
                stepSize: 1;
                onReleased:  {
                    ScreenProvider.setInterval(value);
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

    }

}


