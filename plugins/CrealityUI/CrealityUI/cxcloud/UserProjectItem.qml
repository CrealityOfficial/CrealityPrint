import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import ".."
import "../secondqml"

Rectangle {
  id: root

  property string itemUid: ""
  property string itemName: ""
  property string itemImage: ""
  property string itemPrinterName: ""
  property string itemLayerHeight: ""
  property string itemSparseInfillDensity: ""
  property int itemPlateCount: 0
  property int itemPrintDuraction: 0
  property int itemMaterialWeight: 0
  property string itemAuthorId: ""
  property string itemAuthorName: ""
  property string itemAuthorImage: ""
  property int itemCreationTimepoint: 0

  property bool showImportButton: true

  property bool showAuthorSelf: false
  readonly property bool isAuthorSelf: {
    if (itemAuthorId == "") {
      return true
    }

    return cxkernel_cxcloud.accountService.userId == itemAuthorId
  }

  signal requestImport
  signal requestDelete

  width: 470 * screenScaleFactor
  height: 120 * screenScaleFactor

  color: "#FFFFFF"
  radius: 10 * screenScaleFactor
  border.width: (Constants.currentTheme ? 1 : 0) * screenScaleFactor
  border.color: "#D7D7DD"

  RowLayout {
    anchors.fill: parent
    anchors.margins: root.border.width

    spacing: 8 * screenScaleFactor

    Rectangle {
      Layout.fillHeight: true
      Layout.minimumWidth: height
      Layout.maximumWidth: Layout.minimumWidth

      radius: root.radius
      color: "#D7D7DD"

      layer.enabled: true
      layer.effect: OpacityMask {
        maskSource: image_background
      }

      Rectangle {
        id: image_background

        anchors.fill: parent
        radius: parent.radius
        color: parent.color

        Rectangle {
          anchors.top: parent.top
          anchors.bottom: parent.bottom
          anchors.left: parent.horizontalCenter
          anchors.right: parent.right
          color: parent.color
        }
      }

      Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        source: itemImage
      }
    }

    ColumnLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.topMargin: 8 * screenScaleFactor
      Layout.bottomMargin: 8 * screenScaleFactor

      spacing: 8 * screenScaleFactor

      Text {
        Layout.fillWidth: true
        Layout.preferredHeight: 20 * screenScaleFactor

        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft

        font.family: Constants.labelFontFamily
        font.weight: Font.Bold
        font.pointSize: Constants.labelFontPointSize_11

        clip :true
        maximumLineCount:1
        elide: Text.ElideRight
        wrapMode: Text.WrapAnywhere

        color: "#333333"
        text: root.itemName
      }

      GridLayout {
        Layout.fillWidth: true

        columns: 2

        columnSpacing: 8 * screenScaleFactor
        rowSpacing: 4 * screenScaleFactor

        Control {
          Layout.minimumWidth: 14 * screenScaleFactor
          Layout.minimumHeight: 14 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight

          visible: root.itemPrinterName.length > 0

          BasicTooltip {
            visible: parent.hovered
            text: qsTr("Printer Type")
          }

          background: Item {}
          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 14 * screenScaleFactor
              height: 14 * screenScaleFactor

              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: "qrc:/UI/photo/userInfo_printer_name.svg"
            }
          }
        }

        Text {
          Layout.fillWidth: true
          Layout.preferredHeight: 14 * screenScaleFactor

          visible: root.itemPrinterName.length > 0

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignLeft

          font.family: Constants.labelFontFamily
          font.weight: Font.Bold
          font.pointSize: Constants.labelFontPointSize_10

          color: "#333333"
          text: root.itemPrinterName
        }

        Control {
          Layout.minimumWidth: 14 * screenScaleFactor
          Layout.minimumHeight: 14 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight

          visible: root.itemPlateCount > 0

          BasicTooltip {
            visible: parent.hovered
            text: qsTr("Plate Count")
          }

          background: Item {}
          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 14 * screenScaleFactor
              height: 14 * screenScaleFactor

              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: "qrc:/UI/photo/userInfo_plate_count.svg"
            }
          }
        }

        Text {
          Layout.fillWidth: true
          Layout.preferredHeight: 14 * screenScaleFactor

          visible: root.itemPlateCount > 0

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignLeft

          font.family: Constants.labelFontFamily
          font.weight: Font.Bold
          font.pointSize: Constants.labelFontPointSize_10

          color: "#333333"
          text: root.itemPlateCount
        }

        Control {
          Layout.minimumWidth: 14 * screenScaleFactor
          Layout.minimumHeight: 14 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight

          visible: root.itemLayerHeight.length > 0

          BasicTooltip {
            visible: parent.hovered
            text: qsTranslate("ParameterComponent", "Layer Height")
          }

          background: Item {}
          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 14 * screenScaleFactor
              height: 14 * screenScaleFactor

              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: "qrc:/UI/photo/userInfo_layer_height.svg"
            }
          }
        }

        Text {
          Layout.fillWidth: true
          Layout.preferredHeight: 14 * screenScaleFactor

          visible: root.itemLayerHeight.length > 0

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignLeft

          font.family: Constants.labelFontFamily
          font.weight: Font.Bold
          font.pointSize: Constants.labelFontPointSize_10

          color: "#333333"
          text: "%1mm".arg(root.itemLayerHeight)
        }

        Control {
          Layout.minimumWidth: 14 * screenScaleFactor
          Layout.minimumHeight: 14 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight

          visible: root.itemSparseInfillDensity.length > 0

          BasicTooltip {
            visible: parent.hovered
            text: qsTranslate("ParameterComponent", "Sparse infill density")
          }

          background: Item {}
          contentItem: Item {
            anchors.fill: parent

            Image {
              anchors.centerIn: parent
              width: 14 * screenScaleFactor
              height: 14 * screenScaleFactor

              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: "qrc:/UI/photo/userInfo_sparse_infill_density.svg"
            }
          }
        }

        Text {
          Layout.fillWidth: true
          Layout.preferredHeight: 14 * screenScaleFactor

          visible: root.itemSparseInfillDensity.length > 0

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignLeft

          font.family: Constants.labelFontFamily
          font.weight: Font.Bold
          font.pointSize: Constants.labelFontPointSize_10

          color: "#333333"
          text: root.itemSparseInfillDensity
        }
      }

      Item {
        Layout.fillHeight: true
      }
    }

    ColumnLayout {
      Layout.fillHeight: true
      Layout.topMargin: 8 * screenScaleFactor
      Layout.bottomMargin: 8 * screenScaleFactor
      Layout.rightMargin: 8 * screenScaleFactor

      spacing: 10 * screenScaleFactor

      RowLayout {
        Layout.minimumHeight: 28 * screenScaleFactor
        Layout.maximumHeight: Layout.minimumHeight
        Layout.alignment: Layout.AlignTop | Layout.AlignRight

        spacing: 5 * screenScaleFactor

        Item {
          Layout.fillWidth: true
        }

        Button {
          id: import_button

          Layout.minimumWidth: 28 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.minimumHeight: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight
          Layout.alignment: Layout.AlignVCenter | Layout.AlignRight

          visible: root.showImportButton

          BasicTooltip {
            visible: parent.hovered
            text: qsTr("Import Print Configuration")
          }

          background: Rectangle {
            radius: import_button.height / 2
            color: import_button.hovered ? "#17CC5F" : "#D6FFE6"
          }

          contentItem: Item {
            Image {
              anchors.centerIn: parent
              width: 14 * screenScaleFactor
              height: 14 * screenScaleFactor
              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: import_button.hovered
                  ? "qrc:/UI/photo/userInfo_import_hover.svg"
                  : "qrc:/UI/photo/userInfo_import_normal.svg"
            }
          }

          onClicked: {
            root.requestImport()
          }
        }

        Button {
          id: delete_button

          Layout.minimumWidth: 28 * screenScaleFactor
          Layout.maximumWidth: Layout.minimumWidth
          Layout.minimumHeight: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight
          Layout.alignment: Layout.AlignVCenter | Layout.AlignRight

          visible: root.isAuthorSelf

          BasicTooltip {
            visible: parent.hovered
            text: qsTr("Delete Print Configuration")
          }

          background: Rectangle {
            radius: delete_button.height / 2
            color: delete_button.hovered ? "#17CC5F" : "#D6FFE6"
          }

          contentItem: Item {
            Image {
              anchors.centerIn: parent
              width: 14 * screenScaleFactor
              height: 14 * screenScaleFactor
              sourceSize.width: width
              sourceSize.height: height
              fillMode: Image.PreserveAspectFit
              source: delete_button.hovered
                  ? "qrc:/UI/photo/userInfo_delete_hover.svg"
                  : "qrc:/UI/photo/userInfo_delete_normal.svg"
            }
          }

          onClicked: {
            root.requestDelete()
          }
        }
      }

      Item {
        Layout.fillHeight: true
      }

      RowLayout {
        Item {
          Layout.fillWidth: true
        }

        Text {
          Layout.fillWidth: true
          Layout.preferredHeight: contentHeight
          Layout.alignment: Layout.AlignBottom | Layout.AlignRight

          visible: root.isAuthorSelf ? root.showAuthorSelf : true

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignRight

          font.family: Constants.labelFontFamily
          font.weight: Font.Bold
          font.pointSize: Constants.labelFontPointSize_10

          color: root.isAuthorSelf ? "#17CC5F" : "#999999"
          text: root.isAuthorSelf ? qsTr("me") : root.itemAuthorName
        }
      }

      RowLayout {
        Item {
          Layout.fillWidth: true
        }

        Text {
          Layout.minimumWidth: contentWidth
          Layout.minimumHeight: contentHeight
          Layout.maximumWidth: Layout.minimumWidth
          Layout.maximumHeight: Layout.minimumHeight
          Layout.alignment: Layout.AlignBottom | Layout.AlignRight

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignRight

          font.family: Constants.labelFontFamily
          font.weight: Font.Normal
          font.pointSize: Constants.labelFontPointSize_10

          color: "#999999"
          text: Qt.formatDateTime(new Date(root.itemCreationTimepoint * 1000), 'yyyy-MM-dd')
        }
      }
    }
  }
}
