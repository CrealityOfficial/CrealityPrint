import QtQuick 2.12
import QtQuick.Controls 2.5//2.12
import QtQuick.Shapes 1.12

MenuItem {
    id: control
    property alias separatorVisible:  idSeparator.visible
    property color textColor: !enabled ? control.palette.mid :  "#000000"
    property color buttonColor:  control.hovered ? "#74C9FF" : "#FFFFFF"
    property color indicatorColor

    property var separatorHeight: 1
    property var textPadding: 30


    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 34/*Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)*/
    // set font
    font{
        family: Constants.labelFontFamily
        pointSize: Constants.labelFontPointSize_10
    }
    contentItem:
        Text {
        //没有边距就贴在边上了
        // anchors.left: idColorRect.left
        leftPadding: textPadding
        // rightPadding: control.mirrored ? indicatorPadding : arrowPadding
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font: control.font
        color: control.textColor

    }

    indicator: Item {
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: 20
        implicitHeight: 20
        x:4
        Rectangle {
            x:3
            width: 20
            height: 20
            anchors.centerIn: parent
            color: indicatorColor
            visible: control.checkable//选择后显示，类似单选按钮
            border.color: indicatorColor
            radius: 3
//            Rectangle {
//                width: 14
//                height: 14
//                anchors.centerIn: parent
//                visible: control.checked
//                color: indicatorColor
//                radius: 2
//            }
        }
    }

    arrow: Shape {
        id: item_arrow
        x:  control.width - width
        //y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.subMenu
        enabled: control.enabled
        implicitWidth: 30
        implicitHeight: 22
        ShapePath {
            strokeWidth: 0
            strokeColor: !enabled ? control.palette.mid : "white"
            fillRule: ShapePath.WindingFill
            fillColor: !enabled ? control.palette.mid : "white"
            startX: item_arrow.width/2
            startY: item_arrow.height*3/4
            PathLine { x:item_arrow.width/2; y:item_arrow.height/4 }
            PathLine { x:item_arrow.width/4 *3; y:item_arrow.height/2 }
            PathLine { x:item_arrow.width/2; y:item_arrow.height*3/4 }
        }
    }
    //        Rectangle
    //        {
    //            width: 20
    //            height: control.height -2*separatorHeight
    //            anchors.left: control.left
    //            anchors.leftMargin: 4
    //            anchors.top: control.top
    //            anchors.topMargin: separatorHeight
    //            Rectangle {
    //                id : idColorRect
    //                x:3
    //                anchors.top: parent.top
    //                anchors.topMargin: 2
    //                width: 18
    //                height: 18
    //                color: "red"
    //                border.width: 0.5
    //                border.color: "white"
    //            }
    //            color: control.buttonColor
    //        }
    //Add separator before item
    Rectangle
    {
        id: idSeparator
        anchors.top: control.top
        width:control.width -40//10
        height: separatorHeight
        x:36//5
        color: "#E0E0E0"
        visible:control.subMenu ? true : false
    }

    background: Rectangle {
        anchors.fill: control
        //anchors.top: control.top
        /*anchors.topMargin: separatorHeight*/
        //anchors.bottom: control.bottom
        /*anchors.bottomMargin: separatorHeight*/
        x:1
        width: control.width -2
        color: "#FFFFFFFF"//control.buttonColor
        Rectangle{
            x: 3
            y: 4
            width: parent.width-6
            height: parent.height-8
            color: control.buttonColor
        }
    }
}
