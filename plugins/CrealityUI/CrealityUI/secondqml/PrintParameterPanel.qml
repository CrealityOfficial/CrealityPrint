import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs 1.3 as Dialogs
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import QtGraphicalEffects 1.15

import ".."
import "../components"
import "../qml"
import "../secondqml"
import "../secondqml/printer"

ColumnLayout {
    id: root

    readonly property real layoutMargins: 10 * screenScaleFactor
    required property var processGroupView  // @see process_group_list_view in MainWindow2.qml
    property alias objectTitle: objectPanel
    property alias configLayout: config_layout

    readonly property var currentMachine: kernel_parameter_manager.currentMachineObject
    readonly property var currentProfile: currentMachine ? currentMachine.currentProfileObject : null

    property int curMachineExtruderCount: 1
    property var extruder0
    property var extruder1
    property string extruder0Material: "PLA"
    property string extruder1Material: "PLA"

    property var modifiedProcess

    function checkParam() {}

    function showAddMaterial(extruderIndex, materialName, extruderItem) {
        addMaterialDlg.eItem = extruderItem;
        addMaterialDlg.printMaterialsModel = kernel_parameter_manager.currentMachineObject.materialsModelProxy();
        addMaterialDlg.parent = standaloneWindow;
        addMaterialDlg.startShowMaterialDialog("addState", extruderIndex, materialName, currentMachine);
    }

    function showEditMaterial(extruderIndex, materialName) {
        console.log("extruderIndex = ", extruderIndex, " materialName = ", materialName, "currentMachine = ", currentMachine);
        manageMaterialDlg.startShowMaterialDialog("manageState", extruderIndex, materialName, currentMachine);
    }

    function showEditMaterialDlg() {
        manageMaterialDlg.startShowMaterialDialog("manageState", 0, extruder1Material, currentMachine);
    }

    function showMaterialManage() {
        if (!materialManagerLoader.active)
            materialManagerLoader.active = true;

        materialManagerLoader.item.focusedMaterialIndex = 0;
        materialManagerLoader.item.focusedProcessIndex = process_combo_box.currentIndex;
        materialManagerLoader.item.materialListVisible = true;
        materialManagerLoader.item.show();
    }

    function showEditProfile(_addOrEdit) {
        idParameterProfile.startEditProfile(currentMachine, _addOrEdit, currentMachine.extruderCount());
    }

    function openModelSettingView() {
        if (global_object_switch.leftChecked) {
            global_object_switch.toggle()
        }
    }

    function openProcessSettingView() {
        if (global_object_switch.rightChecked) {
            global_object_switch.toggle()
        }
    }

    function highlightTreeNode(tree_node) {
        if (!tree_node) {
            return
        }

        const type_view = processGroupView
        const type_repeater = slice_config_repeater

        // 找到 tree_node 所属 一级分类节点 (type_node)
        for (var type_index = 0; type_index < type_repeater.count; ++type_index) {
            const type_node = type_repeater.model.get(type_index)
            if (tree_node !== type_node.getChildNode(tree_node.uid, true)) {
                continue
            }

            // 切换到 tree_node 所属 type_node 对应的页面
            type_view.currentIndex = type_index

            // 切换页面后续的 UI 渲染走事件系统, 这里需要异步处理后续操作
            Qt.callLater(_ => {
                // 定位到 tree_node 对应组件处
                const tree_view = type_repeater.itemAt(type_index).item
                const tree_delegate = tree_view.positionDelegate(tree_node, ListView.Contain)

                // 高亮 tree_delegate
                tree_delegate.object.uiModel.requestHighlight()
            })

            return
        }
    }

    function highlightGlobalObjectSwitch() {
        global_object_animation.start()
    }

    function initializePlateSetting() {
        plate_config_layout.initialize()
    }

    function checkedPreModify() {
        return process_combo_box.model
        .get(root.currentMachine.curProfileIndex)
        .object
        .dataModel
        .settingsModifyed
    }

    function saveModify() {
        let currentIndex = process_combo_box.currentIndex
        root.modifiedProcess = root.currentMachine.profileObject(root.currentMachine.curProfileIndex);
        let receiver = {
        };
        receiver.isDefault = true;
        receiver.type = "process";
        receiver.profileName = currentMachine.modifiedProcessObject.uniqueName();
        receiver.onTransfer = function() {
            currentMachine.transferProfile(currentMachine.profileObject(root.currentMachine.curProfileIndex), currentMachine.profileObject(currentIndex));
            if (root.currentMachine.curProfileIndex != currentIndex)
                root.currentMachine.setCurrentProfile(currentIndex);

        };
        receiver.taskQueue = function(){
            root.currentMachine.setCurrentProfile(currentIndex)
        }

        receiver.onIgnore = function() {
            modifiedProcess.reset();
            if (root.currentMachine.curProfileIndex != currentIndex)
            {
                root.currentMachine.setCurrentProfile(currentIndex);
            }

        };
        receiver.onSave = () => {
            currentMachine.saveCurrentProfile();
            if (root.currentMachine.curProfileIndex != currentIndex)
            root.currentMachine.setCurrentProfile(currentIndex);

        };
        receiver.onCloseButtonClicked = () => {
            process_combo_box.currentIndex  = root.currentMachine.curProfileIndex;
        };
        idAllMenuDialog.requestMenuDialog(receiver, 'saveParemDialog');
    }

    Connections {
        function onCurMachineIndexChanged(curMachineIndex) {
            console.log("currentMachine === ", currentMachine);
            //extruder
            curMachineExtruderCount = currentMachine.extruderCount();
            //material
            if (curMachineExtruderCount > 0)
                extruder0 = currentMachine.extruderObject(0);

            if (curMachineExtruderCount > 1)
                extruder1 = currentMachine.extruderObject(1);

            plugin_info.updateMachineInfo();
        }

        target: kernel_parameter_manager
    }

    Dialogs.ColorDialog {
        id: colorDialog

        property var colorBtn
        property real itemIndex

        title: "Please choose a color"
        visible: false
        modality: Qt.ApplicationModal
        onAccepted: {
            colorBtn.setColor(colorDialog.color);
            colorBtn.color = colorDialog.color;
            colorBtn.setTextColor();
            kernel_model_list_data.setColor(-1, colorDialog.color, itemIndex);
            let colorList = kernel_parameter_manager.currentMachineObject.extrudersModel.colorList();
            currentMachine.setFilamentColor(colorList);
        }
        onVisibleChanged: {
            if (!visible)
                colorBtn.checked = false;

        }
    }

    BasicDialogV4 {
        id: paramCheck

        title: qsTr("Tips")
        maxBtnVis: false
        width: 600 * screenScaleFactor
        height: 380 * screenScaleFactor

        bdContentItem: Rectangle {
            property alias dataList: dataList

            color: Constants.themeColor_primary

            StyledLabel {
                id: topLabel

                color: Constants.manager_printer_label_color
                anchors.left: parent.left
                anchors.leftMargin: 41 * screenScaleFactor
                anchors.top: parent.top
                anchors.topMargin: 33 * screenScaleFactor
                text: qsTr("You have redefine some directives.") + "\n" + qsTr("Do you want to keep these changed settings after switching materials?") + "\n" + qsTr("Or you can discard it for changes and use the default parameter configuration.")
            }

            Rectangle {
                width: 517 * screenScaleFactor
                implicitHeight: 180 * screenScaleFactor
                clip: true
                border.width: 1 * screenScaleFactor
                border.color: Constants.dialogItemRectBgBorderColor
                radius: 5 * screenScaleFactor
                color: Constants.themeColor_primary
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: topLabel.bottom
                anchors.topMargin: 10

                //                anchors.bottom: parent.bottom
                //                anchors.bottomMargin: 70 * screenScaleFactor
                Row {
                    id: topTitle

                    spacing: 0

                    Rectangle {
                        width: 172 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        color: Constants.themeColor_primary
                        border.width: 1 * screenScaleFactor
                        border.color: Constants.dialogItemRectBgBorderColor

                        StyledLabel {
                            color: Constants.topBtnTextColor_normal
                            anchors.centerIn: parent
                            text: qsTr("Parameter setting items")
                        }

                    }

                    Rectangle {
                        width: 172 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        color: Constants.themeColor_primary
                        border.width: 1 * screenScaleFactor
                        border.color: Constants.dialogItemRectBgBorderColor

                        StyledLabel {
                            color: Constants.topBtnTextColor_normal
                            anchors.centerIn: parent
                            text: qsTr("Default parameter value")
                        }

                    }

                    Rectangle {
                        width: 172 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        color: Constants.themeColor_primary
                        border.width: 1 * screenScaleFactor
                        border.color: Constants.dialogItemRectBgBorderColor

                        StyledLabel {
                            color: Constants.topBtnTextColor_normal
                            anchors.centerIn: parent
                            text: qsTr("Current parameter value")
                        }

                    }

                }

                ListView {
                    id: dataList

                    clip: true
                    width: topTitle.width
                    height: 152 * screenScaleFactor
                    anchors.top: topTitle.bottom
                    anchors.horizontalCenter: topTitle.horizontalCenter

                    delegate: Row {
                        spacing: 0

                        Rectangle {
                            width: 172 * screenScaleFactor
                            height: 28 * screenScaleFactor
                            color: "transparent"
                            border.width: 1 * screenScaleFactor
                            border.color: Constants.dialogItemRectBgBorderColor

                            StyledLabel {
                                color: Constants.topBtnTextColor_normal
                                anchors.centerIn: parent
                                text: qsTranslate("ParameterComponent", modelData.getKeyStr())
                            }

                        }

                        Rectangle {
                            width: 172 * screenScaleFactor
                            height: 28 * screenScaleFactor
                            color: "transparent"
                            border.width: 1 * screenScaleFactor
                            border.color: Constants.dialogItemRectBgBorderColor

                            StyledLabel {
                                color: Constants.topBtnTextColor_normal
                                anchors.centerIn: parent
                                text: modelData.getDefaultValue()
                            }

                        }

                        Rectangle {
                            width: 172 * screenScaleFactor
                            height: 28 * screenScaleFactor
                            color: "transparent"
                            border.width: 1 * screenScaleFactor
                            border.color: Constants.dialogItemRectBgBorderColor

                            StyledLabel {
                                color: Constants.topBtnTextColor_normal
                                anchors.centerIn: parent
                                text: modelData.getCurrentValue()
                            }

                        }

                    }

                    ScrollBar.vertical: ScrollBar {
                    }

                }

            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20 * screenScaleFactor
                spacing: 10 * screenScaleFactor

                BasicDialogButton {
                    id: keepBtn

                    text: qsTr("Keep Changes")
                    width: 120 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color
                    onSigButtonClicked: {
                        paramCheck.visible = false;
                    }
                }

                BasicDialogButton {
                    id: discardBtn

                    text: qsTr("Discard changes")
                    width: 120 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color
                    onSigButtonClicked: {
                        currentMachine.resetProfileModify();
                        paramCheck.visible = false;
                    }
                }

            }

        }

    }

    AddEditProfile {
        id: idAddProfile

        visible: false
        onSigAddProfile: {
            root.currentMachine.addProfile(newProfileName, templateObject);
            showEditProfile(0);
        }
    }

    FileDialog {
        id: inputDialog

        title: qsTr("Import")
        fileMode: FileDialog.OpenFile
        options: {
            if (Qt.platform.os == "linux")
                return FileDialog.DontUseNativeDialog;

        }
        nameFilters: ["Profiles (*.cxprofile)", "All files (*)"]
        onAccepted: {
            currentMachine.importProfileFromQmlIni(inputDialog.currentFile);
        }
    }

    FileDialog {
        id: saveDialog

        title: qsTr("Export")
        fileMode: FileDialog.SaveFile
        options: {
            if (Qt.platform.os == "linux")
                return FileDialog.DontUseNativeDialog;

        }
        defaultSuffix: "cxprofile"
        nameFilters: ["Profiles (*.cxprofile)", "All files (*)"]
        onAccepted: {
            currentMachine.exportProfileFromQmlIni(saveDialog.currentFile);
        }
    }

    ParameterProfilePanel {
        id: idParameterProfile

        visible: false
        onRemoveProfile: {
            currentMachine.removeProfile(profileObject);
        }
        onVisibleChanged: {
            if (!visible)
                currentMachine.profilesModel().notifyReset();

        }
    }

    UploadMessageDlg {
        id: warningDlg

        parent: standaloneWindow

        property var callback: function() {}

        onSigOkButtonClicked: {
            if (callback) {
                callback()
            }
            warningDlg.visible = false
        }
    }

    ComparePresets {
        id: comparePresets
    }

    AddMaterialDlg {
        id: addMaterialDlg

        property var eItem: null
    }

    ManageMaterialDlg {
        id: manageMaterialDlg
    }

    Loader {
        id: materialManagerLoader

        active: false

        sourceComponent: MaterialManagerPanel_New {
        }

    }

    Loader {
        id: expert_process_setting_loader

        active: false

        sourceComponent: ExpertParamPanel {
            currentProfile: root.currentProfile
        }

    }

    Loader {
        id: popular_process_setting_loader

        active: false

        sourceComponent: ProfileUiParameterDialog {
            treeModel: kernel_ui_parameter.processTreeModel
            onLeafNodeInitialized: function(delegate, model) {
                model.levelCustomizedChanged.connect((_) => {
                                                         delegate.checkBox.checked = model.levelCustomized;
                                                     });
                this.init.connect((_) => {
                                      delegate.checkBox.checked = model.levelCustomized;
                                  });
                this.reset.connect((_) => {
                                       delegate.checkBox.checked = false;
                                   });
                this.save.connect((_) => {
                                      model.levelCustomized = delegate.checkBox.checked;
                                  });
                delegate.checkBox.checked = model.levelCustomized;
            }
            onSave: function() {
                Qt.callLater((_) => {
                                 kernel_ui_parameter.saveProcessCustomizedKeys();
                             });
            }
        }

    }

    FlushVolumeDialog {
        id: flushVolumeDialog
    }

    SyncMaterial {
        id: syncMaterialDialog

        visible: false
    }

    UploadMessageDlg {
        id: nosyncMaterial

        property var receiver

        parent: standaloneWindow
        objectName: "tempWarning"
        visible: false
        messageType: 0
        cancelBtnVisible: false
        msgText: qsTr("No CFS material found. Please select the printer on the 'Device' page and the CFS information will be loaded.")
        onSigOkButtonClicked: nosyncMaterial.visible = false
    }

    UploadMessageDlg {
        id: removeExtruderWarning

        property var materialIndex

        function active(index) {
            materialIndex = index;
            visible = true;
        }

        parent: standaloneWindow
        visible: false
        messageType: 0
        cancelBtnVisible: true
        okBtnText: qsTr("Discard")
        cancelBtnText: qsTr("Inspect")
        onMaterialIndexChanged: {
            let msg = qsTr("Material %1 is referenced by models. Deleting it will discard corresponding paint data on the models. Are you sure you want to discard?");
            msg = msg.replace("%1", materialIndex + 1);
            msgText = msg;
        }
        onSigOkButtonClicked: {
            kernel_kernel.discardMaterial(materialIndex);
            nosyncMaterial.visible = false;
            currentMachine.removeExtruder();
            let colorList = kernel_parameter_manager.currentMachineObject.extrudersModel.colorList();
            currentMachine.setFilamentColor(colorList);
        }
        onSigCancelButtonClicked: {
            nosyncMaterial.visible = false;
            kernel_kernel.selectModelByMaterial(materialIndex);
        }
    }

    spacing: 15 * screenScaleFactor

    // material panel
    PanelTitle {
        id: materialPanel

        property bool isExpend: true

        Layout.fillWidth: true
        Layout.minimumHeight: 18 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        Layout.topMargin: root.layoutMargins
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        title: qsTr("Filament")
        image: Constants.currentTheme
               ? "qrc:/UI/photo/rightDrawer/update/material_light_default.svg"
               : "qrc:/UI/photo/rightDrawer/materialLogo_d.svg"

        MouseArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            onClicked: {
                materialPanel.isExpend = !materialPanel.isExpend
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            visible: colorRecFlow.itemCount > 1
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            btnRadius: 5 * screenScaleFactor
            borderWidth: (hovered ? 1 : 0) * screenScaleFactor
            hoverBorderColor: Constants.themeGreenColor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/flush_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/flush_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            cusTooltip.text: qsTr("Flushing volumes")
            cusTooltip.position: BasicTooltip.Position.BOTTOM

            onClicked: {
                flushVolumeDialog.visible = true
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            btnRadius: 5 * screenScaleFactor
            borderWidth: (hovered ? 1 : 0) * screenScaleFactor
            hoverBorderColor: Constants.themeGreenColor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/addMaterial_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/addMaterial_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            cusTooltip.text: qsTr("Add")
            cusTooltip.position: BasicTooltip.Position.BOTTOM

            onClicked: {
                if (colorRecFlow.itemCount >= 16) {
                    return
                }

                currentMachine.addExtruder()
                const colorList = kernel_parameter_manager.currentMachineObject
                .extrudersModel
                .colorList()
                currentMachine.setFilamentColor(colorList)
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            btnRadius: 5 * screenScaleFactor
            borderWidth: (hovered ? 1 : 0) * screenScaleFactor
            hoverBorderColor: Constants.themeGreenColor
            defaultBtnBgColor:"transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/delMaterial_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/delMaterial_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            cusTooltip.text: qsTr("Delete")
            cusTooltip.position: BasicTooltip.Position.BOTTOM

            onClicked: {
                if (colorRecFlow.itemCount == 1) {
                    return
                }

                const colorList = kernel_parameter_manager.currentMachineObject
                .extrudersModel
                .colorList()
                const removeIndex = colorList.length - 1

                if (kernel_kernel.isMaterialUsed(removeIndex)) {
                    // check colors
                    removeExtruderWarning.active(removeIndex)
                } else {
                    currentMachine.removeExtruder()
                    currentMachine.setFilamentColor(colorList)
                }
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            btnRadius: 5 * screenScaleFactor
            borderWidth: (hovered ? 1 : 0) * screenScaleFactor
            hoverBorderColor: Constants.themeGreenColor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            enabledIconSource:Constants.currentTheme
                              ? "qrc:/UI/photo/rightDrawer/update/sync_light_default.svg"
                              : "qrc:/UI/photo/rightDrawer/update/sync_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            cusTooltip.text: qsTr("Sync Filament")
            cusTooltip.position: BasicTooltip.Position.BOTTOM

            onClicked: {
                if (printerListModel.cSourceModel.hasDeviceOnline()) {
                    syncMaterialDialog.visible = true
                } else {
                    nosyncMaterial.visible = true
                }
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            enabled: !currentMachine.isImportedMachine()
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            btnRadius: 5 * screenScaleFactor
            borderWidth: (hovered ? 1 : 0) * screenScaleFactor
            hoverBorderColor: Constants.themeGreenColor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            disabledIconSource: "qrc:/UI/photo/rightDrawer/update/setting_light_disable.svg"
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/setting_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/setting_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            cusTooltip.text: qsTr("Edit Filaments")
            cusTooltip.position: BasicTooltip.Position.BOTTOM

            onClicked: {
                showAddMaterial(0, "Material", this)
            }
        }

        CusImglButton {
            readonly property string fold: Constants.currentTheme
                                           ? "qrc:/UI/photo/rightDrawer/update/fold2_light_default.svg"
                                           : "qrc:/UI/photo/rightDrawer/fold.svg"
            readonly property string expand: Constants.currentTheme
                                             ? "qrc:/UI/photo/rightDrawer/update/expand2_light_default.svg"
                                             : "qrc:/UI/photo/rightDrawer/expend.svg"
            readonly property string imgSource: materialPanel.isExpend ? fold : expand

            Layout.minimumWidth: 14 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            borderWidth: 0
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 5 * screenScaleFactor
            imgHeight: 7 * screenScaleFactor
            enabledIconSource: imgSource
            highLightIconSource: imgSource
            pressedIconSource: imgSource

            onClicked: {
                materialPanel.isExpend = !materialPanel.isExpend
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    parent.clicked()
                }
            }
        }
    }

    ExtruderMutilColor {
        id: colorRecFlow

        Layout.fillWidth: true
        Layout.topMargin: -3 * screenScaleFactor
        Layout.bottomMargin: -3 * screenScaleFactor
        Layout.leftMargin: root.layoutMargins - 3 * screenScaleFactor
        Layout.rightMargin: root.layoutMargins - 3 * screenScaleFactor

        visible: materialPanel.isExpend && root.curMachineExtruderCount == 1
        currentMachine: root.currentMachine

        onExtruder0MaterialChanged: {
            root.extruder0Material = this.extruder0Material
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.minimumHeight: 1 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        color: Constants.right_panel_menu_split_line_color
    }

    // object panel
    PanelTitle {
        id: objectPanel

        property bool isExpend: true

        Layout.fillWidth: true
        Layout.minimumHeight: 18 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        title: qsTr("Object")
        image: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/object_light_default.svg"
                                      : "qrc:/UI/photo/rightDrawer/object.svg"

        MouseArea {
            Layout.fillWidth: true
            Layout.fillHeight: true

            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor

            onClicked: {
                objectPanel.isExpend = !objectPanel.isExpend
            }
        }

        CusImglButton {
            readonly property string fold: Constants.currentTheme
                                           ? "qrc:/UI/photo/rightDrawer/update/fold2_light_default.svg"
                                           : "qrc:/UI/photo/rightDrawer/fold.svg"
            readonly property string expand: Constants.currentTheme
                                             ? "qrc:/UI/photo/rightDrawer/update/expand2_light_default.svg"
                                             : "qrc:/UI/photo/rightDrawer/expend.svg"
            readonly property string imgSource: objectPanel.isExpend ? fold : expand

            Layout.minimumWidth: 14 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor

            state: "imgOnly"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight

            borderWidth: 0
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            imgWidth: 5 * screenScaleFactor
            imgHeight: 7 * screenScaleFactor
            enabledIconSource: imgSource
            highLightIconSource: imgSource
            pressedIconSource: imgSource

            onClicked: {
                objectPanel.isExpend = !objectPanel.isExpend
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    parent.clicked()
                }
            }
        }
    }

    Rectangle {
        id: objectRec
        layer.enabled: true
        layer.effect: OpacityMask{
            maskSource: Rectangle{
                width: objectRec.width
                height: objectRec.height
                radius: 5* screenScaleFactor
            }
        }
        Layout.fillWidth: true
        Layout.minimumHeight: {
            if (screenScaleFactor > 1) {
                return 96 * screenScaleFactor
            }

            if (!frameLessView.isMax && !frameLessView.isFull) {
                return 96 * screenScaleFactor
            }

            return 176 * screenScaleFactor
        }
        Layout.maximumHeight: Layout.minimumHeight
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        visible: objectPanel.isExpend
        radius: 5 * screenScaleFactor
        border.color: Constants.right_panel_border_default_color
        border.width: 1 * screenScaleFactor
        color: "transparent"

        CusTreeView{
            id: cusTreeViewObj
            objectName: "cusTreeViewObj"
            model: kernel_objects_list_data.objectsTreeModel()
            anchors.fill: parent
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.minimumHeight: 1 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        color: Constants.right_panel_menu_split_line_color
    }

    // process panel
    PanelTitle {
        id: process_title

        Layout.fillWidth: true
        Layout.minimumHeight: 18 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        title: qsTr("Process")
        image: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/profile_light_default.svg"
                                      : "qrc:/UI/photo/rightDrawer/paramPanel_d.svg"

        Rectangle {
            Layout.minimumWidth: 1 * screenScaleFactor
            Layout.maximumWidth: Layout.minimumWidth
            Layout.fillHeight: true
            Layout.margins: 5 * screenScaleFactor
            color: Constants.right_panel_border_default_color
        }

        Switch {
            id: global_object_switch

            readonly property bool leftChecked: !checked
            readonly property bool rightChecked: checked
            property alias leftLabel: global_object_left_switch
            property alias rightLabel: global_object_right_switch

            Layout.minimumWidth: contentItem.implicitWidth
            Layout.preferredWidth: Layout.minimumWidth + 16 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor

            onRightCheckedChanged: {
                if (rightChecked) {
                    objectPanel.isExpend = true
                }
            }

            Image {
                id: global_object_arrow

                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.right
                anchors.leftMargin: 40 * screenScaleFactor
                width: 24 * screenScaleFactor
                height: 16 * screenScaleFactor
                visible: false
                source: "qrc:/UI/photo/arrow.svg"
                sourceSize.width: width
                sourceSize.height: height
                fillMode: Image.PreserveAspectFit

                SequentialAnimation {
                    id: global_object_animation

                    loops: 3

                    NumberAnimation {
                        target: global_object_arrow
                        property: "anchors.leftMargin"
                        to: 40 * screenScaleFactor
                    }

                    PropertyAnimation {
                        target: global_object_arrow
                        property: "visible"
                        to: true
                    }

                    NumberAnimation {
                        target: global_object_arrow
                        property: "anchors.leftMargin"
                        to: 10 * screenScaleFactor
                        duration: 100
                    }

                    PropertyAnimation {
                        target: global_object_arrow
                        property: "visible"
                        to: false
                    }

                }

            }

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.margins: 1 * screenScaleFactor
                spacing: 0
                implicitWidth: global_object_left_switch.contentWidth + global_object_right_switch.contentWidth + 2 * screenScaleFactor

                Label {
                    id: global_object_left_switch

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    leftPadding:6* screenScaleFactor
                    rightPadding:6* screenScaleFactor
                    topPadding:4* screenScaleFactor
                    bottomPadding:4* screenScaleFactor
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    color: parent.parent.leftChecked ? Constants.switch_indicator_text_color : Constants.switch_background_text_color
                    text: qsTr("Global")
                }

                Label {
                    id: global_object_right_switch

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    leftPadding:6* screenScaleFactor
                    rightPadding:6* screenScaleFactor
                    topPadding:4* screenScaleFactor
                    bottomPadding:4* screenScaleFactor
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    color: parent.parent.rightChecked ? "#FFFFFF" : Constants.left_model_list_title_text_color
                    text: qsTr("Object")
                }

            }

            background: Rectangle {
                readonly property real borderWidth: 1 * screenScaleFactor

                radius: 5 * screenScaleFactor
                color: Constants.switch_background_color
                border.width: borderWidth
                border.color: Constants.switch_border_color

                Rectangle {
                    readonly property real borderWidth: parent.borderWidth
                    readonly property var switcher: parent.parent

                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: borderWidth
                    x: switcher.leftChecked ? borderWidth : borderWidth + switcher.leftLabel.width
                    width: switcher.leftChecked ? switcher.leftLabel.width : switcher.rightLabel.width
                    radius: parent.radius
                    color: Constants.switch_indicator_color

                    Rectangle {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        x: parent.switcher.leftChecked ? parent.width - width : 0
                        width: parent.radius
                        color: parent.color
                    }

                }

            }

            indicator: Item {
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            id: advance_button

            readonly property int currentLevel: checked ? 1 : 2

            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.maximumWidth: Layout.minimumWidth
            Layout.minimumHeight: Layout.minimumWidth
            Layout.maximumHeight: Layout.minimumHeight

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
                color: "transparent"
                radius: 5 * screenScaleFactor
                border.width: parent.hovered ? 1 * screenScaleFactor : 0
                border.color: parent.hovered ? Constants.themeGreenColor : "transparent"
            }

            contentItem: Item {
                anchors.fill: parent
                Image {
                    anchors.centerIn: parent
                    width: 14 * screenScaleFactor
                    height: width
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                    source: {
                        if (Constants.currentTheme === 0) {
                            if (!advance_button.checked) {
                                "qrc:/UI/photo/rightDrawer/advanced_process_dark.svg"
                            } else {
                                "qrc:/UI/photo/rightDrawer/advanced_process_dark_checked.svg"
                            }
                        } else {
                            if (!advance_button.checked) {
                                "qrc:/UI/photo/rightDrawer/advanced_process_light.svg"
                            } else {
                                "qrc:/UI/photo/rightDrawer/advanced_process_light_checked.svg"
                            }
                        }
                    }
                }
            }

            BasicTooltip {
                visible: parent.hovered
                text: parent.checked ? qsTr("Click to close advanced parameters")
                                     : qsTr("Click to open advanced parameters")
            }
        }

        CusImglButton {
            visible: false
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            enabledIconSource: Constants.right_panel_process_custom_image
            highLightIconSource: Constants.right_panel_process_custom_image
            pressedIconSource: Constants.right_panel_process_custom_image
            cusTooltip.text: qsTranslate("ProfileUiParameterDialog", "title")
            btnRadius: 5
            borderWidth: hovered?1:0
            hoverBorderColor:Constants.themeGreenColor
            onClicked: {
                if (!popular_process_setting_loader.active)
                    popular_process_setting_loader.active = true;

                popular_process_setting_loader.item.show();
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: Layout.minimumWidth
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            enabled: {
                if (global_object_switch.leftChecked) {
                    return process_combo_box.currnetModelModifyed
                }

                if (config_layout.currentIndex === 0) {
                    return kernel_ui_parameter.processOverrideContext.dataModel.settingsModifyed
                }

                return plate_type_reset_button.enabled
                        || print_sequence_reset_button.enabled
                        || filament_sequence_reset_button.enabled
                        || filament_sequence_string_reset_button.enabled
            }
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor:"transparent"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/update_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/update_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            disabledIconSource: Constants.right_panel_reset_disabled_image
            cusTooltip.text:qsTr("Reset")
            btnRadius: 5
            borderWidth: hovered?1:0
            hoverBorderColor: Constants.themeGreenColor
            onClicked: {
                if (global_object_switch.leftChecked) {
                    warningDlg.callback = _ => {
                        currentProfile.reset()
                    }
                } else if (config_layout.currentIndex === 0) {
                    warningDlg.callback = _ => {
                        kernel_ui_parameter.processOverrideContext.dataModel.reset()
                    }
                } else {
                    warningDlg.callback = _ => {
                        plate_type_reset_button.released()
                        print_sequence_reset_button.released()
                        filament_sequence_reset_button.released()
                        filament_sequence_string_reset_button.released()
                    }
                }

                warningDlg.msgText = qsTranslate("ParameterProfilePanel", "Do you want to reset the selected slice configuration parameters?");
                warningDlg.visible = true;
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: Layout.minimumWidth
            imgWidth: 14 * screenScaleFactor
            imgHeight: 14 * screenScaleFactor
            enabled: process_combo_box.currnetModelModifyed
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/save_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/save_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            disabledIconSource: Constants.right_panel_save_disabled_image
            cusTooltip.text:qsTr("Save as")
            btnRadius: 5
            borderWidth: hovered?1:0
            hoverBorderColor:Constants.themeGreenColor
            onClicked: {
                var receiver = {
                };
                receiver.type = 'process';
                receiver.onOK = function() {
                    printMachineModel.refresh();
                };
                idAllMenuDialog.requestMenuDialog(receiver, 'saveParamSecondDialog');
            }
        }

        CusImglButton {
            Layout.minimumWidth: 24 * screenScaleFactor
            Layout.minimumHeight: 24 * screenScaleFactor
            imgWidth: 14 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            enabledIconSource: Constants.currentTheme
                               ? "qrc:/UI/photo/rightDrawer/update/compare_light_default.svg"
                               : "qrc:/UI/photo/rightDrawer/update/compare_dark_default.svg"
            highLightIconSource: enabledIconSource
            pressedIconSource: enabledIconSource
            cusTooltip.text: qsTr("Compare Presets")
            btnRadius: 5
            borderWidth: hovered?1:0
            hoverBorderColor:Constants.themeGreenColor
            onClicked: {
                comparePresets.visible = true
            }
        }
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.minimumHeight: 24 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        visible: config_layout.currentIndex === 0
        currentIndex: search_button.checked || global_object_switch.rightChecked ? 1 : 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            CustomComboBox {
                id: process_combo_box

                property bool currnetModelModifyed: process_config_layout.dataModel.settingsModifyed
                property var currentProfileObj: root.currentMachine.currentProfileObject

                Layout.fillWidth: true
                Layout.preferredHeight: 24 * screenScaleFactor
                model: root.currentMachine.profilesModel
                textRole: "name"
                displayText: "%1%2".arg(currnetModelModifyed ? "*" : "").arg(currentProfileObj.name)
                currentIndex: root.currentMachine.curProfileIndex

                Connections {
                    target: root.currentMachine.profilesModel

                    function onProfileInserted() {
                        process_combo_box.currentIndex = -1
                        process_combo_box.currentIndex = Qt.binding(_ => root.currentMachine.curProfileIndex)
                    }

                    function onProfileRemoved() {
                        process_combo_box.currentIndex = -1
                        process_combo_box.currentIndex = Qt.binding(_ => root.currentMachine.curProfileIndex)
                    }
                }

                onActivated: {
                    if (root.checkedPreModify()) {
                        root.saveModify()
                    } else {
                        root.currentMachine.setCurrentProfile(currentIndex)
                    }
                }

                Connections {
                    target: root
                    function onCurrentMachineChanged() {
                        process_combo_box.model = Qt.binding(_ => root.currentMachine.profilesModel)
                        process_combo_box.currentIndex = Qt.binding(_ => root.currentMachine.curProfileIndex)
                    }
                }

                view.section.property: "default"
                view.section.delegate: Text {
                    required property bool section

                    width: process_combo_box.implicitDelegateWidth
                    height: process_combo_box.implicitDelegateHeight
                    topPadding: process_combo_box.textTopPadding
                    bottomPadding: process_combo_box.textBottomPadding
                    leftPadding: process_combo_box.textLeftPadding
                    rightPadding: process_combo_box.textRightPadding
                    verticalAlignment: process_combo_box.textVerticalAlignment
                    horizontalAlignment: process_combo_box.textHorizontalAlignment
                    font: Constants.font
                    color: Constants.right_panel_text_default_color
                    text: "------ %1 ------".arg(qsTranslate("CXComboBox", section ? "Default configuration" : "User configuration"))
                }

                delegate: ItemDelegate {
                    readonly property var modelItem: model

                    implicitWidth: process_combo_box.implicitDelegateWidth
                    implicitHeight: process_combo_box.implicitDelegateHeight

                    background: Rectangle {
                        radius: 5 * screenScaleFactor
                        color: parent.hovered ? Constants.right_panel_item_checked_color : "transparent"
                    }

                    contentItem: RowLayout {
                        anchors.fill: parent

                        Text {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            topPadding: process_combo_box.textTopPadding
                            bottomPadding: process_combo_box.textBottomPadding
                            leftPadding: process_combo_box.textLeftPadding
                            rightPadding: process_combo_box.textRightPadding
                            verticalAlignment: process_combo_box.textVerticalAlignment
                            horizontalAlignment: process_combo_box.textHorizontalAlignment

                            font: process_combo_box.textFont
                            elide: process_combo_box.textElide
                            color: parent.parent.hovered ? "#FFFFFF" : Constants.right_panel_text_default_color
                            text: "%1%2".arg(model.modifyed ? "*" : "").arg(model.object.name)
                        }

                        Button {
                            id: process_edit_button

                            Layout.fillHeight: true
                            Layout.minimumWidth: height
                            Layout.maximumWidth: Layout.minimumWidth
                            Layout.margins: 1 * screenScaleFactor
                            visible: !model[process_combo_box.view.section.property] && parent.parent.hovered

                            onClicked: {
                                changeProfileName.profileObj = model.object
                                changeProfileName.defaultName = model.object.name
                                changeProfileName.open()
                            }

                            background: Rectangle {
                                radius: height / 2
                                color: process_edit_button.hovered ? "#FFFFFF" : "transparent"
                            }

                            contentItem: Item {
                                Image {
                                    anchors.centerIn: parent
                                    width: 14 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    source: process_edit_button.hovered ? Constants.right_panel_edit_hovered_image
                                                                        : Constants.right_panel_edit_image
                                }
                            }
                        }

                        Button {
                            id: process_delete_button

                            Layout.fillHeight: true
                            Layout.minimumWidth: height
                            Layout.maximumWidth: Layout.minimumWidth
                            Layout.margins: 1 * screenScaleFactor
                            visible: !model[process_combo_box.view.section.property] && parent.parent.hovered
                            onClicked: {
                                var receiver = {}
                                receiver.exec = function() {
                                    const machine = root.currentMachine
                                    machine.removeProfile(machine.profileObject(model.index))
                                    process_combo_box.currentIndex = -1
                                    process_combo_box.currentIndex = Qt.binding(_ => machine.curProfileIndex)
                                }
                                idAllMenuDialog.requestMenuDialog(receiver, "idDeletePreset");
                            }

                            background: Rectangle {
                                radius: height / 2
                                color: process_delete_button.hovered ? "#FFFFFF" : "transparent"
                            }

                            contentItem: Item {
                                Image {
                                    anchors.centerIn: parent
                                    width: 14 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    source: process_delete_button.hovered ? Constants.right_panel_delete_hovered_image
                                                                          : Constants.right_panel_delete_image
                                }
                            }
                        }
                    }
                }
            }

            CusImglButton {
                id: search_button

                Layout.preferredWidth: 24 * screenScaleFactor
                Layout.preferredHeight: 24 * screenScaleFactor
                imgWidth: 16 * screenScaleFactor
                imgHeight: 16 * screenScaleFactor
                defaultBtnBgColor: "transparent"
                hoveredBtnBgColor: "transparent"
                selectedBtnBgColor: "transparent"
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                enabledIconSource: Constants.currentTheme? "qrc:/UI/photo/rightDrawer/update/search_light_default.svg" : "qrc:/UI/photo/rightDrawer/searchBtn.svg"
                highLightIconSource: enabledIconSource
                pressedIconSource: enabledIconSource
                cusTooltip.text: qsTr("Search")
                btnRadius: 5
                borderWidth: hovered?1:0
                hoverBorderColor:Constants.themeGreenColor
                checkable: true
                checked: false
            }
        }

        SearchComboBox {
            id: search_combo_box

            property var searchContext: null

            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 5 * screenScaleFactor
            textRole: "model_text"
            maximumVisibleCount: 14
            indicatorVisible: global_object_switch.rightChecked
            placeholderText: qsTranslate("ParameterComponent", "Please enter a parameter name")

            onVisibleChanged: {
                if (visible && global_object_switch.leftChecked && search_button.checked) {
                    contentItem.forceActiveFocus()
                    popup.open()
                }
            }

            popup.onOpened: {
                const context = kernel_ui_parameter.generateSearchContext(
                                  kernel_ui_parameter.processTreeModel, "ParameterComponent")

                const proxy_model = context.proxyModel
                proxy_model.filterCaseSensitivity = Qt.CaseInsensitive
                proxy_model.dataModel = process_config_layout.dataModel
                // proxy_model.minimumLevel = Qt.binding(_ => process_level_combo_box.currentLevel)
                proxy_model.minimumLevel = Qt.binding(_ => advance_button.currentLevel)
                proxy_model.searchText = Qt.binding(_ => searchText)
                proxy_model.focusUid = ""

                searchContext = context
                model = proxy_model
            }

            popup.onClosed: {
                searchText = ""
                model = null
                searchContext = null
            }

            onActivated: {
                highlightTreeNode(searchContext.proxyModel.get(currentIndex).model_node)
                search_button.checked = false
            }

            onActiveFocusChanged: {
                if (!activeFocus) {
                    search_button.checked = false
                }
            }
        }
    }

    StackLayout {
        id: config_layout

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 0
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        currentIndex: {
            const global_state = global_object_switch.leftChecked
            const object_model_valid = kernel_ui_parameter.processOverrideContext.valid
            return global_state || object_model_valid ? 0 : 1
        }

        StackLayout {
            id: process_config_layout

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0

            currentIndex: processGroupView.currentIndex

            onCurrentIndexChanged: {
                parameter_component.hoveredItem = null
                parameter_component.focusedItem = null
            }

            ParameterComponent {
                id: parameter_component
                maximumHeight: 100 * screenScaleFactor
                hoverable: true
                focusable: true
            }

            readonly property var dataModel: {
                if (global_object_switch.leftChecked) {
                    return currentProfile ? currentProfile.dataModel : null
                }

                if (kernel_ui_parameter.processOverrideContext.valid) {
                    return kernel_ui_parameter.processOverrideContext.dataModel
                }

                return currentProfile ? currentProfile.dataModel : null
            }

            onDataModelChanged: {
                if (!dataModel) {
                    return
                }

                if (dataModel.dataType !== "model") {
                    return
                }

                if (advance_button.currentLevel !== 2) {
                    return
                }

                advance_button.toggle()
            }

            Repeater {
                id: slice_config_repeater
                model: kernel_ui_parameter.processTreeModel.childNodes
                delegate: Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0

                    asynchronous: true
                    active: model.index === process_config_layout.currentIndex
                         || standaloneWindow.initialized

                    Component.onCompleted: function() {
                        if (model.index === 1) {
                            process_config_layout.currentIndex = 1
                            Qt.callLater(() => {
                                process_config_layout.currentIndex = 0
                                process_config_layout.currentIndex = Qt.binding(() => processGroupView.currentIndex)
                            })
                        }
                    }

                    sourceComponent: FlattenTreeView {
                        id: slice_config_tree_view

                        ScrollBar.vertical: ScrollBar {
                            visible: standaloneWindow.initialized
                            policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                            onActiveChanged: parameter_component.closePopup()
                        }

                        rightMargin: 12 * screenScaleFactor

                        nodeFillWidth: true
                        forkNodeHeight: 40 * screenScaleFactor
                        leafNodeHeight: -1

                        treeModel: model_node.childNodes

                        forkNodeDelegate: ColumnLayout {
                            function showChildNode() {
                                fork_button.checked = true
                            }

                            visible: standaloneWindow.initialized

                            Item {
                                Layout.fillHeight: true
                            }

                            Button {
                                id: fork_button

                                Layout.fillWidth: true

                                focusPolicy: Qt.ClickFocus
                                checkable: true
                                checked: true

                                Component.onCompleted: {
                                    nodeModel.childNodeValid = Qt.binding(_ => checked)
                                }

                                background: Item {}

                                contentItem: RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Image {
                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                        fillMode: Image.PreserveAspectFit
                                        // source: nodeModel.modifyed ? nodeModel.modifyedIcon
                                        //                            : nodeModel.icon
                                        source: nodeModel.icon
                                    }

                                    Text {
                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                        horizontalAlignment: Qt.AlignLeft
                                        verticalAlignment: Qt.AlignVCenter
                                        font.family: Constants.labelFontFamily
                                        font.weight: Constants.labelFontWeight
                                        font.pointSize: Constants.labelFontPointSize_10
                                        color: nodeModel.modifyed
                                                ? Constants.parameter_text_modifyed_color
                                                : Constants.parameter_text_color
                                        text: qsTranslate("ParameterComponent", nodeModel.label)
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.minimumHeight: 1 * screenScaleFactor
                                        Layout.maximumHeight: Layout.minimumHeight
                                        Layout.alignment: Qt.AlignVCenter
                                        color: Constants.right_panel_border_default_color
                                    }

                                    Image {
                                        width: 12 * screenScaleFactor
                                        height: 6 * screenScaleFactor
                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                        horizontalAlignment: Qt.AlignRight
                                        verticalAlignment: Qt.AlignVCenter
                                        fillMode: Image.PreserveAspectFit
                                        source: fork_button.checked
                                                ? "qrc:/UI/photo/comboboxDown2_flip.png"
                                                : "qrc:/UI/photo/comboboxDown2.png"
                                    }
                                }

                                BasicTooltip {
                                    visible: parent.hovered && nodeModel.modifyed
                                    position: parameter_component.toolTipPosition
                                    delay: parameter_component.toopTipDelay
                                    timeout: -1

                                    textWrap: true
                                    textWidth: parameter_component.toolTipWidth
                                    textColor: Constants.parameter_text_modifyed_color
                                    text: nodeModel.modifyedTip
                                }
                            }
                        }

                        leafNodeDelegate: Loader {
                            readonly property var uiModel: nodeModel
                            readonly property var dataModel: {
                                const data_model = process_config_layout.dataModel
                                if (!data_model) {
                                    return null
                                }

                                const model_item = data_model.getDataItem(uiModel.key)
                                if (!model_item) {
                                    return null
                                }

                                model_item.uiModel = Qt.binding(_ => {
                                    if (status !== Loader.Ready) {
                                        return null
                                    }

                                    if (!model_item.uiModel || global_object_switch.leftChecked) {
                                        return model_item === dataModel ? uiModel : null
                                    }

                                    return model_item.uiModel
                                })

                                return model_item
                            }

                            readonly property bool vaild: {
                                // const level_valid = process_level_combo_box.customized
                                //         ? uiModel.levelCustomized
                                //         : uiModel.level >= process_level_combo_box.currentLevel
                                const level_valid = uiModel.level >= advance_button.currentLevel
                                if (!level_valid) {
                                    return false
                                }

                                const data_valid = dataModel && dataModel.enabled
                                if (!data_valid) {
                                    return false
                                }

                                const data_type = process_config_layout.dataModel.dataType
                                if (data_type === "model") {
                                    return dataModel.modelSettable
                                }

                                if (data_type === "model_group") {
                                    return dataModel.modelGroupSettable
                                }

                                return dataModel.globalSettable
                            }

                            active: dataModel
                            sourceComponent: parameter_component.parameter

                            Component.onCompleted: {
                                nodeModel.nodeValid = Qt.binding(_ => vaild)
                                uiModel.modifyed = Qt.binding(_ => dataModel.modifyed)
                                uiModel.translation = Qt.binding(_ => item.labelTranslation)
                            }
                        }
                    }

                    Canvas {
                        readonly property real radius: 5 * screenScaleFactor

                        anchors.fill: parent
                        anchors.leftMargin: -root.layoutMargins

                        contextType: "2d"

                        onPaint: {
                            const context = getContext("2d")
                            context.clearRect(0, 0, width, height)

                            context.beginPath()
                            context.moveTo(0, 0)
                            context.lineTo(width - radius, 0)
                            context.arcTo(width, 0, width, radius, radius, radius)
                            context.lineTo(width, height - radius)
                            context.arcTo(width, height, width - radius, height, radius, radius)
                            context.lineTo(0, height)
                            // context.lineTo(0, 0)
                            context.closePath()

                            context.strokeStyle = Constants.themeGreenColor
                            context.stroke()
                        }
                    }
                }
            }
        }

        // @see PrinterSettingsDialog.qml
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                Layout.fillWidth: true

                spacing: 10 * screenScaleFactor

                StyledLabel {
                    horizontalAlignment: Qt.AlignLeft
                    verticalAlignment: Qt.AlignVCenter
                    font.weight: Font.Bold
                    font.pointSize: Constants.labelFontPointSize_11
                    color: Constants.right_panel_text_default_color
                    text: qsTranslate("PrinterSettingsDialog", "Plate Settings")
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.minimumHeight: 1 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    color: Constants.right_panel_border_default_color
                }
            }

            GridLayout {
                id: plate_config_layout

                readonly property var printer: kernel_reusable_cache.printerManager.selectedPrinter
                property bool initializing: false

                function initialize() {
                    if (!printer) {
                        return
                    }

                    initializing = true;

                    plate_type_combo_box.currentIndex = (_ => {
                                                             switch (printer.parameter("curr_bed_type")) {
                                                                 case "High Temp Plate": return 1
                                                                 case "Textured PEI Plate": return 2
                                                                 case "Epoxy Resin Plate": return 3
                                                                 case "Cool Plate": return 4

                                                                 default: return 0
                                                             }
                                                         })()

                    print_sequence_combo_box.currentIndex = (_ => {
                                                                 switch (printer.parameter("print_sequence")) {
                                                                     case "by layer": return 1
                                                                     case "by object": return 2
                                                                     default: return 0
                                                                 }
                                                             })()

                    filament_sequence_combo_box.currentIndex = (_ => {
                                                                    switch (printer.parameter("custom")) {
                                                                        case "false": return 0
                                                                        case "true": return 1
                                                                        default: return 0
                                                                    }
                                                                })()

                    filament_sequence_editor.init(printer.parameter('first_layer_print_sequence'))

                    initializing = false
                }

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 24 * screenScaleFactor

                rows: 4
                columns: 3
                rowSpacing: 10 * screenScaleFactor
                columnSpacing: 10 * screenScaleFactor

                onPrinterChanged: {
                    initialize()
                }

                Connections {
                    target: plate_config_layout.printer.settingsObject

                    onSequenceChanged: {
                        filament_sequence_editor.init(plate_config_layout.printer.parameter('first_layer_print_sequence'))
                    }
                }

                Connections {
                    target: printer

                    function onParameterChanged() {
                        plate_config_layout.initialize()
                    }
                }

                Connections {
                    target: Constants

                    function onCurrentThemeChanged() {
                        plate_config_layout.initialize()
                    }
                }

                Label {
                    Layout.preferredWidth: 145 * screenScaleFactor
                    Layout.minimumWidth: contentWidth
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    color: plate_type_reset_button.enabled
                           ? Constants.parameter_text_modifyed_color
                           : Constants.parameter_text_color
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    text: qsTranslate("PrinterSettingsDialog", "Plate type") + ":"
                }

                Button {
                    id: plate_type_reset_button

                    Layout.minimumWidth: 20 * screenScaleFactor
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.minimumHeight: Layout.minimumWidth
                    Layout.maximumHeight: Layout.minimumHeight
                    Layout.alignment: Qt.AlignRight

                    enabled: plate_type_combo_box.currentIndex !== 0
                    opacity: enabled ? 100 : 0

                    onReleased: {
                        plate_type_combo_box.currentIndex = 0
                    }

                    background: Rectangle {
                        border.width: 0
                        radius: 5 * screenScaleFactor
                        color: parent.pressed ? Constants.leftBtnBgColor_selected
                                              : parent.hovered ? Constants.leftBtnBgColor_hovered
                                                               : "transparent"
                    }

                    contentItem: Item {
                        Image {
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: width
                            sourceSize.height: height
                            source: Constants.parameter_reset_button_image
                        }
                    }
                }

                CustomComboBox {
                    id: plate_type_combo_box

                    Layout.fillWidth: true
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    font.pointSize: Constants.labelFontPointSize_10

                    background: Rectangle {
                        radius: 5 * screenScaleFactor
                        color: {
                            if (plate_type_reset_button.enabled) {
                                return Constants.parameter_editer_modifyed_color
                            }

                            return "transparent"
                        }

                        border.width: 1 * screenScaleFactor
                        border.color: {
                            if (plate_type_reset_button.enabled) {
                                return Constants.right_panel_border_hovered_color
                            }

                            if (plate_type_combo_box.hovered) {
                                return Constants.right_panel_border_hovered_color
                            }

                            return Constants.right_panel_border_default_color
                        }
                    }

                    translater: source => {
                                    const context = source === "Same as Global Plate Type"
                                    ? "PrinterSettingsDialog"
                                    : "ParameterComponent"
                                    return qsTranslate(context, source)
                                }

                    model: [
                        "Same as Global Plate Type",
                        "Smooth PEI Plate / High Temp Plate",
                        "Textured PEI Plate",
                        "Epoxy Resin Plate",
                        "Cool Plate",
                    ]

                    onCurrentIndexChanged: {
                        if (plate_config_layout.initializing) {
                            return
                        }

                        plate_config_layout.printer.setParameter("curr_bed_type", (_ => {
                                                                                       switch (currentIndex) {
                                                                                           case 1: return "High Temp Plate"
                                                                                           case 2: return "Textured PEI Plate"
                                                                                           case 3: return "Epoxy Resin Plate"
                                                                                           case 4: return "Cool Plate"
                                                                                           default: return ""
                                                                                       }
                                                                                   })())
                    }
                }

                Label {
                    Layout.preferredWidth: 145 * screenScaleFactor
                    Layout.minimumWidth: contentWidth
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    color: print_sequence_reset_button.enabled
                           ? Constants.parameter_text_modifyed_color
                           : Constants.parameter_text_color
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    text: qsTranslate("PrinterSettingsDialog", "Print sequence") + ":"
                }

                Button {
                    id: print_sequence_reset_button

                    Layout.minimumWidth: 20 * screenScaleFactor
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.minimumHeight: Layout.minimumWidth
                    Layout.maximumHeight: Layout.minimumHeight
                    Layout.alignment: Qt.AlignRight

                    enabled: print_sequence_combo_box.currentIndex !== 0
                    opacity: enabled ? 100 : 0

                    onReleased: {
                        print_sequence_combo_box.currentIndex = 0
                    }

                    background: Rectangle {
                        border.width: 0
                        radius: 5 * screenScaleFactor
                        color: parent.pressed ? Constants.leftBtnBgColor_selected
                                              : parent.hovered ? Constants.leftBtnBgColor_hovered
                                                               : "transparent"
                    }

                    contentItem: Item {
                        Image {
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: width
                            sourceSize.height: height
                            source: Constants.parameter_reset_button_image
                        }
                    }
                }

                CustomComboBox {
                    id: print_sequence_combo_box

                    Layout.fillWidth: true
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    font.pointSize: Constants.labelFontPointSize_10

                    background: Rectangle {
                        radius: 5 * screenScaleFactor
                        color: {
                            if (print_sequence_reset_button.enabled) {
                                return Constants.parameter_editer_modifyed_color
                            }

                            return "transparent"
                        }

                        border.width: 1 * screenScaleFactor
                        border.color: {
                            if (print_sequence_reset_button.enabled) {
                                return Constants.right_panel_border_hovered_color
                            }

                            if (print_sequence_combo_box.hovered) {
                                return Constants.right_panel_border_hovered_color
                            }

                            return Constants.right_panel_border_default_color
                        }
                    }

                    translater: source => {
                                    const context = source === "Same as Global Print Sequence"
                                    ? "PrinterSettingsDialog"
                                    : "ParameterComponent"
                                    return qsTranslate(context, source)
                                }

                    model: [
                        "Same as Global Print Sequence",
                        "By layer",
                        "By object",
                    ]

                    onCurrentIndexChanged: {
                        if (plate_config_layout.initializing) {
                            return
                        }

                        plate_config_layout.printer.setParameter("print_sequence", (_ => {
                                                                                        switch (currentIndex) {
                                                                                            case 1: return "by layer"
                                                                                            case 2: return "by object"
                                                                                            default: return ""
                                                                                        }
                                                                                    })())
                    }
                }

                Label {
                    Layout.preferredWidth: 145 * screenScaleFactor
                    Layout.minimumWidth: contentWidth
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    color: filament_sequence_reset_button.enabled
                           ? Constants.parameter_text_modifyed_color
                           : Constants.parameter_text_color
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    text: qsTranslate("PrinterSettingsDialog", "First layer filament sequence") + ":"
                }

                Button {
                    id: filament_sequence_reset_button

                    Layout.minimumWidth: 20 * screenScaleFactor
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.minimumHeight: Layout.minimumWidth
                    Layout.maximumHeight: Layout.minimumHeight
                    Layout.alignment: Qt.AlignRight

                    enabled: filament_sequence_combo_box.currentIndex !== 0
                    opacity: enabled ? 100 : 0

                    onReleased: {
                        filament_sequence_combo_box.currentIndex = 0
                    }

                    background: Rectangle {
                        border.width: 0
                        radius: 5 * screenScaleFactor
                        color: parent.pressed ? Constants.leftBtnBgColor_selected
                                              : parent.hovered ? Constants.leftBtnBgColor_hovered
                                                               : "transparent"
                    }

                    contentItem: Item {
                        Image {
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: width
                            sourceSize.height: height
                            source: Constants.parameter_reset_button_image
                        }
                    }
                }

                CustomComboBox {
                    id: filament_sequence_combo_box

                    Layout.fillWidth: true
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    font.pointSize: Constants.labelFontPointSize_10

                    background: Rectangle {
                        radius: 5 * screenScaleFactor
                        color: {
                            if (filament_sequence_reset_button.enabled) {
                                return Constants.parameter_editer_modifyed_color
                            }

                            return "transparent"
                        }

                        border.width: 1 * screenScaleFactor
                        border.color: {
                            if (filament_sequence_reset_button.enabled) {
                                return Constants.right_panel_border_hovered_color
                            }

                            if (filament_sequence_combo_box.hovered) {
                                return Constants.right_panel_border_hovered_color
                            }

                            return Constants.right_panel_border_default_color
                        }
                    }

                    translater: source => qsTranslate("PrinterSettingsDialog", source)

                    model: [
                        "Auto",
                        "Customize",
                    ]

                    onCurrentIndexChanged: {
                        if (plate_config_layout.initializing) {
                            return
                        }
                        plate_config_layout.printer.setParameter("custom", ((_) => {
                                                                                switch (currentIndex) {
                                                                                    case 0: return "false"
                                                                                    case 1: return "true"
                                                                                    default: return ""
                                                                                }
                                                                            })())

                        if (filament_sequence_combo_box.currentIndex === 1) {
                            plate_config_layout.printer.setParameter(
                                        "first_layer_print_sequence",
                                        filament_sequence_editor.getSequenceString())
                        }
                    }
                }

                Item {
                    Layout.preferredWidth: 145 * screenScaleFactor
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    visible: filament_sequence_editor.visible
                }

                Button {
                    id: filament_sequence_string_reset_button

                    Layout.minimumWidth: 20 * screenScaleFactor
                    Layout.maximumWidth: Layout.minimumWidth
                    Layout.minimumHeight: Layout.minimumWidth
                    Layout.maximumHeight: Layout.minimumHeight
                    Layout.alignment: Qt.AlignRight

                    enabled: filament_sequence_editor.visible && filament_sequence_editor.sequenceString !== "1"
                    opacity: enabled ? 100 : 0

                    onReleased: {
                        filament_sequence_editor.sequenceString = "1"
                    }

                    background: Rectangle {
                        border.width: 0
                        radius: 5 * screenScaleFactor
                        color: parent.pressed ? Constants.leftBtnBgColor_selected
                                              : parent.hovered ? Constants.leftBtnBgColor_hovered
                                                               : "transparent"
                    }

                    contentItem: Item {
                        Image {
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: width
                            sourceSize.height: height
                            source: Constants.parameter_reset_button_image
                        }
                    }
                }

                PrintSequenceEditor {
                    id: filament_sequence_editor

                    Layout.fillWidth: true
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    visible: filament_sequence_combo_box.currentIndex == 1
                    onSequenceChanged: {
                        if (plate_config_layout.initializing) {
                            return
                        }

                        if (filament_sequence_combo_box.currentIndex === 1) {
                            plate_config_layout.printer.setParameter(
                                        "first_layer_print_sequence",
                                        filament_sequence_editor.getSequenceString())
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: root.layoutMargins
        Layout.leftMargin: root.layoutMargins
        Layout.rightMargin: root.layoutMargins

        visible: config_layout.currentIndex === 0

        Rectangle {
            id: parameter_tip

            Layout.fillWidth: true
            Layout.minimumHeight: {
                if (screenScaleFactor > 1) {
                    return 150 * screenScaleFactor
                }

                if (!frameLessView.isMax && !frameLessView.isFull) {
                    return 150 * screenScaleFactor
                }

                return 190 * screenScaleFactor
            }
            Layout.maximumHeight: Layout.minimumHeight

            radius: 5 * screenScaleFactor
            color: Constants.currentTheme ? "transparent" : Constants.mainBackgroundColor
            border.width: (Constants.currentTheme ? 1 : 0) * screenScaleFactor
            border.color: Constants.right_panel_border_default_color

            Connections {
                target: parameter_component

                function onFocusedItemChanged() {
                    parameter_tip_delay_timer.stop()
                    parameter_tip_delay_timer.interval = 0
                    parameter_tip_delay_timer.start()
                }

                function onHoveredItemChanged() {
                    parameter_tip_delay_timer.stop()
                    parameter_tip_delay_timer.interval = parameter_component.toopTipDelay
                    parameter_tip_delay_timer.start()
                }
            }

            Timer {
                id: parameter_tip_delay_timer
                interval: parameter_component.toopTipDelay
                onTriggered: {
                    if (parameter_component.focusedItem) {
                        parameter_tip_text.text = parameter_component.focusedText
                        parameter_tip_image.source = parameter_component.focusedIcon
                        return
                    }

                    if (parameter_component.hoveredItem) {
                        parameter_tip_text.text = parameter_component.hoveredText
                        parameter_tip_image.source = parameter_component.hoveredIcon
                        return
                    }

                    parameter_tip_image.source = "qrc:/UI/photo/parameter/tip_default.png"
                    parameter_tip_text.text = ""
                }
            }

            Text {
                id: parameter_tip_text
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 5 * screenScaleFactor
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                color: Constants.right_panel_text_default_color
                font.pointSize: Constants.labelFontPointSize_13
                font.family: Constants.labelFontFamily
                font.weight: Font.ExtraBold
                text: ""
            }

            Image {
                id: parameter_tip_image
                anchors.top: parameter_tip_text.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 10 * screenScaleFactor
                fillMode: Image.PreserveAspectFit
                source: "qrc:/UI/photo/parameter/tip_default.png"
            }

            Button {
                anchors.top: parent.top
                anchors.right: parent.right

                width: 28 * screenScaleFactor
                height: width

                onClicked: {
                    parameter_tip.visible = false
                }

                background: Item {}

                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 7 * screenScaleFactor
                        height: 5 * screenScaleFactor
                        rotation: 90
                        sourceSize.width: width
                        sourceSize.height: height
                        source: Constants.currentTheme
                                ? "qrc:/UI/photo/parameter/tip_button_light.svg"
                                : "qrc:/UI/photo/parameter/tip_button_dark.svg"
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            visible: !parameter_tip.visible
        }

        Button {
            Layout.minimumWidth: 28 * screenScaleFactor
            Layout.maximumWidth: Layout.minimumWidth
            Layout.minimumHeight: Layout.minimumWidth
            Layout.maximumHeight: Layout.minimumHeight

            visible: !parameter_tip.visible

            onClicked: {
                parameter_tip.visible = true
            }

            background: Rectangle {
                radius: 5 * screenScaleFactor
                color: Constants.currentTheme ? "transparent" : Constants.mainBackgroundColor
                border.width: (Constants.currentTheme ? 1 : 0) * screenScaleFactor
                border.color: Constants.right_panel_border_default_color
            }

            contentItem: Item {
                Image {
                    anchors.centerIn: parent
                    width: 7 * screenScaleFactor
                    height: 5 * screenScaleFactor
                    rotation: -90
                    sourceSize.width: width
                    sourceSize.height: height
                    source: Constants.currentTheme
                            ? "qrc:/UI/photo/parameter/tip_button_light.svg"
                            : "qrc:/UI/photo/parameter/tip_button_dark.svg"
                    fillMode: Image.PreserveAspectFit
                }
            }
        }
    }

    BasicDialogV4{
        property var profileObj
        property string defaultName
        id: changeProfileName
        width: 600 * screenScaleFactor
        height: 216 * screenScaleFactor
        maxBtnVis: false
        title: qsTr("Rename")
        parent: standaloneWindow
        anchors.centerIn: parent

        bdContentItem:Item{
            ColumnLayout {
                id: printerSave
                anchors.fill: parent
                anchors.margins: 20 * screenScaleFactor
                spacing: 10 * screenScaleFactor

                StyledLabel {
                    id: printerLabel

                    Layout.preferredWidth: 60 * screenScaleFactor
                    text: qsTranslate("SaveParameterSecond", "Save Process as") + ":"
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                    Layout.alignment: Qt.AlignVCenter
                }

                BasicDialogTextField {
                    id: printerInput

                    Layout.fillWidth: true
                    Layout.preferredHeight: 28 * screenScaleFactor
                    radius: 5
                    font.pointSize: 9
                    Layout.alignment: Qt.AlignVCenter
                    text: changeProfileName.defaultName
                    onTextChanged: {}

                    validator: RegExpValidator {
                    }
                }

                Item{
                    Layout.fillHeight: true
                }

                BasicButton {
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 30 * screenScaleFactor
                    Layout.alignment : Qt.AlignHCenter
                    text: qsTr("OK")
                    btnRadius: 15 * screenScaleFactor
                    btnBorderW: 1
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        changeProfileName.profileObj.changeUserFileName(printerInput.text)
                        changeProfileName.close()
                    }
                }
            }
        }
    }
}
