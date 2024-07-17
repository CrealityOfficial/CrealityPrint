import QtQuick 2.15
import QtQuick.Controls 2.15

import ".."
import "../.."

Rectangle {
    color: "lightGrey"
    MouseArea {
        id: backgroundArea
        anchors.fill: parent
        onClicked: {
            focus = true
        }
    }
}
