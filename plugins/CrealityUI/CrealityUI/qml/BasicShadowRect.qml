import QtQuick 2.0
import QtGraphicalEffects 1.12
Rectangle
{
    implicitWidth: 240
    color: "transparent"
    Rectangle
    {
        id: mainLayout
        // 一个填满窗口的容器，Page、Rectangle 都可以
        width: parent.width //+ 5
        height: parent.height //+ 5
        anchors.centerIn: parent
//        anchors.fill: parent
        // 当窗口全屏时，设置边距为 0，则不显示阴影，窗口化时设置边距为 10 就可以看到阴影了
        //radius: 5
        anchors.margins : 5

        color: Constants.dialogContentBgColor//control.buttonColor //"transparent"
        opacity: 1
        border.color: Constants.dialogBorderColor//"#061F3B"
        border.width: 1
    }

    DropShadow {
        anchors.fill: mainLayout
//        horizontalOffset: 5
//        verticalOffset: 5
        radius: 8
        samples: 17
        source: mainLayout
        color:  Constants.dropShadowColor
    }

}
