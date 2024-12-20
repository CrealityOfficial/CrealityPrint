import QtQuick 2.0
import CrealityUI 1.0
import QtQuick.Window 2.13
import "qrc:/CrealityUI"

Item {
    id: root

    parent: kernel_ui.appWindow.zSlider
    width: 100 * screenScaleFactor
    height: 100 * screenScaleFactor

    x: -(width - parent.width) / 2
    y: -height

    property var rootX
    property var screenRatio: Screen.devicePixelRatio > 1 ? Screen.devicePixelRatio : 1

    function getGlobalPosition(targetObject){
        let positionX = 0
        let positionY = 0
        let obj = targetObject

        while(obj !== null){
            positionX += obj.x
            positionY += obj.y

            obj = obj.parent
        }

        return x;
    }

    function resetIndicatorAngle() {
        kernel_reusable_cache.resetIndicatorAngle()
    }

    function syncIndicatorPos() {
        let global_pos = root.mapToGlobal(0, 0)
        let window_pos = Qt.point(global_pos.x - kernel_ui.appWindow.x,
                                  global_pos.y - kernel_ui.appWindow.y)
        let opengl_x = window_pos.x * screenRatio
        let opengl_y = (kernel_ui.appWindow.height - window_pos.y - root.height) * screenRatio
        kernel_reusable_cache.setIndicatorScreenPos(Qt.point(opengl_x, opengl_y), 100.0 * screenRatio * screenScaleFactor)
    }

    onRootXChanged: {
        console.log("------------x = ", rootX)

        let global_pos = root.mapToGlobal(0, 0)
        let window_pos = Qt.point(global_pos.x - kernel_ui.appWindow.x,
                                  global_pos.y - kernel_ui.appWindow.y)
        let opengl_x = window_pos.x * screenRatio
        let opengl_y = (kernel_ui.appWindow.height - window_pos.y - root.height) * screenRatio
        kernel_reusable_cache.setIndicatorScreenPos(Qt.point(opengl_x, opengl_y), 100.0 * screenRatio * screenScaleFactor)
    }

    Component.onCompleted: {
        root.syncIndicatorPos()
        rootX = Qt.binding(function() { return getGlobalPosition(this) } )
    }

    Connections {
        target: kernel_ui.appWindow

        onWidthChanged: {
            root.syncIndicatorPos()
        }

        onHeightChanged: {
            root.syncIndicatorPos()
        }
    }

    CusSkinButton_Image {
        id: reset_button

        anchors.left: root.left
        anchors.bottom: root.top
        anchors.leftMargin: 16 * screenScaleFactor

        width: 20 * screenScaleFactor
        height: 20 * screenScaleFactor
        visible: kernel_parameter_manager.functionType === 0

        tipText:qsTr("Angle reset")

        btnImgUrl: hovered ? Constants.homeImg_Hovered : Constants.homeImg
        onClicked: {
            root.resetIndicatorAngle()
        }

        onXChanged: {
            let window_pos = Qt.point(global_pos.x - kernel_ui.appWindow.x,
                                      global_pos.y - kernel_ui.appWindow.y)
            console.log("++++++++ window_pos = ", window_pos)
            root.syncIndicatorPos()
        }
    }
}
