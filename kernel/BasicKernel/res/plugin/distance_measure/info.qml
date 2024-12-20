import QtQuick 2.10
import QtQuick.Controls 2.0

import CrealityUI 1.0

import "qrc:/CrealityUI"

Rectangle {
	property var text_show: ""
	property int x_value: 200
	property int y_value: 100

	id: root

	x: x_value
	y: y_value
	width: Math.max(text.contentWidth + 12 * screenScaleFactor, 60 * screenScaleFactor)
	height: 20 * screenScaleFactor

	radius: 3 * screenScaleFactor
  border.width: 1 * screenScaleFactor
  border.color: Constants.dock_border_color
  color: Constants.themeColor_primary
	opacity: 0.7

	Text {
		id: text

		parent: root.parent
    visible: root.visible

		anchors.centerIn: root
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter

		text: root.text_show
		color: Constants.themeTextColor
		font.pointSize: Constants.labelFontPointSize_9
		font.family: Constants.labelFontFamily
    font.weight: Font.Normal
	}
}
