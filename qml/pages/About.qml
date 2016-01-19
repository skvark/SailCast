import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.paddingLarge
            anchors.rightMargin: Theme.paddingLarge

            PageHeader { title: qsTr("About") }

            Label {
                width: parent.width
                height: 800
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.RichText;
                onLinkActivated: Qt.openUrlExternally(link)
                text: "<h1>SailCast</h1><br /><br />" +

                      "<style> .legend { font-size: 20px;  } </style>" +

                      "<span class=\"legend\">Created by</span><br />Olli-Pekka Heinisuo<br /><br />" +

                      "<font style=\"font-size: 23px;\">SailCast is a screencasting application which can stream your device screen as a motion JPEG over the network.<br /><br />" +
                      "Make sure you are connected to a wireless local area network (WLAN) and press start.
                       After that you can navigate to the given address for example with Firefox browser or VLC media player.</font><br /><br />" +

                      "<font style=\"font-size: 23px;\">This software is released under MIT license.<br />" +
                      "You can get the code and contribute at:</font>\n" +
                      "<style>a:link { color: " + Theme.highlightColor + "; }</style><br />" +
                      "<a href='http://github.com/skvark/SailCast'>GitHub \\ SailCast</a>";
            }
        }
    }
}
