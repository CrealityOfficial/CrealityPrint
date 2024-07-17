import QtQuick 2.15
import QtQuick.Controls 2.15

import "../qml"

ToolTip {
    enum Position {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT
    }

    property var position: BasicTooltip.Position.TOP

    property int rectangleRadius: 5 * screenScaleFactor

    property int trangleWidth: 12 * screenScaleFactor
    property int trangleHeight: 6 * screenScaleFactor
    property int trangleMargins: 4 * screenScaleFactor

    property int textMargins: 6 * screenScaleFactor
    property bool textWrap: false
    property int textWidth: 0 // 0 means no limit
    property color textColor: "#FEFEFE"
    property int textFormat: Text.AutoText
    property alias contentText: contentText

    property color backgroundColor: Constants.currentTheme ? "#67686C" : "#01050F"

    id: root

    width: (textWidth ? textWidth : text.contentWidth) + trangleHeight * 2 + textMargins * 2
    height: contentHeight + trangleHeight * 2 + textMargins * 2

    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    font.pointSize: Constants.labelFontPointSize_9

    delay: 100
    timeout: 2000
    padding: 10

    closePolicy: Popup.CloseOnEscape |
                 Popup.CloseOnPressOutsideParent |
                 Popup.CloseOnReleaseOutsideParent

    onPositionChanged: {
        switch (position) {
        case BasicTooltip.Position.TOP:
            x = Qt.binding(function() { return (parent.width - width) / 2 })
            y = Qt.binding(function() { return -height - trangleMargins })
            trangle.width = Qt.binding(function() { return root.trangleWidth })
            trangle.height = Qt.binding(function() { return root.trangleHeight })
            trangle.anchors.bottom = Qt.binding(function() { return background.bottom })
            trangle.anchors.horizontalCenter = Qt.binding(function() { return background.horizontalCenter })
            break
        case BasicTooltip.Position.BOTTOM:
            x = Qt.binding(function() { return (parent.width - width) / 2 })
            y = Qt.binding(function() { return parent.height + trangleMargins })
            trangle.width = Qt.binding(function() { return root.trangleWidth })
            trangle.height = Qt.binding(function() { return root.trangleHeight })
            trangle.anchors.top = Qt.binding(function() { return background.top })
            trangle.anchors.horizontalCenter = Qt.binding(function() { return background.horizontalCenter })
            break
        case BasicTooltip.Position.LEFT:
            x = Qt.binding(function() { return -width - trangleMargins })
            y = Qt.binding(function() { return (parent.height - height) / 2 })
            trangle.width = Qt.binding(function() { return root.trangleHeight })
            trangle.height = Qt.binding(function() { return root.trangleWidth })
            trangle.anchors.right = Qt.binding(function() { return background.right })
            trangle.anchors.verticalCenter = Qt.binding(function() { return background.verticalCenter })
            break
        case BasicTooltip.Position.RIGHT:
        default:
            x = Qt.binding(function() { return parent.width + trangleMargins })
            y = Qt.binding(function() { return (parent.height - height) / 2 })
            trangle.width = Qt.binding(function() { return root.trangleHeight })
            trangle.height = Qt.binding(function() { return root.trangleWidth })
            trangle.anchors.left = Qt.binding(function() { return background.left })
            trangle.anchors.verticalCenter = Qt.binding(function() { return background.verticalCenter })
            break
        }
    }

    Component.onCompleted: {
        root.positionChanged()
    }

    contentItem: Text {
        id: contentText
        leftPadding: root.textWrap ? root.textMargins : 0

        text: root.visible ? root.text : ""
        font: root.font
        color: root.textColor
        width: root.textWidth
        wrapMode: root.textWrap ? Text.WordWrap : Text.NoWrap
        textFormat: root.textFormat

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
    }

    background: Item {
        id: background

        Rectangle {
            id: rectangle
            anchors.fill: parent
            anchors.margins: root.trangleHeight
            radius: root.rectangleRadius
            color: root.backgroundColor
        }

        Canvas {
            id: trangle

            onPaint: {
                var context = getContext("2d")
                context.clearRect(0, 0, width, height)

                context.beginPath()
                switch (root.position) {
                case BasicTooltip.Position.TOP:
                    context.moveTo(width / 2, height)
                    context.lineTo(0, 0)
                    context.lineTo(width, 0)
                    break
                case BasicTooltip.Position.BOTTOM:
                    context.moveTo(width / 2, 0)
                    context.lineTo(0, height)
                    context.lineTo(width, height)
                    break
                case BasicTooltip.Position.LEFT:
                    context.moveTo(width, height / 2)
                    context.lineTo(0, 0)
                    context.lineTo(0, height)
                    break
                case BasicTooltip.Position.RIGHT:
                    context.moveTo(0, height / 2)
                    context.lineTo(width, 0)
                    context.lineTo(width, height)
                    break
                default:
                    break
                }
                context.closePath()

                context.fillStyle = root.backgroundColor
                context.fill()
            }
        }
    }
}
