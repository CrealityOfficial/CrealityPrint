import CrealityUI 1.0
import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.13
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

Rectangle {
    id: idRoot

    //width: 92 * screenScaleFactor
    color: "transparent"

    CusImglButton {
        id: idSliceAllBtn

        anchors.top: parent.top
        anchors.left: parent.left
        width: 80 * screenScaleFactor
        height: 36 * screenScaleFactor
        cusTooltip.text: qsTr("All Slice")
        cusTooltip.position: BasicTooltip.Position.RIGHT
        onClicked: {
            kernel_slice_flow.sliceAll();
        }

        background: Rectangle {
            property var normalBorder: Constants.currentTheme == 0 ? "#ACACB4" : "#D6D6DC"

            anchors.fill: parent
            border.width: 2
            border.color: idSliceAllBtn.pressed || idSliceAllBtn.hovered ? "#1E9BE2" : normalBorder
            radius: 18
            color: Constants.currentTheme == 0 ? "transparent" : "white"

            Image {
                anchors.centerIn: parent
                source: Constants.currentTheme == 0 ? "qrc:/UI/photo/mult_plate/slice_all_n.png" : "qrc:/UI/photo/mult_plate/slice_all_n_light.png"
            }

        }

    }

    Rectangle {
        anchors.left: idSliceAllBtn.right
        anchors.leftMargin: 10 * screenScaleFactor
        //Layout.fillWidth: true
        width: parent.width - idSliceAllBtn.width
        height: 60 * screenScaleFactor
        radius: 5
        color: Constants.currentTheme == 0 ? "#2B2B2D" : "#D6D6DC"

        BasicScrollView {
            clip: true
            anchors.fill: parent
            hPolicy: Qt.ScrollBarAsNeeded
            vPolicy: Qt.ScrollBarAlwaysOff

            Connections {
                target: kernel_slice_flow
                onSliceIndexChanged: {
                    console.log("change button state: " + kernel_slice_flow.sliceIndex);
                    var button = r.itemAt(kernel_slice_flow.sliceIndex);
                    if (!button.checked)
                        button.checked = true;

                }
            }

            Column {
                anchors.left: parent.left

                ListView {
                    id: r

                    model: kernel_slice_flow.slicePreviewListModel
                    orientation: ListView.Horizontal
                    layoutDirection: Qt.LeftToRight

                    delegate: CusImglButton {
                        id: btn

                        checkable: true
                        width: 80 * screenScaleFactor
                        height: 80 * screenScaleFactor
                        autoExclusive: true
                        checked: kernel_slice_flow.sliceIndex == index
                        cusTooltip.position: BasicTooltip.Position.RIGHT
                        cusTooltip.text: model.isSliced ? qsTr("") : qsTr("Click Slice")
                        onClicked: {
                            if (!model.isSliced)
                                kernel_slice_flow.slice(index);

                        }
                        onCheckedChanged: {
                            if (checked) {
                                console.log("set slice index: " + index);
                                kernel_slice_flow.sliceIndex = index;
                            } else {
                                console.log("************ cancel select ************ " + index);
                            }
                        }

                        background: Rectangle {
                            property var backgroundColor1: Constants.currentTheme == 0 ? "#505052" : Qt.rgba(1, 1, 1, 1)
                            property var backgroundColor2: Constants.currentTheme == 0 ? Qt.rgba(0.22, 0.22, 0.22, 1) : Qt.rgba(0.9, 0.9, 0.9, 1)
                            property var normalBorder: Constants.currentTheme == 0 ? "#2B2B2D" : "#D6D6DC"

                            anchors.fill: parent
                            border.width: btn.pressed || btn.hovered || btn.checked ? 2 : 1
                            border.color: btn.pressed || btn.checked ? "#1E9BE2" : (btn.hovered ? "#ACACB4" : normalBorder)
                            color: model.isSliced ? backgroundColor1 : backgroundColor2
                            radius: 5

                            Image {
                                anchors.fill: parent
                                anchors.margins: 12
                                source: model.imageSource
                                fillMode: Image.PreserveAspectFit
                            }

                            Text {
                                anchors.fill: parent
                                anchors.margins: 5
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignTop
                                text: index + 1
                                color: Constants.currentTheme == 0 ? Qt.rgba(1, 1, 1, 1) : Qt.rgba(0, 0, 0, 1)
                            }

                        }

                    }

                }

            }

        }

    }

}
