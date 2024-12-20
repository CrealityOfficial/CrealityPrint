import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQml 2.3
import "../CusBackground"
import "../CusText"
import "../CusImage"
import "../"

Button{
    property color normalColor: Constants.btnNormalColor
    property color hoveredColor: Constants.btnHoverColor
    property color pressedColor: Constants.btnCheckedColor

    property color normalBdColor: "transparent"
    property color hoveredBdColor: "transparent"
    property color pressedBdColor: "transparent"

    property alias btnBg: btnBg
    property alias bText: txt
    property alias bImg: img
    id: btn
    state: "textOnly"
    states: [
        State {
            name: "imageOnly"
            PropertyChanges {
                target: img
                visible: true
                anchors.centerIn: btnBg
            }

            PropertyChanges {
                target: txt
                visible: false
            }
        },
        State {
            name: "textOnly"
            PropertyChanges {
                target: txt
                visible: true
                anchors.centerIn: btnBg
            }
            PropertyChanges {
                target: img
                visible: false
            }
        },
        State {
            name: "imgLeft"
            AnchorChanges {
                target: img
                anchors.left: btnBg.left
                anchors.verticalCenter: btnBg.verticalCenter
            }
            PropertyChanges {
                target: img
                anchors.leftMargin: 10
            }
            AnchorChanges {
                target: txt
                anchors.left: img.right
                anchors.verticalCenter: btnBg.verticalCenter
            }
            PropertyChanges {
                target: txt
                anchors.leftMargin: 10
            }
        },
        State {
            name: "imgTop"
            AnchorChanges {
                target: img
                anchors.top: btnBg.top
                anchors.horizontalCenter: btnBg.horizontalCenter
            }
            PropertyChanges {
                target: img
                anchors.topMargin: 2
            }
            AnchorChanges {
                target: txt
                anchors.top: img.bottom
                anchors.horizontalCenter: btnBg.horizontalCenter
            }
            PropertyChanges {
                target: txt
                anchors.topMargin: 2
            }
        },
        State {
            name: "imgRight"
            AnchorChanges {
                target: img
                anchors.right: btnBg.right
                anchors.verticalCenter: btnBg.verticalCenter
            }
            PropertyChanges {
                target: img
                anchors.rightMargin: 10
            }
            AnchorChanges {
                target: txt
                anchors.right: img.left
                anchors.verticalCenter: btnBg.verticalCenter
            }
            PropertyChanges {
                target: txt
                anchors.rightMargin: 10
            }
        },
        State {
            name: "imgBottom"
            AnchorChanges {
                target: img
                anchors.bottom: btnBg.bottom
                anchors.horizontalCenter: btnBg.horizontalCenter
            }
            PropertyChanges {
                target: img
                anchors.bottomMargin: 2
            }
            AnchorChanges {
                target: txt
                anchors.bottom: img.top
                anchors.horizontalCenter: btnBg.horizontalCenter
            }
            PropertyChanges {
                target: txt
                anchors.bottomMargin: 2
            }
        }
    ]

    background: CusRoundedBg {
        id: btnBg
        property bool showToolTip: false
        signal clicked()

        allRadius: false
        color: (btn.checked && checkable) ? pressedColor : (btn.hovered ? hoveredColor : normalColor)
        opacity : enabled ? 1 : 0.3
        borderColor: btn.hovered ? (btn.checked ? pressedBdColor : hoveredBdColor) : normalBdColor

        CusImage{
            id:img
        }

        CusText {
            id: txt
            fontColor: Constants.textColor
            font.weight: Constants.labelFontWeight
            font.family: Constants.labelFontFamily
            font.pointSize : Constants.labelFontPointSize_9
        }
    }
}

