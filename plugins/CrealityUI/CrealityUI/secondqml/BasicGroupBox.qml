import QtQuick 2.0
import QtQuick.Controls 2.12
import "../qml"
GroupBox {
    id: control
    property color defaultBgColor: Constants.themeColor //"#061F3B"
    property color textColor: Constants.textColor
    property color textBgColor: Constants.themeColor
    property color borderColor: "#585C5E"
    property var borderWidth: 0//0.5
    property real backRadius: 0
    property alias textY: idText.y
    property bool isExpand: true
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitLabelWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)
    spacing: 0
    padding: 10
    font.family: Constants.labelFontFamily
    font.bold: true
    font.pointSize: Constants.labelFontPointSize_12
    topPadding: padding + (implicitLabelWidth > 0 ? implicitLabelHeight + spacing : 0)
    state:"canNotExpend"
    states: [
        State {
            name: "canExpend"
            PropertyChanges {
                target: expendImg
                visible: true
            }
            PropertyChanges {
                target: expendArea
                enabled: true
            }
        },
        State {
            name: "canNotExpend"
            PropertyChanges {
                target: expendImg
                visible: false
            }
            PropertyChanges {
                target: expendArea
                enabled: false
            }
        }
    ]

    label: Rectangle{
        y : height/2 * (-1)
        x: control.leftPadding
        color: textBgColor
        implicitWidth: idText.contentWidth + 25* screenScaleFactor
        implicitHeight: idText.contentHeight
        Text {
            id : idText
            anchors.centerIn: parent
            text: control.title
            font: control.font
            color: textColor
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Image {
            id: expendImg
            width: sourceSize.width
            height: sourceSize.height
            anchors.left: idText.right
            anchors.leftMargin: 2 * screenScaleFactor
            anchors.verticalCenter: idText.verticalCenter
            source: isExpand ? "qrc:/UI/photo/comboboxDown2.png" : "qrc:/UI/photo/comboboxDown2_flip.png"
            fillMode: Image.Pad
        }

        MouseArea{
            id: expendArea
            anchors.fill: parent
            onClicked: {
                isExpand = !isExpand
                contentItem.visible = isExpand
                console.log("++++++++++ = ", isExpand)
            }
        }
    }

    background: Rectangle {
        y: control.topPadding - control.bottomPadding - idText.height
        radius: backRadius
        width: parent.width
        height: parent.height - control.topPadding + control.bottomPadding + 10
        color: defaultBgColor
        border.color: borderColor
        border.width: borderWidth
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
