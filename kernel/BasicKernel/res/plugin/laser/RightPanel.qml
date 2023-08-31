import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item
{
    //anchors.right: parent.right
    id:idSelectInfo
    anchors.fill: parent
    LaserPanel {
        id: idLaserPanel
        anchors.fill: parent
        visible: true

    }
    onVisibleChanged: {

    }


}

