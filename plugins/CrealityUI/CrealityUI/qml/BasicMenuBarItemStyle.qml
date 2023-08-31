//import QtQuick 2.0
//import QtQuick.Controls 2.12
import QtQuick 2.12
import QtQuick.Templates 2.12 as T
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
MenuBarItem {
    id:control
    property color textColor: control.hovered ? Constants.menuTextColor_hovered : Constants.menuTextColor_normal
    property color backgroundColor: control.hovered ? Constants.menuBarBtnColor_hovered : Constants.menuBarBtnColor
    // self-adaption width and height

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)
//    leftPadding : 8
//    rightPadding: 10
//    topPadding: 5

//    padding: 0
    leftPadding: 0
    rightPadding: 0
    // set font
    font{
        family: Constants.labelFontFamilyBold
        pointSize: Constants.labelFontPointSize_9
        weight: Font.Bold
        //            bold: true
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
        //            radius:5
        color: control.backgroundColor
        border.width: 0//hovered ? 1 :0
        border.color: Constants.menuBarBtnColor_border
    }
}
