import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item
{
    //anchors.right: parent.right
    id:idSelectInfo

    width: 50
    height: 200
    property var size
    property var modelname:""
    property var verticessize:0
    property var facesize:0
    property var modeldata

    LaserToolBar {
        id: idLaserToolBar
        anchors.fill: parent
        visible: true

    }
    onVisibleChanged: {
        leftToolBar.visible = !idLaserToolBar.visible;
    }


}

