import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQuick.Shapes 1.12
MenuItem {
    id: control
    property bool highlightSeparator:  false
    property bool separatorVisible: true
    property color textColor: !enabled ? control.palette.mid : (control.hovered ? "white" : Constants.menuTextColor)//"#333333"
    property color buttonColor:   control.hovered ? Constants.menuStyleBgColor_hovered : Constants.menuStyleBgColor//Constants.menuStyleBgColor_hovered/*"#3AC2D7"*/: Constants.menuStyleBgColor//"#061F3B"
    property bool itemChecked: false
    property var actionShortcut: ""
    property var separatorHeight: 1
    property real rectLeftPadding: control.hasIcon ? 36 * screenScaleFactor : 0 * screenScaleFactor
    property var imageUrl : control.hovered  ? "qrc:/UI/photo/submenu_d.png" : "qrc:/UI/photo/submenu.png"
    property var hasIcon: true
    property var centerText: false
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: 34 * screenScaleFactor //22
    // set font
    font{
        family: Constants.labelFontFamily
        pointSize: Constants.labelFontPointSize_9
    }
    contentItem:RowLayout{
        height : 20
        Text {
            id:txt1
            Layout.leftMargin: control.rectLeftPadding
            Layout.minimumWidth: 100 * screenScaleFactor
            Layout.maximumWidth: 1200 * screenScaleFactor
            Layout.fillWidth: true
            horizontalAlignment: centerText ? Text.AlignHCenter : Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: control.text
            font: control.font
            color: control.textColor
            elide : Text.ElideMiddle
        }

        Text {
            id:txt2
            Layout.rightMargin: 20 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor
            verticalAlignment: Text.AlignVCenter
            text: control.actionShortcut
            font: control.font
            color: control.textColor
        }
    }


    indicator: Item {
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: 14 * screenScaleFactor
        implicitHeight: 14 * screenScaleFactor
        x:12 * screenScaleFactor
        Rectangle {
            x:3 * screenScaleFactor
            width: 14 * screenScaleFactor
            height: 14 * screenScaleFactor
            anchors.centerIn: parent
            visible: control.itemChecked//选择后显示，类似单选按钮
            border.color: "#292929"
            radius: 7
            Rectangle {

                width: 8 * screenScaleFactor
                height: 8 * screenScaleFactor
                anchors.centerIn: parent
                visible: control.itemChecked
                color: Constants.themeGreenColor
                radius: 4
            }
        }
    }

    arrow: Item {
        id: item_arrow
        x:  control.width - width
        //y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.subMenu
        enabled: control.enabled
        implicitWidth: 30 * screenScaleFactor
        implicitHeight: control.height
        Image {
            id: name
            anchors.centerIn: parent
            width: 4  * screenScaleFactor
            height: 7  * screenScaleFactor
            source: imageUrl
        }
    }

    Item
    {
        width: 16  * screenScaleFactor
        height: control.height -2*separatorHeight
        anchors.left: control.left
        anchors.leftMargin: 2  * screenScaleFactor
        anchors.top: control.top
        anchors.topMargin: separatorHeight
        Image {
            x:3
            anchors.centerIn: parent
            width: 16 * screenScaleFactor
            height: 16 * screenScaleFactor
            source: icon.source
            visible: false
        }
    }

    //Add separator before item
    Rectangle
    {
        id: idSeparator
        anchors.top: control.top
        width:control.width - rectLeftPadding
        height: separatorHeight
        x: rectLeftPadding //36//5
        color: "#E0E0E0"//"white"
        visible: !highlightSeparator && separatorVisible
    }

    //Add highlight separator before item
    Rectangle
    {
        id: idHighlightSeparator
        anchors.top: control.top
        width:control.width
        height: separatorHeight
        x:0
        color: "#BBBBBB"//"white"
        visible: highlightSeparator && separatorVisible

    }

    background: Rectangle {
        anchors.fill: control
        x:1
        width: control.width -2
        color: "#FFFFFF"//control.buttonColor
        Rectangle{
            x: 3
            y: 4
            width: parent.width-6
            height: parent.height-8
            color: control.buttonColor
        }
        //radius: 3
    }
}
