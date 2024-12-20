import QtQuick 2.9
import QtQuick.Controls 2.2
import "../secondqml"
import "../qml"

CusRoundedBg {
    id: rec
    property alias txtContent: txt.text
    property alias txtFamily: txt.font.family
    property alias txtWeight: txt.font.weight
    property alias txtBold: txt.font.bold
    property alias txtPointSize: txt.font.pointSize
    property color txtColor: Constants.textColor
    property bool isHovered: false
    property bool isChecked: false
    property color normalColor: Constants.profileBtnColor
    property color hoveredColor: Constants.profileBtnHoverColor
    property color pressedColor: Constants.profileBtnHoverColor

    property color normalBdColor: "transparent"
    property color hoveredBdColor: "transparent"
    property color pressedBdColor: "transparent"
    property bool checkable: false
    property bool showToolTip: false

    property alias toolTipText: idTooptip.text
    property alias toolTipPosition: idTooptip.position

    allRadius: true
    clickedable: false
    color: isChecked ? pressedColor : (isHovered ? hoveredColor : normalColor)
//    opacity : enabled ? 1 : 0.3
    borderColor: isHovered ? (isChecked ? pressedBdColor : hoveredBdColor) : normalBdColor

    BasicTooltip {
        id: idTooptip
        visible: isHovered && text && txt.width < txt.contentWidth
    }

    Text {
        id: txt
        text: ""
        color: txtColor
        horizontalAlignment: Text.AlignHCenter
        width: rec.width - 10 * screenScaleFactor
        anchors.centerIn: parent
        font.weight: Constants.labelFontWeight
        font.family: Constants.labelFontFamily
        font.pointSize : Constants.labelFontPointSize_9
        elide: Text.ElideRight
    }

    MouseArea {
        hoverEnabled: true
        anchors.fill: parent

        onEntered: isHovered = true

        onExited: isHovered = false

        onReleased: rec.clicked()
    }
}
