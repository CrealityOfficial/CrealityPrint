import QtQuick 2.5
import QtQuick.Controls 2.12
Button {
    id: cusButtonImage
    implicitWidth: 24
    implicitHeight: 24
    width: 24
    height: 24
//    property alias tipText: toolTip.text
//    property alias tipItem: toolTip
//    property alias tipVisible: toolTip.visible
//    property alias tipDelay: toolTip.delay
//    property alias tipTimeout: toolTip.timeout
    property bool selected: false
    property string btnImgNormal
    property string btnImgHovered
    property string btnImgPressed
    property string btnImgDisbaled

    property color btnImgNormalColor : "#222222"
    property color btnImgHoveredColor : "#222222"
    property color btnImgPressedColor  :"#0A0A0A"

    property color borderColor : "#353535"
    property string btnImgUrl: {
        if (cusButtonImage.pressed || selected) {
            return btnImgPressed
        } else if (cusButtonImage.hovered) {
            return btnImgHovered
        } else {
            return btnImgNormal
        }
    }
    property color bgColor :
    {
        if (cusButtonImage.pressed || selected) {
            return btnImgPressedColor
        } else if (cusButtonImage.hovered) {
            return btnImgHoveredColor
        } else {
            return btnImgNormalColor
        }
    }
    property bool borderEnabled:
    {
        if (cusButtonImage.pressed || selected) {
            return true
        } else if (cusButtonImage.hovered) {
            return true
        } else {
            return false
        }
    }

    background: Rectangle {
        width: cusButtonImage.width
        height: cusButtonImage.height
        color:bgColor
        border.width: borderEnabled ? 1 : 0
        border.color: borderColor
        Image {
            id: backImage
            source: btnImgUrl
            anchors.centerIn: parent
            width: sourceSize.width
            height: sourceSize.height
        }
    }
//    ToolTip {
//        id: toolTip
//        visible: cusButtonImage.hovered && String(text).length
//        delay: 500
//    }
}
