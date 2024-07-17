import QtQuick 2.12
import QtQuick.Controls 2.12
import "../qml"
 RadioButton {
      id: control
      text: ""
      checked: true
      leftPadding: 0
      font.family:Constants.labelFontFamily
      font.pointSize: Constants.labelFontPointSize_12
      indicator: Rectangle {
          implicitWidth: 16
          implicitHeight: 16
          x: control.leftPadding
          y: parent.height / 2 - height / 2
          radius: 13
          border.color: control.down ? "#333333" : "#333333"

          Rectangle {
              width: 8
              height: 8
              x: 4
              y: 4
              radius: 7
              color: control.down ? "#424242" : "#424242"
              visible: control.checked
          }
      }

      contentItem: Text {
          text: control.text
          font: control.font
          opacity: enabled ? 1.0 : 0.3
          color: Constants.textColor // control.down ? "#FFFFFF" : "#FDFDFD"
          verticalAlignment: Text.AlignVCenter
          leftPadding: control.indicator.width + control.spacing
      }
  }
