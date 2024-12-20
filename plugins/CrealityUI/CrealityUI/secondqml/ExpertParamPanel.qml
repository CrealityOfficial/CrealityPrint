import QtQuick 2.5
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1
import QtQml 2.15

import "./"
import "../qml"
import "../components"

DockItem
{
    property var currentProfile
    id: expertParamDoc
    width: 823 * screenScaleFactor
    height: 640 * screenScaleFactor
    title: qsTr("Expert Setting")
    titleHeight: 30* screenScaleFactor

    BusyIndicator{
        id: bi
        running: true
    }

    Component.onCompleted: {
        bi.running = false
    }

    Item{
        anchors.fill: parent
        ColumnLayout{
            anchors.fill: parent
            anchors.topMargin: expertParamDoc.titleHeight + 21*screenScaleFactor
            anchors.margins: 21* screenScaleFactor

            SearchComboBox{
                Layout.minimumHeight: 28 * screenScaleFactor
                Layout.maximumHeight: Layout.minimumHeight
                Layout.minimumWidth: 420 * screenScaleFactor
                Layout.maximumWidth: Layout.minimumWidth
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                property var searchContext: null

                textRole: "model_text"
                placeholderText: qsTranslate("ParameterComponent", "Please enter a parameter name")

                popup.onOpened: function() {
                    searchContext = kernel_ui_parameter.generateSearchContext(
                        kernel_ui_parameter.processTreeModel, "ParameterComponent")
                    searchContext.proxyModel.filterCaseSensitivity = Qt.CaseInsensitive
                    searchContext.proxyModel.searchText = Qt.binding(() => this.searchText)
                    model = searchContext.proxyModel
                }

                popup.onClosed: function() {
                  model = null
                  searchContext = null
                }

                onActivated: function() {
                    // 获取 搜索结果节点 (result_node)
                    const result_item = searchContext.proxyModel.get(currentIndex)
                    const result_uid = result_item.model_uid
                    const result_node = result_item.model_node

                    // 找到 result_node 所属 一级分类节点 (type_node)
                    const tree_model = kernel_ui_parameter.processTreeModel
                    const type_list_view = idCategoryRepeater
                    for (let type_index = 0; type_index < type_list_view.count; ++type_index) {
                        const type_uid = type_list_view.model.get(type_index).uid
                        const type_node = tree_model.getChildNode(type_uid)
                        if (result_node === type_node.getChildNode(result_uid, true)) {
                            // 切换到 result_node 所属 type_node 对应的页面
                            type_list_view.currentIndex = type_index

                            // 遍历 result_node 到 type_node 之间的所有节点 (sub_type_node)
                            // 展开其中对应组件 (sub_type_delegate) 被折叠的项
                            const sub_type_repeater = rep
                            const sub_type_item = sub_type_repeater.itemAt(type_index)
                            const tree_view = sub_type_item.treeView
                            for (let sub_type_node = result_node.parentNode;
                                 sub_type_node !== type_node;
                                 sub_type_node = sub_type_node.parentNode) {
                                const sub_type_delegate = tree_view.findDelegate(sub_type_node)
                                if (typeof sub_type_delegate.showChildNode === "function") {
                                    sub_type_delegate.showChildNode()
                                }
                            }

                            // 滚动条移动到 result_node 对应组件 (result_delegate) 的 y 轴坐标
                            const scroll_view = sub_type_item.scrollView
                            const result_delegate = tree_view.findDelegate(result_node)
                            const offset = result_delegate.mapToItem(scroll_view, Qt.point(0, 0))
                            scroll_view.contentItem.contentY += offset.y

                            // 高亮 result_delegate
                            result_delegate.nodeData.requestHighlight()
                            return
                        }
                    }
                }
            }

            RowLayout{
                Layout.fillWidth: true
                Layout.fillHeight: true
                ListView{
                    id: idCategoryRepeater
                    Layout.preferredWidth: 200*screenScaleFactor
                    Layout.preferredHeight: 482*screenScaleFactor
                    spacing: 3*screenScaleFactor
                    clip : true
                    model: kernel_ui_parameter.processTreeModel.childNodes

                    delegate: CusImglButton{
                        width: 200*screenScaleFactor
                        height : 28*screenScaleFactor
                        btnRadius : 3
                        state: "imgLeft"
                        checkable: true
                        textAlign: 0
                        borderWidth: 0
                        leftMargin: 10 *screenScaleFactor
                        font.pointSize:Constants.labelFontPointSize_10
                        bottonSelected: idCategoryRepeater.currentItem === this
                        allowTooltip: false
                        text: qsTranslate("ParameterComponent", model.model_node.label)
                        defaultBtnBgColor: "transparent"
                        hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
                        selectedBtnBgColor: Constants.leftBtnBgColor_selected
                        enabledIconSource: model.model_node.icon
                        highLightIconSource: model.model_node.icon
                        pressedIconSource: model.model_node.checkedIcon
                        btnTextColor : Constants.model_library_type_button_text_default_color
                        btnTextColor_d : Constants.model_library_type_button_text_checked_color
                        onClicked: {
                            console.log("currentIndex++++++++++++++ = ", model.index)
                            idCategoryRepeater.currentIndex = model.index
                        }
                    }
                }

                Item{
                    Layout.fillWidth: true
                }

                Rectangle{
                    Layout.preferredWidth: 561*screenScaleFactor
                    Layout.preferredHeight: 482*screenScaleFactor
                    color:"transparent"
                    radius: 5*screenScaleFactor
                    border.width: 1*screenScaleFactor
                    border.color: Constants.right_panel_border_default_color

                    StackLayout{
                        anchors.fill: parent
                        anchors.margins: 20*screenScaleFactor
                        anchors.rightMargin: 8*screenScaleFactor
                        currentIndex: idCategoryRepeater.currentIndex
                        Repeater{
                            id:rep
                            model: kernel_ui_parameter.processTreeModel.childNodes
                            delegate: treeView
                        }
                    }
                }
            }

            RowLayout{
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10*screenScaleFactor
                Item{
                    Layout.fillWidth: true
                }


                BasicButton {
                    id: saveAsBtn
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80* screenScaleFactor
                    Layout.preferredHeight: 28* screenScaleFactor
                    text: qsTr("Save As")
                    btnRadius: 14* screenScaleFactor
                    btnBorderW:1* screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                         menuDialog.openSaveAs(currentMachine)
                    }
                }

                BasicButton {
                    id: resetBtn
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80* screenScaleFactor
                    Layout.preferredHeight: 28* screenScaleFactor
                    text: qsTr("Reset")
                    btnRadius: 14* screenScaleFactor
                    btnBorderW:1* screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        warningDlg.msgText = qsTranslate("ParameterProfilePanel", "Do you want to reset process configuration parameters?")
                        warningDlg.visible = true
                    }
                }

                BasicButton {
                    id: cancelBtn
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 80* screenScaleFactor
                    Layout.preferredHeight: 28* screenScaleFactor
                    text: qsTr("Cancel")
                    btnRadius: 14* screenScaleFactor
                    btnBorderW:1* screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        expertParamDoc.close()
                    }
                }

                Item{
                    Layout.fillWidth: true
                }
            }
        }
    }

    Component{
        id: treeView
        Rectangle{
            property alias scrollView: slice_config_scroll_view
            property alias treeView: slice_config_tree_view
            color: "transparent"
            ScrollView {
                id: slice_config_scroll_view
                clip: true
                anchors.fill: parent
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

                CustomTreeView {
                    id: slice_config_tree_view

                    y: expertParamCom.minimumHeight - forkNodeHeight
                    nodeFillWidth: true
                    forkNodeHeight: 40 * screenScaleFactor
                    width: slice_config_scroll_view.availableWidth - 12 * screenScaleFactor

                    treeModel: model.model_node

                    forkNodeDelegate: ColumnLayout {
                        function showChildNode() {
                            fork_button.checked = true
                        }

                        Button {
                            id: fork_button

                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignBottom

                            checkable: true
                            checked: true

                            Component.onCompleted: {
                                childNodeVisible = Qt.binding(() => checked)
                            }

                            background: Item {}

                            contentItem: RowLayout {
                                spacing: 10 * screenScaleFactor

                                Image {
                                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

                                    horizontalAlignment: Qt.AlignLeft
                                    verticalAlignment: Qt.AlignVCenter
                                    fillMode: Image.PreserveAspectFit
                                    source: nodeModel.icon
                                }

                                StyledLabel {
                                    Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

                                    horizontalAlignment: Qt.AlignLeft
                                    verticalAlignment: Qt.AlignVCenter

                                    font.weight: Font.bold
                                    font.pointSize: Constants.labelFontPointSize_12
                                    color: Constants.right_panel_text_default_color
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
                        }
                    }

                    leafNodeDelegate: Loader {
                        id: leaf_node_loader
                        readonly property var uiModel: nodeModel
                        readonly property var dataModel: profileModelData.getDataItem(nodeModel.key)
                        visible: dataModel && dataModel.enabled
                        sourceComponent: expertParamCom.parameter
                        Component.onCompleted: {
                            nodeHeight = Qt.binding(_ => leaf_node_loader.item ? leaf_node_loader.item.componentHeight : 0)
                        }
                    }
                }
            }
        }
    }

    ParameterComponent{
        id: expertParamCom
        editerWidth: 220 * screenScaleFactor
    }


}
