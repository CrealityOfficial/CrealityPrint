import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12

ScrollBar {
    id: control

    property color handleNormalColor: "darkCyan"  //滑块颜色
    property color backgroundBarColor: "white"       //背景颜色
    property color handleHoverColor: Qt.lighter(handleNormalColor)
    property color handlePressColor: Qt.darker(handleNormalColor)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    //滑块样式
    contentItem: Rectangle {
        implicitWidth: control.interactive ? 10 : 2
        implicitHeight: control.interactive ? 10 : 2

        radius: width / 2
        color: control.pressed
               ?handlePressColor
               :control.hovered
                 ?handleHoverColor
                 :handleNormalColor

        opacity:(control.policy === ScrollBar.AlwaysOn || control.size < 1.0)?1.0:0.0
    }

    //背景
    background: Rectangle {
        implicitWidth: control.interactive ? 10 : 2
        implicitHeight: control.interactive ? 10 : 2
        color: backgroundBarColor
    }
}