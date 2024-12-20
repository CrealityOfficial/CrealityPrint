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
                id: printer_combo_box

                readonly property bool currnetModelModifyed: {
                    const machine = model.get(kernel_parameter_manager.curMachineIndex).object
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
                displayText: "%1%2".arg(currnetModelModifyed ? "*" : "")
                                   .arg(kernel_parameter_manager.currentMachineObject.name())

                popup.height: Math.min(
                        popup.implicitContentHeight + popup.padding * 2, 400 * screenScaleFactor)

                onActivated: function() {
                    if (!kernel_parameter_manager.isAnyModified()) {
                        kernel_parameter_manager.setCurrentMachine(currentIndex)
                        return
                    }

                    idAllMenuDialog.requestMenuDialog({
                        type: "all",
                        comboBox: printer_combo_box,
                        parameterManager: kernel_parameter_manager,
                        onSaved: function() {
                            this.parameterManager.setCurrentMachine(this.comboBox.currentIndex)
                        },
                        onIgnore: function() {
                            this.parameterManager.abandonAll()
                            this.parameterManager.setCurrentMachine(this.comboBox.currentIndex)
                        },
                        onCloseButtonClicked: function() {
                            this.comboBox.currentIndex =
                                    Qt.binding(() => kernel_parameter_manager.curMachineIndex)
                        },
                    }, "saveParemDialog")
                }

                view.model: popup.visible ? this.delegateModel : null
                view.currentIndex: popup.visible ? this.currentIndex : -1

                view.section.property: "default"
                view.section.delegate: Item {
                    required property bool section
                    implicitWidth: printer_combo_box.implicitDelegateWidth
                    implicitHeight: printer_combo_box.implicitDelegateHeight

                    Text {
                        anchors.fill: parent
                        topPadding: printer_combo_box.textTopPadding
                        bottomPadding: printer_combo_box.textBottomPadding
                        leftPadding: printer_combo_box.textLeftPadding
                        rightPadding: printer_combo_box.textRightPadding
                        verticalAlignment: printer_combo_box.textVerticalAlignment
                        horizontalAlignment: printer_combo_box.textHorizontalAlignment
                        font: Constants.font
                        color: Constants.right_panel_text_default_color
                        text: "------ %1 ------".arg(qsTranslate("CXComboBox",
                                parent.section ? "Default configuration" : "User configuration"))
                    }
                }

                view.footerPositioning: ListView.OverlayFooter
                view.footer: Button {
                    z: 2
                    implicitWidth: printer_combo_box.implicitDelegateWidth
                    implicitHeight: printer_combo_box.implicitDelegateHeight + 4 * screenScaleFactor

                    background: Rectangle {
                        radius: printer_combo_box.radius
                        color: printer_combo_box.popup.background.color
                    }

                    contentItem: Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2 * screenScaleFactor
                        radius: printer_combo_box.radius
                        color: hovered ? Constants.profileBtnHoverColor : Constants.profileBtnColor

                        Text {
                            anchors.fill: parent
                            topPadding: printer_combo_box.textTopPadding
                            bottomPadding: printer_combo_box.textBottomPadding
                            leftPadding: printer_combo_box.textLeftPadding
                            rightPadding: printer_combo_box.textRightPadding
                            verticalAlignment: printer_combo_box.textVerticalAlignment
                            horizontalAlignment: Text.AlignHCenter
                            font: Constants.font
                            color: Constants.right_panel_text_default_color
                            text: qsTr("Add")
                        }
                    }

                    onClicked: {
                        kernel_ui.requestQmlDialog("idAddPrinterDlgNew")
                        printer_combo_box.popup.close()
                    }
                }

                delegate: ItemDelegate {
                    enabled: !ListView.view.ScrollBar.vertical.active
                    implicitWidth: printer_combo_box.implicitDelegateWidth
                    implicitHeight: printer_combo_box.implicitDelegateHeight
                    property var preIndex: ListView.view.currentIndex

                    background: Rectangle {
                        radius: 5 * screenScaleFactor
                        color: hovered ? Constants.right_panel_item_checked_color : "transparent"
                    }

                    contentItem: RowLayout {
                        anchors.fill: parent

                        Text {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            topPadding: printer_combo_box.textTopPadding
                            bottomPadding: printer_combo_box.textBottomPadding
                            leftPadding: printer_combo_box.textLeftPadding
                            rightPadding: printer_combo_box.textRightPadding
                            verticalAlignment: printer_combo_box.textVerticalAlignment
                            horizontalAlignment: printer_combo_box.textHorizontalAlignment
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
                                const machine = kernel_parameter_manager.machineObject(index)
                                kernel_parameter_manager.removeMachine(machine)
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
                                    source: parent.parent.hovered
                                            ? Constants.right_panel_delete_hovered_image
                                            : Constants.right_panel_delete_image
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
                    printer_combo_box.popup.visible = false;
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
                text: qsTr("Plate Type")+":"
                color: Constants.right_panel_text_default_color
                font.pointSize: Constants.labelFontPointSize_9
                wrapMode: Text.NoWrap
                font.family: Constants.labelFontFamily
            }

            CustomComboBox {
                Layout.fillWidth: true
                Layout.preferredHeight: 24 * screenScaleFactor
                model: kernel_parameter_manager.currrentBedTypes()
                translater: source => qsTranslate('ParameterComponent', source)
                currentIndex: kernel_parameter_manager.currBedTypeIndex
                onActivated: {
                    kernel_parameter_manager.setCurrBedType(kernel_parameter_manager.currrentBedKeys()[currentIndex])
                }
                onModelChanged:
                {
                    currentIndex = Qt.binding(function() { return kernel_parameter_manager.currBedTypeIndex})
                }
            }
        }
    }
}
