import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1
import QtQml 2.3
import "../CusTip"
import "../"
Item {
    property int borderRadius
    property int borderWidth: 1
    property string borderColor: "#ccc"
    property color textColor: "#fff"
    property int fontSize: 14
    property color normalColor: Constants.btnNormalColor
    property color hoveredColor: Constants.btnHoverColor
    property color pressedColor: Constants.btnCheckedColor
    property string imgSource:"qrc:/res/img/profile_normal.png"
    property variant imgSourceSize : [10,10]
    property string mtext: "按钮"
    property string mtip: "提示"
    signal clicked()
    id: root

    Rectangle {
        border.color: borderColor
        border.width: borderWidth
        radius: borderRadius || height/2
        height: parent.height
        width: parent.width
        anchors.centerIn: parent
        color: isHover ? hoveredColor : normalColor
        property bool isHover: false
        BasicTooltip {
            visible: parent.isHover && !!mtip
            text: mtip
        }
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onClicked: clicked()
            cursorShape: Qt.PointingHandCursor
            onEntered: parent.isHover = true
            onExited: parent.isHover = false
            onPressed: parent.color = pressedColor
            onReleased: parent.color = normalColor
        }
        Rectangle{
            anchors.centerIn: parent
            width: imageId.width + textId.width + gap
            height: parent.height
            color: "transparent"
            property int gap: 5
            RowLayout {
                anchors.fill: parent
                spacing: parent.gap
                Image {
                    id: imageId
                    source: imgSource
                    visible: !!imgSource
                    width: imgSourceSize[0]
                    height: imgSourceSize[1]
                    Layout.alignment:  Qt.AlignVCenter | !textId.visible && Qt.AlignHCenter
                }
                Text {
                    id: textId
                    visible: !!mtext
                    text: mtext
                    color: textColor
                    font.pixelSize: fontSize
                    Layout.alignment:  Qt.AlignVCenter | !textId.visible && Qt.AlignHCenter
                }

            }
        }

    }
}
