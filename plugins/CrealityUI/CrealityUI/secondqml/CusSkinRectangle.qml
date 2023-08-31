import QtQuick 2.0

Item {
    id:root
    property var verOffset: 24

    Rectangle {
            id: bar
            rotation: 45
            width: verOffset
            height: width
            color: "white"//barColor
            border.color: "red"//borderColor
            border.width: 2
            //水平居中
            anchors.horizontalCenter: parent.horizontalCenter
            //垂直方向上，由ToolTip的y值，决定位置
            anchors.top: root.top
            z:root.z + 1
        }

//    Rectangle {
//        id: background
//        anchors.top:bar.bottom
//        anchors.topMargin: verOffset
//        color: parent.color
//        radius: 8
//        border.color:parent.border.color
//        border.width: parent.border.width
//        width: root.width
//        height: root.height
//    }


}
