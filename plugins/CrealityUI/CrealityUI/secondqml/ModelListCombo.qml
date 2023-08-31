import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import QtQml 2.13

import ".."
import "../qml"

Item {
    id: mlRec
    width: 58 * screenScaleFactor
    height: 58 * screenScaleFactor
    property real sizeX
    property real sizeY
    property real sizeZ
    property var tempSelectNum : kernel_model_selector.selectedNum

    onTempSelectNumChanged:
    {
        var size = kernel_model_selector.selectionmsSize()
        idSizeText.text = Qt.binding(function () {
            return (kernel_model_selector.selectedNum > 0) ? (qsTr("Size") + `: ${size.x.toFixed(1)} x ${size.y.toFixed(1)} x ${size.z.toFixed(1)} mm`) : ""
        })
    }

    onSizeXChanged: {

    }

    CusImglButton {
        visible: true
        id: mouseButton
        width: mlRec.width
        height: mlRec.height
        anchors.centerIn: mlRec

        borderWidth: Constants.currentTheme ? 1 : (hovered ? 0 : 1)
        borderColor: Constants.currentTheme ? "#D6D6DC" : (hovered ? "transparent" : "#56565C")
        enabledIconSource: Constants.currentTheme ? "qrc:/UI/photo/modelListIcon_lite.svg" :  "qrc:/UI/photo/modelLIstIcon.svg"
        highLightIconSource: Constants.currentTheme ?  "qrc:/UI/photo/modelListIcon_lite.svg" : "qrc:/UI/photo/modelLIstIcon_h.svg"
        pressedIconSource: "qrc:/UI/photo/modelLIstIcon.svg"
        defaultBtnBgColor: Constants.currentTheme ? "#F2F2F5" : "#363638"
        hoveredBtnBgColor: Constants.currentTheme ? "#DBF2FC" : "#6D6D71"
        selectedBtnBgColor: "transparent"
        onClicked: mouseOptPopup.open()
        cusTooltip.text : qsTr("Model List")
        state: "imgOnly"
    }

    Rectangle {
        radius: height / 2
        width: 20 * screenScaleFactor
        height: 20 * screenScaleFactor
        anchors.rightMargin: -10 * screenScaleFactor
        anchors.right: parent.right
        anchors.top: parent.top

        visible: !mouseOptPopup.visible
        color: Constants.currentTheme ? "#1E9BE2" : "#FFFFFF"

        Text {
            anchors.centerIn: parent
            verticalAlignment: Text.AlignVCenter
            font.weight: Font.Bold
            font.family: Constants.mySystemFont.name
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.currentTheme ? "#F2F2F5" : "#363638"
            text: kernel_modelspace.modelNNum
        }
    }

    Popup {
        id: mouseOptPopup
        padding: 0
        margins: 0
        background: null
        x: mouseButton.x
        y: mouseButton.y - mouseOptPopup.height + mouseButton.height

        width: 280 * screenScaleFactor
        height: 180 * screenScaleFactor
        // closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        closePolicy: Popup.NoAutoClose

        Connections {
            target: kernel_parameter_manager
            onFunctionTypeChanged: {
                mouseOptPopup.close()
            }
        }

        Connections {
            target: kernel_kernel
            onCurrentPhaseChanged: {
                mouseOptPopup.close()
            }
        }

        contentItem: Rectangle {
            radius: 5
            visible: true
            id: mouseOptRect

            border.width: 1
            border.color: Constants.currentTheme ? "#D6D6DC" : "#56565C"
            color: Constants.currentTheme ? "#F2F2F5" : "#363638"

            ColumnLayout {
                spacing: 0
                anchors.fill: parent

                CusPopViewTitle {
                    Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                    Layout.preferredHeight: 24 * screenScaleFactor
                    Layout.alignment: Qt.AlignHCenter

                    radius: 5
                    leftTop: true
                    rightTop: true
                    maxBtnVis: false
                    clickedable: false
                    //closeBtnVis: false
                    title: qsTr("Model List")
                    color: mouseOptRect.color
                    fontColor: Constants.themeTextColor
                    borderColor: mouseOptRect.border.color
                    closeBtnNColor: "transparent"//color
                    closeBtnWidth: 8 * screenScaleFactor
                    closeBtnHeight: 8 * screenScaleFactor
                    titleLeftMargin: 6 *screenScaleFactor
                    closeIcon: "qrc:/UI/photo/closeBtn.svg"
                    closeIcon_d: "qrc:/UI/photo/closeBtn_d.svg"

                    onCloseClicked: mouseOptPopup.close()
                }

                ColumnLayout {
                    spacing: 10 * screenScaleFactor
                    Layout.leftMargin: 11 * screenScaleFactor
                    Layout.rightMargin: 11 * screenScaleFactor

                    RowLayout {
                        width: parent.width
                        height: 35 * screenScaleFactor
                        spacing: 6 * screenScaleFactor

                        RowLayout {
                            spacing: 4 * screenScaleFactor

                            CusButton {
                                radius: 3
                                borderWidth: 1
                                normalBdColor: "#6C6C70"
                                isChecked: kernel_model_list_data.isCheckedAll
                                normalColor: "transparent"
                                hoveredColor: "transparent"
                                hoveredBdColor: "#1E9BE2"
                                pressedColor: "#009CFF"

                                Layout.preferredWidth: 14 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                Image {
                                    anchors.centerIn: parent
                                    width: 10* screenScaleFactor
                                    height: 10* screenScaleFactor
                                    source: "qrc:/UI/images/check.png"//Constants.checkBtnImg
                                    visible: parent.isChecked
                                }

                                onClicked:
                                {
                                    kernel_model_list_data.checkAll(!kernel_model_list_data.isCheckedAll)
                                }
                            }

                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                verticalAlignment: Text.AlignVCenter
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: Constants.themeTextColor
                                text: qsTr("All")
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        CusButton {
                            id: addModel
                            radius: 5
                            borderWidth: 0
                            Layout.preferredWidth: 24 * screenScaleFactor
                            Layout.preferredHeight: 24 * screenScaleFactor

                            toolTipText: qsTr("Add Model")
                            normalColor: "transparent"
                            hoveredColor: Constants.currentTheme ? "#ECECED" : "#3e3e41"
                            pressedColor: "transparent"

                            Image {
                                anchors.centerIn: parent
                                width: sourceSize.width * screenScaleFactor
                                height: sourceSize.height * screenScaleFactor
                                source: addModel.isHovered ? "qrc:/UI/photo/rightDrawer/profile_addBtn_d.svg" : "qrc:/UI/photo/rightDrawer/profile_addBtn.svg"
                            }

                            onClicked: kernel_kernel.openFile()
                        }

                        CusButton {
                            id: deleteBtn
                            radius: 5
                            borderWidth: 0
                            Layout.preferredWidth: 24 * screenScaleFactor
                            Layout.preferredHeight: 24 * screenScaleFactor

                            toolTipText: qsTr("Delete Model")
                            enabled: kernel_model_selector.selectedNum > 0
                            normalColor: "transparent"
                            hoveredColor: Constants.currentTheme ? "#ECECED" : "#3e3e41"
                            pressedColor: "transparent"

                            Image {
                                anchors.centerIn: parent
                                width: sourceSize.width * screenScaleFactor
                                height: sourceSize.height * screenScaleFactor
                                source: deleteBtn.isHovered ? "qrc:/UI/photo/rightDrawer/profile_delBtn_d.svg" : "qrc:/UI/photo/rightDrawer/profile_delBtn.svg"
                            }

                            onClicked: kernel_model_list_data.deleteSelections()
                        }
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 258 * screenScaleFactor
                        Layout.preferredHeight: 85 * screenScaleFactor

                        radius: 5
                        border.width: 1
                        border.color: mouseOptRect.border.color
                        color: mouseOptRect.color

                        CusListView {
                            clip: true
                            id: modelListView
                            width: parent.width
                            height: parent.height - 2
                            anchors.centerIn: parent
                            property int mulBegin: 0
                            model: kernel_model_list_data
                        }
                    }

                    Text {
                        id: idSizeText
                        verticalAlignment: Text.AlignVCenter
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color: Constants.themeTextColor
                    }
                }
            }
        }
    }
}
