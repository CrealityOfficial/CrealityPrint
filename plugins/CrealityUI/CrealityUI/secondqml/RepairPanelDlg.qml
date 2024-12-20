import QtQuick 2.10
import QtQuick.Controls 2.12
import "../components"
import "../qml"
DockItem
{
    id: idDialog
    width: 700
    height: 320
    titleHeight : 30
    title: qsTr("Model Repair")
    signal accept()
    signal cancel()
    property var faceSize : 0
    property var errorNormals : 0
    property var verticessize : 0
    property var shells :0
    property var errorEdges : 0
    property var errorHoles :0
    property var errorIntersects :0
    //    bdContentItem: Item{
    Column
    {
        id : idRootPanel
        x:5
        y:40
        width: parent.width - 10

        Item {
            height: 40
            width: parent.width

            StyledLabel
            {

                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Damaged Plane")  //破面

            }


            StyledLabel
            {
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.verticalCenter: parent.verticalCenter
                text: errorEdges
                font.bold: true
                font.pointSize: Constants.labelFontPointSize_16
                horizontalAlignment: Qt.RightToLeft
            }
            Rectangle
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.bottom: parent.bottom
                height: 1
                color: Constants.splitLineColor
            }
        }

        Item {
            height: 40
            width: parent.width

            StyledLabel
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Hole")      // 孔洞

            }


            StyledLabel
            {
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.verticalCenter: parent.verticalCenter
                text: errorHoles
                font.bold: true
                font.pointSize: Constants.labelFontPointSize_16
                horizontalAlignment: Qt.RightToLeft
            }
            Rectangle
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.bottom: parent.bottom
                //                    width: parent.width -40
                height: 1
                color: Constants.splitLineColor
            }
        }
        Item {
            height: 40
            width: parent.width

            StyledLabel
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Redundant Shell")       //多余壳体

            }


            StyledLabel
            {
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.verticalCenter: parent.verticalCenter
                text: shells
                font.bold: true
                font.pointSize: Constants.labelFontPointSize_16
                horizontalAlignment: Qt.RightToLeft
            }
            Rectangle
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.bottom: parent.bottom
                height: 1
                color: Constants.splitLineColor
            }
        }
        Item {
            height: 40
            width: parent.width

            StyledLabel
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Normal")        //法线

            }


            StyledLabel
            {
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.verticalCenter: parent.verticalCenter
                text: errorNormals
                font.bold: true
                font.pointSize: Constants.labelFontPointSize_16
                horizontalAlignment: Qt.RightToLeft
            }
            Rectangle
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.bottom: parent.bottom
                height: 1
                color: Constants.splitLineColor
            }
        }
        Item {
            height: 40
            width: parent.width

            StyledLabel
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Crossed Model Planes")       //自交

            }


            StyledLabel
            {
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.verticalCenter: parent.verticalCenter
                text: errorIntersects
                font.bold: true
                font.pointSize: Constants.labelFontPointSize_16
                horizontalAlignment: Qt.RightToLeft
            }
            Rectangle
            {
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.right: parent.right
                anchors.rightMargin : 20
                anchors.bottom: parent.bottom
                //                    width: parent.width
                height: 1
                color: Constants.splitLineColor
            }
        }

        Item {
            width: parent.width
            height: 60
            BasicDialogButton
            {
                id:idMoreButton
                text: qsTr("Model Repair")
                //leftPadding: -10
                width: 125
                height: 28
                anchors.centerIn: parent
                enabled:  !(faceSize === 0 &&
                            errorNormals === 0 &&
                            verticessize === 0 &&
                            shells === 0 &&
                            errorHoles === 0 &&
                            errorIntersects === 0 &&
                            errorEdges === 0)

                onClicked:
                {
                    close()
                    accept()
                }
            }
        }
    }
    //    }
}
