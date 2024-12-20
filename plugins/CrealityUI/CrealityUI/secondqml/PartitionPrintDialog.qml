import QtQml 2.13

import QtQuick 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.0

import Qt.labs.platform 1.1

import "../components"
import "../qml"
import "./"

DockItem {
  id: root

  title: "分区打印"
  width: 800 * screenScaleFactor
  height: 878 * screenScaleFactor

  onVisibleChanged: {
    if (visible) {
      Qt.callLater(() => {
        rect.printerWidth = kernel_parameter_manager.currentMachineWidth()
        rect.printerLength = kernel_parameter_manager.currentMachinedepth()
        rect.originAtCenter = kernel_parameter_manager.currentMachineCenterIsZero()
        rect.resetLayout()

        cf_slice.reset()
        cf_slice.setSliceType(2) // chengfeiSplit::Type::CONTOUR_PARTITION

      })
    } else {
      partition_canvas.clearShape()
    }
  }

  RowLayout {
    id: control_layout

    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: root.titleHeight + anchors.margins

    height: 28 * screenScaleFactor
    spacing: 10 * screenScaleFactor

    Text {
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      text: "功能:"
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_9
    }

    ComboBox {
      id: shape_combobox

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.minimumWidth: 110 * screenScaleFactor
      Layout.fillHeight: true

      currentIndex: 0
      readonly property var currentModel: model.get(currentIndex)
      displayText: currentModel.model_text

      background: Rectangle {
        border.width: 1 * screenScaleFactor
        border.color: Constants.rectBorderColor
        radius: 5 * screenScaleFactor
        color: Constants.dialogItemRectBgColor
      }

      contentItem: Text {
        anchors.left: parent.left
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter

        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
        color: Constants.textColor
        text: parent.displayText
      }

      indicator: Image {
        anchors.right: parent.right
        anchors.rightMargin: 10 * screenScaleFactor
        anchors.verticalCenter: parent.verticalCenter

        width: 7 * screenScaleFactor
        height: 5 * screenScaleFactor
        source: Constants.downBtnImg
        opacity: parent.hovered ? 1 : 0.5
      }

      popup: Popup {
        y: parent.height
        width: parent.width
        implicitHeight: contentItem.implicitHeight

        background: Rectangle {
          border.width: 1 * screenScaleFactor
          border.color: Constants.rectBorderColor
          radius: 5 * screenScaleFactor
          color: shape_combobox.background.color
        }

        contentItem: ListView {
          anchors.fill: parent
          anchors.topMargin: 5 * screenScaleFactor
          anchors.bottomMargin: 5 * screenScaleFactor
          anchors.leftMargin: 1 * screenScaleFactor
          anchors.rightMargin: 1 * screenScaleFactor

          clip: true
          implicitHeight: contentHeight + 10 * screenScaleFactor
          boundsBehavior: Flickable.StopAtBounds
          snapMode: ListView.SnapToItem

          spacing: 0
          model: shape_combobox.delegateModel
          currentIndex: shape_combobox.highlightedIndex
        }
      }

      delegate: ItemDelegate {
        id: item_delegate
        width: parent.width
        height: 18 * screenScaleFactor

        readonly property string modelText: model_text
        readonly property int modelType: model_type

        background: Rectangle {
          color: parent.hovered ? "lightsteelblue" : shape_combobox.background.color
          Text {
            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            text: item_delegate.modelText
          }
        }
      }

      model: ListModel {
        ListElement {
          model_type: PartitionCanvas.Shape.NONE
          model_text: "分区排序"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.SEGMENT
          model_text: "线段切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.RAY
          model_text: "射线切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.LINEAR
          model_text: "直线切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.HORIZONTAL_LINEAR
          model_text: "水平直线切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.VERTICAL_LINEAR
          model_text: "垂直直线切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.CIRCLE
          model_text: "圆形切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.ELLIPSE
          model_text: "椭圆形切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.RECTANGLE
          model_text: "矩形切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.GRID
          model_text: "网格切割"
        }

        ListElement {
          model_type: PartitionCanvas.Shape.STAGGERED_GRID
          model_text: "交错网格切割"
        }
      }
    }

    Text {
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      text: "切割间隙:"
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_9
    }

    Rectangle {
      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      Layout.minimumWidth: 70 * screenScaleFactor

      radius: 5 * screenScaleFactor
      border.width: 1 * screenScaleFactor
      border.color: Constants.rectBorderColor
      color: "transparent"

      TextInput {
        id: interval_input

        readonly property bool vaild: text != "" && Number(text) <= validator.top

        anchors.left: parent.left
        anchors.right: interval_unit.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.rightMargin: 5 * screenScaleFactor
        text: "0.4"
        validator: DoubleValidator {
          notation: DoubleValidator.StandardNotation
          decimals: 1
          bottom: 0.1
          top: 99.9
        }
        color: vaild ? Constants.textColor : "#FF365C"
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9

        onEditingFinished: {
          cf_slice.setInterval(Number(text))
        }
      }

      Text {
        id: interval_unit
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 10 * screenScaleFactor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        text: "mm"
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9
      }
    }

    Text {
      visible: partition_canvas.shapeWidthUsed

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      text: "宽:"
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_9
    }

    Rectangle {
      visible: partition_canvas.shapeWidthUsed

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      Layout.minimumWidth: 90 * screenScaleFactor

      radius: 5 * screenScaleFactor
      border.width: 1 * screenScaleFactor
      border.color: Constants.rectBorderColor
      color: "transparent"

      TextInput {
        id: width_input

        readonly property bool vaild: text != "" && Number(text) <= validator.top

        anchors.left: parent.left
        anchors.right: width_unit.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.rightMargin: 5 * screenScaleFactor
        text: "200.0"
        validator: DoubleValidator {
          notation: DoubleValidator.StandardNotation
          decimals: 1
          bottom: 10.0
          top: 9999.0
        }
        color: vaild ? Constants.textColor : "#FF365C"
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9

        onEditingFinished: {
          partition_canvas.shapeWidth = Number(text)
        }
      }

      Text {
        id: width_unit
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 10 * screenScaleFactor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        text: "mm"
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9
      }
    }

    Text {
      visible: partition_canvas.shapeHeightUsed

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      text: "高:"
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_9
    }

    Rectangle {
      visible: partition_canvas.shapeHeightUsed

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      Layout.minimumWidth: 90 * screenScaleFactor

      radius: 5 * screenScaleFactor
      border.width: 1 * screenScaleFactor
      border.color: Constants.rectBorderColor
      color: "transparent"

      TextInput {
        readonly property bool vaild: text != "" && Number(text) <= validator.top

        anchors.left: parent.left
        anchors.right: height_unit.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.rightMargin: 5 * screenScaleFactor
        text: "100.0"
        validator: DoubleValidator {
          notation: DoubleValidator.StandardNotation
          decimals: 1
          bottom: 10.0
          top: 9999.0
        }
        color: vaild ? Constants.textColor : "#FF365C"
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9

        onEditingFinished: {
          partition_canvas.shapeHeight = Number(text)
        }
      }

      Text {
        id: height_unit
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 10 * screenScaleFactor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        text: "mm"
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9
      }
    }

    Text {
      visible: partition_canvas.shapeRadiusUsed

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      text: "半径:"
      color: Constants.textColor
      font.family: Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_9
    }

    Rectangle {
      visible: partition_canvas.shapeRadiusUsed

      Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
      Layout.fillHeight: true
      Layout.minimumWidth: 90 * screenScaleFactor

      radius: 5 * screenScaleFactor
      border.width: 1 * screenScaleFactor
      border.color: Constants.rectBorderColor
      color: "transparent"

      TextInput {
        readonly property bool vaild: text != "" && Number(text) <= validator.top

        anchors.left: parent.left
        anchors.right: radius_unit.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.rightMargin: 5 * screenScaleFactor
        text: "100.0"
        validator: DoubleValidator {
          notation: DoubleValidator.StandardNotation
          decimals: 1
          bottom: 10.0
          top: 9999.0
        }
        color: vaild ? Constants.textColor : "#FF365C"
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9

        onEditingFinished: {
          partition_canvas.shapeRadius = Number(text)
        }
      }

      Text {
        id: radius_unit
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 10 * screenScaleFactor
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        text: "mm"
        color: Constants.textColor
        font.family: Constants.labelFontFamily
        font.pointSize: Constants.labelFontPointSize_9
      }
    }

    Item {
      Layout.fillWidth: true
    }

    BasicDialogButton {
      Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
      Layout.fillHeight: true
      Layout.minimumWidth: 100 * screenScaleFactor

      btnRadius: 5 * screenScaleFactor
      enabled: interval_input.vaild
      text: "切片"

      onClicked: {
        cf_slice.setLineList(partition_canvas.makeLineList())
        cf_slice.setOrderedPointList(contour_cancas.makeOrderedExamplePointList())
        cf_slice.setInterval(Number(interval_input.text))
        kernel_slice_flow.sliceChengfei()
        root.close()
      }
    }
  }

  Item {
    x: control_layout.x
    y: control_layout.y + control_layout.height + 20 * screenScaleFactor
    width: control_layout.width
    height: root.height - y - 20 * screenScaleFactor

    Item {
      id: rect

      property double printerWidth: 0.0
      property double printerLength: 0.0
      property double scaleFactor: printerWidth / width
      property bool originAtCenter: false

      function resetLayout() {
        if (printerWidth > printerLength) {
          width = parent.width
          height = width * printerLength / printerWidth
        } else {
          height = parent.height
          width = height * printerWidth / printerLength
        }
      }

      anchors.centerIn: parent
    }

    ContourCanvas {
      id: contour_cancas

      anchors.fill: rect

      scaleFactor: rect.scaleFactor
      originAtCenter: rect.originAtCenter
      contourListModel: cf_slice.modelContourList
      enableChangeOrder: partition_canvas.currentShapeType == PartitionCanvas.Shape.NONE
    }

    PartitionCanvas {
      id: partition_canvas

      anchors.fill: rect

      scaleFactor: rect.scaleFactor
      originAtCenter: rect.originAtCenter
      drawHistoryShape: false
      currentShapeType: shape_combobox.currentModel.model_type

      onNewSegmentAdded: {
        cf_slice.setLineList(partition_canvas.makeLineList())
        cf_slice.setInterval(Number(interval_input.text))
        cf_slice.preSlice()
      }
    }
  }
}
