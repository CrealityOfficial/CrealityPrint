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

    property var curExtruderObj
    property bool materialListVisible: true
    property int focusedMaterialIndex: 0
    property int focusedProcessIndex: 0
    readonly property var currentMachine: kernel_parameter_manager.currentMachineObject
    property var focusedMaterial: currentMachine ? currentMachine.materialObject(kernel_ui_parameter.focusedMaterialName) : null
    property var focusedProcess: currentMachine ? currentMachine.profileObject(focusedProcessIndex) : null
    readonly property var extruderModelData: currentMachine ? currentMachine.extruder1DataModel() : null
    property var materialModelData: focusedMaterial ? focusedMaterial.basicDataModel() : null
    readonly property var overrideExtruderModelData: focusedMaterial ? focusedMaterial.extruderDataModel() : null
    readonly property var overrideMaterialModelData: focusedProcess && focusedMaterial ? focusedMaterial.processDataModel(focusedProcess.uniqueName()) : null
    readonly property var materialTreeModel: kernel_ui_parameter.materialTreeModel
    readonly property var overrideTreeModel: kernel_ui_parameter.overrideTreeModel

    width: 823 * screenScaleFactor
    height: 640 * screenScaleFactor
    title: qsTr("Manage Filaments")
    titleHeight: 30 * screenScaleFactor
    onCurrentMachineChanged: {
    }
    Component.onCompleted: function() {
        //        kernel_ui_parameter.focusedMaterialName = Qt.binding((_) => {
        //                                                                 return focusedMaterial ? focusedMaterial.uniqueName() : "";
        //                                                             });
        kernel_ui_parameter.focusedProcessName = Qt.binding((_) => {
                                                                return focusedProcess ? focusedProcess.uniqueName() : "";
                                                            });
    }

    onVisibleChanged: {
        if(visible){
            cloader.item.materialView.model = null
            cloader.item.materialView.model = currentMachine.materialsModel
            cloader.item.materialView.currentIndex = focusedMaterialIndex
        }
    }

    Connections{
        target: kernel_ui_parameter
        onFocusedMaterialNameChanged:{
            console.log("kernel_ui_parameter.focusedMaterialName = ", kernel_ui_parameter.focusedMaterialName)

            focusedMaterial = Qt.binding(function(){ return currentMachine ? currentMachine.materialObject(kernel_ui_parameter.focusedMaterialName) : null; })
        }
    }

    ParameterComponent {
        id: parameter_component

        editerWidth: 220 * screenScaleFactor
        toolTipPosition: BasicTooltip.Position.TOP
    }

    ProfileUiParameterDialog {
        id: override_process_setting_dialog

        treeModel: root.overrideTreeModel.getChildNode("/process")
        onReset: function() {
            this.close();
        }
        onSave: function() {
            this.close();
        }
        onLeafNodeInitialized: function(delegate, model) {
            const sync_checked = (_) => {
                return delegate.checkBox.checked = model.overrode;
            };
            model.overrodeChanged.connect(sync_checked);
            this.init.connect(sync_checked);
            this.reset.connect((_) => {
                                   kernel_ui_parameter.eraseOverrideParameter(model.key);
                               });
            this.save.connect((_) => {
                                  if (delegate.checkBox.checked == model.overrode)
                                  return ;
                                  else if (delegate.checkBox.checked && !model.overrode)
                                  kernel_ui_parameter.emplaceOverrideParameter(model.key);
                                  else if (!delegate.checkBox.checked && model.overrode)
                                  kernel_ui_parameter.eraseOverrideParameter(model.key);
                              });
        }
    }

    UploadMessageDlg {
        property var receiver
        id: warning_dialog
        anchors.centerIn: parent

        onSigOkButtonClicked: function() {
            focusedMaterial.reset();
            warning_dialog.visible = false;
        }
    }

    bdContentItem:Item{
        property var materialView: material_view
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 21 * screenScaleFactor
            spacing: 20 * screenScaleFactor

            RowLayout {
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true

                SearchComboBox {
                    property var searchContext: null

                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                    Layout.minimumHeight: 28 * screenScaleFactor
                    Layout.maximumHeight: Layout.minimumHeight
                    Layout.minimumWidth: 282 * screenScaleFactor
                    Layout.maximumWidth: Layout.minimumWidth
                    textRole: "model_text"
                    placeholderText: qsTranslate("ParameterComponent", "Please enter a parameter name")

                    popup.onOpened: _ => {
                                        const context = kernel_ui_parameter.generateSearchContext(root.materialTreeModel, "ParameterComponent")
                                        const proxy_model = context.proxyModel

                                        proxy_model.filterCaseSensitivity = Qt.CaseInsensitive
                                        proxy_model.filterUidList = ["/overrides"]
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
                                     const type_view = parameter_view
                                     const type_repeater = parameter_page_repeater
                                     const tree_model = root.materialTreeModel
                                     const proxy_model = searchContext.proxyModel

                                     // 获取 搜索结果节点 (result_node)
                                     const result_item = proxy_model.get(currentIndex)
                                     const result_uid = result_item.model_uid
                                     const result_node = result_item.model_node

                                     // 找到 result_node 所属 一级分类节点 (type_node)
                                     for (var type_index = 0; type_index < type_repeater.count; ++type_index) {
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
                spacing: 20 * screenScaleFactor

                ListView {
                    id: material_view
                    Layout.preferredWidth: 200 * screenScaleFactor
                    Layout.fillHeight: true
                    currentIndex: focusedMaterialIndex
                    visible: materialListVisible
                    spacing: 3 * screenScaleFactor
                    clip: true
                    model: currentMachine.materialsModel
                    section.property: "defaultOrSync"
                    section.criteria: ViewSection.FullString
                    delegate: ItemDelegate {
                        readonly property var modelItem: model

                        implicitWidth: 200 * screenScaleFactor
                        implicitHeight: 28 * screenScaleFactor
                        leftPadding: 10 * screenScaleFactor
                        highlighted: ListView.isCurrentItem
                        enabled: materialNameText.text && materialNameText.text != "/" && materialNameText.text != "?"
                        onClicked: function() {
                            if (currentMachine.isMaterialModified()) {
                                var receiver = {
                                };
                                receiver.type = 'material';
                                receiver.profileName = currentMachine.modifiedMaterialName();
                                receiver.onIgnore = function() {
                                    currentMachine.abandonMaterialMod();
                                    ListView.view.currentIndex = index;
                                };
                                receiver.materialListView = material_view
                                receiver.currentMachine = root.currentMachine
                                receiver.onSaved = function(tindex) {
                                    if (tindex >= 0){
                                        receiver.materialListView.model = null
                                        receiver.materialListView.model = receiver.currentMachine.materialsModel
                                        ListView.view.currentIndex = tindex;
                                    }

                                };
                                idAllMenuDialog.requestMenuDialog(receiver, "saveParemDialog");
                            } else {
                                ListView.view.currentIndex = index;
                            }

                            kernel_ui_parameter.focusedMaterialName = model.name + "-1.75"
                        }

                        background: Rectangle {
                            radius: 3 * screenScaleFactor
                            color: parent.highlighted ? Constants.themeGreenColor : parent.hovered ? Constants.leftBtnBgColor_hovered : "transparent"
                        }

                        contentItem: Item{
                            Row{
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 5*screenScaleFactor
                                spacing: 5*screenScaleFactor
                                Rectangle{
                                    id: colorRec
                                    visible: model.defaultOrSync === "2"
                                    width: 26*screenScaleFactor
                                    height: 16*screenScaleFactor
                                    border.width: 1
                                    border.color: "#ffffff"
                                    color: model.materialColor
                                    radius: 2*screenScaleFactor

                                    onColorChanged: {
                                        colorIndexText.color = Constants.setTextColor(color)
                                    }


                                    Text{
                                        id: colorIndexText
                                        anchors.centerIn: parent
                                        text: model.syncBoxIndex
                                    }
                                }
                                Text {
                                    id: materialNameText
                                    verticalAlignment: Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft
                                    font: Constants.font
                                    text: "%1%2".arg(model.modifyed ? "*" : "").arg(model.name)
                                    color: parent.highlighted ? Constants.model_library_type_button_text_checked_color : Constants.model_library_type_button_text_default_color
                                }
                            }
                        }
                    }

                    section.delegate: Item {
                        width: parent.width
                        height: titleText.height * 2

                        Text {
                            id: titleText

                            text:{
                                if(section === "0"){
                                    "------" + qsTranslate("CXComboBox", "Default configuration") + "------"
                                }else if(section === "1"){
                                    "------" + qsTranslate("CXComboBox", "User configuration") + "------"
                                }else {
                                    "------" + qsTranslate("CXComboBox", "CFS") + "------"
                                }
                            }
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 0
                            font.bold: true
                            color: Constants.model_library_type_button_text_default_color
                        }

                    }

                }

                CustomTabView {
                    id: parameter_view

                    readonly property real itemHeight: 24 * screenScaleFactor

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    titleBarRightMargin: 0
                    titleReverse: true

                    Repeater {
                        id: parameter_page_repeater

                        model: root.materialTreeModel.childNodes

                        delegate: CustomTabViewItem {
                            id: parameter_page

                            readonly property string titleSource: model.model_node.label
                            readonly property bool overrode: titleSource === "Overrides"
                            readonly property var treeView: !overrode ? page_loader.item : null
                            readonly property real scrollRightMargin: 12 * screenScaleFactor

                            title: qsTranslate("ParameterComponent", titleSource)

                            Component.onCompleted: function() {
                                if (model.index === 0) {
                                    parameter_view.defaultPanel = this
                                    parameter_view.currentPanel = this
                                } else if (model.index === 1) {
                                    parameter_view.currentPanel = this
                                    Qt.callLater(_ => parameter_view.currentPanel = parameter_view.defaultPanel)
                                }
                            }

                            Loader {
                                id: page_loader

                                readonly property var pageTreeModel: model_node

                                anchors.fill: parent
                                anchors.margins: 20 * screenScaleFactor
                                sourceComponent: parameter_page.overrode ? override_component : normal_component
                            }

                            Component {
                                id: normal_component

                                FlattenTreeView {
                                    id: normal_tree_view

                                    ScrollBar.vertical: ScrollBar {
                                        policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                                        onActiveChanged: parameter_component.closePopup()
                                    }

                                    clip: true
                                    rightMargin: 12 * screenScaleFactor

                                    nodeFillWidth: true
                                    forkNodeHeight: -1
                                    leafNodeHeight: -1

                                    treeModel: pageTreeModel

                                    forkNodeDelegate: ColumnLayout {
                                        readonly property bool isTinyLabel: nodeModel.component === "fork_tiny_label"

                                        function showChildNode() {
                                            fork_button.checked = true
                                        }

                                        Component.onCompleted: {
                                            nodeHeight = (isTinyLabel ? 24 : 40) * screenScaleFactor
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
                                            const data_model = root.materialModelData
                                            if (!data_model) {
                                                return null
                                            }

                                            return data_model.getDataItem(uiModel.key)
                                        }

                                        readonly property bool vaild: {
                                            return uiModel.level >= advance_button.currentLevel && dataModel && dataModel.enabled
                                        }

                                        active: dataModel
                                        sourceComponent: parameter_component.parameter

                                        Component.onCompleted: {
                                            nodeModel.nodeValid = Qt.binding(_ => vaild)
                                        }
                                    }
                                }
                            }

                            Component {
                                id: override_component

                                ColumnLayout {
                                    id: override_layout

                                    Repeater {
                                        model: root.overrideTreeModel.childNodes

                                        delegate: GroupBox {
                                            id: override_box

                                            readonly property var node: model.model_node
                                            readonly property bool staticed: title !== "Process"
                                            readonly property bool expand: label.checked

                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            Layout.preferredHeight: expand ? (override_layout.height - override_layout.spacing) / 2 : label.height + topPadding + bottomPadding
                                            topInset: implicitLabelHeight / 2
                                            leftPadding: 10 * screenScaleFactor
                                            rightPadding: leftPadding
                                            bottomPadding: expand ? leftPadding : 0
                                            topPadding: expand ? implicitLabelHeight : 0
                                            title: node.label

                                            label: Button {
                                                x: parent.leftPadding
                                                height: 16 * screenScaleFactor
                                                checkable: true
                                                checked: true

                                                background: Rectangle {
                                                    color: parameter_view.panelColor
                                                }

                                                contentItem: RowLayout {
                                                    Text {
                                                        verticalAlignment: Qt.AlignVCenter
                                                        horizontalAlignment: Qt.AlignHCenter
                                                        font: Constants.font
                                                        color: Constants.textColor
                                                        text: qsTranslate("ParameterComponent", override_box.title)
                                                    }

                                                    Image {
                                                        verticalAlignment: Qt.AlignVCenter
                                                        horizontalAlignment: Qt.AlignHCenter
                                                        fillMode: Image.Pad
                                                        source: override_box.expand ? "qrc:/UI/photo/comboboxDown2_flip.png" : "qrc:/UI/photo/comboboxDown2.png"
                                                    }

                                                }

                                            }

                                            background: Rectangle {
                                                color: "transparent"
                                                radius: 5 * screenScaleFactor
                                                border.width: 1 * screenScaleFactor
                                                border.color: Constants.dialogItemRectBgBorderColor
                                            }

                                            contentItem: ColumnLayout {
                                                id: override_box_layout

                                                spacing: 10 * screenScaleFactor
                                                visible: parent.expand

                                                ScrollView {
                                                    id: override_view

                                                    Layout.fillWidth: true
                                                    Layout.fillHeight: true
                                                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                                                    ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                                                    clip: true

                                                    ColumnLayout {
                                                        width: override_view.availableWidth - parameter_page.scrollRightMargin
                                                        spacing: override_box_layout.spacing

                                                        Loader {
                                                            Layout.fillWidth: true
                                                            Layout.minimumHeight: sourceComponent.implicitHeight + override_box_layout.spacing
                                                            active: !override_box.staticed
                                                            visible: active

                                                            sourceComponent: ColumnLayout {
                                                                spacing: override_box_layout.spacing

                                                                RowLayout {
                                                                    Layout.fillWidth: true
                                                                    Layout.minimumHeight: process_box.implicitHeight

                                                                    Label {
                                                                        Layout.fillWidth: true
                                                                        Layout.minimumHeight: parameter_component.minimumHeight
                                                                        Layout.maximumHeight: Layout.minimumHeight
                                                                        horizontalAlignment: Qt.AlignLeft
                                                                        verticalAlignment: Qt.AlignVCenter
                                                                        font: Constants.font
                                                                        color: Constants.right_panel_text_default_color
                                                                        text: "%1 :".arg(qsTr("Select Process"))
                                                                    }

                                                                    CXComboBox {
                                                                        id: process_box

                                                                        Layout.minimumWidth: 260 * screenScaleFactor
                                                                        Layout.maximumWidth: Layout.minimumWidth
                                                                        Layout.minimumHeight: parameter_component.minimumHeight
                                                                        Layout.maximumHeight: Layout.minimumHeight
                                                                        clip: true
                                                                        showCount: 20
                                                                        textRole: "name"
                                                                        model: root.currentMachine ? root.currentMachine.profilesModel : []
                                                                        onCurrentIndexChanged: function() {
                                                                            if (root.focusedProcessIndex !== process_box.currentIndex)
                                                                                root.focusedProcessIndex = process_box.currentIndex;

                                                                        }

                                                                        Connections {
                                                                            function onFocusedProcessIndexChanged() {
                                                                                if (process_box.currentIndex !== root.focusedProcessIndex)
                                                                                    process_box.currentIndex = root.focusedProcessIndex;
                                                                            }
                                                                            target: root
                                                                        }
                                                                    }
                                                                }

                                                                Rectangle {
                                                                    Layout.fillWidth: true
                                                                    Layout.minimumHeight: 1 * screenScaleFactor
                                                                    Layout.maximumHeight: Layout.minimumHeight
                                                                    Layout.alignment: Qt.AlignVCenter
                                                                    color: Constants.right_panel_border_default_color
                                                                }
                                                            }
                                                        }

                                                        ColumnLayout {
                                                            Layout.fillWidth: true
                                                            spacing: 0

                                                            Repeater {
                                                                model: override_box.node.childNodes

                                                                delegate: RowLayout {
                                                                    readonly property var node: model.model_node

                                                                    Layout.fillWidth: true
                                                                    Layout.minimumHeight: parameter_view.itemHeight
                                                                    Layout.maximumHeight: Layout.minimumHeight
                                                                    visible: override_box.staticed || node.overrode
                                                                    spacing: 0

                                                                    CusCheckBox {
                                                                        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                                                        visible: override_box.staticed
                                                                        checked: node.overrode
                                                                        onClicked: {
                                                                            if (node.overrode)
                                                                                kernel_ui_parameter.eraseOverrideParameter(node.key);
                                                                            else
                                                                                kernel_ui_parameter.emplaceOverrideParameter(node.key);
                                                                        }
                                                                    }

                                                                    Loader {
                                                                        readonly property var uiModel: node
                                                                        readonly property var dataModel: {
                                                                            const data_item = ((_) => {
                                                                                                   // 固定(挤出机)覆盖参数
                                                                                                   if (override_box.staticed) {
                                                                                                       let extruder = null;
                                                                                                       if (!node.overrode && root.extruderModelData)
                                                                                                       extruder = root.extruderModelData;
                                                                                                       else if (node.overrode && root.overrideExtruderModelData)
                                                                                                       extruder = root.overrideExtruderModelData;
                                                                                                       return extruder ? extruder.getDataItem(node.key) : "";
                                                                                                   }
                                                                                                   // 动态(工艺)覆盖参数
                                                                                                   if (node.overrode && root.overrideMaterialModelData)
                                                                                                   return root.overrideMaterialModelData.getDataItem(node.key);

                                                                                                   return null;
                                                                                               })();
                                                                            return data_item ? data_item : {
                                                                                                   "enabled": true,
                                                                                                   "userModify": false,
                                                                                                   "key": node.key,
                                                                                                   "label": node.key,
                                                                                                   "tips": node.key,
                                                                                                   "type": "",
                                                                                                   "unit": "",
                                                                                                   "str": ""
                                                                                               };
                                                                        }

                                                                        Layout.fillWidth: true
                                                                        Layout.fillHeight: true
                                                                        enabled: node.overrode
                                                                        onItemChanged: function() {
                                                                            if (item)
                                                                                item.hideValueOnDisabled = true;

                                                                        }
                                                                        sourceComponent: parameter_component.profileItemRow
                                                                    }

                                                                }

                                                            }

                                                        }

                                                    }

                                                }

                                                Loader {
                                                    Layout.minimumHeight: parameter_view.itemHeight
                                                    active: !override_box.staticed
                                                    visible: active

                                                    sourceComponent: RowLayout {
                                                        spacing: 10 * screenScaleFactor

                                                        Button {
                                                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                                            Layout.fillHeight: true
                                                            Layout.minimumWidth: height
                                                            Layout.maximumWidth: Layout.minimumWidth
                                                            onClicked: function() {
                                                                override_process_setting_dialog.show();
                                                            }

                                                            background: Rectangle {
                                                                radius: 5 * screenScaleFactor
                                                                border.width: 1 * screenScaleFactor
                                                                border.color: Constants.right_panel_border_default_color
                                                                color: parent.pressed ? Constants.right_panel_button_checked_color : parent.hovered ? Constants.right_panel_button_hovered_color : Constants.right_panel_button_default_color
                                                            }

                                                            contentItem: Image {
                                                                anchors.centerIn: parent
                                                                width: 14 * screenScaleFactor
                                                                height: width
                                                                horizontalAlignment: Qt.AlignHCenter
                                                                verticalAlignment: Qt.AlignVCenter
                                                                fillMode: Image.PreserveAspectFit
                                                                source: parent.pressed ? "qrc:/UI/photo/rightDrawer/right_add_d.svg" : "qrc:/UI/photo/rightDrawer/right_add.svg"
                                                            }

                                                        }

                                                        Button {
                                                            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                                            Layout.fillHeight: true
                                                            Layout.minimumWidth: height
                                                            Layout.maximumWidth: Layout.minimumWidth
                                                            onClicked: function() {
                                                                override_process_setting_dialog.show();
                                                            }

                                                            background: Rectangle {
                                                                radius: 5 * screenScaleFactor
                                                                border.width: 1 * screenScaleFactor
                                                                border.color: Constants.right_panel_border_default_color
                                                                color: parent.pressed ? Constants.right_panel_button_checked_color : parent.hovered ? Constants.right_panel_button_hovered_color : Constants.right_panel_button_default_color
                                                            }

                                                            contentItem: Image {
                                                                anchors.centerIn: parent
                                                                width: 14 * screenScaleFactor
                                                                height: width
                                                                horizontalAlignment: Qt.AlignHCenter
                                                                verticalAlignment: Qt.AlignVCenter
                                                                fillMode: Image.PreserveAspectFit
                                                                source: parent.pressed ? "qrc:/UI/photo/rightDrawer/right_remove_d.svg" : "qrc:/UI/photo/rightDrawer/right_remove.svg"
                                                            }

                                                        }

                                                    }

                                                }

                                            }

                                        }

                                    }

                                }

                            }

                        }

                    }

                }

            }

            RowLayout {
                Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
                spacing: 10 * screenScaleFactor

                BasicButton {
                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    enabled: material_view.currentItem.modelItem.modifyed
                    text: qsTranslate("ExpertParamPanel", "Save")
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: function() {
                        if (currentMachine.isMaterialModified()) {
                            var receiver = {
                            };
                            receiver.type = 'material';
                            receiver.onOK = function() {
                            };
                            receiver.onSave = function(index) {
                                console.log("saveParamSecondDialog:" + index);
                                if (index >= 0){
                                    material_view.model = null
                                    material_view.model = currentMachine.materialsModel
                                    material_view.currentIndex = index;
                                }

                            };
                            idAllMenuDialog.requestMenuDialog(receiver, 'saveParamSecondDialog');
                        }
                    }
                }

                BasicButton {
                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    enabled: material_view.currentItem.modelItem.modifyed
                    text: qsTranslate("ExpertParamPanel", "Reset")
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: function() {
                        warning_dialog.msgText = qsTr("Do you want to reset the selected filament parameters?");
                        warning_dialog.visible = true;
                    }
                }

                BasicButton {
                    //root.currentMachine.setCurrentProfile(0);

                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    enabled: focusedMaterial.isUserDef()
                    text: qsTranslate("ExpertParamPanel", "Delete")
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: function() {
                        var receiver = {
                        };
                        receiver.materialListView = material_view
                        receiver.currentMachine = root.currentMachine

                        receiver.exec = function() {
                            const machine = root.currentMachine;
                            machine.deleteUserMaterial(root.focusedMaterial.uniqueName(), 0);
                            receiver.materialListView.model = null
                            receiver.materialListView.model = receiver.currentMachine.materialsModel
                            receiver.materialListView.currentIndex = curExtruderObj.materialIndex
                            console.log("focusedMaterialIndex ========", focusedMaterialIndex)
                            kernel_ui_parameter.focusedMaterialName = currentMachine.materialObject(0).uniqueName()
                        };
                        idAllMenuDialog.requestMenuDialog(receiver, "idDeletePreset");
                    }
                }

                BasicButton {
                    Layout.alignment: Qt.AlignCenter
                    Layout.preferredWidth: 80 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    text: qsTranslate("ExpertParamPanel", "Cancel")
                    btnRadius: 14 * screenScaleFactor
                    btnBorderW: 1 * screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: function() {
                        root.close();
                    }
                }

            }

        }

    }
}
