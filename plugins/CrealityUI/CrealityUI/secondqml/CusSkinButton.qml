import QtQuick 2.9
import QtQuick.Controls 2.2
//import TaoQuick 1.0
import ".."
import "../qml"
CusSkinButton_Image {
    id:idSkinBtn
    btnImgUrl: (hovered || pressed ? "qrc:/UI/images/skin_white.png" : "qrc:/UI/images/skin_gray.png")
    tipText: qsTr("skin") //+ trans.transString
    onClicked: {
        skinBox.show()
    }
    //矩形旋转45度，一半被toolTip遮住(重合)，另一半三角形和ToolTip组成一个带箭头的ToolTip
//    Rectangle {
//        id: bar
//        visible: skinBox.visible
//        rotation: 45
//        width: 24
//        height: width
//        color: "blue"//barColor
//        border.color: "red"//borderColor
//        border.width: 2
//        //水平居中
//        anchors.horizontalCenter: parent.horizontalCenter
//        //垂直方向上，由ToolTip的y值，决定位置
//        anchors.verticalCenter: idSkinBtn.bottom
//        anchors.verticalCenterOffset: 24
//        z:100
//    }

    CusSkinPopup {
        id: skinBox
        barColor: Constants.controlColor
        backgroundWidth: 270 //3*90
        backgroundHeight: 100 //180 2*90
        borderColor: Constants.controlBorderColor
        contentItem: GridView {
            id:idGrid
            anchors.fill: parent
            anchors.margins: 10
            model: Constants.themes
            cellWidth: 80
            cellHeight: 80
//            clip: true
            delegate: Item {
                width: 80
                height: 80
                Rectangle {         //6ge Rectangle color
                    anchors.fill: parent
                    anchors.margins: 4
                    color: model.themeColor
                }
                Rectangle {         //Select rectangle border color
                    anchors.fill: parent
                    color: "transparent"
                    border.color: model.themeColor
                    border.width: 2
                    visible: a.containsMouse
                }
                Label {           //center label  color name
                    anchors {
                        centerIn: parent
                    }
                    horizontalAlignment: Text.AlignHCenter
                    color: "white"
                    text: qsTr(model.name)
                }
                Rectangle {         //select circle show
                    x: parent.width - width
                    y: parent.height - height
                    width: 20
                    height: width
                    radius: width / 2
                    color: model.themeColor
                    border.width: 3
                    border.color: Constants.controlBorderColor
                    visible: Constants.currentTheme === index
                }
                MouseArea {
                    id: a
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        Constants.currentTheme = index
                        skinBox.hide()
                    }
                }
            }
        }

    }
}
