import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Window {
    id: root
    color: "transparent"
    width: 400 * screenScaleFactor
    height: 300 * screenScaleFactor
    modality: Qt.ApplicationModal
    flags: Qt.FramelessWindowHint | Qt.Dialog

    property string title: ""
    property string titleSource: ""
    property string shadowColor: ""
    property string borderColor: ""
    property string titleFtColor: ""
    property string titleBgColor: ""
    property string backgroundColor: ""

    property size titleImageSize: Qt.size(0, 0)

    property bool titleImgVisible: false
    property bool extraBtnVisible: false
    property bool extraBtnEnabled: false

    property real borderWidth: 1 * screenScaleFactor
    property real shadowWidth: 5 * screenScaleFactor
    property real titleHeight: 30 * screenScaleFactor
    property real borderRadius: 5 * screenScaleFactor

    signal sigExtraBtnClicked()
    property Component bdContentItem
    property alias cloader: contentLoader

    Rectangle {
        anchors.fill: parent
        anchors.margins: shadowWidth

        border.width: borderWidth
        border.color: borderColor
        color: backgroundColor
        radius: borderRadius

        CusRoundedBg {
            id: cusTitle
            width: parent.width - 2 * borderWidth
            height: titleHeight

            anchors.top: parent.top
            anchors.topMargin: borderWidth
            anchors.horizontalCenter: parent.horizontalCenter

            leftTop: true
            rightTop: true
            color: titleBgColor
            radius: borderRadius

            MouseArea {
                anchors.fill: parent
                property point clickPos: "0,0"

                onPressed: clickPos = Qt.point(mouse.x, mouse.y)

                onPositionChanged: {
                    var cursorPos = WizardUI.cursorGlobalPos()
                    root.x = cursorPos.x - clickPos.x
                    root.y = cursorPos.y - clickPos.y
                }
            }

            RowLayout {
                anchors.left: parent.left
                anchors.leftMargin: 10 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10 * screenScaleFactor

                Image {
                    source: titleSource
                    visible: titleImgVisible
                    sourceSize: titleImageSize
                    Layout.preferredWidth: sourceSize.width
                    Layout.preferredHeight: sourceSize.height
                }

                Text {
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 14 * screenScaleFactor

                    font.weight: Font.Bold
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Text.AlignVCenter
                    color: titleFtColor
                    text: title
                }
            }

            Rectangle {
                id: extraBtn
                width: 80 * screenScaleFactor
                height: 20 * screenScaleFactor

                visible: extraBtnVisible
                enabled: extraBtnEnabled
                opacity: enabled ? 1.0 : 0.7

                anchors.right: closeBtn.left
                anchors.rightMargin: 10 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter

                border.width: refreshArea.containsMouse ? 0 : 1
                border.color: Constants.currentTheme ? "#DDDDE1" : "#A2A2A9"
                color: refreshArea.containsMouse ? "#1E9BE2" : "transparent"
                radius: 10 * screenScaleFactor

                Row {
                    anchors.centerIn: parent
                    spacing: 4 * screenScaleFactor

                    Image {
                        width: 10 * screenScaleFactor
                        height: 10 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        source: (refreshArea.containsMouse || !Constants.currentTheme) ? "qrc:/UI/photo/lanPrinterSource/refresh_dark.svg" : "qrc:/UI/photo/lanPrinterSource/refresh_light.svg"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        verticalAlignment: Text.AlignVCenter
                        color: refreshArea.containsMouse ? "white" : titleFtColor
                        text: qsTr("Refresh")
                    }
                }

                MouseArea {
                    id: refreshArea
                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: sigExtraBtnClicked()
                }
            }

            MouseArea {
                id: closeBtn
                width: titleHeight
                height: titleHeight

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    anchors.centerIn: parent
                    width: 10 * screenScaleFactor
                    height: 10 * screenScaleFactor
                    source: Constants.currentTheme ? "qrc:/UI/photo/lanPrinterSource/closeBtn_light.svg" : "qrc:/UI/photo/lanPrinterSource/closeBtn_dark.svg"
                }

                hoverEnabled: true
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.visible = false
            }
        }

        Loader {
            id: contentLoader
            width: parent.width
            height: parent.height - cusTitle.height
            anchors.top: cusTitle.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            sourceComponent: bdContentItem
        }

        layer.enabled: true

        layer.effect: DropShadow {
            radius: 8
            spread: 0.2
            samples: 17
            color: shadowColor
        }
    }
}
