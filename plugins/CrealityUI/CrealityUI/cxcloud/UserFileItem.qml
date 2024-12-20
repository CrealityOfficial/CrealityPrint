import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtGraphicalEffects 1.15

import ".."
import "../secondqml"
import "../components"

Rectangle {
  id: root

  property string itemUid: ""
  property string itemName: ""
  property string itemImage: ""
  property bool itemRestricted: false

  signal requestImport
  signal requestExport
  signal requestDelete
  signal requestDetail

  state: "uploaded_model"
  states: [
    State {
      name: "uploaded_model"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Model")
        buttonImage: "qrc:/UI/photo/userInfo_import_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_import_hover.svg"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Share Model (Copy Link)")
        buttonImage: "qrc:/UI/photo/userInfo_share_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_share_hover.svg"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Model")
        buttonImage: "qrc:/UI/photo/userInfo_delete_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_delete_hover.svg"
      }

      PropertyChanges {
        target: detail_button
        visible: true
      }
    },

    State {
      name: "collected_model"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Model")
        buttonImage: "qrc:/UI/photo/userInfo_import_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_import_hover.svg"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Share Model (Copy Link)")
        buttonImage: "qrc:/UI/photo/userInfo_share_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_share_hover.svg"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Model")
        buttonImage: "qrc:/UI/photo/userInfo_delete_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_delete_hover.svg"
      }

      PropertyChanges {
        target: detail_button
        visible: true
      }
    },

    State {
      name: "purchased_model"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Model")
        buttonImage: "qrc:/UI/photo/userInfo_import_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_import_hover.svg"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Share Model (Copy Link)")
        buttonImage: "qrc:/UI/photo/userInfo_share_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_share_hover.svg"
      }

      PropertyChanges {
        target: delete_button
        visible: false
      }

      PropertyChanges {
        target: detail_button
        visible: true
      }
    },

    State {
      name: "uploaded_slice"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Slice")
        buttonImage: "qrc:/UI/photo/userInfo_import_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_import_hover.svg"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Print Slice")
        buttonImage: "qrc:/UI/photo/userInfo_print_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_print_hover.svg"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Slice")
        buttonImage: "qrc:/UI/photo/userInfo_delete_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_delete_hover.svg"
      }

      PropertyChanges {
        target: detail_button
        visible: false
      }
    },

    State {
      name: "cloud_slice"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Slice")
        buttonImage: "qrc:/UI/photo/userInfo_import_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_import_hover.svg"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Print Slice")
        buttonImage: "qrc:/UI/photo/userInfo_print_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_print_hover.svg"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Slice")
        buttonImage: "qrc:/UI/photo/userInfo_delete_normal.svg"
        buttonHoveredImage: "qrc:/UI/photo/userInfo_delete_hover.svg"
      }

      PropertyChanges {
        target: detail_button
        visible: false
      }
    }
  ]

  width: 225 * screenScaleFactor
  height: 285 * screenScaleFactor

  color: "#FFFFFF"
  radius: 10 * screenScaleFactor
  border.width: (Constants.currentTheme ? 1 : 0) * screenScaleFactor
  border.color: "#D7D7DD"

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: root.border.width

    spacing: 0

    ImageRectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true

      radius: root.radius
      color: "#D7D7DD"

      imageSource: root.itemImage
      imageFillMode: Image.PreserveAspectCrop
      blurEnabled: root.itemRestricted && cxkernel_cxcloud.modelRestriction != "ignore"
      bottomLeftRadiusEnabled: false
      bottomRightRadiusEnabled: false

      Image {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parent.radius

        enabled: root.itemRestricted && cxkernel_cxcloud.modelRestriction != "ignore"
        visible: enabled
        width: sourceSize.width * screenScaleFactor
        height: sourceSize.height * screenScaleFactor
        source: "qrc:/UI/photo/model_restricted.png"
      }

      Image {
        anchors.centerIn: parent

        enabled: root.itemRestricted && cxkernel_cxcloud.modelRestriction != "ignore"
        visible: enabled
        width: sourceSize.width * screenScaleFactor
        height: sourceSize.height * screenScaleFactor
        source: "qrc:/UI/photo/model_hide.png"
      }
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.minimumHeight: 28 * screenScaleFactor
      Layout.margins: 8 * screenScaleFactor

      spacing: 5 * screenScaleFactor

      Text {
        Layout.fillWidth: true

        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft

        font.family: Constants.labelFontFamily
        font.weight: Font.Bold
        font.pointSize: Constants.labelFontPointSize_11

        clip: true
        maximumLineCount: 1
        elide: Text.ElideRight
        wrapMode: Text.WrapAnywhere
        color: "#333333"
        text: root.itemName
      }

      Item {
        Layout.fillWidth: true
      }

      Button {
        id: import_button

        property string buttonImage: ""
        property string buttonHoveredImage: ""
        property string tipText: ""

        Layout.minimumWidth: 28 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        BasicTooltip {
          visible: parent.hovered
          text: parent.tipText
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
            source: import_button.hovered ? import_button.buttonHoveredImage
                                          : import_button.buttonImage
          }
        }

        onClicked: {
          root.requestImport()
        }
      }

      Button {
        id: export_button

        property string buttonImage: ""
        property string buttonHoveredImage: ""
        property string tipText: ""

        Layout.minimumWidth: 28 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        BasicTooltip {
          visible: parent.hovered
          text: parent.tipText
        }

        background: Rectangle {
          radius: export_button.height / 2
          color: export_button.hovered ? "#17CC5F" : "#D6FFE6"
        }

        contentItem: Item {
          Image {
            anchors.centerIn: parent
            width: 14 * screenScaleFactor
            height: 14 * screenScaleFactor
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            source: export_button.hovered ? export_button.buttonHoveredImage
                                          : export_button.buttonImage
          }
        }

        onClicked: {
          root.requestExport()
        }
      }

      Button {
        id: delete_button

        property string buttonImage: ""
        property string buttonHoveredImage: ""
        property string tipText: ""

        Layout.minimumWidth: 28 * screenScaleFactor
        Layout.maximumWidth: Layout.minimumWidth
        Layout.minimumHeight: Layout.minimumWidth
        Layout.maximumHeight: Layout.minimumHeight

        BasicTooltip {
          visible: parent.hovered
          text: parent.tipText
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
            source: delete_button.hovered ? delete_button.buttonHoveredImage
                                          : delete_button.buttonImage
          }
        }

        onClicked: {
          root.requestDelete()
        }
      }
    }

    Button {
      id: detail_button

      Layout.fillWidth: true
      Layout.minimumHeight: 24 * screenScaleFactor
      Layout.maximumHeight: Layout.minimumHeight
      Layout.margins: 8 * screenScaleFactor
      Layout.topMargin: -3 * screenScaleFactor

      background: Rectangle {
        radius: height / 2
        border.width: 1 * screenScaleFactor
        border.color: detail_button.hovered ? "#17CC5F" : "#D7D7DD"
        color: detail_button.hovered ? "#17CC5F" : "transparent"
      }

      contentItem: Text {
        Layout.fillHeight: true
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignHCenter
        font.family: Constants.labelFontFamily
        font.weight: Font.Bold
        font.pointSize: Constants.labelFontPointSize_11
        color: detail_button.hovered ? "#FFFFFF" : "#333333"
        text: qsTr("Print Configuration")
      }

      onClicked: {
        root.requestDetail()
      }
    }
  }
}
