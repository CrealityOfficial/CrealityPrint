import QtQuick 2.0
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Rectangle
{
    id:idColorPanel
    width: parent.width
    height: parent.height
    color: "transparent"
    property int colorSelectIndex: 1
    objectName: "fdmcolor"
    property var nNozzleCount
    SpeedColorPanel
    {
        id: idSpeedPanel
        anchors.right: idColorPanel.right
        anchors.rightMargin: 10
        anchors.top: idColorPanel.top
        anchors.topMargin: 20
        visible: colorSelectIndex === 0 ? true : false
    }

    StructureColorPanel
    {
        id : idStructure
        anchors.right: idColorPanel.right
        anchors.rightMargin: 10
        anchors.top: idColorPanel.top
        anchors.topMargin: 20
        visible: colorSelectIndex ===1 ? true : false
    }

    NozzleColorPanel
    {
        id:idNozzlePanel
        anchors.right: idColorPanel.right
        anchors.rightMargin: 10
        anchors.top: idColorPanel.top
        anchors.topMargin: 20
        visible: colorSelectIndex === 2 ? true : false
        objectName: "NozzleColorPanel"
        Component.onCompleted:
        {
        }
    }
}
