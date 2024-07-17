import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/cxcloud"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/secondqml/frameless"
import "qrc:/CrealityUI/secondqml/printer"

BasicDialogV4 {

    title: qsTr("Printer")
    maxBtnVis: false
    modal: false

    bdContentItem: ColumnLayout {
        Layout.fillWidth: true
        spacing: 10 * screenScaleFactor

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 24 * screenScaleFactor
            Layout.topMargin: 10 * screenScaleFactor
            Layout.leftMargin: 10 * screenScaleFactor
            Layout.rightMargin:10 * screenScaleFactor
            spacing: 10 * screenScaleFactor

            CustomComboBox {
                id: idPrinterComboBox

                readonly property bool currnetModelModifyed: {
                    const machine = model.get(currentIndex).object
                    if (!machine) {
                        return false
                    }

                    const machine_data = machine.dataModel
                    if (machine_data && machine_data.settingsModifyed) {
                        return true
                    }

                    const extruder_1_data = machine.extruder1DataModel()
                    if (extruder_1_data && extruder_1_data.settingsModifyed) {
                        return true
                    }

                    const extruder_2_data = machine.extruder2DataModel()
                    if (extruder_2_data && extruder_2_data.settingsModifyed) {
                        return true
                    }

                    return false
                }

                Layout.fillWidth: true
                Layout.preferredHeight: 24 * screenScaleFactor

                model: kernel_parameter_manager.machineListModel
                currentIndex: kernel_parameter_manager.curMachineIndex
                textRole: "name"
                displayText: "%1%2".arg(currnetModelModifyed ? "*" : "").arg(kernel_parameter_manager.currentMachineObject.name())

                popup: Popup {
                    y: idPrinterComboBox.height + 5 * screenScaleFactor
                    width: idPrinterComboBox.width
                    height: Math.min(implicitContentHeight + padding * 2, 400 * screenScaleFactor)
                    padding: background.border.width

                    background: Rectangle {
                        radius: idPrinterComboBox.radius
                        border.width: 1 * screenScaleFactor
                        border.color: Constants.right_panel_border_default_color
                        color: Constants.right_panel_item_default_color
                    }

                    contentItem: Loader {
                        active: standaloneWindow.initialized
                        sourceComponent: ListView {
                            id : __listview

                            clip: true
                            snapMode: ListView.SnapToItem
                            implicitHeight: contentHeight
                            model: idPrinterComboBox.model

                            section.property: "default"
                            section.criteria: ViewSection.FullString
                            section.delegate: Item {
                                required property bool section
                                implicitWidth: idPrinterComboBox.implicitDelegateWidth
                                implicitHeight: idPrinterComboBox.implicitDelegateHeight

                                Text {
                                    anchors.fill: parent
                                    topPadding: idPrinterComboBox.textTopPadding
                                    bottomPadding: idPrinterComboBox.textBottomPadding
                                    leftPadding: idPrinterComboBox.textLeftPadding
                                    rightPadding: idPrinterComboBox.textRightPadding
                                    verticalAlignment: idPrinterComboBox.textVerticalAlignment
                                    horizontalAlignment: idPrinterComboBox.textHorizontalAlignment
                                    font: Constants.font
                                    color: Constants.right_panel_text_default_color
                                    text: "------ %1 ------".arg(qsTranslate("CXComboBox",
                                            section ? "Default configuration" : "User configuration"))
                                }
                            }

                            delegate: ItemDelegate {
                                implicitWidth: idPrinterComboBox.implicitDelegateWidth
                                implicitHeight: idPrinterComboBox.implicitDelegateHeight
                                property var preIndex: currentIndex

                                function saveParaDialog(index) {
                                    var receiver = {}
                                    receiver.type = "all"
                                    receiver.onSave = function() {
                                        kernel_parameter_manager.setCurrentMachine(index)
                                    }
                                    receiver.onCloseButtonClicked = function() {
                                        __listview.currentIndex = preIndex
                                    }
                                    receiver.onIgnore = function() {
                                        kernel_parameter_manager.abandonAll();
                                        kernel_parameter_manager.setCurrentMachine(index)
                                    }
                                    idAllMenuDialog.requestMenuDialog(receiver, "saveParemDialog")
                                }

                                onClicked: {
                                    if (kernel_parameter_manager.isAnyModified()) {
                                        saveParaDialog(index)
                                    } else {
                                        kernel_parameter_manager.setCurrentMachine(index)
                                    }
                                    preIndex = index
                                    idPrinterComboBox.popup.close()
                                }

                                background: Rectangle {
                                    radius: 5 * screenScaleFactor
                                    color: hovered ? Constants.right_panel_item_checked_color : "transparent"
                                }

                                contentItem: RowLayout {
                                    anchors.fill: parent

                                    Text {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        topPadding: idPrinterComboBox.textTopPadding
                                        bottomPadding: idPrinterComboBox.textBottomPadding
                                        leftPadding: idPrinterComboBox.textLeftPadding
                                        rightPadding: idPrinterComboBox.textRightPadding
                                        verticalAlignment: idPrinterComboBox.textVerticalAlignment
                                        horizontalAlignment: idPrinterComboBox.textHorizontalAlignment
                                        font: Constants.font
                                        color: Constants.right_panel_text_default_color
                                        text: "%1%2".arg(model.modifyed ? "*" : "").arg(model.name)
                                    }

                                    Button {
                                        Layout.fillHeight: true
                                        Layout.minimumWidth: height
                                        Layout.maximumWidth: Layout.minimumWidth
                                        Layout.margins: 1 * screenScaleFactor

                                        visible: !model.default && parent.parent.hovered

                                        onClicked: {
                                            kernel_parameter_manager.removeMachine(kernel_parameter_manager.machineObject(index))
                                        }

                                        background: Rectangle {
                                            radius: height / 2
                                            color: parent.hovered ? "#FFFFFF" : "transparent"
                                        }

                                        contentItem: Item {
                                            Image {
                                                anchors.centerIn: parent
                                                width: 14 * screenScaleFactor
                                                height: 14 * screenScaleFactor
                                                source: parent.parent.hovered ? Constants.right_panel_delete_hovered_image
                                                                            : Constants.right_panel_delete_image
                                            }
                                        }
                                    }
                                }
                            }

                            footerPositioning: ListView.OverlayFooter
                            footer: Item {
                                implicitWidth: idPrinterComboBox.implicitDelegateWidth
                                implicitHeight: idPrinterComboBox.implicitDelegateHeight + 4 * screenScaleFactor

                                CusButton {
                                    anchors.fill: parent
                                    anchors.margins: 2 * screenScaleFactor
                                    radius: 5 * screenScaleFactor
                                    txtContent: qsTr("Add")
                                    onClicked: {
                                        kernel_ui.requestQmlDialog("idAddPrinterDlgNew")
                                        idPrinterComboBox.popup.close()
                                    }
                                }
                            }
                        }
                    }
                }
            }

            CusImglButton {
                Layout.preferredWidth: 28 * screenScaleFactor
                Layout.preferredHeight: 24 * screenScaleFactor
                defaultBtnBgColor: Constants.right_panel_button_default_color
                hoveredBtnBgColor: defaultBtnBgColor
                selectedBtnBgColor:defaultBtnBgColor
                hoverBorderColor: Constants.themeGreenColor
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                enabledIconSource: Constants.currentTheme? "qrc:/UI/photo/rightDrawer/update/edit_light_default.svg" :"qrc:/UI/photo/rightDrawer/profile_editBtn.svg"
                highLightIconSource: "qrc:/UI/photo/rightDrawer/profile_editBtn.svg"
                pressedIconSource: "qrc:/UI/photo/rightDrawer/profile_editBtn.svg"
                cusTooltip.text: qsTranslate("PrintParameterPanel", "Edit")
                btnRadius: 5
                borderWidth: 1
                onClicked: {
                    idPrinterComboBox.popup.visible = false;
                    if (kernel_parameter_manager.machinesCount > 0) {
                        if (!machineManagerLoader.active)
                            machineManagerLoader.active = true;

                        machineManagerLoader.item.show();
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 24 * screenScaleFactor
            Layout.bottomMargin: 10 * screenScaleFactor
            Layout.leftMargin: 10 * screenScaleFactor
            Layout.rightMargin:10 * screenScaleFactor
            spacing:10 * screenScaleFactor

            StyledLabel {
                text: qsTr("Bed Type")+":"
                color: Constants.right_panel_text_default_color
                font.pointSize: Constants.labelFontPointSize_9
                wrapMode: Text.NoWrap
                font.family: Constants.labelFontFamily
            }

            CustomComboBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 24 * screenScaleFactor
                model: kernel_parameter_manager.currrentBedTypes()
                translater: source => qsTranslate("ParameterComponent", source)
                currentIndex: kernel_parameter_manager.currBedTypeIndex
                onActivated: {
                    kernel_parameter_manager.setCurrBedType(kernel_parameter_manager.currrentBedKeys()[currentIndex])
                }
            }
        }
    }
}
