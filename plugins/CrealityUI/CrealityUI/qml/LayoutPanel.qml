import QtQuick 2.10
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

import "../qml"
import "../secondqml"

LeftPanelDialog {
    width: 300 * screenScaleFactor
    height: 194 * screenScaleFactor
    title: qsTr("Layout")
    property var com: ""
    property int gridRowCount: 2
    property int gridColCount: 4

    ListModel {
        id: buttonModel
        ListElement {
            show_text: qsTr("From Left to Right")
            image_dark: "qrc:/UI/photo/layoutSvg/layout_vertical_dark.svg"
            image_white: "qrc:/UI/photo/layoutSvg/layout_vertical_light.svg"
            layout_type: 6//LEFT_TO_RIGHT
            layout_state: "left2right"
        }
        ListElement {
            show_text: qsTr("From Right to Left")
            image_dark: "qrc:/UI/photo/layoutSvg/layout_vertical_dark.svg"
            image_white: "qrc:/UI/photo/layoutSvg/layout_vertical_light.svg"
            layout_type: 7//RIGHT_TO_LEFT
            layout_state: "right2left"
        }
        ListElement {
            show_text: qsTr("From Top to Bottom")
            image_dark: "qrc:/UI/photo/layoutSvg/layout_horizontal_dark.svg"
            image_white: "qrc:/UI/photo/layoutSvg/layout_horizontal_light.svg"
            layout_type: 4//TOP_TO_BOTTOM
            layout_state: "top2bottom"
        }
        ListElement {
            show_text: qsTr("From Bottom to Top")
            image_dark: "qrc:/UI/photo/layoutSvg/layout_horizontal_dark.svg"
            image_white: "qrc:/UI/photo/layoutSvg/layout_horizontal_light.svg"
            layout_type: 5//BOTTOM_TO_TOP
            layout_state: "bottom2top"
        }
        ListElement {
            show_text: qsTr("Centered")
            image_dark: "qrc:/UI/photo/layoutSvg/layout_center_dark.svg"
            image_white: "qrc:/UI/photo/layoutSvg/layout_center_light.svg"
            layout_type: 0//CENTER_TO_SIDE
            layout_state: "center"
        }
        ListElement {
            show_text: qsTr("Intelligent")
            image_dark: "qrc:/UI/photo/layoutSvg/layout_auto_dark.svg"
            image_white: "qrc:/UI/photo/layoutSvg/layout_auto_light.svg"
            layout_type: 8//CONCAVE_ALL
            layout_state: "auto"
        }
    }

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }

    Item {
        anchors.fill: parent.panelArea

        Grid {
            rows: gridRowCount
            columns: gridColCount
            spacing: 10 * screenScaleFactor

            anchors.top: parent.top
            anchors.topMargin: 20 * screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter

            Repeater {
                model: buttonModel
                delegate: RadioButton {
                    id: sourceBtn
                    indicator: null
                    width: 53 * screenScaleFactor
                    height: 53 * screenScaleFactor
                    property color normalBorder: Constants.currentTheme ? "#DDDDE1" : "#6E6E72"
                    property color selectBorder: Constants.currentTheme ? "#009CFF" : "#00A0E9"

                    onClicked: com.layoutByType(index)

                    background: Rectangle {
                        color: "transparent"
                        border.color: (sourceBtn.hovered || sourceBtn.checked)  ? sourceBtn.selectBorder : sourceBtn.normalBorder
                        border.width: (sourceBtn.hovered || sourceBtn.checked) ? 2 : 1
                        radius: 5
                    }

                    Image {
                        id: sourceImage
                        width: sourceSize.width * screenScaleFactor
                        height: sourceSize.height * screenScaleFactor
                        source: Constants.currentTheme ? image_white : image_dark
                    }

                    ToolTip {
                        x: 0
                        y: (index > gridColCount - 1) ? sourceBtn.height + offset : -(implicitHeight + offset)

                        margins: 6 * screenScaleFactor
                        padding: 6 * screenScaleFactor
                        property real offset: 6 * screenScaleFactor

                        contentItem: Text {
                            font.weight: Font.Bold
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            verticalAlignment: Text.AlignVCenter
                            color: Constants.currentTheme ? "#333333" : "#A9A9B1"
                            text: show_text
                        }

                        background: Rectangle {
                            color: Constants.currentTheme ? "white" : "black"
                            border.color: Constants.currentTheme ? "#95959C" : "transparent"
                            border.width: 1
                            radius: 5
                        }

                        visible: sourceBtn.hovered
                    }

                    state: layout_state

                    states: [
                        State {
                            name: "left2right"
                            AnchorChanges {
                                target: sourceImage
                                anchors.top: sourceBtn.top
                                anchors.left: sourceBtn.left
                            }
                            PropertyChanges {
                                target: sourceImage
                                anchors.topMargin: 4 * screenScaleFactor
                                anchors.leftMargin: 4 * screenScaleFactor
                            }
                        },
                        State {
                            name: "right2left"
                            AnchorChanges {
                                target: sourceImage
                                anchors.top: sourceBtn.top
                                anchors.right: sourceBtn.right
                            }
                            PropertyChanges {
                                target: sourceImage
                                anchors.topMargin: 4 * screenScaleFactor
                                anchors.rightMargin: 4 * screenScaleFactor
                            }
                        },
                        State {
                            name: "top2bottom"
                            AnchorChanges {
                                target: sourceImage
                                anchors.top: sourceBtn.top
                                anchors.left: sourceBtn.left
                            }
                            PropertyChanges {
                                target: sourceImage
                                anchors.topMargin: 4 * screenScaleFactor
                                anchors.leftMargin: 4 * screenScaleFactor
                            }
                        },
                        State {
                            name: "bottom2top"
                            AnchorChanges {
                                target: sourceImage
                                anchors.left: sourceBtn.left
                                anchors.bottom: sourceBtn.bottom
                            }
                            PropertyChanges {
                                target: sourceImage
                                anchors.leftMargin: 4 * screenScaleFactor
                                anchors.bottomMargin: 4 * screenScaleFactor
                            }
                        },
                        State {
                            name: "center"
                            AnchorChanges {
                                target: sourceImage
                                anchors.verticalCenter: sourceBtn.verticalCenter
                                anchors.horizontalCenter: sourceBtn.horizontalCenter
                            }
                        },
                        State {
                            name: "auto"
                            AnchorChanges {
                                target: sourceImage
                                anchors.verticalCenter: sourceBtn.verticalCenter
                                anchors.horizontalCenter: sourceBtn.horizontalCenter
                            }
                        }
                    ]
                }
            }
        }
    }
}
