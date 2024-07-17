import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "../qml"
import "../components"

DockItem {
  id: root

  property var treeModel: kernel_ui_parameter.processTreeModel

  signal forkNodeInitialized(var delegate, var node)
  signal leafNodeInitialized(var delegate, var node)
  signal init()
  signal reset()
  signal save()

  width: 822 * screenScaleFactor
  height: 640 * screenScaleFactor
  title: qsTr("title")

  onVisibleChanged: function() {
    if (visible) {
      init()
    } else {
      search_combo_box.searchText = ""
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: 20 * screenScaleFactor + root.currentTitleHeight

    spacing: 10 * screenScaleFactor

    RowLayout {
      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      SearchComboBox {
        id: search_combo_box

        Layout.minimumWidth: 420 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        property var searchContext: null

        textRole: "model_text"
        placeholderText: qsTranslate("ParameterComponent", "Please enter a parameter name")

        popup.onOpened: function() {
          searchContext = kernel_ui_parameter.generateSearchContext(root.treeModel, "ParameterComponent")
          searchContext.proxyModel.filterCaseSensitivity = Qt.CaseInsensitive
          searchContext.proxyModel.searchText = Qt.binding(() => this.searchText)
          model = searchContext.proxyModel
        }

        popup.onClosed: function() {
          model = null
          searchContext = null
        }

        onActivated: function() {
          // 打开所有折叠的父节点
          const result_model = searchContext.proxyModel.get(currentIndex).model_node
          for (let fork = result_model.parentNode; !fork.isRootNode; fork = fork.parentNode) {
            tree_view.findDelegate(fork).showChildNode()
          }
          // 移动到目标节点
          const result_delegate = tree_view.findDelegate(result_model)
          const offset = result_delegate.mapToItem(scroll_view, Qt.point(0, 0))
          scroll_view.contentItem.contentY += offset.y
          // 高亮目标节点
          result_delegate.startHightlightAnimation()
        }
      }

      Item {
        Layout.fillWidth: true
      }

      CheckBox {
        id: select_all_check_box

        Layout.minimumWidth: 16 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        Connections {
          target: root
          function onReset() {
            select_all_check_box.checkState = Qt.Unchecked
          }
        }

        background: Rectangle {
          radius: 3 * screenScaleFactor
          border.width: 1 * screenScaleFactor
          border.color: parent.checkState !== Qt.Unchecked || parent.hovered
              ? Constants.textRectBgHoveredColor : Constants.dialogItemRectBgBorderColor
          color: parent.checkState !== Qt.Unchecked ? Constants.themeGreenColor : "transparent"
        }

        contentItem: Item {}

        indicator: Image {
          width: 9 * screenScaleFactor
          height: 6 * screenScaleFactor
          anchors.centerIn: parent
          visible: parent.checkState === Qt.Checked
          source: "qrc:/UI/images/check2.png"
        }
      }

      Text {
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        color: Constants.manager_printer_button_text_color
        font: Constants.font
        text: qsTr("select_all")
      }
    }

    ScrollView {
      id: scroll_view

      Layout.fillWidth: true
      Layout.fillHeight: true

      clip: true

      ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
      ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

      CustomTreeView {
        id: tree_view

        property var keyLeafMap: new Map()

        width: scroll_view.availableWidth - 12 * screenScaleFactor

        nodeFillWidth: true
        forkNodeHeight: 24 * screenScaleFactor
        leafNodeHeight: 24 * screenScaleFactor
        nodeSpacing: 10 * screenScaleFactor
        nodeLeftMargin: 38 * screenScaleFactor

        treeModel: root.treeModel

        forkNodeDelegate: ColumnLayout {
          id: fork_delegate

          function showChildNode() {
            fork_button.checked = true
          }

          Component.onCompleted: root.forkNodeInitialized(fork_delegate, nodeModel)

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

                font.weight: Constants.labelFontWeight
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
                source: fork_button.checked ? "qrc:/UI/photo/comboboxDown2_flip.png"
                                            : "qrc:/UI/photo/comboboxDown2.png"
              }
            }
          }
        }

        leafNodeDelegate: RowLayout {
          id: leaf_delegate

          readonly property var checkBox: leaf_check_box

          spacing: 10 * screenScaleFactor

          function startHightlightAnimation() {
            animation.start()
          }

          Component.onCompleted: {
            tree_view.keyLeafMap.set(nodeModel.key, leaf_delegate)
            root.leafNodeInitialized(leaf_delegate, nodeModel)
          }

          Component.onDestruction: {
            tree_view.keyLeafMap.delete(nodeModel.key)
          }

          Connections {
            target: select_all_check_box

            function onToggled() {
              leaf_check_box.checkState = select_all_check_box.checkState
            }
          }

          Connections {
            target: leaf_check_box

            function onToggled() {
              if (leaf_check_box.checkState !== Qt.Checked) {
                select_all_check_box.checkState = Qt.Unchecked
                return
              }

              for (let [key, leaf] of tree_view.keyLeafMap) {
                if (leaf.checkBox.checkState !== Qt.Checked) {
                  select_all_check_box.checkState = Qt.Unchecked
                  return
                }
              }

              select_all_check_box.checkState = Qt.Checked
            }
          }

          CheckBox {
            id: leaf_check_box
            Layout.minimumWidth: 16 * screenScaleFactor
            Layout.maximumWidth: Layout.minimumWidth
            Layout.minimumHeight: Layout.minimumWidth
            Layout.maximumHeight: Layout.minimumHeight
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

            background: Rectangle {
              radius: 3 * screenScaleFactor
              border.width: 1 * screenScaleFactor
              border.color: parent.checkState !== Qt.Unchecked || parent.hovered
                  ? Constants.textRectBgHoveredColor : Constants.dialogItemRectBgBorderColor
              color: parent.checkState !== Qt.Unchecked ? Constants.themeGreenColor : "transparent"
            }

            contentItem: Item {}

            indicator: Image {
              width: 9 * screenScaleFactor
              height: 6 * screenScaleFactor
              anchors.centerIn: parent
              visible: parent.checkState === Qt.Checked
              source: "qrc:/UI/images/check2.png"
            }
          }

          Text {
            id: leaf_node_text

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter

            font: Constants.font
            color: Constants.right_panel_text_default_color
            text: qsTranslate("ParameterComponent", nodeModel.label)

            SequentialAnimation {
              id: animation
              loops: 10

              ColorAnimation {
                target: leaf_node_text
                property: "color"
                to: Constants.right_panel_tab_button_checked_color
                duration: 100
              }

              ColorAnimation {
                target: leaf_node_text
                property: "color"
                to: Constants.themeTextColor
                duration: 100
              }
            }
          }
        }
      }
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: 10 * screenScaleFactor

      Item {
        Layout.fillWidth: true
      }

      Repeater {
        readonly property list<QtObject> modelObjectList: [
          QtObject {
            readonly property string text: qsTr("confirm")
            readonly property var callback: function() {
              root.save()
              root.close()
            }
          },

          QtObject {
            readonly property string text: qsTr("reset")
            readonly property var callback: function() {
              root.reset()
            }
          },

          QtObject {
            readonly property string text: qsTr("cancel")
            readonly property var callback: function() {
              root.close()
            }
          }
        ]

        model: modelObjectList

        delegate: Button {
          Layout.minimumWidth: 80 * screenScaleFactor
          Layout.fillHeight: true
          Layout.alignment: Qt.AlignCenter

          background: Rectangle {
            radius: height / 2
            border.width: 1 * screenScaleFactor
            border.color: Constants.manager_printer_button_border_color
            color:
              parent.hovered && !parent.pressed ? Constants.manager_printer_button_checked_color
                                                : Constants.manager_printer_button_default_color
          }

          contentItem: Text {
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            color: Constants.manager_printer_button_text_color
            font: Constants.font
            text: model.text
          }

          onClicked: function() {
            model.callback()
          }
        }
      }

      Item {
        Layout.fillWidth: true
      }
    }
  }
}
