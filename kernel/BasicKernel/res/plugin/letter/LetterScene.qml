import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.13
import QtQml 2.13

Item {
    id: root
    property double factor: 1.05
    property int winWidth: 1920
    property int winHeight: 1080
    property var curSharp: null

    property alias wordSelectStatus: draw_panel.hasWordSelect
    Keys.onPressed: function(event) {
      if (!root.visible) { return }
      event.accepted = true;
      switch(event.key) {

        case Qt.Key_Delete:
//          draw_panel.deleteSelectObject()
          break
        case /*Qt.ControlModifier &&  */Qt.Key_A :
            draw_panel.selectAll()
        break
        default:

          break
      }
    }

    function deleteSelectDraw()
    {
        draw_panel.deleteSelectObject()
    }

    function resetCoordinate() {
      let control_origin = Qt.point(content.width / 2, content.height / 2)
      let control_width = root.width
      let control_height = root.height

      // 有效区
//      control.origin = control_origin

      // 坐标系
      content_grid.x = control_origin.x - control_width / 2
      content_grid.y = control_origin.y - control_height / 2
      content_grid.width = control_width
      content_grid.height = control_height

      // 画布
      content.x = -content_grid.x + (root.winWidth - control_width) / 2
      content.y = -content_grid.y + (root.winHeight - control_height) / 2
    }

    function allArry()
    {
        return draw_panel.allArray
    }

    Component.onCompleted: {
        draw_panel.createDragText(content.width /2 ,content.height/2,1,1)
    }

    Item {
      id: content
      anchors.fill: parent

      DrawPanel {
        id: draw_panel
        anchors.fill: parent
//        z:parent.z + 1
        visible: root.visible
        onAddRect: function(obj, sharpType) {
            curSharp = obj
        }

        onSelectChanged: function(obj) {
            curSharp = obj
        }

        onVerifyFocus: {
          draw_panel.forceActiveFocus();
          draw_panel.Keys.enabled = true;
        }
      }
    }


}
