import QtQuick 2.15
import QtQuick.Layouts 1.15

RowLayout {
  property string image: ""
  property string title: ""

  id: root

  spacing: 4 * screenScaleFactor

  Image {
    id: image

    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    Layout.minimumWidth: 16 * screenScaleFactor
    Layout.minimumHeight: 16 * screenScaleFactor
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight

    sourceSize.width: Layout.minimumWidth
    sourceSize.height: Layout.minimumHeight
    source: root.image
  }

  Text {
    id: text

    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
    Layout.minimumWidth: contentWidth
    Layout.minimumHeight: contentHeight
    Layout.maximumWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter

    font.family: Constants.labelFontFamily
    font.weight: Font.ExtraBold
    font.pointSize: Constants.labelFontPointSize_14
    color: Constants.right_panel_text_default_color
    text: root.title
  }
}
