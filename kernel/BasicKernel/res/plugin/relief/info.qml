import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import CrealityUI 1.0

import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

LeftPanelDialog {
  id: root

  // width: panel.implicitWidth + 2 * panel.margins
  // height: panel.implicitHeight + 2 * panel.margins + titleHeight
  width: 300 * screenScaleFactor
  height: 355 * screenScaleFactor + (com ? com.canEmboss : 0) * 112 * screenScaleFactor
  title: command.name

  property var com: null

  readonly property var command: com ? com : {
    name: "",
    typeModel: [],
    shapeModel: [],
    fontModel: [],
    type: "",
    shape: "",
    text: "",
  }

  readonly property string imageFormat: "qrc:/relief/image/%1%2%3.svg"
  readonly property string themeSuffix: Constants.currentTheme ? "_light" : ""
  readonly property string checkedSuffix: "_checked"
  readonly property real editerWidth: 174 * screenScaleFactor

  function execute() {
    synchronizeData()
  }

  function synchronizeData() {
    if (idText.text != com.text)
      idText.text = com.text;

    if (idFontSize.realValue != com.fontSize)
      idFontSize.realValue = com.fontSize

    idFont.setFontFamily(com.font)

    if (idWordSpace.realValue != com.wordSpace)
      idWordSpace.realValue = com.wordSpace

    if (idLineSpace.realValue != com.lineSpace)
      idLineSpace.realValue = com.lineSpace

    if (idHeight.realValue != com.height)
      idHeight.realValue = com.height

    if (idDistance.realValue != com.distance)
      idDistance.realValue = com.distance

    if (idEmbossType.value != com.embossType)
      idEmbossType.value = com.embossType

    if (idReliefModelType.value != com.reliefModelType)
      idReliefModelType.value = com.reliefModelType

    if (idItalic.checked != com.italic)
      idItalic.checked = com.italic

    if (idBold.checked != com.bold)
      idBold.checked = com.bold

  }

  Connections {
    target: com

    onFontConfigChanged: {
      synchronizeData()
    }

    onHoverOnModelChanged: {
      if (com.hoverOnModel)
            idReliefIndicator.setColor("white")
        else 
            idReliefIndicator.setColor("red")
    }

  }

  MouseArea {
    anchors.fill: parent

    onClicked: {
      parent.focus = true
    }
  }

  ReliefIndicator {
      id : idReliefIndicator
      parent: gGlScene
      anchors.fill: parent

      pos: com ? com.pos : Qt.point(-1000, -1000)
      needIndicate: com ? com.needIndicate : false

      visible: root.visible
  }

  ColumnLayout {
    id: panel

    readonly property real margins: 20 * screenScaleFactor

    anchors.fill: root.panelArea
    anchors.margins: margins
    spacing: 5 * screenScaleFactor

    // implicitWidth: Math.max(text_title.width,
    //                         font_title.width,
    //                         letter_spacing_title.width,
    //                         line_spacing_title.width,
    //                         font_size_title.width,
    //                         height_title.width,
    //                         depth_title.width,
    //                         model_style_title.width,
    //                         model_type_title.width) + spacing + root.editerWidth

    // implicitHeight: type_title.height + spacing
    //               + type_view.height + type_view.Layout.topMargin + spacing
    //               + shape_title.height + shape_title.Layout.topMargin + spacing
    //               + shape_layout.height + shape_layout.Layout.topMargin + spacing
    //               + text_layout.height + text_layout.Layout.topMargin + spacing
    //               + font_layout.height + spacing
    //               + letter_spacing_layout.height + spacing
    //               + line_spacing_layout.height + spacing
    //               + font_size_layout.height + spacing
    //               + height_layout.height + spacing
    //               + depth_layout.height + spacing
    //               + model_style_layout.height + spacing
    //               + model_type_layout.height

    // components

    component CustomText: Text {
      verticalAlignment: Qt.AlignVCenter
      horizontalAlignment: Qt.AlignLeft
      font.family: Constants.labelFontFamily
      font.weight: Constants.labelFontWeight
      font.pointSize: Constants.labelFontPointSize_10
      color: Constants.textColor
      clip: true
    }

    component CustomRadioButton: RadioButton {
      implicitWidth: indicator.width + contentItem.contentWidth + 3 * screenScaleFactor
      implicitHeight: Math.max(contentItem.contentHeight, indicator.height)

      clip: true

      contentItem: CustomText {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.indicator.right
        anchors.leftMargin: 3 * screenScaleFactor
        text: parent.text
      }

      indicator: Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        width: 14 * screenScaleFactor
        height: width
        radius: height / 2
        border.width: 1 * screenScaleFactor
        border.color: parent.checked ? Constants.themeColor_New
                                     : Constants.dialogItemRectBgBorderColor
        color: parent.checked ? Constants.themeColor_New : "transparent"

        Rectangle {
          anchors.centerIn: parent
          width: 6 * screenScaleFactor
          height: width
          radius: height / 2
          color: parent.parent.checked ? "#FFFFFF" : "transparent"
        }
      }
    }

    // type

    CustomText {
      id: type_title

      Layout.fillWidth: true
      Layout.minimumHeight: contentHeight
      text: qsTranslate("ReliefCommand", "Relief Type")
    }

    ListView {
      id: type_view

      Layout.minimumWidth: contentWidth
      Layout.maximumWidth: Layout.minimumWidth
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.topMargin: 5 * screenScaleFactor

      orientation: ListView.Horizontal
      spacing: 10 * screenScaleFactor
      clip: true

      model: command.typeModel
      delegate: ItemDelegate {
        width: 28 * screenScaleFactor
        height: 28 * screenScaleFactor

        highlighted: ListView.view.currentIndex === model.index

        Component.onCompleted: {
          if (model.name === command.type) {
            ListView.view.currentIndex = model.index
          }
        }

        onClicked: {
          ListView.view.currentIndex = model.index
          command.type = model.name
        }

        BasicTooltip {
          visible: parent.hovered
          text: qsTranslate("ReliefCommand", model.name)
        }

        background: Rectangle {
          radius: 5 * screenScaleFactor
          border.width: 1 * screenScaleFactor
          border.color: highlighted || hovered ? Constants.themeColor_New
                                               : Constants.dialogItemRectBgBorderColor
          color: highlighted ? Constants.themeColor_New : "transparent"
        }

        contentItem: Item {
          anchors.fill: parent

          Image {
            anchors.centerIn: parent
            width: 16 * screenScaleFactor
            height: 16 * screenScaleFactor
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: root.imageFormat.arg(model.name)
                                    .arg(root.themeSuffix)
                                    .arg(highlighted ? root.checkedSuffix : "")
          }
        }
      }
    }

    // shape

    CustomText {
      id: shape_title

      visible: command.type === "shape"
      Layout.fillWidth: true
      Layout.minimumHeight: 25 * screenScaleFactor
      Layout.topMargin: topMargin
      text: qsTranslate("ReliefCommand", "shape-title")
    }

    RowLayout {
      id: shape_layout

      visible: command.type === "shape"

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.topMargin: 5 * screenScaleFactor

      spacing: 2 * screenScaleFactor

      ListView {
        Layout.minimumWidth: contentWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        orientation: ListView.Horizontal
        spacing: 10 * screenScaleFactor
        clip: true

        model: command.shapeModel
        delegate: ItemDelegate {
          implicitWidth: 28 * screenScaleFactor
          implicitHeight: 28 * screenScaleFactor

        highlighted: ListView.view.currentIndex === model.index

        Component.onCompleted: {
          if (model.name === command.shape) {
            ListView.view.currentIndex = model.index
          }
        }

        onClicked: {
          ListView.view.currentIndex = model.index
          command.shape = model.name
        }

          BasicTooltip {
            visible: parent.hovered
            text: qsTranslate("ReliefCommand", model.name)
          }

          background: Rectangle {
            radius: 5 * screenScaleFactor
            border.width: 1 * screenScaleFactor
            border.color: highlighted || hovered ? Constants.themeColor_New
                                                 : Constants.dialogItemRectBgBorderColor
            color: highlighted ? Constants.themeColor_New : "transparent"
          }

          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 16 * screenScaleFactor
              height: 16 * screenScaleFactor
              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: root.imageFormat.arg(model.name)
                                      .arg(root.themeSuffix)
                                      .arg(highlighted ? root.checkedSuffix : "")
            }
          }
        }
      }

      Button {
        Layout.minimumWidth: 16 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight
        Layout.leftMargin: 14 * screenScaleFactor

        BasicTooltip {
          visible: parent.hovered
          text: qsTranslate("ReliefCommand", "shape-custom")
        }

        background: Item {}
        contentItem: Item {
          anchors.fill: parent

          Image {
            anchors.centerIn: parent
            width: 16 * screenScaleFactor
            height: 16 * screenScaleFactor
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: root.imageFormat.arg("load")
                                    .arg(root.themeSuffix)
                                    .arg(parent.hovered ? root.checkedSuffix : "")
          }
        }
      }

      Button {
        Layout.minimumWidth: 18 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        BasicTooltip {
          visible: parent.hovered
          text: qsTranslate("ReliefCommand", "shape-tip")
        }

        background: Item {}
        contentItem: Item {
          anchors.fill: parent

          Image {
            anchors.centerIn: parent
            width: 18 * screenScaleFactor
            height: 18 * screenScaleFactor
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: root.imageFormat.arg("tip")
                                    .arg(root.themeSuffix)
                                    .arg(parent.hovered ? root.checkedSuffix : "")
          }
        }
      }

      Item {
        Layout.fillWidth: true
      }
    }

    // text

    RowLayout {
      id: text_layout

      visible: command.type === "text"

      Layout.fillWidth: true
      Layout.minimumHeight: 48 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.topMargin: 15 * screenScaleFactor

      spacing: panel.spacing

      CustomText {
        id: text_title

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
        text: qsTranslate("ReliefCommand", "Text")
      }

      ScrollView {
        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        ScrollBar.vertical.policy:
            implicitContentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy:
            implicitContentWidth > width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

        background: Rectangle {
          radius: 5 * screenScaleFactor
          border.width: 1 * screenScaleFactor
          border.color: Constants.dialogItemRectBgBorderColor
          color: "transparent"
        }

        TextArea {
          id: idText
          font.family: Constants.labelFontFamily
          font.weight: Constants.labelFontWeight
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.parameter_text_color
          textMargin: 1

          placeholderText: qsTranslate("ReliefCommand", "The content cannot be empty")

          selectByMouse: true
          selectionColor: Constants.themeColor_New
          selectedTextColor: "#FFFFFF"

          function isWhitespaceOrNewline(str) {
              return /^\s*$/.test(str);
          }

          property var error: (text == "") || isWhitespaceOrNewline(text)

          background: Rectangle {
            property var defaultColor: Constants.currentTheme == 0 ? "#6E6E72" : "#D6D6DC";
            border.color: idText.error ? "red" : (idText.hovered ? Constants.textRectBgHoveredColor : defaultColor)
            border.width: 1
            radius: 5
            color: "transparent"
        }

          Component.onCompleted: {
            if (isWhitespaceOrNewline(text))
              return;

            text = command.text
          }

          onTextChanged: {
            if (isWhitespaceOrNewline(text))
              return;

            if (com && com.text != text)
              com.text = text
          }
        }
      }
    }

    // font

    RowLayout {
      id: font_layout

      visible: command.type === "text"

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: panel.spacing

      CustomText {
        id: font_title

        Layout.fillWidth: true
        Layout.fillHeight: true
        text: qsTranslate("ReliefCommand", "Font")
      }

      RowLayout {
        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        CustomComboBox {
          id: idFont
          Layout.fillWidth: true
          Layout.fillHeight: true
          maximumVisibleCount: 10
          // model: Qt.fontFamilies()
          property var fontFamilies: com ? com.fliterFontFamilies(Qt.fontFamilies()) : Qt.fontFamilies()
          model: fontFamilies

          function setFontFamily(fontFamily)
          {
              for (let i = 0, count = fontFamilies.length; i < count; ++i) {
                let f = fontFamilies[i]
                if (fontFamily == f) {
                  if (currentIndex != i)
                    currentIndex = i;
                  break;
                }

              }
          }
          
          onCurrentTextChanged: {
            if (com && com.font != currentText)
              com.font = currentText
          } 
        }

        Button {
          id: idBold
          Layout.minimumWidth: 12 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.minimumHeight: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight
          Layout.leftMargin: 5 * screenScaleFactor

          checkable: true

          onCheckedChanged: {
            if (com && checked != com.bold)
              com.bold = checked
          }

          BasicTooltip {
            visible: parent.hovered
            text: qsTranslate("ReliefCommand", "Bold")
          }

          background: Item {}
          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 12 * screenScaleFactor
              height: 12 * screenScaleFactor
              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: root.imageFormat.arg("font_bold")
                                      .arg(root.themeSuffix)
                                      .arg(parent.parent.checked ? root.checkedSuffix : "")
            }
          }
        }

        Button {
          id: idItalic
          Layout.minimumWidth: 12 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.minimumHeight: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight

          checkable: true

          onCheckedChanged: {
            if (com && checked != com.italic)
              com.italic = checked
          }

          BasicTooltip {
            visible: parent.hovered
            text: qsTranslate("ReliefCommand", "Italic")
          }

          background: Item {}
          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 12 * screenScaleFactor
              height: 12 * screenScaleFactor
              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: root.imageFormat.arg("font_italic")
                                      .arg(root.themeSuffix)
                                      .arg(parent.parent.checked ? root.checkedSuffix : "")
            }
          }
        }
      }
    }

    // letter spacing

    RowLayout {
      id: letter_spacing_layout

      visible: command.type === "text"

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: panel.spacing

      CustomText {
        id: letter_spacing_title

        Layout.fillWidth: true
        Layout.fillHeight: true
        text: qsTranslate("ReliefCommand", "Word Space")
      }
      
      CusStyledSpinBox {
        id: idWordSpace

        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        stepSize: 1 * Math.pow(10, decimals)
        from: -99999 * Math.pow(10, decimals)
        to: 99999 * Math.pow(10, decimals)
        decimals: 0
        unitchar: "px"
        font.pointSize: Constants.labelFontPointSize_9
        textValidator: RegExpValidator {
            regExp: /^[-+]?(\d{1,5})$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
        }

        onTextContentChanged: {
            if (realValue != result)
              realValue = result
              
        }

        onRealValueChanged: {
            command.wordSpace = realValue
        }

      }
    }

    // line spacing

    RowLayout {
      id: line_spacing_layout

      visible: command.type === "text"

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: panel.spacing

      CustomText {
        id: line_spacing_title

        Layout.fillWidth: true
        Layout.fillHeight: true
        text: qsTranslate("ReliefCommand", "Line Space")

        opacity: idLineSpace.opacity
      }

      CusStyledSpinBox {
        id: idLineSpace

        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        enabled: idText.text.indexOf("\n") != -1

        stepSize: 1 * Math.pow(10, decimals)
        from: -99999 * Math.pow(10, decimals)
        to: 99999 * Math.pow(10, decimals)
        decimals: 0
        unitchar: "px"
        font.pointSize: Constants.labelFontPointSize_9

        textValidator: RegExpValidator {
            regExp: /^[-+]?(\d{1,5})$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
        }

        onTextContentChanged: {
            if (realValue != result)
              realValue = result
              
        }

        onRealValueChanged: {
            command.lineSpace = realValue
        }

      }
    }

    // font size

    RowLayout {
      id: font_size_layout

      visible: command.type === "text"

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: panel.spacing

      CustomText {
        id: font_size_title

        Layout.fillWidth: true
        Layout.fillHeight: true
        text: qsTranslate("ReliefCommand", "Font Size")
      }

      CusStyledSpinBox {
        id: idFontSize

        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        stepSize: 1 * Math.pow(10, decimals)
        from: 1 * Math.pow(10, decimals)
        to: 100 * Math.pow(10, decimals)
        decimals: 0
        unitchar: "pt"
        font.pointSize: Constants.labelFontPointSize_9
        textValidator: RegExpValidator {
            regExp: /^[+]?(\d{1,3})$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
        }

        onTextContentChanged: {
            if (realValue != result)
              realValue = result
              
        }

        onRealValueChanged: {
            if (command.fontSize != realValue)
              command.fontSize = realValue
        }

      }
    }

    // height

    RowLayout {
      id: height_layout

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: panel.spacing

      CustomText {
        id: height_title

        Layout.fillWidth: true
        Layout.fillHeight: true
        text: qsTranslate("ReliefCommand", "Thickness")
      }
      
      CusStyledSpinBox {
        id: idHeight

        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        stepSize: 1 * Math.pow(10, decimals)
        from: 0.01 * Math.pow(10, decimals)
        to: 99999 * Math.pow(10, decimals)
        decimals: 2
        unitchar: "mm"
        font.pointSize: Constants.labelFontPointSize_9
        textValidator: RegExpValidator {
            regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
        }

        onTextContentChanged: {
            if (realValue != result)
              realValue = result
              
        }

        onRealValueChanged: {
            command.height = realValue
        }

      }
    }

    // depth

    RowLayout {
      id: depth_layout

      visible: com ? com.canEmboss : false

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight

      spacing: panel.spacing

      CustomText {
        id: depth_title

        Layout.fillWidth: true
        Layout.fillHeight: true
        text: qsTranslate("ReliefCommand", "Embeded Depth")

        opacity: idDistance.opacity
      }

      CusStyledSpinBox {
        id: idDistance

        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth
        Layout.fillHeight: true

        // enabled: com ? com.distanceEnabled : false

        stepSize: 1 * Math.pow(10, decimals)
        from: -99999 * Math.pow(10, decimals)
        to: 99999 * Math.pow(10, decimals)
        decimals: 2
        unitchar: "mm"
        font.pointSize: Constants.labelFontPointSize_9
        textValidator: RegExpValidator {
            regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
        }

        onTextContentChanged: {
            if (realValue != result)
              realValue = result
              
        }

        onRealValueChanged: {
            command.distance = realValue
        }

      }

    }

    // model style

    RowLayout {
      id: model_style_layout

      visible: com ? com.canEmboss : false

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor

      spacing: panel.spacing

      CustomText {
        id: model_style_title

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        text: qsTranslate("ReliefCommand", "Emboss")
      }

      Flow {
        id: idEmbossType

        property var value: 0
        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth

        padding: 0
        spacing: 0 * screenScaleFactor

        // enabled: false

        onValueChanged: {
          if (value == 0) {
            idEmboss1.checked = false 
            idEmboss2.checked = false 
          } else if (value == 1) {
            idEmboss1.checked = true 
            idEmboss2.checked = false  
          } else if (value == 2) {
            // idDistance.realValue = 0
            idEmboss1.checked = false  
            idEmboss2.checked = true 
          } else if (value == 3) {
            // idDistance.realValue = 0 
            idEmboss1.checked = true 
            idEmboss2.checked = true 
          }
        }

        StyleCheckBox {
            id: idEmboss1
            width: parent.width / 2
            // anchors.top: parent.top
            // anchors.bottom: parent.bottom
            textColor: Constants.textColor
            text: qsTranslate("ReliefCommand", "Horizontal")
            font.pointSize: Constants.labelFontPointSize_10
            checked: false
            visible: false

            onCheckedChanged: {
                let type = 0
                if (idEmboss1.checked) 
                    type += 1 
                
                if (idEmboss2.checked) 
                    type += 2

                if (com && com.embossType != type)
                    com.embossType = type;

                if (idEmbossType.value != type)
                    idEmbossType.value = type;
            }
        }

        StyleCheckBox {
            id: idEmboss2
            width: parent.width / 2
            // anchors.top: parent.top
            // anchors.bottom: parent.bottom
            textColor: Constants.textColor
            text: qsTranslate("ReliefCommand", "Surface")
            font.pointSize: Constants.labelFontPointSize_14
            checked: false

            onCheckedChanged: {
                let type = 0
                if (idEmboss1.checked) 
                    type += 1 
                
                if (idEmboss2.checked) 
                    type += 2

                if (com && com.embossType != type)
                    com.embossType = type;

                if (idEmbossType.value != type)
                    idEmbossType.value = type;
            }
        }
      }
    }

    // model type

    RowLayout {
      id: model_type_layout

      visible: com ? com.canEmboss : false

      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor

      spacing: panel.spacing

      CustomText {
        id: model_type_title

        height: parent.height / 2
        anchors.top: parent.top

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        text: qsTranslate("ReliefCommand", "Relief Attribute")
      }

      Grid {
        id: idReliefModelType

        property var value: 0
        Layout.minimumWidth: root.editerWidth
        Layout.maximumWidth: Layout.minimumWidth

        columns: 2
        rows: 2

        padding: 0
        spacing: 0 * screenScaleFactor
        rowSpacing: 7 * screenScaleFactor

        onValueChanged: {
          if (value == 0) {
            idModelType1.checked = true
          } else if (value == 1) {
            idModelType2.checked = true
          } else if (value == 2) {
            idModelType3.checked = true
          }
        }

        CustomRadioButton {
          id: idModelType1
          width: parent.width / 2
          text: qsTranslate("ReliefCommand", "Part")
          checked: true

          onCheckedChanged: {
            if (checked && com && com.reliefModelType != 0) {
              com.reliefModelType = 0
            }
          }
        }

        CustomRadioButton {
          id: idModelType2
          width: parent.width / 2
          text: qsTranslate("ReliefCommand", "Negative Part")

          onCheckedChanged: {
            if (checked && com && com.reliefModelType != 1) {
              com.reliefModelType = 1
            }
          }
        }

        CustomRadioButton {
          id: idModelType3
          width: parent.width / 2
          text: qsTranslate("ReliefCommand", "Modifier")

          onCheckedChanged: {
            if (checked && com && com.reliefModelType != 2) {
              com.reliefModelType = 2
            }
          }
        }
      }
    }

    Item {
      Layout.fillHeight: true
    }
  }
}
