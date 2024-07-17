import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

import ".."
import "../secondqml"

Rectangle {
  id: root

  property string itemName: ""
  property string itemImage: ""

  signal requestImport
  signal requestExport
  signal requestDelete

  state: "uploaded_model"
  states: [
    State {
      name: "uploaded_model"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Download Model")
        btnImgNormal: "qrc:/UI/photo/userInfo_import_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_import_hover.png"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Share Model (Copy Link)")
        btnImgNormal: "qrc:/UI/photo/userInfo_share_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_share_hover.png"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Model")
        btnImgNormal: "qrc:/UI/photo/userInfo_delete_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_delete_hover.png"
      }
    },

    State {
      name: "collected_model"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Download Model")
        btnImgNormal: "qrc:/UI/photo/userInfo_import_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_import_hover.png"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Share Model (Copy Link)")
        btnImgNormal: "qrc:/UI/photo/userInfo_share_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_share_hover.png"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Uncollect Model")
        btnImgNormal: "qrc:/UI/photo/userInfo_delete_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_delete_hover.png"
      }
    },

    State {
      name: "purchased_model"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Download Model")
        btnImgNormal: "qrc:/UI/photo/userInfo_import_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_import_hover.png"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Share Model (Copy Link)")
        btnImgNormal: "qrc:/UI/photo/userInfo_share_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_share_hover.png"
      }

      PropertyChanges {
        target: delete_button
        visible: false
      }
    },

    State {
      name: "uploaded_slice"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Gcode")
        btnImgNormal: "qrc:/UI/photo/userInfo_import_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_import_hover.png"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Print Gcode")
        btnImgNormal: "qrc:/UI/photo/userInfo_print_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_print_hover.png"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Gcode")
        btnImgNormal: "qrc:/UI/photo/userInfo_delete_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_delete_hover.png"
      }
    },

    State {
      name: "cloud_slice"

      PropertyChanges {
        target: import_button
        tipText: qsTr("Import Gcode")
        btnImgNormal: "qrc:/UI/photo/userInfo_import_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_import_hover.png"
      }

      PropertyChanges {
        target: export_button
        tipText: qsTr("Print Gcode")
        btnImgNormal: "qrc:/UI/photo/userInfo_print_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_print_hover.png"
      }

      PropertyChanges {
        target: delete_button
        tipText: qsTr("Delete Gcode")
        btnImgNormal: "qrc:/UI/photo/userInfo_delete_normal.png"
        btnImgHovered: "qrc:/UI/photo/userInfo_delete_hover.png"
      }
    }
  ]

  width: 225 * screenScaleFactor
  height: 285 * screenScaleFactor

  color: "#FFFFFF"
  radius: 10 * screenScaleFactor
  border.color: Constants.dock_border_color
  border.width: 1 * screenScaleFactor

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: parent.border.width

    spacing: 0

    Rectangle {
      Layout.fillWidth: true
      Layout.minimumHeight: 241 * screenScaleFactor
      Layout.alignment: Qt.AlignTop

      radius: root.radius
      color: "#E0E0E0"

      Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: root.radius
        color: parent.color
      }

      Image {
        anchors.fill: parent
        smooth: true
        cache: false
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: itemImage
      }
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.margins: 8 * screenScaleFactor

      spacing: 20 * screenScaleFactor

      Text {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        Layout.fillWidth: true

        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft

        font.weight: Font.Medium
        font.family: Constants.mySystemFont.name
        font.pointSize: Constants.labelFontPointSize_9

        clip :true
        maximumLineCount:1
        elide: Text.ElideRight
        wrapMode: Text.WrapAnywhere
        text: "<font color='#333333'>%1</font>".arg(itemName)
      }

      Row {
        id: button_row
        spacing: 5 * screenScaleFactor

        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

        CusSkinButton_Image {
          id: import_button

          width: 28 * screenScaleFactor
          height: 28 * screenScaleFactor
          radius: 14 * screenScaleFactor

          defaultBtnBgColor: "#E2F5FF"
          hoveredBtnBgColor: "#009CFF"
          selectedBtnBgColor: defaultBtnBgColor

          btnImgPressed: btnImgNormal

          onClicked: {
            root.requestImport()
          }
        }

        CusSkinButton_Image {
          id: export_button

          width: 28 * screenScaleFactor
          height: 28 * screenScaleFactor
          radius: 14 * screenScaleFactor

          defaultBtnBgColor: "#E2F5FF"
          hoveredBtnBgColor: "#009CFF"
          selectedBtnBgColor: defaultBtnBgColor

          btnImgPressed: btnImgNormal

          onClicked: {
            root.requestExport()
          }
        }

        CusSkinButton_Image {
          id: delete_button

          width: 28 * screenScaleFactor
          height: 28 * screenScaleFactor
          radius: 14 * screenScaleFactor

          defaultBtnBgColor: "#E2F5FF"
          hoveredBtnBgColor: "#009CFF"
          selectedBtnBgColor: defaultBtnBgColor

          btnImgPressed: btnImgNormal

          onClicked: {
            root.requestDelete()
          }
        }
      }
    }
  }
}
