import QtQuick 2.10
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.12

import "../../qml"
import "../../secondqml"

Rectangle {
    id: root
    height: actualHeight(idTitle) + actualHeight(idMessage) + 30 * screenScaleFactor
    property var messageObj
    z: 100

    property var typeStyle: {
        if (messageObj.level() == 2) {
            return {
                color: "#2A2A2A",
                type: "Error",
                titleColor: "#FF0000",
                icon: "qrc:/UI/photo/message/error_message.svg"
            }
        } else if (messageObj.level() == 1) {
            return {
                color: "#2A2A2A",
                type: "Warn",
                titleColor: "#FCE100",
                icon: "qrc:/UI/photo/message/warning_message.svg"
            }
        } else {
            return {
                color: "#2A2A2A",
                type: "Tip",
                titleColor: "#17CC5F",
                icon: "qrc:/UI/photo/message/normal_message.svg"
            }
        }
    }

    // color: typeStyle.color
    color: Constants.currentTheme ? Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(0, 0, 0, 0.3)
    // border.color: "gray"
    border.color: "transparent"
    radius: 5

    function actualHeight(item) {
        return item.visible * item.height
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 30
        anchors.topMargin: 10
        spacing: 10

        Item {
            id: idTitle
            height: 20 * screenScaleFactor
            width: parent.width

            Image {
                id: idIcon
                source: root.typeStyle.icon
                width: 20 * screenScaleFactor
                height: parent.height
            }

            Text {
                id: idType
                anchors.left: idIcon.right
                anchors.leftMargin: 4
                height: parent.height
                width: parent.width
                font.family: Constants.panelFontFamily
                font.weight: Constants.labelFontWeight
                verticalAlignment: Qt.AlignVCenter
                text: qsTr(root.typeStyle.type) + ":"
                color: root.typeStyle.titleColor
            }

        }

        Text {
            id: idMessage
            // height: 18
            height: contentHeight
            width: parent.width - 20
            wrapMode: Text.Wrap
            color: Constants.textColor
            // font.pointSize: Constants.labelFontPointSize_normal
            font.family: Constants.panelFontFamily
            // font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            text: messageObj.message() + (messageObj.linkString() ? "&nbsp;&nbsp;<a href='https://www.example.com'><font color='#17CC5F'>" + messageObj.linkString() + "</font></a>" : "")
            // onLinkActivated: {
            //     console.log("execute handle")
            //     messageObj.handle()
            // }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    console.log("execute handle")
                    messageObj.handle()
                }
            }

        }

    }


    Button {
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 30
        height: 30

        background: Rectangle {
            anchors.fill: parent
            radius: 15
            color: parent.hovered || parent.pressed ? (Constants.currentTheme ? Qt.rgba(1.0, 1.0, 1.0, 0.9) : Qt.rgba(1.0, 1.0, 1.0, 0.1)): "transparent"

            Image {
                anchors.centerIn: parent
                width: 10
                height: 10
                source: (Constants.currentTheme ? "qrc:/UI/photo/message/cancel_light.svg" : "qrc:/UI/photo/message/cancel.svg")
            }
        } 
        

        onClicked: {
            root.visible = false
            root.parent.layout()
            // root.parent.destroyMessage(messageObj.code())
        }
    }

}
