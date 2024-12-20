import QtQml 2.15

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

QtObject {
  property real minimumHeight: 24 * screenScaleFactor  // line parameter height
  property real maximumHeight: 200 * screenScaleFactor  // block parameter height
  property real editerWidth: 130 * screenScaleFactor

  property string labelTextColor: Constants.parameter_text_color
  property string labelTextModifyedColor: Constants.parameter_text_modifyed_color

  property bool toolTipEnabled: true
  property real toolTipWidth: 240 * screenScaleFactor
  property int toolTipPosition: BasicTooltip.Position.LEFT
  property int toopTipDelay: 300

  property bool focusable: false
  property var focusedItem: null
  property string focusedText: ""
  property string focusedIcon: ""

  property bool hoverable: false
  property var hoveredItem: null
  property string hoveredText: ""
  property string hoveredIcon: ""

  signal closePopup()

  id: root

  readonly property Component parameter: Component {
    // required: dataModel, uiModel
    Control {
      id: parameter_control

      readonly property var parameterUiModel: uiModel
      readonly property var parameterDataModel: dataModel

      // internal
      readonly property bool parameterOverrideable: {
        return parameterUiModel.overrideable
      }
      readonly property bool parameterOverrode: {
        if (kernel_slice_flow.engineType === 0) {  // cura
          return parameterUiModel.overrode
        }

        if (kernel_slice_flow.engineType === 1) {  // orca
          return parameterDataModel && parameterDataModel.value != "nil"
        }

        return false
      }

      readonly property string labelText: {
        if (uiModel && uiModel.label && uiModel.label !== "") {
          return uiModel.label
        }

        if (dataModel && dataModel.label && dataModel.label !== "") {
          return dataModel.label
        }

        if (uiModel && uiModel.key && uiModel.key !== "") {
          return uiModel.key
        }

        if (dataModel && dataModel.key && dataModel.key !== "") {
          return dataModel.key
        }

        return ""
      }
      readonly property string customLabel: "%1.label".arg(parameterDataModel.key)
      readonly property string customLabelTranslation: qsTr(customLabel)
      readonly property string labelTranslation:
          customLabelTranslation !== customLabel ? customLabelTranslation : qsTr(labelText)

      readonly property string descriptionText: {
        if (dataModel && dataModel.description && dataModel.description !== "") {
          return dataModel.description
        }

        return labelText
      }
      readonly property string customDescriptionText: "%1.description".arg(parameterDataModel.key)
      readonly property string customDescriptionTranslation: qsTr(customDescriptionText)
      readonly property string descriptionTranslation:
          customDescriptionTranslation !== customDescriptionText ? customDescriptionTranslation
                                                                 : qsTr(descriptionText)

      visible: parameterUiModel && labelText !== ""
      enabled: parameterDataModel ? parameterDataModel.enabled : false
      opacity: enabled ? 1 : 0.5
      focusPolicy: Qt.ClickFocus

      Connections {
        target: parameter_loader.item.focuseableItem
        enabled: root.focusable
        ignoreUnknownSignals: true

        function onActiveFocusChanged() {
          if (target.activeFocus && root.focusedItem != target) {
            root.focusedItem = target
            root.focusedText = parameter_loader.labelTranslation
            root.focusedIcon = parameter_control.parameterUiModel.localIcon
          } else if (!target.activeFocus && root.focusedItem == target) {
            root.focusedItem = null
            root.focusedText = ""
            root.focusedIcon = ""
          }
        }
      }

      onHoveredChanged: {
        if (!root.hoverable) {
          return
        }

        if (hovered && root.hoveredItem != this) {
          root.hoveredItem = this
          root.hoveredText = parameter_loader.labelTranslation
          root.hoveredIcon = parameterUiModel.localIcon
        } else if (!hovered && root.hoveredItem == this) {
          root.hoveredItem = null
          root.hoveredText = ""
          root.hoveredIcon = ""
        }
      }

      background: Item {}

      contentItem: Loader {
        id: parameter_loader

        readonly property var parameterUiModel: parameter_control.parameterUiModel
        readonly property var parameterDataModel: parameter_control.parameterDataModel
        readonly property bool parameterOverrideable: parameter_control.parameterOverrideable
        readonly property bool parameterOverrode: parameter_control.parameterOverrode
        readonly property string labelText: parameter_control.labelText
        readonly property string labelTranslation: parameter_control.labelTranslation
        readonly property string descriptionText: parameter_control.descriptionText
        readonly property string descriptionTranslation: parameter_control.descriptionTranslation

        property bool hideValueOnDisabled: false

        active: standaloneWindow.initialized

        sourceComponent: {
          switch (parameterUiModel.component) {
            case "text_edit": {
              return root.blockParameter
            }
            case "array_text_edit": {
              return root.blockParameter
            }
            default: {
              return root.lineParameter
            }
          }
        }
      }
    }
  }

  readonly property Component lineParameter: Component {
    RowLayout {
      id: line_component_layout

      // for parent
      readonly property var focuseableItem:
          line_component_loader.item ? line_component_loader.item.focuseableItem : null

      enabled: parameterDataModel ? parameterDataModel.enabled : false
      visible: labelText !== ""
      spacing: 5 * screenScaleFactor

      CheckBox {
        id: override_check_box

        enabled: parameterOverrideable
        visible: parameterOverrideable
        Layout.minimumWidth: 16 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        readonly property bool overrode: parameterOverrode

        checkState: overrode ? Qt.Checked : Qt.Unchecked
        focusPolicy: Qt.ClickFocus

        onOverrodeChanged: {
          checkState = Qt.binding(_ => overrode ? Qt.Checked : Qt.Unchecked)
        }

        onCheckStateChanged: {
          const overrode_state = checkState === Qt.Checked
          if (overrode_state == overrode) {
            return
          }

          if (overrode_state) {
            kernel_ui_parameter.emplaceOverrideParameter(parameterDataModel.key)
          } else {
            kernel_ui_parameter.eraseOverrideParameter(parameterDataModel.key)
          }
        }

        background: Rectangle {
          radius: 3 * screenScaleFactor
          border.width: 1 * screenScaleFactor
          border.color: override_check_box.hovered ? Constants.right_panel_border_hovered_color
                                                   : Constants.right_panel_border_default_color
          color: override_check_box.checkState === Qt.Checked ? Constants.themeGreenColor
                                                              : "transparent"
        }

        contentItem: Item {}

        indicator: Image {
          width: 9 * screenScaleFactor
          height: 6 * screenScaleFactor
          anchors.centerIn: parent
          visible: override_check_box.checkState === Qt.Checked
          source: "qrc:/UI/images/check2.png"
        }
      }

      Text {
        id: parameter_label_text

        Layout.fillWidth: true
        Layout.fillHeight: true
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_10
        color: reset_button.actived ? root.labelTextModifyedColor : root.labelTextColor
        wrapMode: Text.WordWrap
        text: labelTranslation

        Connections {
          target: parameterUiModel
          ignoreUnknownSignals: true

          function onRequestHighlight() {
            highlight_animation.color = parameter_label_text.color
            highlight_animation.restart()
          }
        }

        SequentialAnimation {
          id: highlight_animation

          property string color: ""

          loops: 5

          ColorAnimation {
            target: parameter_label_text
            property: "color"
            to: Constants.themeGreenColor
            duration: 200
          }

          ColorAnimation {
            target: parameter_label_text
            property: "color"
            to: highlight_animation.color
            duration: 200
          }
        }

        Button {
          anchors.fill: parent
          background: Item {}
          contentItem: Item {}
          focusPolicy: Qt.ClickFocus

          BasicTooltip {
            visible: root.toolTipEnabled && parent.hovered
            timeout: -1
            position: root.toolTipPosition
            font.pointSize: Constants.labelFontPointSize_10
            textWidth: root.toolTipWidth
            textWrap: true
            text: descriptionTranslation
            delay: root.toopTipDelay
          }
        }
      }

      Button {
        Layout.fillWidth: true
        Layout.minimumHeight: root.minimumHeight
        Layout.maximumHeight: Layout.minimumHeight
        background: Item {}
        contentItem: Item {}
        focusPolicy: Qt.ClickFocus
      }

      Button {
        id: reset_button

        implicitWidth: 20 * screenScaleFactor
        Layout.minimumWidth: implicitWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        readonly property bool actived: {
          if (hideValueOnDisabled && !enabled) {
            return false
          }

          return parameterDataModel && parameterDataModel.modifyed
        }

        opacity: actived ? 100 : 0
        focusPolicy: Qt.ClickFocus

        onReleased: {
          if (parameterDataModel) {
            parameterDataModel.resetValue()
          }
        }

        background: Rectangle {
          border.width: 0
          radius: 5 * screenScaleFactor
          color: reset_button.pressed ? Constants.leftBtnBgColor_selected
               : reset_button.hovered ? Constants.leftBtnBgColor_hovered
                                      : "transparent"
        }

        contentItem: Item {
          Image {
            anchors.centerIn: parent
            width: 16 * screenScaleFactor
            height: width
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: Constants.parameter_reset_button_image
          }
        }
      }

      Loader {
        id: line_component_loader

        // for item
        readonly property var componentUiModel: parameterUiModel
        readonly property var componentDataModel: parameterDataModel
        readonly property bool componentOverrideable: parameterOverrideable
        readonly property bool componentOverrode: parameterOverrode
        readonly property string componentLabel: labelText
        readonly property string componentLabelTranslation: labelTranslation
        readonly property string componentDescription: descriptionText
        readonly property string componentDescriptionTranslation: descriptionTranslation
        property var textFieldRegExp: /.*/
        property bool emptyEnabled: false
        property bool hasImage: false
        property bool hasTextField: false

        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: root.minimumHeight
        Layout.maximumHeight: Layout.minimumHeight

        sourceComponent: {
          if (parameterOverrideable && !parameterOverrode) {
            return root.mask
          }

          if (hideValueOnDisabled && !enabled) {
            return root.mask
          }

          switch (parameterUiModel.component) {
            case "text_field": {
              return root.textField
            }
            case "int_text_field": {
              textFieldRegExp = /|[\-\[\],0-9\x22]+/
              return root.textField
            }
            case "float_text_field": {
              textFieldRegExp = /|(^-?[1-9]\d*\.\d+$|^-?0\.\d+$|^-?[1-9]\d*$|^0$)/
              return root.textField
            }
            case "check_box": {
              return root.checkBox
            }
            case "combo_box": {
              return root.comboBox
            }
            case "image_combo_box": {
              hasImage = true
              return root.comboBox
            }
            case "text_field_combo_box": {
              hasTextField = true
              return root.comboBox
            }
            case "int_text_field_combo_box": {
              hasTextField = true
              textFieldRegExp = /|[\-\[\],0-9\x22]+/
              return root.comboBox
            }
            case "material_combo_box": {
              return root.materialComboBox
            }
            default: {
              return root.mask
            }
          }
        }
      }
    }
  }

  readonly property Component blockParameter: Component {
    Loader {
      // for parent
      readonly property var focuseableItem: item ? item.focuseableItem : null

      // for item
      readonly property var componentUiModel: parameterUiModel
      readonly property var componentDataModel: parameterDataModel
      readonly property string componentLabel: labelText
      readonly property string componentLabelTranslation: labelTranslation
      readonly property string componentDescription: descriptionText
      readonly property string componentDescriptionTranslation: descriptionTranslation
      property string formatRole: ""

      sourceComponent: {
        switch (parameterUiModel.component) {
          case "text_edit": {
            formatRole = ""
            return root.textEdit
          }
          case "array_text_edit": {
            formatRole = "array"
            return root.textEdit
          }
          default: {
            return root.mask
          }
        }
      }
    }
  }

  readonly property Component mask: Component {
    Text {
      verticalAlignment: Qt.AlignVCenter
      horizontalAlignment: Qt.AlignRight
      font: Constants.font
      color: Constants.manager_printer_label_color
      text: "N/A"
    }
  }

  readonly property Component textField: Component {
    TextField {
      // for parent
      readonly property var focuseableItem: this

      topPadding: 2 * screenScaleFactor
      bottomPadding: 2 * screenScaleFactor
      leftPadding: 4 * screenScaleFactor
      rightPadding: text_field_unit.contentWidth + 4 * screenScaleFactor
      font.family: Constants.labelFontFamily
      font.weight: Constants.labelFontWeight
      font.pointSize: Constants.labelFontPointSize_10
      color: enabled ? Constants.textColor : Constants.disabledTextColor

      selectByMouse: true
      selectionColor: Constants.themeColor_New
      selectedTextColor: "#FFFFFF"

      property bool _editingFinished: false

      Binding on text {
        value: {
          // force refresh when editing finished
          const _ = _editingFinished
          _editingFinished = false

          if (!componentDataModel) {
            return ""
          }

          if (componentDataModel.partiallyModifyed) {
            return ""
          }

          return componentDataModel.value
        }
      }

      onEditingFinished: {
        if (!emptyEnabled && text.length === 0) {
          _editingFinished = true
          return
        }

        if (componentDataModel.value != text) {
          componentDataModel.value = text
        }

        _editingFinished = true
      }

      Component.onCompleted: {
        _editingFinished = true
      }

      validator: RegularExpressionValidator {
        regularExpression: textFieldRegExp
      }

      background: Rectangle {
        radius: 5 * screenScaleFactor
        color: {
          if (!componentDataModel) {
            return "transparent"
          }

          if (componentDataModel.modifyed) {
            return Constants.parameter_editer_modifyed_color
          }

          return "transparent"
        }

        border.width: 1 * screenScaleFactor
        border.color: {
          if (!componentDataModel) {
            return Constants.right_panel_border_default_color
          }

          if (componentDataModel.exceeded) {
            return Constants.left_model_list_close_button_hovered_color
          }

          if (componentDataModel.modifyed) {
            return Constants.right_panel_border_hovered_color
          }

          if (hovered) {
            return Constants.right_panel_border_hovered_color
          }

          return Constants.right_panel_border_default_color
        }

        Text {
          id: text_field_unit

          anchors.right: parent.right
          anchors.rightMargin: 4 * screenScaleFactor
          anchors.verticalCenter: parent.verticalCenter
          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignRight
          font: parent.parent.font
          color: Constants.parameter_unit_text_color
          text: componentDataModel ? qsTr(componentDataModel.unit) : ""
        }
      }
    }
  }

  readonly property Component textEdit: Component {
    Item {
      // for parent
      readonly property var focuseableItem: this

      implicitHeight: {
        const opened_height = text_edit_title.height + text_edit_text.implicitHeight
        const closed_height = text_edit_title.height + 10 * screenScaleFactor
        return text_edit_button.checked ? opened_height : closed_height
      }

      clip: true

      RowLayout {
        id: text_edit_title

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 10 * screenScaleFactor

        spacing: 0

        Button {
          id: text_edit_button

          focusPolicy: Qt.ClickFocus
          checkable: true
          checked: true

          BasicTooltip {
            visible: root.toolTipEnabled && text_edit_button.hovered
            timeout: -1
            position: root.toolTipPosition
            font.pointSize: Constants.labelFontPointSize_10
            textWidth: root.toolTipWidth
            textWrap: true
            text: componentDescriptionTranslation
            delay: root.toopTipDelay
          }

          background: Item {}

          contentItem: RowLayout {
            spacing: 5 * screenScaleFactor

            Text {
              id: text_edit_label_text

              Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignHCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: root.labelTextColor
              text: componentLabelTranslation

              SequentialAnimation {
                id: text_edit_highlight_animation

                property string color: ""

                loops: 5

                ColorAnimation {
                  target: text_edit_label_text
                  property: "color"
                  to: Constants.themeGreenColor
                  duration: 200
                }

                ColorAnimation {
                  target: text_edit_label_text
                  property: "color"
                  to: text_edit_highlight_animation.color
                  duration: 200
                }
              }

              Connections {
                target: componentUiModel
                ignoreUnknownSignals: true

                function onRequestHighlight() {
                  text_edit_highlight_animation.color = text_edit_label_text.color
                  text_edit_highlight_animation.restart()
                }
              }
            }

            Text {
              Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignHCenter
              visible: text !== ""
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: Constants.parameter_unit_text_color
              text: componentDataModel ? qsTr(componentDataModel.unit) : ""
            }

            Image {
              Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignHCenter
              source: text_edit_button.checked ? "qrc:/UI/photo/comboboxDown2_flip.png"
                                               : "qrc:/UI/photo/comboboxDown2.png"
            }
          }
        }

        Button {
          id: text_edit_reset_button

          implicitWidth: 25 * screenScaleFactor
          implicitHeight: 20 * screenScaleFactor
          Layout.minimumWidth: implicitWidth
          Layout.maximumWidth: Layout.minimumWidth
          Layout.minimumHeight: implicitHeight
          Layout.maximumHeight: Layout.minimumHeight

          visible: componentDataModel && componentDataModel.modifyed
          focusPolicy: Qt.ClickFocus

          onReleased: {
            if (componentDataModel) {
              componentDataModel.resetValue()
            }
          }

          background: Item {}

          contentItem: Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: height

            border.width: 0
            radius: 5 * screenScaleFactor
            color: text_edit_reset_button.pressed ? Constants.leftBtnBgColor_selected
                 : text_edit_reset_button.hovered ? Constants.leftBtnBgColor_hovered
                                                  : "transparent"

            Image {
              anchors.centerIn: parent
              width: 16 * screenScaleFactor
              height: width
              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: Constants.parameter_reset_button_image
            }
          }
        }
      }

      Canvas {
        id: text_edit_background

        readonly property real radius: 5 * screenScaleFactor
        readonly property real beginX: text_edit_title.x + text_edit_title.width
        readonly property real closeX: text_edit_title.x

        anchors.top: text_edit_title.verticalCenter
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        contextType: "2d"

        Connections {
          target: Constants

          function onCurrentThemeChanged(){
            text_edit_background.requestPaint()
          }
        }

        onBeginXChanged: {
          requestPaint()
        }

        onVisibleChanged: {
          if (visible) {
            requestPaint()
          }
        }

        onPaint: {
          const context = getContext("2d")
          context.clearRect(0, 0, width, height)

          context.beginPath()
          context.moveTo(beginX, 0)
          context.lineTo(width - radius, 0)
          context.arcTo(width, 0, width, radius, radius, radius)
          context.lineTo(width, height - radius)
          context.arcTo(width, height, width - radius, height, radius, radius)
          context.lineTo(radius, height)
          context.arcTo(0, height, 0, height - radius, radius, radius)
          context.lineTo(0, radius)
          context.arcTo(0, 0, radius, 0, radius, radius)
          context.lineTo(closeX, 0)

          context.strokeStyle = Constants.right_panel_border_default_color
          context.stroke()
        }
      }

      function source2format(source) {
        switch (formatRole) {
          case "array": {
            return source.split("],[").join("],\n [")
          }
          default: {
            return source
          }
        }
      }

      function format2source(format) {
        switch (formatRole) {
          case "array": {
            return format.split(" ").join("")
                         .split("\n").join("")
          }
          default: {
            return format
          }
        }
      }

      ScrollView {
        id: text_edit_text

        anchors.top: text_edit_title.bottom
        anchors.bottom: text_edit_background.bottom
        anchors.left: text_edit_background.left
        anchors.right: text_edit_background.right
        implicitHeight: text_edit_button.checked ? root.maximumHeight : 0
        topPadding: 0
        bottomPadding: 5 * screenScaleFactor
        leftPadding: 5 * screenScaleFactor
        rightPadding: 5 * screenScaleFactor

        TextArea {
          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_10
          color: root.labelTextColor

          selectByMouse: true
          selectionColor: Constants.themeColor_New
          selectedTextColor: "#FFFFFF"

          property bool _editingFinished: false

          text: {
            // force refresh when editing finished
            const _ = _editingFinished
            _editingFinished = false

            if (!componentDataModel) {
              return ""
            }

            if (componentDataModel.partiallyModifyed) {
              return ""
            }

            return source2format(componentDataModel.value)
          }

          onEditingFinished: {
            const source = format2source(text)
            if (componentDataModel.value != source) {
              componentDataModel.value = source
            }
            _editingFinished = true
          }
        }
      }
    }
  }

  readonly property Component checkBox: Component {
    Item {
      // for parent
      readonly property var focuseableItem: check_box

      CheckBox {
        id: check_box

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 16 * screenScaleFactor
        height: width

        checkState: {
          if (!componentDataModel) {
            return Qt.Unchecked
          }

          if (componentDataModel.partiallyModifyed) {
            return Qt.PartiallyChecked
          }

          return componentDataModel.checked ? Qt.Checked : Qt.Unchecked
        }

        onCheckStateChanged: {
          const checked = checkState === Qt.Checked;
          if (componentDataModel && componentDataModel.checked != checked) {
            componentDataModel.checked = checked
          }
        }

        Keys.onPressed: event => {
          if (event.key === Qt.Key_Space) {
            event.accepted = true
          }
        }

        background: Rectangle {
          radius: 3 * screenScaleFactor
          border.width: 1 * screenScaleFactor
          border.color: check_box.checkState !== Qt.Unchecked || check_box.hovered
              ? Constants.textRectBgHoveredColor : Constants.dialogItemRectBgBorderColor
          color: check_box.checkState !== Qt.Unchecked ? Constants.themeGreenColor
                                                       : "transparent"
        }

        contentItem: Item {}

        indicator: Image {
          width: 9 * screenScaleFactor
          height: 6 * screenScaleFactor
          anchors.centerIn: parent
          source: {
            switch (check_box.checkState) {
              case Qt.Checked: {
                return "qrc:/UI/images/check2.png"
              }
              case Qt.PartiallyChecked: {
                return ""
              }
              case Qt.Unchecked:
              default: {
                return ""
              }
            }
          }
        }
      }
    }
  }

  readonly property Component comboBox: Component {
    CustomComboBox {
      id: combo_box

      // for parent
      readonly property var focuseableItem: this

      // internal
      readonly property bool needImage: hasImage
      readonly property bool needTextField: hasTextField
      readonly property int focusedIndex: componentDataModel ? componentDataModel.currentIndex : 0
      readonly property var sourceModel: componentDataModel ? componentDataModel.enumListModel : null
      readonly property var filterModel: sourceModel ? sourceModel.filterModel : null
      readonly property bool filterModelActived: model == filterModel

      maximumVisibleCount: 10

      model: filterModel ? filterModel : sourceModel

      translater: item => {
        const custom_source = "%1.options.%2.label".arg(componentDataModel.key).arg(item.uid)
        const custom_translation = qsTr(custom_source)
        if (custom_translation !== custom_source) {
          return custom_translation
        }

        return qsTr(item.name)
      }

      displayText: {
        if (!componentDataModel) {
          return ""
        }

        if (componentDataModel.partiallyModifyed) {
          return ""
        }

        const focused_index = filterModelActived ? model.mapIndexFromSource(focusedIndex) : focusedIndex
        return focused_index < 0 ? componentDataModel.value : translater(model.get(focused_index))
      }

      onActivated: {
        const current_index = filterModelActived ? model.mapIndexToSource(currentIndex) : currentIndex
        if (componentDataModel && componentDataModel.currentIndex != current_index) {
          componentDataModel.currentIndex = current_index
        }
      }

      Connections {
        target: root

        function onClosePopup() {
          combo_box.popup.close()
        }
      }

      background: Rectangle {
        radius: 5 * screenScaleFactor
        color: {
          if (!componentDataModel) {
            return "transparent"
          }

          if (componentDataModel.modifyed) {
            return Constants.parameter_editer_modifyed_color
          }

          return "transparent"
        }

        border.width: 1 * screenScaleFactor
        border.color: {
          if (!componentDataModel) {
            return Constants.right_panel_border_default_color
          }

          if (componentDataModel.modifyed) {
            return Constants.right_panel_border_hovered_color
          }

          if (hovered) {
            return Constants.right_panel_border_hovered_color
          }

          return Constants.right_panel_border_default_color
        }
      }

      contentItem: RowLayout {
        anchors.top: combo_box.background.top
        anchors.bottom: combo_box.background.bottom
        anchors.left: combo_box.background.left
        anchors.right: combo_box.indicator.left

        spacing: 3 * screenScaleFactor

        Image {
          id: combo_box_image

          enabled: combo_box.needImage
          visible: combo_box.needImage

          Layout.margins: 2 * screenScaleFactor
          Layout.minimumHeight: parent.height - Layout.margins * 2
          Layout.maximumHeight: Layout.minimumHeight
          Layout.minimumWidth: Layout.minimumHeight
          Layout.maximumWidth: Layout.minimumWidth

          sourceSize.width: width
          sourceSize.height: height
          fillMode: Image.PreserveAspectFit
          source: combo_box.model.get(combo_box.focusedIndex).image

          Connections {
            target: combo_box.model
            function onDataChanged() {
              combo_box_image.source = Qt.binding(_ => {
                return combo_box.model.get(combo_box.focusedIndex).image
              })
            }
          }
        }

        Text {
          enabled: !combo_box.needTextField
          visible: !combo_box.needTextField

          Layout.fillWidth: true
          Layout.fillHeight: true

          topPadding: combo_box.textTopPadding
          bottomPadding: combo_box.textBottomPadding
          leftPadding: combo_box.textLeftPadding
          rightPadding: combo_box.textRightPadding
          verticalAlignment: combo_box.textVerticalAlignment
          horizontalAlignment: combo_box.textHorizontalAlignment

          elide: combo_box.textElide
          color: combo_box.textColor
          font: combo_box.textFont
          text: combo_box.displayText
        }

        TextField {
          id: combo_box_text_field

          enabled: combo_box.needTextField
          visible: combo_box.needTextField

          Layout.fillWidth: true
          Layout.fillHeight: true

          background: Item {}

          topPadding: combo_box.textTopPadding
          bottomPadding: combo_box.textBottomPadding
          leftPadding: combo_box.textLeftPadding
          rightPadding: combo_box_text_field_unit.contentWidth + combo_box.textRightPadding
          verticalAlignment: combo_box.textVerticalAlignment
          horizontalAlignment: combo_box.textHorizontalAlignment

          font: Constants.font
          color: Constants.right_panel_text_default_color

          selectByMouse: true
          selectionColor: Constants.themeColor_New
          selectedTextColor: "#FFFFFF"

          property bool _editingFinished: false

          Binding on text {
            value: {
              // force refresh when editing finished
              const _ = combo_box_text_field._editingFinished
              combo_box_text_field._editingFinished = false
              return combo_box.displayText
            }
          }

          onEditingFinished: {
            if (!emptyEnabled && text.length === 0) {
              _editingFinished = true
              return
            }

            if (componentDataModel.value != text) {
              componentDataModel.value = text
            }

            _editingFinished = true
          }

          Component.onCompleted: {
            _editingFinished = true
          }

          validator: RegularExpressionValidator {
            regularExpression: textFieldRegExp
          }

          Text {
            id: combo_box_text_field_unit

            anchors.right: parent.right
            anchors.rightMargin: 4 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
            font: parent.font
            color: Constants.parameter_unit_text_color
            text: componentDataModel ? qsTr(componentDataModel.unit) : ""
          }
        }
      }

      delegate: ItemDelegate {
        implicitWidth: combo_box.implicitDelegateWidth
        implicitHeight: combo_box.implicitDelegateHeight

        background: Rectangle {
          radius: combo_box.radius
          color: parent.hovered ? Constants.right_panel_item_checked_color : "transparent"
        }

        contentItem: RowLayout {
          anchors.fill: parent

          spacing: 3 * screenScaleFactor

          Image {
            visible: combo_box.needImage

            Layout.margins: 2 * screenScaleFactor
            Layout.minimumHeight: parent.height - Layout.margins * 2
            Layout.maximumHeight: Layout.minimumHeight
            Layout.minimumWidth: Layout.minimumHeight
            Layout.maximumWidth: Layout.minimumWidth

            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: parent.parent.hovered ? model.hoveredImage : model.image
          }

          Text {
            Layout.fillWidth: true
            Layout.fillHeight: true

            topPadding: combo_box.textTopPadding
            bottomPadding: combo_box.textBottomPadding
            leftPadding: combo_box.textLeftPadding
            rightPadding: combo_box.textRightPadding
            verticalAlignment: combo_box.textVerticalAlignment
            horizontalAlignment: combo_box.textHorizontalAlignment

            font: combo_box.textFont
            elide: combo_box.textElide
            color: parent.parent.hovered ? "#FFFFFF" : combo_box.textColor
            text: combo_box.translater(model)
          }
        }
      }
    }
  }

  readonly property Component materialComboBox: Component {
    CustomComboBox {
      id: material_combo_box

      // for parent
      readonly property var focuseableItem: this

      // internal
      readonly property int focusedIndex: componentDataModel ? Number(componentDataModel.value) : 0

      function getMaterialIndexColor(material_color) {
        const background_color = [material_color.r * 255,
                                  material_color.g * 255,
                                  material_color.b * 255];
        const white_contrast = Constants.getContrast(background_color, [255, 255, 255]);
        const black_contrast = Constants.getContrast(background_color, [0, 0, 0]);
        return white_contrast > black_contrast ? "#FFFFFF" : "#000000"
      }

      model: componentDataModel ? componentDataModel.enumListModel : null

      translater: item => item.uid === "0" ? qsTr(item.name) : item.name

      onActivated: {
        componentDataModel.value = model.get(currentIndex).uid
      }

      Connections {
        target: root

        function onClosePopup() {
          material_combo_box.popup.close()
        }
      }

      Connections {
        target: model

        function onCountChanged() {
          if (focusedIndex >= model.count) {
            componentDataModel.resetValue()
          }
        }
      }

      background: Rectangle {
        radius: 5 * screenScaleFactor
        color: {
          if (!componentDataModel) {
            return "transparent"
          }

          if (componentDataModel.modifyed) {
            return Constants.parameter_editer_modifyed_color
          }

          return "transparent"
        }

        border.width: 1 * screenScaleFactor
        border.color: {
          if (!componentDataModel) {
            return Constants.right_panel_border_default_color
          }

          if (componentDataModel.modifyed) {
            return Constants.right_panel_border_hovered_color
          }

          if (hovered) {
            return Constants.right_panel_border_hovered_color
          }

          return Constants.right_panel_border_default_color
        }
      }

      contentItem: RowLayout {
        anchors.top: material_combo_box.background.top
        anchors.bottom: material_combo_box.background.bottom
        anchors.left: material_combo_box.background.left
        anchors.right: material_combo_box.indicator.left
        anchors.rightMargin: 10 * screenScaleFactor

        spacing: 5 * screenScaleFactor

        Rectangle {
          id: material_combo_box_color

          Layout.margins: 2 * screenScaleFactor
          Layout.minimumHeight: parent.height - Layout.margins * 2
          Layout.maximumHeight: Layout.minimumHeight
          Layout.minimumWidth: Layout.minimumHeight
          Layout.maximumWidth: Layout.minimumWidth

          radius: 5 * screenScaleFactor

          color: {
            if (!componentDataModel) {
              return "transparent"
            }

            if (componentDataModel.partiallyModifyed) {
              return "transparent"
            }

            return model.get(focusedIndex).color
          }

          Connections {
            target: material_combo_box.model

            function onDataChanged() {
              material_combo_box_color.color = Qt.binding(_ => {
                if (!componentDataModel) {
                  return "transparent"
                }

                if (componentDataModel.partiallyModifyed) {
                  return "transparent"
                }

                return model.get(focusedIndex).color
              })
            }
          }

          Text {
            id: material_combo_box_index

            anchors.centerIn: parent
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter

            color: getMaterialIndexColor(parent.color)
            font: Constants.font
            text: {
              if (!componentDataModel) {
                return ""
              }

              if (componentDataModel.partiallyModifyed) {
                return ""
              }

              const uid = model.get(focusedIndex).uid
              return uid === "0" ? "" : uid
            }
          }
        }

        Text {
          id: material_combo_box_name

          Layout.fillWidth: true
          Layout.fillHeight: true

          topPadding: material_combo_box.textTopPadding
          bottomPadding: material_combo_box.textBottomPadding
          leftPadding: material_combo_box.textLeftPadding
          rightPadding: material_combo_box.textRightPadding
          verticalAlignment: material_combo_box.textVerticalAlignment
          horizontalAlignment: material_combo_box.textHorizontalAlignment

          elide: material_combo_box.textElide
          color: material_combo_box.textColor
          font: material_combo_box.textFont
          text: {
            if (!componentDataModel) {
              return ""
            }

            if (componentDataModel.partiallyModifyed) {
              return ""
            }

            return translater(model.get(focusedIndex))
          }

          Connections {
            target: material_combo_box.model

            function onDataChanged() {
              material_combo_box_name.text = Qt.binding(_ => {
                if (!componentDataModel) {
                  return ""
                }

                if (componentDataModel.partiallyModifyed) {
                  return ""
                }

                return translater(model.get(focusedIndex))
              })
            }
          }
        }
      }

      delegate: ItemDelegate {
        implicitWidth: material_combo_box.popup.width - material_combo_box.popup.padding * 2
        implicitHeight: material_combo_box.height
        highlighted: material_combo_box.highlightedIndex === index

        background: Rectangle {
          radius: 5 * screenScaleFactor
          color: parent.highlighted ? Constants.right_panel_item_checked_color : "transparent"
        }

        contentItem: RowLayout {
          anchors.fill: parent
          spacing: 5 * screenScaleFactor

          Rectangle {
            Layout.margins: 2 * screenScaleFactor
            Layout.minimumHeight: parent.height - Layout.margins * 2
            Layout.maximumHeight: Layout.minimumHeight
            Layout.minimumWidth: Layout.minimumHeight
            Layout.maximumWidth: Layout.minimumWidth

            radius: 5 * screenScaleFactor
            color: model.color

            Text {
              anchors.centerIn: parent
              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignHCenter
              color: getMaterialIndexColor(parent.color)
              font: Constants.font
              text: model.uid === "0" ? "" : model.uid
            }
          }

          Text {
            Layout.fillWidth: true
            Layout.fillHeight: true

            topPadding: material_combo_box.textTopPadding
            bottomPadding: material_combo_box.textBottomPadding
            leftPadding: material_combo_box.textLeftPadding
            rightPadding: material_combo_box.textRightPadding
            verticalAlignment: material_combo_box.textVerticalAlignment
            horizontalAlignment: material_combo_box.textHorizontalAlignment

            font: material_combo_box.textFont
            elide: material_combo_box.textElide
            color: parent.parent.hovered ? "#FFFFFF" : material_combo_box.textColor
            text: translater(model)
          }
        }
      }
    }
  }
}
