import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Rectangle {
    id: idRoot
    width: (height < recommendedHeight ? 92 : 80) * screenScaleFactor
    color: "transparent"
    property int recommendedHeight: kernel_printer_manager.printersCount * 80 * screenScaleFactor + idSliceAllBtn.height + 10
    
    CusImglButton {
        id: idSliceAllBtn
        anchors.top: parent.top
        anchors.left: parent.left
        width: 80 * screenScaleFactor
        height: 36 * screenScaleFactor

        cusTooltip.text: qsTr("All Slice")
        cusTooltip.position: BasicTooltip.Position.RIGHT

        onClicked: {
            kernel_slice_flow.sliceAll()
        }

        background: Rectangle {
            anchors.fill: parent
            property var normalBorder: Constants.currentTheme == 0 ? "#ACACB4" : "#D6D6DC";
            border.width: 2
            border.color: idSliceAllBtn.pressed || idSliceAllBtn.hovered ? Constants.themeGreenColor : normalBorder
            radius: 18
            color: Constants.currentTheme == 0 ? "transparent" : "white"

            Image {
                anchors.centerIn: parent
                source: Constants.currentTheme == 0 ? "qrc:/UI/photo/mult_plate/slice_all_n.png" : "qrc:/UI/photo/mult_plate/slice_all_n_light.png" 
            } 
        }
    }

    Rectangle {
        anchors.top: idSliceAllBtn.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        width: parent.width
        radius: 5
        color: Constants.currentTheme == 0 ? "#2B2B2D" : "#D6D6DC"

        BasicScrollView {
            clip: true
            anchors.fill: parent
            hPolicy: Qt.ScrollBarAlwaysOff 
            vPolicy: Qt.ScrollBarAsNeeded

            Connections {
                target: kernel_slice_flow 

                onSliceIndexChanged: {
                    var button = r.itemAt(kernel_slice_flow.sliceIndex)
                    if (!button.checked)
                        button.checked = true
                }
            }


            Column {
                anchors.left: parent.left
                Repeater {
                    id: r
                    
                    model: kernel_slice_flow.slicePreviewListModel

                    delegate: CusImglButton {
                        id: btn
                        checkable: true
                        width: 80 * screenScaleFactor
                        height: 80 * screenScaleFactor
                        autoExclusive: true
                        checked: kernel_slice_flow.sliceIndex === index

                        cusTooltip.position: BasicTooltip.Position.RIGHT
                        cusTooltip.text: model.isSliced || standaloneWindow.isErrorExist ? qsTr("") : qsTr("Click Slice")

                        onClicked: {
                            if (!model.isSliced && !standaloneWindow.isErrorExist)
                                kernel_slice_flow.slice(index)
                        }

                        onCheckedChanged: {
                            if (checked) {
                                kernel_slice_flow.sliceIndex = index
                            }
                        }

                        background: Rectangle {
                            anchors.fill: parent
                            clip: true

                            property var backgroundColor1: Constants.currentTheme == 0 ? "#505052" : Qt.rgba(1, 1, 1, 1)
                            property var backgroundColor2: Constants.currentTheme == 0 ? Qt.rgba(0.22, 0.22, 0.22, 1) : Qt.rgba(0.9, 0.9, 0.9, 1)
                            property var normalBorder: Constants.currentTheme == 0 ? "#2B2B2D" : "#D6D6DC";

                            border.width: btn.pressed || btn.hovered || btn.checked ? 2 : 1
                            border.color: btn.pressed || btn.checked ? Constants.themeGreenColor : (btn.hovered ? "#ACACB4" : normalBorder)
                            color: model.isSliced ? backgroundColor1 : backgroundColor2
                            radius: 5

                            Image {
                                cache: false
                                anchors.fill: parent
                                anchors.margins: 12
                                source: model.imageSource
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }

                            Text {
                                anchors.fill: parent
                                anchors.margins: 5
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignTop
                                text: index + 1
                                color: Constants.currentTheme == 0 ? Qt.rgba(1, 1, 1, 1) : Qt.rgba(0, 0, 0, 1)
                            }

                            // Canvas {
                            //     x: 2
                            //     y: basicY + vaildHeight - height
                            //     width: parent.width - 2 * offset
                            //     height: vaildHeight * value
                            //     // z: -1

                            //     property var offset: parent.border.width
                            //     property var vaildHeight: parent.height - 2 * offset
                            //     property var basicY: offset
                            //     property var value: 0.3

                            //     onPaint: {
                            //         var ctx = getContext("2d");
                            //         ctx.lineWidth = 0
                            //         ctx.strokeStyle = "transparent"
                            //         ctx.fillStyle = Qt.rgba(0, 1, 0, 0.1)

                            //         let radius = 3
                            //         ctx.beginPath();
                            //         ctx.moveTo(width, 0)
                            //         ctx.arcTo(width, 0, width, height, radius)
                            //         ctx.arcTo(width, height, 0, height, radius)
                            //         ctx.arcTo(0, height, 0, 0, radius)
                            //         ctx.lineTo(0, 0)
                            //         ctx.lineTo(width, 0)
                            //         ctx.closePath();
                            //         ctx.fill();
                            //         ctx.stroke();
                            //     }
                            // }
                        }
                    }
                }
            }
        }
    }
}
