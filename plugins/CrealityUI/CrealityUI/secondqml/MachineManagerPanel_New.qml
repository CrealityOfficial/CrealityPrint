import "../components"
import "../qml"
import "./"
import Qt.labs.platform 1.1
import QtQml 2.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

BasicDialogV4 {
    id: root

    property var machine: kernel_parameter_manager.currentMachineObject

    readonly property var extruders: {
        const model = machine ? machine.extrudersModel : null
        if (!model) {
            return []
        }

        let list = []
        for (let index = 0; index < model.count; ++index) {
            const item = model.get(index)
            if (!item) {
                continue
            }

            const extruder = item.extruderObj
            if (!extruder || !extruder.physical) {
                continue
            }

            list.push(extruder)
        }
        return list
    }

    function excuteOpt(optName) {
        Qt.callLater(function(opt_name) {
            actionCommands.getOpt(opt_name).execute();
        }, optName);
    }

    maxBtnVis: false
    width: 843 * screenScaleFactor
    height: 660 * screenScaleFactor
    title: qsTr("Manage Printer")
    titleHeight: 30 * screenScaleFactor
    onVisibleChanged: {
        if(visible){
            cloader.item.curMachineIndex = kernel_parameter_manager.curMachineIndex
            cloader.item.machineView.itemAtIndex(cloader.item.curMachineIndex).clicked()
            console.log("kernel_parameter_manager.curMachineIndex ==========", kernel_parameter_manager.curMachineIndex)
        }
    }

    ParameterComponent {
        id: parameter_component

        editerWidth: 220 * screenScaleFactor
    }

    UploadMessageDlg {
        id: warningDlg

        property var receiver

        parent: standaloneWindow
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        onSigOkButtonClicked: {
            receiver.exec();
            warningDlg.visible = false;
        }
    }

    Component {
        id: basic_param_view_com

        FlattenTreeView {
            anchors.fill: parent
            anchors.margins: 20 * screenScaleFactor

            ScrollBar.vertical: ScrollBar {
                policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                onActiveChanged: parameter_component.closePopup()
            }

            clip: true
            rightMargin: 12 * screenScaleFactor

            nodeFillWidth: true
            forkNodeHeight: -1
            leafNodeHeight: -1

            treeModel: itemModel.model_node

            forkNodeDelegate: ColumnLayout {
                readonly property bool isTinyLabel: nodeModel.component === "fork_tiny_label"

                function showChildNode() {
                    fork_button.checked = true
                }

                Component.onCompleted: {
                    nodeHeight = (isTinyLabel ? 24 : 40) * screenScaleFactor
                }

                Item {
                    Layout.fillHeight: true
                }

                Button {
                    id: fork_button

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom

                    focusPolicy: Qt.ClickFocus
                    checkable: !isTinyLabel
                    checked: !isTinyLabel

                    Component.onCompleted: {
                        nodeModel.childNodeValid = Qt.binding(_ => checked || isTinyLabel)
                    }

                    background: Item {}

                    contentItem: RowLayout {
                        spacing: (isTinyLabel ? 5 : 10) * screenScaleFactor

                        StyledLabel {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                            horizontalAlignment: Qt.AlignLeft
                            verticalAlignment: Qt.AlignVCenter
                            font.weight: isTinyLabel ? Font.Normal : Font.Bold
                            font.pointSize: isTinyLabel ? Constants.labelFontPointSize_10 : Constants.labelFontPointSize_12
                            color: isTinyLabel ? Constants.manager_printer_label_color : Constants.right_panel_text_default_color
                            text: qsTranslate("ParameterComponent", nodeModel.label)
                        }

                        Rectangle {
                            visible: !isTinyLabel
                            Layout.fillWidth: true
                            Layout.minimumHeight: 1 * screenScaleFactor
                            Layout.maximumHeight: Layout.minimumHeight
                            Layout.alignment: Qt.AlignVCenter
                            color: Constants.right_panel_border_default_color
                        }

                        Image {
                            visible: !isTinyLabel
                            width: 12 * screenScaleFactor
                            height: 6 * screenScaleFactor
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                            horizontalAlignment: Qt.AlignRight
                            verticalAlignment: Qt.AlignVCenter
                            fillMode: Image.PreserveAspectFit
                            source: fork_button.checked ? "qrc:/UI/photo/comboboxDown2_flip.png" : "qrc:/UI/photo/comboboxDown2.png"
                        }
                    }
                }
            }

            leafNodeDelegate: Loader {
                id: leaf_node_loader

                readonly property var uiModel: nodeModel
                readonly property var dataModel: {
                    if (kernel_parameter_manager.machineListModel.count === 0) {
                        return null
                    }

                    const index_regex = /extruder_([1-9][0-9]*)/
                    const match_result = uiModel.uid.match(index_regex)
                    if (match_result) {
                        const index = parseInt(match_result[1]) - 1
                        const extruder = extruders.length > index ? extruders[index] : null
                        const data_model = extruder ? extruder.dataModel : null
                        return data_model ? data_model.getDataItem(uiModel.key) : null
                    }

                    const data_model = machine ? machine.dataModel : null
                    return data_model ? data_model.getDataItem(uiModel.key) : null
                }

                readonly property bool vaild: {
                    return uiModel.level >= currentLevel && dataModel && dataModel.enabled
                }

                active: dataModel
                sourceComponent: parameter_component.parameter

                Component.onCompleted: {
                    nodeModel.nodeValid = Qt.binding(_ => vaild)
                }
            }
        }
    }

    AddPrinterDlgNewStep {
        id: addPrinterStep

        visible: false
    }

    bdContentItem: Item {
        property alias curMachineIndex: machine_view.currentIndex
        property alias machineView: machine_view
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20 * screenScaleFactor

            RowLayout {
                Layout.fillWidth: true

                SearchComboBox {
                    property var searchContext: null

                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    Layout.minimumWidth: 282 * screenScaleFactor
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    textRole: "model_text"
                    placeholderText: qsTranslate("ParameterComponent", "Please enter a parameter name")

                    popup.onOpened: _ => {
                                        const context = kernel_ui_parameter.generateSearchContext(tap_view_repeater.model, "ParameterComponent")
                                        const proxy_model = context.proxyModel

                                        proxy_model.filterCaseSensitivity = Qt.CaseInsensitive
                                        if (extruders.length === 1) {
                                            proxy_model.filterUidList = ["/extruder_2"]
                                        }
                                        proxy_model.minimumLevel = Qt.binding(_ => advance_button.currentLevel)
                                        proxy_model.searchText = Qt.binding(_ => searchText)

                                        searchContext = context
                                        model = proxy_model
                                    }

                    popup.onClosed: _ => {
                                        searchText = ""
                                        model = null
                                        searchContext = null
                                    }

                    onActivated: _ => {
                                     const type_view = tab_view
                                     const type_repeater = tap_view_repeater
                                     const tree_model = tap_view_repeater.model
                                     const proxy_model = searchContext.proxyModel

                                     // 获取 搜索结果节点 (result_node)
                                     const result_item = searchContext.proxyModel.get(currentIndex)
                                     const result_uid = result_item.model_uid
                                     const result_node = result_item.model_node

                                     // 找到 result_node 所属 一级分类节点 (type_node)
                                     for (let type_index = 0; type_index < type_repeater.count; ++type_index) {
                                         const type_node = type_repeater.model.get(type_index)
                                         if (result_node !== type_node.getChildNode(result_uid, true)) {
                                             continue
                                         }

                                         // 切换到 result_node 所属 type_node 对应的页面
                                         const type_delegate = type_repeater.itemAt(type_index)
                                         type_view.currentPanel = type_delegate

                                         // 切换页面后续的 UI 渲染走事件系统, 这里需要异步处理后续操作
                                         Qt.callLater(_ => {
                                                          // 定位到 result_node 对应组件处
                                                          const tree_view = type_delegate.treeView
                                                          const tree_delegate = tree_view.positionDelegate(result_node, ListView.Contain)

                                                          // 高亮 tree_delegate
                                                          tree_delegate.object.uiModel.requestHighlight()
                                                      })

                                         return
                                     }
                                 }
                }

                Item {
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: 20 * screenScaleFactor
                }

                Text {
                    Layout.preferredWidth: contentWidth
                    Layout.preferredHeight: contentHeight
                    horizontalAlignment: Qt.AlignRight
                    verticalAlignment: Qt.AlignVCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_12
                    color: Constants.parameter_text_color
                    text: qsTranslate("ExpertParamPanel", "Advanced")
                }

                Switch {
                    id: advance_button

                    readonly property int currentLevel: checked ? 1 : 2

                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    Layout.preferredWidth: 44 * screenScaleFactor
                    Layout.preferredHeight: 24 * screenScaleFactor

                    checkable: true
                    checked: false

                    Component.onCompleted: {
                        checked = kernel_ui_parameter.advanceSettingEnabled
                    }

                    onCheckedChanged: {
                        if (kernel_ui_parameter.advanceSettingEnabled != checked) {
                            kernel_ui_parameter.advanceSettingEnabled = checked
                        }
                    }

                    Connections {
                        target: kernel_ui_parameter
                        function onAdvanceSettingEnabledChanged() {
                            if (advance_button.checked != kernel_ui_parameter.advanceSettingEnabled) {
                                advance_button.checked = kernel_ui_parameter.advanceSettingEnabled
                            }
                        }
                    }

                    background: Rectangle {
                        radius: height / 2
                        color: parent.checked ? Constants.themeGreenColor : "#6E6E73"
                    }

                    contentItem: Item {}

                    indicator: Rectangle {
                        readonly property real borderWidth: 2 * screenScaleFactor

                        anchors.verticalCenter: parent.verticalCenter
                        x: parent.checked ? parent.width - width - borderWidth : borderWidth
                        width: parent.height - borderWidth * 2
                        height: width
                        radius: height / 2
                        color: parent.checked ? "#ffffff" : Constants.mainBackgroundColor

                        Behavior on x {
                            NumberAnimation {
                                duration: 100
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: machine_view

                    property bool curItemIsModified: {
                        if (!currentItem) {
                            return false
                        }

                        const model_item = currentItem.modelItem
                        if (!model_item) {
                            return false
                        }

                        return model_item.modifyed
                    }

                    currentIndex: kernel_parameter_manager.curMachineIndex
                    Layout.preferredWidth: 200 * screenScaleFactor
                    Layout.preferredHeight: 482 * screenScaleFactor
                    spacing: 3 * screenScaleFactor
                    clip: true
                    model: kernel_parameter_manager.machineListModel
                    section.property: "default"
                    section.criteria: ViewSection.FullString

                    delegate: CusImglButton {
                        readonly property var modelItem: model

                        width: 200 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        btnRadius: 3
                        state: "wordsOnly"
                        checkable: true
                        textAlign: 0
                        borderWidth: 0
                        leftMargin: 10 * screenScaleFactor
                        font.pointSize: Constants.labelFontPointSize_10
                        bottonSelected: model.index === machine_view.currentIndex
                        allowTooltip: false
                        text: "%1%2".arg(model.modifyed ? "*" : "").arg(model.name)
                        defaultBtnBgColor: "transparent"
                        hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
                        selectedBtnBgColor: Constants.themeGreenColor //Constants.leftBtnBgColor_selected
                        btnTextColor: Constants.model_library_type_button_text_default_color
                        btnTextColor_d: Constants.model_library_type_button_text_checked_color
                        onClicked: {
                            machine_view.curItemIsModified = Qt.binding(function() {
                                return machine_view.currentItem.modelItem.modifyed;
                            });
                            if (machine_view.curItemIsModified) {
                                var receiver = {
                                };
                                receiver.type = 'machine';
                                receiver.profileName = kernel_parameter_manager.modifiedMachineName();
                                receiver.onCloseButtonClicked = function() {
                                };
                                receiver.onIgnore = function() {
                                    kernel_parameter_manager.abandonMachine();
                                    machine_view.currentIndex = model.index;
                                    machine = model.object;
                                };
                                receiver.onSaved = function(tindex) {
                                    machine_view.currentIndex = tindex;
                                };
                                idAllMenuDialog.requestMenuDialog(receiver, "saveParemDialog");
                            } else {
                                machine_view.currentIndex = model.index;
                                machine = model.object;
                            }
                        }
                    }

                    section.delegate: Item {
                        required property bool section

                        width: parent.width
                        height: titleText.height * 2

                        Text {
                            id: titleText

                            text: "%1".arg(qsTranslate("CXComboBox", section ? "Default configuration" : "User configuration"))
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 0
                            font.bold: true
                            color: Constants.model_library_type_button_text_default_color
                        }

                    }

                }

                Item {
                    Layout.fillWidth: true
                }

                CustomTabView {
                    id: tab_view

                    Layout.preferredWidth: 561 * screenScaleFactor
                    Layout.preferredHeight: 482 * screenScaleFactor
                    titleReverse: true

                    Repeater {
                        id: tap_view_repeater

                        model: kernel_ui_parameter.machineTreeModel

                        delegate: CustomTabViewItem {
                            id: basic_param_view

                            readonly property var treeView: basic_param_view_loader.item

                            title: {
                                const label = model.model_node.label
                                const array = label.split("_")
                                if (array.length == 2 && array[0] == "Extruder") {
                                    return "%1 %2".arg(qsTranslate("ParameterComponent", array[0]))
                                    .arg(array[1])
                                }

                                return qsTranslate("ParameterComponent", label)
                            }

                            enabled: {
                                const index_regex = /extruder_([1-9][0-9]*)/
                                const match_result = model.model_node.uid.match(index_regex)
                                if (!match_result) {
                                    return true
                                }

                                const index = parseInt(match_result[1]) - 1
                                return extruders.length > index
                            }

                            Component.onCompleted: function() {
                                if (model.index === 0) {
                                    tab_view.defaultPanel = this
                                    tab_view.currentPanel = this
                                } else if (model.index === 1) {
                                    tab_view.currentPanel = this
                                    Qt.callLater(_ => tab_view.currentPanel = tab_view.defaultPanel)
                                }
                            }

                            Loader {
                                id: basic_param_view_loader
                                property var itemModel: model
                                readonly property int currentLevel: advance_button.currentLevel

                                anchors.fill: parent
                                sourceComponent: basic_param_view_com
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10 * screenScaleFactor

                Item {
                    Layout.fillWidth: true
                }

                BasicButton {
                    id: saveBtn

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    enabled: {
                        let data_model = machine ? machine.dataModel : null
                        if (data_model && data_model.settingsModifyed) {
                            return true
                        }

                        for (let extruder of root.extruders) {
                            data_model = extruder ? extruder.dataModel : null
                            if (data_model && data_model.settingsModifyed) {
                                return true
                            }
                        }

                        return false
                    }
                    text: qsTranslate("ExpertParamPanel", qsTr("Save"))
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        if (kernel_parameter_manager.isMachineModified()) {
                            var receiver = {
                            };
                            receiver.type = 'machine';
                            receiver.onSave = function() {
                                printMachineModel.refresh();
                            };
                            receiver.onSave = function(index) {
                                if (index > 0)
                                    machine_view.currentIndex = index;

                            };
                            idAllMenuDialog.requestMenuDialog(receiver, 'saveParamSecondDialog');
                        }
                    }
                }

                BasicButton {
                    id: resetBtn

                    function exec() {
                        machine.reset();
                    }

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    enabled: saveBtn.enabled
                    text: qsTranslate("ExpertParamPanel", qsTr("Reset"))
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        warningDlg.msgText = qsTr("Do you want to reset the selected printer parameters?");
                        warningDlg.receiver = this;
                        warningDlg.visible = true;
                    }
                }

                BasicButton {
                    id: createBtn

                    function exec() {
                        machine.reset();
                    }

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    text: qsTranslate("ExpertParamPanel", qsTr("Create"))
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        addPrinterStep.visible = true;
                    }
                }

                BasicButton {
                    id: deleteBtn

                    function exec() {
                        kernel_parameter_manager.removeMachine(machine);
                        machine = Qt.binding(_ => kernel_parameter_manager.currentMachineObject);
                        machine_view.currentIndex = Qt.binding(_ => kernel_parameter_manager.curMachineIndex);
                        if (!machine) {
                            root.close();
                        }
                    }

                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    text: qsTranslate("ExpertParamPanel", qsTr("Delete"))
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        warningDlg.msgText = qsTranslate("ExpertParamPanel", "Do you want to delete the selected printer preset") + "?"
                        warningDlg.receiver = this;
                        warningDlg.visible = true;
                    }
                }

                BasicButton {
                    //warningDlg.msgText = qsTr("Do you want to reset the selected printer parameters?");
                    //warningDlg.receiver = this;
                    //warningDlg.visible = true;

                    id: importBtn

                    function exec() {
                        machine.reset();
                    }

                    enabled: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    text: qsTranslate("ExpertParamPanel", qsTr("Import"))
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        actionCommands.getOpt("Import SubMenu").getOpt("Import Preset").execute();
                    }
                }

                BasicButton {
                    id: exportBtn
                    visible: false
                    enabled: false
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    text: qsTranslate("ExpertParamPanel", qsTr("Export"))
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

            }

        }

    }

}
