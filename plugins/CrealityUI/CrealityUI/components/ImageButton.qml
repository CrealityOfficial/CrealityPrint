import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

Button {
  property alias image: image
  property alias toolTip: tool_tip

  id: root

  width: 24 * screenScaleFactor
  height: width

  BasicTooltip {
    id: tool_tip

    visible: text !== "" && root.hovered
    text: root.text
    textWrap: false
  }

  background: Rectangle {
    radius: 5 * screenScaleFactor
    border.width: 1 * screenScaleFactor
    border.color: hovered ? Constants.right_panel_border_hovered_color : "transparent"
    color: "transparent"
  }

  contentItem: Item {
    Image {
      id: image

      anchors.centerIn: parent

      width: 14 * screenScaleFactor
      height: width
      sourceSize.width: width
      sourceSize.height: height
    }
  }
}
