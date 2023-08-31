import QtQuick 2.10
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4 as Control14
import QtQml 2.13

import "../qml"
import "../secondqml"

DockItem
{
    property int themeType: -1
    property var sourceTheme: ""
    id: root
    title: qsTr("Add Printer")
    modality: Qt.ApplicationModal
    width: 1214 * screenScaleFactor
    height: 730 * screenScaleFactor
    panelColor: sourceTheme.background_color

    AddPrinterDlgChoseNuzzle {
        id: idAddPrinterDlg
        onChoseAccepted: Qt.callLater(function() { root.close() })
    }

    ListModel {
        id: themeModel
        ListElement {
            //Dark Theme
            background_color: "#4B4B4D"
            text_light_color: "#CBCBCC"
            text_weight_color: "#333333"
            text_disable_color: "#999999"

            scrollbar_color: "#7E7E84"
            item_border_color: "#7A7A7F"
            btn_default_color: "#4B4B4D"
            btn_hovered_color: "#7A7A7F"

            textedit_text_color: "#FEFEFE"
            textedit_selection_color: "#1E9BE2"

            treeview_indicator_border: "#CBCBCC"
            treeview_text_selection_color: "#00A3FF"

            search_btn_img: "qrc:/UI/photo/search.png"
            treeView_plus_img: "qrc:/UI/photo/treeView_plus_dark.png"
            treeView_minus_img: "qrc:/UI/photo/treeView_minus_dark.png"
            treeView_checked_img: "qrc:/UI/photo/treeView_checked_icon.png"
        }
        ListElement {
            //Light Theme
            background_color: "#FFFFFF"
            text_light_color: "#333333"
            text_weight_color: "#333333"
            text_disable_color: "#999999"

            scrollbar_color: "#D6D6DC"
            item_border_color: "#CBCBCC"
            btn_default_color: "#FFFFFF"
            btn_hovered_color: "#CBCBCC"

            textedit_text_color: "#333333"
            textedit_selection_color: "#00A3FF"

            treeview_indicator_border: "#CBCBCC"
            treeview_text_selection_color: "#00A3FF"

            search_btn_img: "qrc:/UI/photo/search.png"
            treeView_plus_img: "qrc:/UI/photo/treeView_plus_light.png"
            treeView_minus_img: "qrc:/UI/photo/treeView_minus_light.png"
            treeView_checked_img: "qrc:/UI/photo/treeView_checked_icon.png"
        }
    }

    Binding on themeType {
        value: Constants.currentTheme
    }

    onThemeTypeChanged: sourceTheme = themeModel.get(themeType)//切换主题

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: root.currentTitleHeight + anchors.bottomMargin
        anchors.bottomMargin: 30 * screenScaleFactor
        spacing: 0 * screenScaleFactor

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 40 * screenScaleFactor
            Layout.maximumWidth: 54 * screenScaleFactor
        }

        ColumnLayout {
            spacing: 25 * screenScaleFactor

            BasicScrollView {
                clip: true
                id: idListScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 159 * screenScaleFactor
                Layout.maximumWidth: 185 * screenScaleFactor
                hpolicyVisible: contentWidth > width
                vpolicyVisible: contentHeight > height
                hpolicyindicator: Rectangle {
                    radius: height / 2
                    color: sourceTheme.scrollbar_color
                    implicitWidth: 200 * screenScaleFactor
                    implicitHeight: 6 * screenScaleFactor
                }
                vpolicyindicator: Rectangle {
                    radius: width / 2
                    color: sourceTheme.scrollbar_color
                    implicitWidth: 6 * screenScaleFactor
                    implicitHeight: 200 * screenScaleFactor
                }

                Column {
                    width: 125 * screenScaleFactor
                    spacing: 0

                    Repeater {
                        model: kernel_add_printer_model.getTypeNameList()
                        delegate: Column {
                            width: 125 * screenScaleFactor
                            spacing: 0

                            AddPrinterButton {
                                id: type_button
                                typeName: modelData
                                subButtonDefaultImage: root.sourceTheme.treeView_plus_img
                                subButtonSelectedImage: root.sourceTheme.treeView_minus_img
                                textColor: root.sourceTheme.text_light_color
                            }

                            Column {
                                width: 125 * screenScaleFactor
                                spacing: 0

                                Repeater {
                                    model: kernel_add_printer_model.getPrinterNameList(type_button.typeName)
                                    delegate: AddPrinterButton {
                                        id: printer_button
                                        visible: type_button.isOpened
                                        typeName: type_button.typeName
                                        printerName: modelData
                                        textColor: root.sourceTheme.text_light_color
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.preferredWidth: 20 * screenScaleFactor
            Layout.fillHeight: true
        }

        ColumnLayout {
            spacing: 25 * screenScaleFactor

            BasicScrollView {
                clip: true
                id: idModelScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 983 * screenScaleFactor
                Layout.maximumWidth: 1637 * screenScaleFactor
                hpolicyVisible: contentWidth > width
                vpolicyVisible: contentHeight > height
                hpolicyindicator: Rectangle {
                    radius: height / 2
                    color: sourceTheme.scrollbar_color
                    implicitWidth: 200 * screenScaleFactor
                    implicitHeight: 6 * screenScaleFactor
                }
                vpolicyindicator: Rectangle {
                    radius: width / 2
                    color: sourceTheme.scrollbar_color
                    implicitWidth: 6 * screenScaleFactor
                    implicitHeight: 200 * screenScaleFactor
                }

                Flow {
                    width: idModelScrollView.width
                    spacing: 20 * screenScaleFactor

                    Repeater {
                        id: itemRepeater
                        model: kernel_add_printer_model
                        delegate: AddPrinterViewItem
                        {
                            imageUrl     : model_image_url
                            printerName  : model_name
                            machineDepth : model_depth
                            machineWidth : model_width
                            machineHeight: model_height
                            nozzleSize   : model_nozzle_size
                            addEnabled   : !model_added

                            onAddMachine: {
                                idAddPrinterDlg.machineName = printerName
                                idAddPrinterDlg.nozzleNum = model_nozzle_count
                                idAddPrinterDlg.isGranular = model_granular

                                idAddPrinterDlg.height = 600 * screenScaleFactor
                                idAddPrinterDlg.visible = true
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 18 * screenScaleFactor
            Layout.maximumWidth: 24 * screenScaleFactor
        }
    }
}
