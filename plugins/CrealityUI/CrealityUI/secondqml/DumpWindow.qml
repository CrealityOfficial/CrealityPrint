import QtQml 2.13

import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13

import "qrc:/CrealityUI/"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml/"
import "qrc:/CrealityUI/secondqml/"

DockItem {
  id: root

  visible: false
  title: qsTr("title")
  width: 800 * screenScaleFactor
  height: 420 * screenScaleFactor
  titleFontFamily: Constants.panelFontFamily

  onCloseButtonClicked: {
    Qt.quit()
  }

  function resetWidth(content_width) {
    root.width = Math.max(content_width, root.width)
  }

  Connections {
    target: dumpTool
    function onSendReoprtFinished(successed) {
      if (!successed) {
        accept_button.enabled = true
        cancel_button.enabled = true
        return
      }

      root.closeButtonClicked()
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: anchors.margins + root.currentTitleHeight

    spacing: 30 * screenScaleFactor

    RowLayout {
      Layout.fillWidth: true
      Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

      spacing: 10 * screenScaleFactor

      Image {
        id: tip_image
        width: 20 * screenScaleFactor
        height: 20 * screenScaleFactor
        Layout.alignment: Qt.AlignTop | Qt.AlignLeft

        source: "qrc:/UI/photo/lanPrinterSource/addPrinter_infomation.png"
      }

      ColumnLayout {
        Text {
          color: Constants.textColor

          font.weight: Font.Normal
          font.family: Constants.panelFontFamily
          font.pointSize: Constants.labelFontPointSize_11

          text: qsTr("dump_tip_format_0")
                .arg(dumpTool.getApplicationName())

          onContentWidthChanged: {
            root.resetWidth(this.contentWidth + tip_image.width + 60 * screenScaleFactor)
          }
        }

        Text {
          color: Constants.textColor

          font.weight: Font.Normal
          font.family: Constants.panelFontFamily
          font.pointSize: Constants.labelFontPointSize_9

          text: qsTr("dump_tip_format_1")

          onContentWidthChanged: {
            root.resetWidth(this.contentWidth + tip_image.width + 60 * screenScaleFactor)
          }
        }
      }
    }

    Rectangle {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.alignment: Qt.AlignCenter

      color: "transparent"

      radius: 5 * screenScaleFactor
      border.width: 1 * screenScaleFactor
      border.color: Constants.dock_border_color

      Text {
        anchors.fill: parent
        anchors.margins: 10 * screenScaleFactor

        color: Constants.textColor

        font.weight: Font.Normal
        font.family: Constants.panelFontFamily
        font.pointSize: Constants.labelFontPointSize_9

        text: qsTr("dump_info_format")
              .arg(dumpTool.getApplicationName())
              .arg(dumpTool.getApplicationVersion())
              .arg(dumpTool.getApplicationName())
              .arg(dumpTool.getApplicationLanguage())
              .arg(dumpTool.getOperatingSystemName())
              .arg(dumpTool.getGraphicsCardName())
              .arg(dumpTool.getOpenglVersion())
              .arg(dumpTool.getOpenglVendor())
              .arg(dumpTool.getOpenglRenderer())
      }
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter

      spacing: 10 * screenScaleFactor

      BasicDialogButton {
        id: accept_button

        Layout.minimumWidth: 120 * screenScaleFactor
        Layout.minimumHeight: 28 * screenScaleFactor
        Layout.alignment: Qt.AlignCenter

        text: qsTr("accept")
        font.family: Constants.panelFontFamily

        btnRadius: height / 2
        btnBorderW: 1 * screenScaleFactor
        btnTextColor: Constants.manager_printer_button_text_color
        borderColor: Constants.manager_printer_button_border_color
        defaultBtnBgColor: Constants.manager_printer_button_default_color
        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
        selectedBtnBgColor: Constants.manager_printer_button_checked_color

        onSigButtonClicked: {
          accept_button.enabled = false
          cancel_button.enabled = false
          dumpTool.sendReport()
        }
      }

      BasicDialogButton {
        id: cancel_button

        Layout.minimumWidth: 120 * screenScaleFactor
        Layout.minimumHeight: 28 * screenScaleFactor
        Layout.alignment: Qt.AlignCenter

        text: qsTr("cancel")
        font.family: Constants.panelFontFamily

        btnRadius: height / 2
        btnBorderW: 1 * screenScaleFactor
        btnTextColor: Constants.manager_printer_button_text_color
        borderColor: Constants.manager_printer_button_border_color
        defaultBtnBgColor: Constants.manager_printer_button_default_color
        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
        selectedBtnBgColor: Constants.manager_printer_button_checked_color

        onSigButtonClicked: {
          root.closeButtonClicked()
        }
      }
    }
  }
}
