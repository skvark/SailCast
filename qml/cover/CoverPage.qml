import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {

    Label {
        id: label
        anchors.centerIn: parent
        text: "Idle"
    }

    Connections {
        target: ScreenProvider

        onFpsChanged: {
            if(fps !== 0) {
                label.text = "Streaming \n\n FPS: " + fps;
            } else {
                label.text = "Idle"
            }
        }
    }
}


