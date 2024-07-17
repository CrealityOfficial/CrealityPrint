import QtQuick 2.0

import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
CheckBox {
    id:control
    property color textColor : Constants.textColor
    property var keyStr: ""
    property var strToolTip: ""
    property var fontSize: Constants.labelFontPointSize_9
    property var fontWeight: Font.Normal
    property var isGroupPrintUsed: false

    property string indicatorImage: isGroupPrintUsed ? "qrc:/UI/images/check3.png"
                                                     : checked ? "qrc:/UI/images/check2.png" : ""
    property color indicatorColor: isGroupPrintUsed ? "#F9F9F9"
                                                    : checked ? "#17CC5F" : "transparent"
    property color indicatorBorderColor:
        isGroupPrintUsed && hovered ? "#829087"
                                    : checked || hovered ? Constants.textRectBgHoveredColor
                                                         : Constants.dialogItemRectBgBorderColor

    property int indicatorWidth: 16* screenScaleFactor
    property int indicatorHeight: 16* screenScaleFactor
    signal styleCheckChanged(var key, var item)

    ToolTip {
        id: tipCtrl
        visible: hovered&&strToolTip ? true : false
        //timeout: 2000
        delay: 100
        width: 400
        implicitHeight: idTextArea.contentHeight + 40

        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }

        contentItem: TextArea{
            id: idTextArea
            text: strToolTip
            wrapMode: TextEdit.WordWrap
            color: Constants.textColor
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_9
            readOnly: true
            background: Rectangle
            {
                anchors.fill : parent
                color: Constants.tooltipBgColor
                border.width:1
                //border.color:hovered ? "lightblue" : "#bdbebf"
            }
        }
    }
    height: indicatorHeight
    contentItem: Label {
        color: enabled ? textColor : "gray"
        text: control.text
        /*font: Constants.labelFontFamily*/
        font.family: Constants.labelFontFamily
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        renderType: Text.NativeRendering
        elide: Text.ElideRight
        leftPadding: control.indicator.width
        rightPadding: control.rightPadding
    }

    indicator: Item{
        implicitWidth: indicatorWidth
        implicitHeight: indicatorHeight
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            implicitWidth: control.indicatorWidth
            implicitHeight: control.indicatorHeight
            anchors.verticalCenter: parent.verticalCenter
            radius: 3
            opacity: control.enabled ? 1 : 0.7
            border.color: control.indicatorBorderColor
            border.width: 1
            color: control.indicatorColor
            Image {
                id: checkbox_image
                width: 9* screenScaleFactor
                height: 6* screenScaleFactor
                anchors.centerIn: parent
                source: control.indicatorImage
                visible: control.checked
            }
        }
    }
    onCheckedChanged: {
        styleCheckChanged(keyStr, this)
    }
}
