import QtQuick 2.0
import QtQuick.Controls 2.0

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
                                                     : checkState==Qt.Checked ? "qrc:/UI/images/check2.png" : ""
    property color indicatorColor: isGroupPrintUsed ? Constants.themeGreenColor
                                                    : checkState==Qt.Checked ? Constants.themeGreenColor : "transparent"
    property color indicatorBorderColor:
        isGroupPrintUsed && hovered ? Constants.themeGreenColor
                                    : checkState==Qt.Checked || hovered ? Constants.themeGreenColor
                                                         : Constants.dialogItemRectBgBorderColor

    property int indicatorWidth: 16* screenScaleFactor
    property int indicatorHeight: 16* screenScaleFactor

    property int indicatorTextSpacing: 4 * screenScaleFactor

    signal styleCheckedChanged(var key, var value)
    signal styleCheckChanged(var key, var item)

    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    font.pointSize: Constants.labelFontPointSize_9
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
            font : control.font

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

    background: Item{
        Text {
            anchors.left: parent.left
            anchors.leftMargin: checkbox_indicator.width + indicatorTextSpacing
            anchors.verticalCenter: parent.verticalCenter
            color: enabled ? textColor : "gray"
            text: control.text;
            font.family: Constants.labelFontFamily
            font.pointSize: control.fontSize
            font.weight: control.fontWeight
        }
    }

    contentItem:Item{

    }

    indicator: Rectangle {
        id: checkbox_indicator
        implicitWidth: checkBoxVisible ? control.indicatorWidth : 0
        implicitHeight:checkBoxVisible ? control.indicatorHeight : 0
        anchors.verticalCenter: parent.verticalCenter
        radius: 3
        opacity: control.enabled ? 1 : 0.7
        border.color: control.indicatorBorderColor
        border.width: 1
        color: control.indicatorColor
        visible: checkBoxVisible

        Image {
            id: checkbox_image
            width: 9* screenScaleFactor
            height: 6* screenScaleFactor
            anchors.centerIn: parent
            source: control.indicatorImage
            visible: control.checked
        }
    }

    onCheckedChanged: {
        styleCheckedChanged(keyStr, checked.toString())
        styleCheckChanged(keyStr, this)
    }
}
