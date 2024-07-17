import QtQml 2.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import CrealityUI 1.0

import "qrc:/CrealityUI/qml"

Control {
  readonly property int modelCount : kernel_model_selector.selectedNum
  property string printerName: ""
  property string modelName: ""
  property string modelSize: ""
     property string projectName: ""
  readonly property string imageFormat:
      "qrc:/UI/photo/model_info/%2%1.svg".arg(Constants.currentTheme ? "_light" : "")

  function updateInfo() {
    printerName = plugin_info.currentPrinter()
    modelName = plugin_info.modelName()
    const size = plugin_info.size
    const x = size.x.toFixed(2)
    const y = size.y.toFixed(2)
    const z = size.z.toFixed(2)
    modelSize = "%1*%2*%3".arg(x).arg(y).arg(z)
    projectName =  kernel_project_ui.getProjectNameNoSuffix()
  }

  Connections {
    target: kernel_parameter_manager

    function onCurMachineIndexChanged() {
      updateInfo()
    }
  }

  Connections {
    target: plugin_info

    function onInfoChanged() {
      updateInfo()
    }
  }

  width: implicitContentWidth + 40 * screenScaleFactor
  height: implicitContentHeight + 40 * screenScaleFactor

  background: Rectangle {
    radius: 5 * screenScaleFactor
    color: Constants.currentTheme ? Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(0, 0, 0, 0.3)
  }

  contentItem: Item {
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.topMargin: 20 * screenScaleFactor
    anchors.leftMargin: 20 * screenScaleFactor

    readonly property var currentItem: modelCount > 1 ? model_count_text : info_layout
    implicitWidth: currentItem.implicitWidth
    implicitHeight: currentItem.implicitHeight

    ColumnLayout {
      id: info_layout

      visible: modelCount <= 1
      spacing: 10 * screenScaleFactor
      RowLayout {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        spacing: 5 * screenScaleFactor

        Image {
          source: imageFormat.arg("project")
          sourceSize.width: 12 * screenScaleFactor
          sourceSize.height: 12 * screenScaleFactor
        }

        Text {
          Layout.minimumWidth: 200 * screenScaleFactor
          Layout.maximumWidth: 300 * screenScaleFactor
          Layout.preferredWidth: contentWidth
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignVCenter
          font.pointSize: Constants.labelFontPointSize_normal
          font.family: Constants.panelFontFamily
          color: Constants.textColor
          elide: Text.ElideRight
          text: "%1:%2".arg(qsTranslate("MenuCmdObj","Project")).arg(projectName)
          Text {

            anchors.left: parent.left
            anchors.leftMargin: parent.contentWidth + 2* screenScaleFactor
            horizontalAlignment: Qt.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            font.pointSize: Constants.labelFontPointSize_normal
            font.family: Constants.panelFontFamily
            color: "red"
            elide: Text.ElideRight
            text: "*"
            visible:  kernel_project_ui.projectDirty //kernel_undo_proxy.canUndo
          }
        }


      }

      RowLayout {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        spacing: 5 * screenScaleFactor

        Image {
          source: imageFormat.arg("printer")
          sourceSize.width: 12 * screenScaleFactor
          sourceSize.height: 12 * screenScaleFactor
        }

        Text {
          Layout.minimumWidth: 200 * screenScaleFactor
          Layout.maximumWidth: 300 * screenScaleFactor
          Layout.preferredWidth: contentWidth
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignVCenter
          font.pointSize: Constants.labelFontPointSize_normal
          font.family: Constants.panelFontFamily
          color: Constants.textColor
          elide: Text.ElideRight
          text: "%1:%2".arg(qsTr("Printer")).arg(printerName)

        }
      }

      RowLayout {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        spacing: 5 * screenScaleFactor
        visible: modelCount === 1
        Image {
          source: imageFormat.arg("model")
          sourceSize.width: 12 * screenScaleFactor
          sourceSize.height: 12 * screenScaleFactor
        }

        Text {
          Layout.minimumWidth: 200 * screenScaleFactor
          Layout.maximumWidth: 300 * screenScaleFactor
          Layout.preferredWidth: contentWidth
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignVCenter
          font.pointSize: Constants.labelFontPointSize_normal
          font.family: Constants.panelFontFamily
          color: Constants.textColor
          elide: Text.ElideRight
          text: "%1:%2".arg(qsTr("Model")).arg(modelName)
        }
      }

      RowLayout {
        Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
        spacing: 5 * screenScaleFactor
        visible: modelCount === 1
        Image {
          source: imageFormat.arg("size")
          sourceSize.width: 12 * screenScaleFactor
          sourceSize.height: 12 * screenScaleFactor
        }

        Text {
          Layout.minimumWidth: 200 * screenScaleFactor
          Layout.maximumWidth: 300 * screenScaleFactor
          Layout.preferredWidth: contentWidth
          horizontalAlignment: Qt.AlignLeft
          verticalAlignment: Qt.AlignVCenter
          font.pointSize: Constants.labelFontPointSize_normal
          font.family: Constants.panelFontFamily
          color: Constants.textColor
          elide: Text.ElideRight
          text: "%1:%2".arg(qsTr("Size")).arg(modelSize)
        }
      }

    }

    Text {
      id: model_count_text

      visible: modelCount > 1
      width: contentWidth
      horizontalAlignment: Qt.AlignHCenter
      verticalAlignment: Qt.AlignVCenter
      font.pointSize: Constants.labelFontPointSize_normal
      font.family: Constants.panelFontFamily
      color: Constants.textColor
      wrapMode: Text.NoWrap
      text: qsTr("The current number of selected models:") + modelCount
    }
  }
  Connections
  {
      target: kernel_project_ui
      function onProjectNameChanged() {
           projectName = kernel_project_ui.getProjectNameNoSuffix()
      }
  }

//  Connections
//  {
//      target: kernel_undo_proxy
//      function onCanUndoChanged() {
//          console.log("onCanUndoChanged =",kernel_undo_proxy.canUndo)
//          if(kernel_undo_proxy.canUndo)
//           projectName = kernel_project_ui.getProjectNameNoSuffix() + "*"
//      }
//  }

}
