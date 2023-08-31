import QtQuick 2.5
import QtQuick.Controls 2.12
Button {
    id: cusButtonImage
    property bool isRound: false
    width: 24
    height: 24
    checkable: true
    //signal sigBtnClicked(var key)

    //property var keystr: 0
    property bool sync: false
    property bool selected: false
    property alias tipText: toolTip.text
    property alias tipItem: toolTip
    property alias tipVisible: toolTip.visible
    property alias tipDelay: toolTip.delay
    property alias tipTimeout: toolTip.timeout
    property alias radius: backRec.radius
    property string btnImgNormal
    property string btnImgHovered
    property string btnImgPressed
    property string btnImgDisbaled
    property color defaultBtnBgColor:"transparent"
    property color hoveredBtnBgColor:"transparent"
    property color selectedBtnBgColor:"transparent"

    property string btnImgUrl: {
        if (!cusButtonImage.enabled) {
            return btnImgDisbaled
        } else if (cusButtonImage.pressed || selected) {
            return btnImgPressed
        } else if (cusButtonImage.hovered) {
            return btnImgHovered
        } else {
            return btnImgNormal
        }
    }

    background: Rectangle {
        id:backRec
        radius: isRound ? cusButtonImage.width/2 : 0
        color: (sync ? cusButtonImage.selected : cusButtonImage.checked) ? selectedBtnBgColor : (cusButtonImage.hovered ? hoveredBtnBgColor : defaultBtnBgColor)
        Image {
            id: backImage
            anchors.centerIn: parent
            source: btnImgUrl
            asynchronous: true
            mipmap: true
            smooth: true
            cache: false
            fillMode: Image.PreserveAspectFit
        }
    }
    ToolTip {
        id: toolTip
        visible: cusButtonImage.hovered && String(text).length
        delay: 500
    }
}
