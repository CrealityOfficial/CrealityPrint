import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"

DockItem {
  id: root

  signal confirm()
  signal cancel()

  width: 600 * screenScaleFactor
  height: 352 * screenScaleFactor
  title: qsTranslate("parameter_update", "Parameter Package Update")

  onCloseButtonClicked: {
    cancel()
  }

  Control {
    id: panel

    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor
    anchors.topMargin: 20 * screenScaleFactor + root.currentTitleHeight

    background: Rectangle {
      radius: 5 * screenScaleFactor
      border.width: 1 * screenScaleFactor
      border.color: Constants.right_panel_border_default_color
      color: "transparent"
    }

    contentItem: ListView {
      id: list_view

      anchors.fill: parent
      anchors.margins: 1 * screenScaleFactor

      ScrollBar.vertical: ScrollBar {
        policy: list_view.contentHeight > list_view.height ? ScrollBar.AlwaysOn
                                                           : ScrollBar.AlwaysOff
      }

      ScrollBar.horizontal: ScrollBar {
        policy: ScrollBar.AlwaysOff
      }

      clip: true

      model: cxkernel_cxcloud.profileService.printerUpdateListModel

      delegate: Control {
        id: list_delegate

        implicitWidth: ListView.view.width
        implicitHeight: 56 * screenScaleFactor

        background: Rectangle {
          radius: 5 * screenScaleFactor
          readonly property color evenLineColor: Constants.currentTheme ? "#ECECEC" : "#535358"
          color: model.index % 2 === 0 ? evenLineColor : "transparent"
        }

        contentItem: RowLayout {
          anchors.fill: parent
          anchors.margins: 10 * screenScaleFactor

          spacing: 20 * screenScaleFactor

          ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft

            spacing: 0

            Text {
              Layout.fillWidth: true
              Layout.fillHeight: true

              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignLeft

              font.family: Constants.labelFontFamily
              font.weight: Font.Bold
              font.pointSize: Constants.labelFontPointSize_10
              color: Constants.parameter_text_color
              text: model_display_name
            }

            Text {
              visible: model_local_version !== ""

              Layout.fillWidth: true
              Layout.fillHeight: true
              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignLeft

              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: Constants.parameter_text_color
              text: "%1: %2".arg(qsTranslate("parameter_update", "Current version"))
                            .arg(model_local_version)
            }
          }

          Text {
            visible: model_updateable

            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter

            font.family: Constants.labelFontFamily
            font.weight: Font.Bold
            font.pointSize: Constants.labelFontPointSize_10
            color: Constants.themeGreenColor
            text: "%1: %2".arg(qsTranslate("parameter_update", "Found new version"))
                          .arg(model_online_version)
          }

          Button {
            visible: model_updateable

            Layout.minimumWidth: 18 * screenScaleFactor
            Layout.maximumWidth: Layout.minimumWidth
            Layout.minimumHeight: Layout.minimumWidth
            Layout.maximumHeight: Layout.minimumHeight
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            background: Item {}

            contentItem: Image {
              anchors.fill: parent
              fillMode: Image.PreserveAspectFit
              sourceSize.width: width
              sourceSize.height: height
              source: parent.hovered ? "qrc:/UI/photo/parameter/update_tip_hovered.svg"
                                     : "qrc:/UI/photo/parameter/update_tip.svg"
            }

            BasicTooltip {
              visible: parent.hovered
              timeout: -1
              position: BasicTooltip.Position.LEFT
              textWidth: 256 * screenScaleFactor
              textWrap: true
              text: model_change_log
            }
          }

          Button {
            visible: model_updateable

            Layout.minimumHeight: 28 * screenScaleFactor
            Layout.maximumHeight: Layout.minimumHeight
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            onClicked: {
              if (!model_updating) {
                cxkernel_cxcloud.profileService.updateOfficalPrinter(model_uid)
              }
            }

            background: Rectangle {
              radius: height / 2
              color: model_updating ? "transparent" : Constants.themeGreenColor
            }

            contentItem: Text {
              verticalAlignment: Qt.AlignVCenter
              horizontalAlignment: Qt.AlignHCenter
              font.family: Constants.labelFontFamily
              font.weight: Constants.labelFontWeight
              font.pointSize: Constants.labelFontPointSize_10
              color: "#FFFFFF"
              text: model_updating ? qsTranslate("parameter_update", "Updating")
                                   : qsTranslate("parameter_update", "Update Now")
            }
          }

          Text {
            visible: !model_updateable

            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight

            font.family: Constants.labelFontFamily
            font.weight: Font.Bold
            font.pointSize: Constants.labelFontPointSize_10
            color: Constants.parameter_text_color
            text: qsTranslate("parameter_update", "Latest version already")
          }
        }
      }
    }
  }
}
