import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.12

Rectangle{
    id: rect

    property alias imgUrl: img.source
    property alias title: titleTxt.fontText

    property bool topLineEnabled: false
    property bool bottomLineEnabled: false
    property color lineColor: Constants.right_panel_menu_split_line_color
    property int lineHeight: 1
    property alias sourceSize: img.sourceSize
    property int radiusClipAlign: 0 //0: 不需要切; 1: top 两个角；2：bottom 两个角
    width: 300 * screenScaleFactor
//    height: 32 * screenScaleFactor
    implicitHeight: titleTxt.height
    color: "transparent"

    Rectangle {
      id: topLine
      anchors.top: rect.top
      width: rect.width
      height: rect.lineHeight
      color: rect.topLineEnabled ? rect.lineColor : rect.color
    }

    Image{
        id:img
        source: ""
        sourceSize.width: 12 * screenScaleFactor
        sourceSize.height: 12 * screenScaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 9
        anchors.verticalCenter: parent.verticalCenter
    }

    CusText{
        id:titleTxt
        anchors.left: img.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        fontPointSize: Constants.labelFontPointSize_12
        fontWidth: 200 * screenScaleFactor
        hAlign: 0
        fontColor: Constants.right_panel_text_default_color
        fontWeight: Font.Medium
    }

    Rectangle {
      id: bottomLine
      anchors.bottom: rect.bottom
      width: rect.width
      height: rect.lineHeight
      color: rect.bottomLineEnabled ? rect.lineColor : rect.color
    }
}
