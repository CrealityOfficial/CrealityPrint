import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import QtQuick.Templates 2.12 as T
import "../qml"
import "qrc:/CrealityUI"
T.TabButton {
    id: control

    property color textColor: (control.checked||control.hovered) ? Constants.textColor : Constants.textColor
    property color buttonColor: (control.checked||control.hovered)? Constants.tabButtonSelectColor: Constants.tabButtonNormalColor
    property var strTooptip: ""
    property var positionTooptip: BasicTooltip.Position.TOP
    property var tooltipX: control.width*0.75
    property var tooltipY: 2

    property alias buttonBorder: borderRect.border
    property int borderWidth: 0
    property int lBorderwidth : borderWidth
    property int rBorderwidth : borderWidth
    property int tBorderwidth : borderWidth
    property int bBorderwidth : borderWidth
    property alias pointSize: curText.fontPointSize
    property string fontText: ""

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)
    width: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                    implicitContentWidth + leftPadding + rightPadding)
    padding: 0
    leftPadding: 10
    rightPadding: 10
    spacing: 4
    font{
        family: Constants.labelFontFamily
        pointSize: Constants.labelFontPointSize_9
    }

    BasicTooltip {
        id: idTooptip
        visible: text !== "" && control.hovered
        text: strTooptip
    }

    //自定义的QmlIconLabel
    contentItem: QmlIconLabel {
        text: control.text
        font: control.font
        color: control.textColor
        spacing: control.spacing
        source: control.icon.source
        height: 20
        width: 20
        CusText{
            id: curText
            anchors.centerIn: parent
            fontText: control.fontText
            fontPointSize: Constants.labelFontPointSize_9
            fontWeight: Font.Bold
            fontColor: control.textColor
//            fontPointSize: control.font.pointSize
        }
    }

    background: Rectangle {
        id: borderRect
        width:control.width
        height: control.height// - 1
        color: control.buttonColor
        border.width: hovered ? 1 : 0
        border.color: Constants.rectBorderColor
        radius: 5
//        Rectangle
//        {
//            id: innerRect
//            color: control.buttonColor
//            anchors {
//                fill: parent
//                leftMargin: lBorderwidth
//                rightMargin: rBorderwidth
//                topMargin: tBorderwidth
//                bottomMargin: bBorderwidth
//            }
//        }
    }
}
