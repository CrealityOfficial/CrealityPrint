import QtQuick 2.0

import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.0 as QQC2

CheckBox {
    id:control
    property color textColor : Constants.textColor
    property var keyStr: ""
    property var strToolTip: ""
    property var fontSize: Constants.labelFontPointSize_9
    property var fontWeight: Font.Normal
    property var isGroupPrintUsed: false
    property var checkBoxVisible: true
    property string indicatorImage: isGroupPrintUsed ? "qrc:/UI/images/check3.png"
                                                     : checked ? "qrc:/UI/images/check2.png" : ""
    property color indicatorColor: isGroupPrintUsed ? "#F9F9F9"
                                                    : checked ? "#009CFF" : "transparent"
    property color indicatorBorderColor:
        isGroupPrintUsed && hovered ? "#828790"
                                    : checked || hovered ? Constants.textRectBgHoveredColor
                                                         : Constants.dialogItemRectBgBorderColor

    property int indicatorWidth: 16* screenScaleFactor
    property int indicatorHeight: 16* screenScaleFactor

    property int indicatorTextSpacing: 4 * screenScaleFactor

    signal styleCheckClicked(var key, var value)
    signal styleCheckedChanged(var key, var value)
	signal styleCheckChanged(var key, var item)

    QQC2.ToolTip {
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

        contentItem: QQC2.TextArea{
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

    style: CheckBoxStyle {

        spacing: control.indicatorTextSpacing

        label: Text {
            color: enabled ? textColor : "gray"
            text: control.text;
            font.family: Constants.labelFontFamily
            font.pointSize: control.fontSize
            font.weight: control.fontWeight
        }

        indicator: Rectangle {
            id: checkbox_indicator
            implicitWidth: checkBoxVisible ? control.indicatorWidth : 0
            implicitHeight:checkBoxVisible ? control.indicatorHeight : 0
            radius: 3
            opacity: control.enabled ? 1 : 0.7
            border.color: control.indicatorBorderColor
            border.width: 1
            color: control.indicatorColor
            visible: checkBoxVisible
            /*Canvas{
                  anchors.fill: parent;
                  anchors.margins: 3;
                  visible: control.checked;
                  onPaint: {
                      var ctx = getContext("2d");
                      ctx.save();
                      ctx.strokeStyle = "#FFFFFF"//"#42BDD8"
                      ctx.lineWidth = 2;
                      ctx.beginPath();
                      ctx.moveTo(2,height/2);
                      ctx.lineTo(width/2 , height-2);
                      ctx.moveTo(width/2 , height-2);
                      ctx.lineTo(width-2 , 2);
                      ctx.stroke();
                      ctx.restore();
                  }
              }*/
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

    onClicked: {
        styleCheckClicked(keyStr, checked.toString())
    }

    onCheckedChanged: {
        styleCheckedChanged(keyStr, checked.toString())
		styleCheckChanged(keyStr, this)
    }
}
