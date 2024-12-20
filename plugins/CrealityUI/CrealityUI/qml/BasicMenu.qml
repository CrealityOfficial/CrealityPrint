import QtQuick 2.12
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
Menu {
    id: idEditMenu
    //title: "&Edit"
    signal loadEditSouce()
    property var parentLoad
    property var triggerSource
    property var listObj
    property var maxWidth: 200* screenScaleFactor
    x:3
    topPadding: -4
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    delegate: BasicMenuItemStyle
    {
        highlightSeparator : false
        separatorVisible : false
    }

//    contentWidth : 200


    background: Rectangle {
        implicitWidth: Math.max(maxWidth,240)* screenScaleFactor
        color: "white"
        Rectangle {
            id: mainLayout
            // 一个填满窗口的容器，Page、Rectangle 都可以
            anchors.fill: parent
            //            // 当窗口全屏时，设置边距为 0，则不显示阴影，窗口化时设置边距为 10 就可以看到阴影了
            //            anchors.margins: 5
            color: Constants.menuStyleBgColor //Constants.themeColor //"transparent"
            opacity: 1
        }
        DropShadow {
            anchors.fill: mainLayout
            horizontalOffset: 3
            verticalOffset: 3
            //                radius: 8
            samples: 9
            source: mainLayout
            color: Constants.dropShadowColor // "#222222"
        }
    }
}

