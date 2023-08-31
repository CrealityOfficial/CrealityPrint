//import QtQuick 2.0
//import QtQuick.Controls 2.12
import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
Button {
    id:control
    property color textColor: control.hovered ? Constants.menuTextColor_hovered : Constants.menuTextColor_normal
    property color backgroundColor: control.hovered ? Constants.menuBarBtnColor_hovered : Constants.menuBarBtnColor
    // self-adaption width and height

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
    leftPadding : 8
    rightPadding: 10
    topPadding: 5
    // set font
    font{
            family: Constants.labelFontFamily
            pointSize: Constants.labelFontPointSize_9
            weight: Constants.labelFontWeight

        }
    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display
        alignment: Qt.AlignCenter
        icon: control.icon
        text: control.text
        font: control.font
        color: control.textColor
    }
    background: Rectangle {
            y : 5
            x:2
            radius:5
            color: control.backgroundColor
            height: control.height - 10
            width: control.width-2
            border.width: 0//hovered ? 1 :0
            border.color: Constants.menuBarBtnColor_border
        }
}
