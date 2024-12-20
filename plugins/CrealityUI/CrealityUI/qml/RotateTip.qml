import QtQuick 2.10
import QtQuick.Controls 2.0

Rectangle {
  property bool isVisible: false
  property int posX: 0
  property int posY: 0
  property string rotateAxis: "X"
  property real rotateValue: 0.0

  id: root

  visible: isVisible

  x: posX
  y: posY
  width: 140 * screenScaleFactor
  height: 40 * screenScaleFactor

  radius: 5 * screenScaleFactor
  border.width: 1 * screenScaleFactor
  border.color: Constants.dock_border_color
  color: Constants.themeColor_primary
  opacity: 0.7

  Text {
    parent: root.parent
    visible: root.visible

    anchors.centerIn: root
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter

    text: "%1: %2".arg(root.rotateAxis).arg(root.rotateValue.toFixed(3))
    color: Constants.themeTextColor
    font.pointSize: Constants.labelFontPointSize_12
    font.family: Constants.labelFontFamily
    font.weight: Font.Bold
  }
}
