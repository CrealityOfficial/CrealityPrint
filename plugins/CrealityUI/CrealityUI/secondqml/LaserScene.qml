import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.13
import QtQml 2.13

import "../qml/"

Item {
  id: root

  property var control: null
  property var itemModel: null

  readonly property bool enableGenerate: itemModel.modelCount > 0

  property double factor: 1.05
  property int winWidth: 1920
  property int winHeight: 1080

  clip: true

  Keys.onPressed: function(event) {
    if (!root.visible) { return }
    switch(event.key) {
      case Qt.Key_Delete:
        root.deleteSelectObject()
        break
      default:
        break
    }
  }

  function syncSelectedIndexList(selected_index_list) {
    object_list_popup.listView.syncSelectedIndexList(selected_index_list)
  }

  function deleteUndoObject(obj) {
    control.deleteObject(obj)
    obj.visible = false
  }

  function deleteSelectObject() {
    if (draw_panel.ctrlSelArray.length > 0) {
      control.deleteMulObject(draw_panel.ctrlSelArray);
      for (var i = 0; i < draw_panel.ctrlSelArray.length; i++) {
        if (draw_panel.ctrlSelArray[i]) {
          if (draw_panel.ctrlSelArray[i].selected) {
            //draw_panel.ctrlSelArray[i].destroy()
            draw_panel.ctrlSelArray[i].visible = false
          }
        }
      }

      draw_panel.ctrlSelArray = [];
      return
    }

    if (draw_panel.currentSel && draw_panel.currentSel.selected) {
      control.deleteObject(draw_panel.currentSel)
      draw_panel.currentSel.destroy()
      draw_panel.currentSel = null;
      return
    }
  }

  function generateGCode(){
    for (let index = 0; index < object_list_popup.listView.count; ++index) {
      let object = control.selectObject(index)
      if (object == null || object.hasError) {
        idOutPlatform.visible = true
        return
      }
    }
    control.genLaserGcode()
  }

  function resetCoordinate() {
    let control_origin = Qt.point(content.width / 2, content.height / 2)
    let control_width = control.width
    let control_height = control.height

    // 有效区
    control.origin = control_origin

    // 坐标系
    content_grid.x = control_origin.x - control_width / 2
    content_grid.y = control_origin.y - control_height / 2
    content_grid.width = control_width
    content_grid.height = control_height

    // 画布
    content.x = -content_grid.x + (root.winWidth - control_width) / 2
    content.y = -content_grid.y + (root.winHeight - control_height) / 2
  }

  Component.onCompleted: {
    resetCoordinate()
  }

  Item {
    id: content

    width: root.width * 100
    height: root.height * 100

    LaserGrid {
      id: content_grid
    }

    MouseArea {
      anchors.fill: parent
      //propagateComposedEvents: true
      acceptedButtons: Qt.LeftButton | Qt.RightButton

      property bool ismove: false
      property bool isMoving:false
      property int pressX: 0
      property int pressY: 0
      property int releaseX: 0
      property int releaseY: 0

      onWheel: {
        let zoomFactor = 1

        if (wheel.angleDelta.y > 0) { // zoom in
          zoomFactor = factor
        } else {                      // zoom out
          zoomFactor = 1 / factor
        }

        if(content.width * content_transform.xScale * zoomFactor < root.width * 100 ||
           content.width * content_transform.xScale * zoomFactor > root.width * 10 * 100) {
          return;
        }

        content_transform.xScale *= zoomFactor
        content_transform.yScale *= zoomFactor
      }

      onPressed: {
        if (mouse.button == Qt.RightButton) {
          pressX = mouse.x;
          pressY = mouse.y;
          ismove = true
        }
      }

      onReleased: {
        ismove = false

        if (mouse.button === Qt.RightButton) {
          if (draw_panel.ctrlSelArray.length > 0 && !isMoving) {
            rclick_menu.popup()
            rclick_menu.actionEnabled= true
          } else {
            rclick_menu.actionEnabled=false
          }
        }

        isMoving = false
      }

      onPositionChanged: {
        releaseX = mouse.x
        releaseY = mouse.y
        if (ismove) {
          content.x += (releaseX-pressX)
          content.y += (releaseY-pressY)
          isMoving = true
        }
      }
    }

    transform: Scale {
      id: content_transform
      origin.x: content.width / 2
      origin.y: content.height / 2
    }

    DrawPanel {
      id: draw_panel
      anchors.fill: parent

      onAddRect: function(obj, sharpType) {
        control.addDragRectObject(obj, sharpType)
      }

      onSelectChanged: function(obj) {
        control.setSelectObject(obj)
      }

      onSelectedMul: function(objArray) {
        control.setSelectMulObject(objArray)
      }

      onClearSelectedMul: {
        control.setClearSelectAllObject()
      }

      onVerifyFocus: {
        draw_panel.forceActiveFocus();
        draw_panel.Keys.enabled = true;
      }

      onObjectChanged: {
        control.objectChanged(obj, oldX, oldY, oldWidth, oldHeight, oldRotation,
                                   newX, newY, newWidth, newHeight, newRotation)
      }
    }
  }

  ColumnLayout {
    anchors.left: parent.left
    anchors.leftMargin: 10 * screenScaleFactor
    anchors.top: parent.top
    anchors.topMargin: 10 * screenScaleFactor
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 10 * screenScaleFactor
    spacing: 10 * screenScaleFactor

    LaserToolBar {
      id: tool_bar

      Layout.alignment: Qt.AlignTop

      onEllipseMode: {
        draw_panel.currentSharp = DrawPanel.SharpType.ELIPSE
      }

      onRectMode: {
        draw_panel.currentSharp = DrawPanel.SharpType.RECT
      }

      onPickMode: {
        draw_panel.currentSharp = DrawPanel.SharpType.NONE
      }

      onImageMode: {
        control.open()
      }

      onTextMode: {
        draw_panel.currentSharp = DrawPanel.SharpType.TEXT
      }

      onLineMode: {
        draw_panel.currentSharp = DrawPanel.SharpType.PATH
      }
    }

    CusImglButton {
      id: object_list_button

      Layout.alignment: Qt.AlignBottom
      Layout.minimumWidth: tool_bar.width
      Layout.minimumHeight: Layout.minimumWidth

      btnRadius: 5 * screenScaleFactor
      cusTooltip.text: object_list_popup.titleText

      borderWidth: 1
      borderColor: hovered ? Constants.left_model_list_button_border_hovered_color
                           : Constants.left_model_list_button_border_default_color

      imgWidth: 20 * screenScaleFactor
      imgHeight: 20 * screenScaleFactor
      imgFillMode: Image.PreserveAspectFit

      enabledIconSource: Constants.left_model_list_button_default_icon
      pressedIconSource: Constants.left_model_list_button_hovered_icon
      highLightIconSource: Constants.left_model_list_button_hovered_icon

      defaultBtnBgColor: Constants.left_model_list_button_default_color
      hoveredBtnBgColor: Constants.left_model_list_button_hovered_color
      selectedBtnBgColor: Constants.left_model_list_button_hovered_color

      onClicked: {
        object_list_popup.open()
      }

      Rectangle {
        id: object_count_view

        width: 20 * screenScaleFactor
        height: width
        radius: width / 2
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: -radius

        color: Constants.left_model_list_count_background_color

        Text {
          anchors.centerIn: parent
          font.family: Constants.labelFontFamily
          font.pointSize: Constants.labelFontPointSize_9
          color: Constants.left_model_list_count_text_color
          text: root.itemModel.modelCount
        }
      }
    }

    LaserPlotterModelList {
      id: object_list_popup

      control: root.control
      itemModel: root.itemModel

      y: parent.height - height
      width: 280 * screenScaleFactor
      height: 180 * screenScaleFactor
    }
  }

  LaserRClickMenu {
    id: rclick_menu

    onRClickDeleteSelect: {
      deleteSelectObject()
    }

    onSigSetTop: {
      let list_view = object_list_popup.listView
      let selected_list = list_view.selectIndexList
      let control = root.control

      if (list_view.count === selected_list.length) {
        return
      }

      var max_zpos = 0
      for (let idx = 0; idx < list_view.count; ++idx) {
        max_zpos = Math.max(max_zpos, control.selectObject(idx).z)
      }

      for (let selected_idx in selected_list) {
        let top_idx = selected_list[selected_idx]
        control.selectObject(top_idx).z = ++max_zpos
      }
    }

    onSigSetBottom: {
      let list_view = object_list_popup.listView
      let selected_list = list_view.selectIndexList
      let control = root.control

      if (list_view.count === selected_list.length) {
        return
      }

      var min_zpos = 0
      for (let idx = 0; idx < list_view.count; ++idx) {
        min_zpos = Math.min(min_zpos, control.selectObject(idx).z)
      }

      for (let selected_idx in selected_list) {
        let bottom_idx = selected_list[selected_idx]
        control.selectObject(bottom_idx).z = --min_zpos
      }
    }
  }

  onVisibleChanged: {
    if (visible) {
      forceActiveFocus();
      Keys.enabled = true;

      content_grid.width = control.width
      content_grid.height = control.height
      content_grid.repaint()

      control.origin = Qt.point(content_grid.x,content_grid.y+content_grid.height)
    } else {
      content_grid.clear()
    }
  }

  function canvasZoomOut() {
    scaleCanvas(1 / factor);
  }

  function canvasZoomIn(){
    scaleCanvas(factor);
  }

  function scaleCanvas(scale_factor){
    content_transform.xScale *= scale_factor;
    content_transform.yScale *= scale_factor;
  }

  function createImageSharp() {
    var obj = draw_panel.createSharp(0, 0, 100, 100, DrawPanel.SharpType.IMAGE)
    obj.opacity = 1
    return obj;
  }

  function addRectObject(sharp_name, obj) {
    obj.visible = true
    control.addRedoRectObject(obj, sharp_name)
  }

  UploadMessageDlg {
    id: idOutPlatform

    visible: false
    cancelBtnVisible: false
    messageType: 0
    msgText: qsTr("Model out of range, please put 'red' model in the printer!")

    onSigOkButtonClicked: {
      idOutPlatform.visible = false
    }
  }
}
