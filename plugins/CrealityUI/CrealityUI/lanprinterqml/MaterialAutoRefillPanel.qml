import QtQml 2.15

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import "qrc:/CrealityUI/qml"

Rectangle {
  id: root

  property var model: []

  MouseArea {
    anchors.fill: parent
  }

  Rectangle {
    anchors.top: parent.top
    anchors.right: parent.right
    width: 30 * screenScaleFactor
    height: 30 * screenScaleFactor

    color: 'transparent'

    Image {
      anchors.centerIn: parent
      width: 10 * screenScaleFactor
      height: 10 * screenScaleFactor
      source: sourceTheme.img_closeBtn
    }

    MouseArea {
      anchors.fill: parent

      cursorShape: Qt.PointingHandCursor

      onClicked: function() {
        root.visible = false
      }
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 20 * screenScaleFactor

    spacing: 20 * screenScaleFactor

    ColumnLayout {
      Layout.fillWidth: true

      spacing: 5 * screenScaleFactor

      Text {
        Layout.fillWidth: true
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft

        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_12
        color: sourceTheme.text_normal_color
        text: cxTr('material_auto_refill_panel_title')
      }

      Text {
        Layout.fillWidth: true
        verticalAlignment: Qt.AlignVCenter
        horizontalAlignment: Qt.AlignLeft

        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_10
        color: sourceTheme.text_special_color
        text: material_repeater.count > 0 ? cxTr('material_auto_refill_panel_description')
                                          : cxTr('material_auto_refill_panel_empty_description')
      }
    }

    RowLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true

      spacing: 20 * screenScaleFactor

      Rectangle {
        Layout.minimumWidth: 20 * screenScaleFactor
        Layout.fillHeight: true

        enabled: root.model.pageCount > 1
        opacity: enabled ? 1 : 0
        color: 'transparent'

        Image {
          anchors.centerIn: parent
          width: 8 * screenScaleFactor
          height: 14 * screenScaleFactor
          source: 'qrc:/UI/photo/upload3mf/pre_arrow.svg'
        }

        MouseArea {
          anchors.fill: parent

          enabled: parent.opacity != 0
          cursorShape: Qt.PointingHandCursor

          onClicked: function() {
            root.model.currentPage = Math.max(0, root.model.currentPage - 1)
          }
        }
      }

      Repeater {
        id: material_repeater

        model: root.visible ? root.model : []

        delegate: Item {
          Layout.fillWidth: true
          Layout.fillHeight: true

          Canvas {
            anchors.centerIn: parent

            width: 128 * screenScaleFactor
            height: width

            readonly property real arcLineWidth: 26 * screenScaleFactor
            readonly property real arrowLineWidth: 2 * screenScaleFactor
            readonly property real splitLineWidth: 2 * screenScaleFactor
            readonly property string textFont: Constants.labelFontFamily
            readonly property int textSize: Constants.labelFontPointSize_12

            onPaint: function() {
              const context = getContext('2d')
              context.clearRect(0, 0, width, height)

              // 同心圆背景
              const center_x = width / 2
              const center_y = height / 2
              const arc_outside_radius = width / 2
              const arc_midside_radius = arc_outside_radius - arcLineWidth / 2
              const arc_inside_radius = arc_outside_radius - arcLineWidth

              context.lineWidth = arcLineWidth
              context.strokeStyle = model.materialColor
              context.beginPath()
              context.arc(center_x, center_y, arc_midside_radius, 0, 2 * Math.PI)
              context.closePath()
              context.stroke()

              // 分隔线 & 料盒名
              context.lineWidth = splitLineWidth
              context.strokeStyle = root.color
              context.textAlign = 'center'
              context.textBaseline = 'middle'
              context.font = '%1px bold "%2"'.arg(textSize).arg(textFont)

              for (let index = 0; index < model.boxNameList.length; ++index) {
                const line_angle = index * 360 / model.boxNameList.length - 90
                const line_x = center_x + arc_outside_radius * Math.cos(line_angle * Math.PI / 180)
                const line_y = center_y + arc_outside_radius * Math.sin(line_angle * Math.PI / 180)

                context.beginPath()
                context.moveTo(center_x, center_y)
                context.lineTo(line_x, line_y)
                context.closePath()
                context.stroke()

                const next_line_angle = (index + 1) * 360 / model.boxNameList.length - 90
                const text_angle = (line_angle + next_line_angle) / 2
                const text_x = center_x + arc_midside_radius * Math.cos(text_angle * Math.PI / 180)
                const text_y = center_y + arc_midside_radius * Math.sin(text_angle * Math.PI / 180)
                const background_color = [ model.materialColor.r * 255,
                                           model.materialColor.g * 255,
                                           model.materialColor.b * 255, ]
                const white_contrast = Constants.getContrast(background_color, [255, 255, 255])
                const black_contrast = Constants.getContrast(background_color, [0, 0, 0])

                context.fillStyle = white_contrast > black_contrast ? '#FFFFFF' : '#000000'
                context.fillText(model.boxNameList[index], text_x, text_y)
              }

              // 双箭头
              const arrow_left_x = center_x - arc_inside_radius + arrowLineWidth * 3
              const arrow_right_x = center_x + arc_inside_radius - arrowLineWidth * 3
              const arrow_radius = Math.abs(center_x - arrow_left_x)

              context.lineWidth = arrowLineWidth
              context.strokeStyle = model.materialColor
              context.fillStyle = model.materialColor

              context.beginPath()
              context.arc(center_x, center_y, arrow_radius, Math.PI / 10, Math.PI)
              context.stroke()

              context.beginPath()
              context.moveTo(arrow_left_x - arrowLineWidth * 2, center_y)
              context.lineTo(arrow_left_x, center_y - arrowLineWidth * 2)
              context.lineTo(arrow_left_x + arrowLineWidth * 2, center_y)
              context.closePath()
              context.fill()

              context.beginPath()
              context.arc(center_x, center_y, arrow_radius, Math.PI + Math.PI / 10, 0)
              context.stroke()

              context.beginPath()
              context.moveTo(arrow_right_x - arrowLineWidth * 2, center_y)
              context.lineTo(arrow_right_x, center_y + arrowLineWidth * 2)
              context.lineTo(arrow_right_x + arrowLineWidth * 2, center_y)
              context.closePath()
              context.fill()
            }
          }

          Text {
            anchors.centerIn: parent

            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_10
            color: sourceTheme.text_normal_color
            text: model.materialName
          }
        }
      }

      Rectangle {
        Layout.minimumWidth: 20 * screenScaleFactor
        Layout.fillHeight: true

        enabled: root.model.pageCount > 1
        opacity: enabled ? 1 : 0
        color: 'transparent'

        Image {
          anchors.centerIn: parent
          width: 8 * screenScaleFactor
          height: 14 * screenScaleFactor
          source: 'qrc:/UI/photo/upload3mf/next_arrow.svg'
        }

        MouseArea {
          anchors.fill: parent

          enabled: parent.opacity != 0
          cursorShape: Qt.PointingHandCursor

          onClicked: function() {
            root.model.currentPage = Math.min(root.model.currentPage + 1, root.model.pageCount)
          }
        }
      }
    }

    Text {
      Layout.fillWidth: true

      enabled: root.model.pageCount > 1
      opacity: enabled ? 1 : 0

      verticalAlignment: Qt.AlignVCenter
      horizontalAlignment: Qt.AlignHCenter

      font.family: Constants.labelFontFamily
      font.weight: Constants.labelFontWeight
      font.pointSize: Constants.labelFontPointSize_10
      color: sourceTheme.text_normal_color
      text: '%1/%2'.arg(root.model.currentPage + 1).arg(root.model.pageCount)
    }
  }
}
