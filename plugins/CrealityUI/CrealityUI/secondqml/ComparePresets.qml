import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "../qml"
import "../components"
import "../secondqml/frameless"

DockItem {
    id: rootDlg

    readonly property bool isPrinterDifference: contenteLoader.item.machineDiffKeys.length > 0
    readonly property bool isExtruderDifference: contenteLoader.item.materialDiffKeys.length > 0
    readonly property bool isProcessDifference: contenteLoader.item.processDiffKeys.length > 0
    readonly property bool hasAnyDifference: isPrinterDifference || isExtruderDifference || isProcessDifference

    visible: false
    width: 802 * screenScaleFactor
    height: hasAnyDifference ? 636 * screenScaleFactor : 236 * screenScaleFactor
    title: qsTranslate("ParameterComponent", "Compare Presets")

    onVisibleChanged: {
        if (visible) {
            contenteLoader.active = true
        }
    }

    Loader {
        id: contenteLoader

        anchors.fill: parent
        anchors.margins: 20 * screenScaleFactor
        anchors.topMargin: rootDlg.titleHeight + 20 * screenScaleFactor

        active: false

        sourceComponent: ColumnLayout {
            readonly property var machineObj1: kernel_parameter_manager.machineObject(printer1.currentIndex)
            readonly property var machineObj2: kernel_parameter_manager.machineObject(printer2.currentIndex)

            readonly property var materialObj1: machineObj1.materialObject(extruder1.currentIndex)
            readonly property var materialObj2: machineObj2.materialObject(extruder2.currentIndex)

            readonly property var processObj1: machineObj1.profileObject(process1.currentIndex)
            readonly property var processObj2: machineObj2.profileObject(process2.currentIndex)

            readonly property var machineItemDataObj1: machineObj1.dataModel
            readonly property var machineItemDataObj2: machineObj2.dataModel

            readonly property var materialItemDataObj1: materialObj1.basicDataModel()
            readonly property var materialItemDataObj2: materialObj2.basicDataModel()

            readonly property var processItemDataObj1: processObj1.basicDataModel()
            readonly property var processItemDataObj2: processObj2.basicDataModel()

            readonly property var machineDiffKeys: kernel_parameter_manager.printDiffKeys(printer1.currentIndex, printer2.currentIndex)
            readonly property var materialDiffKeys: materialObj1.materialDiffKeys(materialObj2)
            readonly property var processDiffKeys: processObj1.processDiffKeys(processObj2)

            readonly property string equalImg: "qrc:/UI/photo/rightDrawer/equal.svg"
            readonly property string unequalImg: "qrc:/UI/photo/rightDrawer/unequal.svg"

            readonly property list<QtObject> compareBlockModel: [
                QtObject {
                    readonly property string imgSource: "qrc:/UI/photo/rightDrawer/printer_printer.svg"
                    readonly property string comp1Text: printer1.currentText
                    readonly property string comp2Text: printer2.currentText
                    readonly property bool isVisible: rootDlg.isPrinterDifference
                    readonly property var diffkeys: machineDiffKeys
                    readonly property var treeModel: kernel_ui_parameter.machineCompareTreeModel
                    readonly property var itemModel1: machineItemDataObj1
                    readonly property var itemModel2: machineItemDataObj2
                },

                QtObject {
                    readonly property string imgSource: "qrc:/UI/photo/rightDrawer/extruder_extruder.svg"
                    readonly property string comp1Text: extruder1.currentText
                    readonly property string comp2Text: extruder2.currentText
                    readonly property bool isVisible: rootDlg.isExtruderDifference
                    readonly property var diffkeys: materialDiffKeys
                    readonly property var treeModel: kernel_ui_parameter.materialCompareTreeModel
                    readonly property var itemModel1: materialItemDataObj1
                    readonly property var itemModel2: materialItemDataObj2
                },

                QtObject {
                    readonly property string imgSource: "qrc:/UI/photo/paramPanel.svg"
                    readonly property string comp1Text: process1.currentText
                    readonly property string comp2Text: process2.currentText
                    readonly property bool isVisible: rootDlg.isProcessDifference
                    readonly property var diffkeys: processDiffKeys
                    readonly property var treeModel: kernel_ui_parameter.processCompareTreeModel
                    readonly property var itemModel1: processItemDataObj1
                    readonly property var itemModel2: processItemDataObj2
                }
            ]

            StyledLabel {
                text: qsTranslate("ParameterComponent", "Select a process to compare")
            }

            GridLayout {
                id: gridLayout
                columns: 3
                Layout.fillWidth: true

                CXComboBox {
                    id: printer1
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * screenScaleFactor
                    currentIndex: kernel_parameter_manager.curMachineIndex
                    model: kernel_parameter_manager.machineNameList
                    onActivated: {
                        // machineObj1 = kernel_parameter_manager.machineObject(index)
                        // machineDiffKeys = kernel_parameter_manager.printDiffKeys(printer1.currentIndex, printer2.currentIndex)

                        // rootDlg.isPrinterDifference = machineDiffKeys.length
                        // compareBlockModel.set(0, {"itemModel1": machineObj1.dataModel})
                        // compareBlockModel.set(0, {"isVisible": machineDiffKeys.length > 0})
                        // compareBlockModel.set(0, {"diffKeys": machineDiffKeys})
                    }
                }

                Image {
                    source: isPrinterDifference ? unequalImg : equalImg
                }

                CXComboBox {
                    id: printer2
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * screenScaleFactor
                    currentIndex: kernel_parameter_manager.curMachineIndex
                    model: kernel_parameter_manager.machineNameList
                    onActivated: {
                        // machineObj2 = kernel_parameter_manager.machineObject(index)
                        // machineDiffKeys = kernel_parameter_manager.printDiffKeys(printer1.currentIndex, printer2.currentIndex)

                        // rootDlg.isPrinterDifference = machineDiffKeys.length
                        // compareBlockModel.set(0, {"itemModel2": machineObj2.dataModel})
                        // compareBlockModel.set(0, {"isVisible": machineDiffKeys.length > 0})
                        // compareBlockModel.set(0, {"diffKeys": machineDiffKeys})
                    }
                }

                CXComboBox {
                    id: extruder1
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * screenScaleFactor
                    model: kernel_parameter_manager.machineObject(printer1.currentIndex).materialsName

                    onCurrentTextChanged: {
                        // materialObj1 = machineObj1.materialObject(extruder1.currentIndex)
                        // materialDiffKeys = materialObj2.materialDiffKeys(materialObj1)

                        // rootDlg.isExtruderDifference = materialDiffKeys.length
                        // compareBlockModel.set(1, {"isVisible": materialDiffKeys.length > 0})
                        // compareBlockModel.set(1, {"itemModel1": materialObj1.basicDataModel() })
                        // compareBlockModel.set(1, {"diffKeys": materialDiffKeys})
                    }
                }

                Image {
                    source: isExtruderDifference ? unequalImg : equalImg
                }

                CXComboBox {
                    id: extruder2
                    Layout.fillWidth: true
                    //                        Layout.preferredWidth: 360 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    model: kernel_parameter_manager.machineObject(printer2.currentIndex).materialsName

                    onCurrentTextChanged: {
                        // materialObj2 = machineObj2.materialObject(extruder2.currentIndex)
                        // materialDiffKeys = materialObj1.materialDiffKeys(materialObj2)

                        // rootDlg.isExtruderDifference = materialDiffKeys.length
                        // compareBlockModel.set(1, {"isVisible": materialDiffKeys.length > 0})
                        // compareBlockModel.set(1, {"itemModel2": materialObj2.basicDataModel() })
                        // compareBlockModel.set(1, {"diffKeys": materialDiffKeys})
                    }
                }

                CXComboBox {
                    id: process1
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * screenScaleFactor
                    currentIndex: kernel_parameter_manager.currentMachineObject.curProfileIndex
                    model: kernel_parameter_manager.machineObject(printer1.currentIndex).profilesModel
                    textRole: "name"
                    onCurrentTextChanged: {
                        // processObj1 = machineObj1.profileObject(process1.currentIndex)
                        // processObj2 = machineObj2.profileObject(process2.currentIndex)
                        // processDiffKeys = processObj2.processDiffKeys(processObj1)

                        // rootDlg.isProcessDifference = processDiffKeys.length
                        // compareBlockModel.set(2, {"isVisible": processDiffKeys.length > 0})
                        // compareBlockModel.set(2, {"itemModel1": processObj1.basicDataModel() })
                        // compareBlockModel.set(2, {"diffKeys": processDiffKeys})
                    }
                }

                Image {
                    source: isProcessDifference ? unequalImg : equalImg
                }

                CXComboBox {
                    id: process2
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * screenScaleFactor
                    currentIndex: kernel_parameter_manager.currentMachineObject.curProfileIndex
                    model: kernel_parameter_manager.machineObject(printer2.currentIndex).profilesModel
                    textRole: "name"
                    onCurrentTextChanged: {
                        // processObj1 = machineObj1.profileObject(process1.currentIndex)
                        // processObj2 = machineObj2.profileObject(process2.currentIndex)
                        // processDiffKeys = processObj1.processDiffKeys(processObj2)

                        // rootDlg.isProcessDifference = processDiffKeys.length
                        // compareBlockModel.set(2, {"isVisible": processDiffKeys.length > 0})
                        // compareBlockModel.set(2, {"itemModel2": processObj2.basicDataModel() })
                        // compareBlockModel.set(2, {"diffKeys": processDiffKeys})

                        // console.log("brabbit_1 %1".arg(compareBlockModel[2]))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].comp1Text))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].comp2Text))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].isVisible))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].diffkeys))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].imgSource))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].treeModel))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].itemModel1))
                        // console.log("brabbit_1 %1".arg(compareBlockModel[2].itemModel2))
                    }
                }
            }

            ScrollView {
                id: scroll_view

                Layout.fillWidth: true
                Layout.fillHeight: true

                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

                clip: true
                visible: hasAnyDifference

                background: Rectangle {
                    border.width: 1 * screenScaleFactor
                    border.color: Constants.darkThemeColor_secondary
                    color: "transparent"
                }

                ColumnLayout {
                    width: scroll_view.availableWidth
                    height: implicitHeight

                    spacing: 1 * screenScaleFactor

                    Repeater {
                        model: compareBlockModel
                        delegate: compareBlock
                    }
                }
            }

            Component {
                id: compareBlock

                ColumnLayout {
                    id: compare_layout

                    readonly property var context: model

                    visible: context.isVisible
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Button {
                        id: titleBtn

                        Layout.fillWidth: true
                        Layout.minimumHeight: 36 * screenScaleFactor
                        Layout.maximumHeight: Layout.minimumHeight

                        hoverEnabled: true
                        checkable: true
                        checked: true

                        background: Rectangle {
                            color: Constants.right_panel_border_default_color

                            RowLayout {
                                spacing: 10 * screenScaleFactor
                                anchors.fill: parent
                                anchors.leftMargin: 20 * screenScaleFactor

                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter

                                    spacing: 6 * screenScaleFactor

                                    Image {
                                        source: model.imgSource
                                    }

                                    StyledLabel {
                                        Layout.fillWidth: true
                                        font.weight: Font.Medium
                                        font.pointSize: Constants.labelFontPointSize_10
                                        text: qsTranslate("ParameterComponent", "Compare")
                                    }
                                }

                                StyledLabel {
                                    font.weight: Font.Medium
                                    font.pointSize: Constants.labelFontPointSize_10
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.fillWidth: true
                                    text: model.comp1Text
                                }

                                StyledLabel{
                                    font.weight: Font.Medium
                                    font.pointSize: Constants.labelFontPointSize_10
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredWidth: 70 * screenScaleFactor
                                    text: "VS"
                                    color:"#00a3ff"
                                }

                                StyledLabel {
                                    font.weight: Font.Medium
                                    font.pointSize: Constants.labelFontPointSize_10
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.fillWidth: true
                                    text: model.comp2Text
                                }

                                Image {
                                    property string upSource: !titleBtn.hovered ? "qrc:/UI/photo/lanPrinterSource/upBtn.svg" : "qrc:/UI/photo/lanPrinterSource/upBtn_hover.svg"
                                    property string downSource: !titleBtn.hovered ? "qrc:/UI/photo/downBtn.svg" : "qrc:/UI/photo/downBtn_d.svg"
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.rightMargin: 9 * screenScaleFactor
                                    source: titleBtn.checked ? downSource : upSource
                                }
                            }

                        }
                    }

                    FlattenTreeView {
                        id: slice_config_tree_view

                        visible: titleBtn.checked
                        Layout.fillWidth: true
                        Layout.minimumHeight: contentHeight
                        Layout.maximumHeight: Layout.minimumHeight

                        asynchronous: true
                        interactive: false

                        nodeFillWidth: true
                        forkNodeHeight: 40 * screenScaleFactor
                        leafNodeHeight: 24 * screenScaleFactor
                        nodeLeftMargin: 0 * screenScaleFactor
                        nodeSpacing: 0

                        treeModel: compare_layout.context.treeModel

                        forkNodeDelegate: Button {
                            id: fork_button

                            checkable: true
                            checked: true
                            padding: 0

                            Component.onCompleted: _ => {
                                nodeModel.childNodeValid = Qt.binding(_ => checked)
                            }

                            background: Rectangle {
                                border.width: 1* screenScaleFactor
                                border.color: Constants.darkThemeColor_secondary
                                color:{
                                    let str = nodeModel.uid
                                    const matches = str.match(new RegExp('/', 'g'));
                                    let isFirstLevel =  matches ? (matches.length === 1) : 0;
                                    return isFirstLevel ? "#454547" : "transparent"
                                }
                            }

                            contentItem: RowLayout {
                                spacing: 10 * screenScaleFactor

                                Image {
                                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                    Layout.leftMargin: {
                                        let str = nodeModel.uid
                                        const matches = str.match(new RegExp('/', 'g'));
                                        return matches ? matches.length*20*screenScaleFactor : 0;
                                    }
                                    horizontalAlignment: Qt.AlignLeft
                                    verticalAlignment: Qt.AlignVCenter
                                    fillMode: Image.PreserveAspectFit
                                    source: {
                                        let str = nodeModel.uid
                                        const matches = str.match(new RegExp('/', 'g'));
                                        let isFirstLevel =  matches ? (matches.length === 1) : 0;
                                        return isFirstLevel ? nodeModel.icon : ""
                                    }
                                }

                                StyledLabel {
                                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                    Layout.fillWidth: true
                                    horizontalAlignment: Qt.AlignLeft
                                    verticalAlignment: Qt.AlignVCenter
                                    font.weight: Font.Medium
                                    font.pointSize: Constants.labelFontPointSize_12
                                    color: Constants.right_panel_text_default_color
                                    text: qsTranslate("ParameterComponent", nodeModel.label)
                                }

                                Item{
                                    Layout.fillWidth: true
                                }

                                Image {
                                    property string upSource: !fork_button.hovered ? "qrc:/UI/photo/lanPrinterSource/upBtn.svg" : "qrc:/UI/photo/lanPrinterSource/upBtn_hover.svg"
                                    property string downSource: !fork_button.hovered ? "qrc:/UI/photo/downBtn.svg" : "qrc:/UI/photo/downBtn_d.svg"
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.rightMargin: 7 * screenScaleFactor

                                    visible: {
                                        let str = nodeModel.uid
                                        const matches = str.match(new RegExp('/', 'g'));
                                        return matches ? (matches.length === 1) : 0;
                                    }
                                    fillMode: Image.PreserveAspectFit
                                    source: fork_button.checked ? downSource : upSource
                                }
                            }
                        }

                        leafNodeDelegate: Rectangle {
                            readonly property var itemModel1: compare_layout.context.itemModel1.getDataItem(nodeModel.key)
                            readonly property var itemModel2: compare_layout.context.itemModel2.getDataItem(nodeModel.key)

                            border.width: 1 * screenScaleFactor
                            border.color: Constants.darkThemeColor_secondary
                            color: "transparent"

                            Component.onCompleted: _ => {
                                nodeModel.nodeValid = Qt.binding(_ => diffkeys.includes(nodeModel.key))
                            }

                            RowLayout {
                                anchors.fill: parent

                                StyledLabel{
                                    id: textLabel
                                    activeFocusOnTab: false
                                    Layout.leftMargin: {
                                        let str = nodeModel.uid
                                        const matches = str.match(new RegExp('/', 'g'));
                                        return matches ? matches.length*20*screenScaleFactor : 0;
                                    }
                                    Layout.fillWidth: true
                                    text: itemModel1 != undefined ? "• " + qsTranslate("ParameterComponent", itemModel1.label) : ""
                                    font.weight: Font.Medium
                                    font.pointSize:Constants.labelFontPointSize_10
                                    // color: Constants.manager_printer_label_color
                                    strToolTip: itemModel1 != undefined ? qsTranslate("ParameterComponent", itemModel1.description) : ""
                                    toolTipPosition: BasicTooltip.Position.LEFT
                                    wrapMode: Text.WordWrap
                                }

                                StyledLabel{
                                    id: compareText1
                                    Layout.preferredWidth: 240 * screenScaleFactor
                                    text: itemModel1 ? itemModel1.value : ""
                                    font.weight: Font.Medium
                                    font.pointSize:Constants.labelFontPointSize_10
                                    color: Constants.manager_printer_gcode_text_color
                                    elide: Text.ElideRight
                                    wrapMode: Text.NoWrap
                                    maximumLineCount: 1
                                }

                                StyledLabel{
                                    font.weight: Font.Medium
                                    font.pointSize: Constants.labelFontPointSize_10
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.preferredWidth: 70 * screenScaleFactor
                                    text: "VS"
                                    color:"#00a3ff"
                                }

                                StyledLabel{
                                    id: compareText2
                                    Layout.preferredWidth: 240 * screenScaleFactor
                                    text: itemModel2 ? itemModel2.value : ""
                                    font.weight: Font.Medium
                                    font.pointSize:Constants.labelFontPointSize_10
                                    color: Constants.manager_printer_gcode_text_color
                                    elide: Text.ElideRight
                                    wrapMode: Text.NoWrap
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }
                }
            }

            function isMachineInclude(itemKey){
                if(machineDiffKeys.includes(itemKey))
                    return true
                else
                    return false
            }


            function isMaterialInclude(itemKey){
                if(materialDiffKeys.includes(itemKey))
                    return true
                else
                    return false
            }


            function isProcessInclude(itemKey){
                if(processDiffKeys.includes(itemKey))
                    return true
                else
                    return false
            }
        }
    }

    Image{
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        //        visible: false
        source: "qrc:/UI/photo/rightDrawer/cornerIcon.svg"
        CusDragItem {
            id: rightBottomHandle
            posType: posRightBottom

            anchors.fill: parent
            onPosChange: {
                if (rootDlg.width + xOffset > 802 * screenScaleFactor)
                    rootDlg.width += xOffset;
                if (rootDlg.height + yOffset > 236 * screenScaleFactor)
                    rootDlg.height += yOffset;
            }
        }
    }
}
