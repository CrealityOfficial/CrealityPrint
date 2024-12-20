import CrealityUI 1.0
import QtQml 2.3
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Window 2.13
import "qrc:/CrealityUI"

Item {
    id: root

    property var rootX
    property var screenRatio: Screen.devicePixelRatio > 1 ? Screen.devicePixelRatio : 1

    function getGlobalPosition(targetObject) {
        let positionX = 0;
        let positionY = 0;
        let obj = targetObject;
        while (obj !== null) {
            positionX += obj.x;
            positionY += obj.y;
            obj = obj.parent;
        }
        return x;
    }

    function resetIndicatorAngle() {
        kernel_reusable_cache.resetIndicatorAngle();
    }

    function setTopCameraAngle() {
        kernel_reusable_cache.setTopCameraAngle();
    }

    function syncIndicatorPos() {
        let global_pos = root.mapToGlobal(0, 0);
        let window_pos = Qt.point(global_pos.x - frameLessView.x, global_pos.y - frameLessView.y);
        let opengl_x = window_pos.x * screenRatio;
        let opengl_y = (frameLessView.height - window_pos.y - root.height) * screenRatio;
        kernel_reusable_cache.setIndicatorScreenPos(Qt.point(opengl_x, opengl_y), 100 * screenRatio * screenScaleFactor);
    }

    width: 100 * screenScaleFactor
    height: 100 * screenScaleFactor
    onRootXChanged: {
        root.syncIndicatorPos();
    }
    Component.onCompleted: {
        root.syncIndicatorPos();
        rootX = Qt.binding(function() {
            return getGlobalPosition(this);
        });
    }

    Connections {
        target: frameLessView
        onIsMaxChanged: {
            console.log("=============================================");
            root.syncIndicatorPos();
        }
    }

    Connections {
        target: kernel_ui.appWindow
        onWidthChanged: {
            root.syncIndicatorPos();
        }
        onHeightChanged: {
            root.syncIndicatorPos();
        }
    }

    CusSkinButton_Image {
        id: reset_button

        anchors.left: root.left
        anchors.bottom: root.top
        anchors.leftMargin: 16 * screenScaleFactor
        width: 20 * screenScaleFactor
        height: 20 * screenScaleFactor
        tipText: qsTr("Angle reset")
        btnImgUrl: hovered ? Constants.homeImg_Hovered : Constants.homeImg
        onClicked: {
            root.resetIndicatorAngle();
        }
    }

    Item {
        id: reset_button_shortcut

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+Home"
            onActivated: root.resetIndicatorAngle()
        }

          Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+0"
            onActivated: root.resetIndicatorAngle()
        }


        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+5"
            onActivated: root.resetIndicatorAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+8"
            onActivated: kernel_reusable_cache.setTopCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+2"
            onActivated: kernel_reusable_cache.setBottomCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+4"
            onActivated: kernel_reusable_cache.setLeftCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+6"
            onActivated: kernel_reusable_cache.setRightCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+7"
            onActivated: kernel_reusable_cache.setFrontCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+1"
            onActivated: kernel_reusable_cache.setFrontCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+9"
            onActivated: kernel_reusable_cache.setBackCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "Ctrl+3"
            onActivated: kernel_reusable_cache.setBackCameraAngle()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: StandardKey.ZoomIn
            onActivated: kernel_reusable_cache.setCameraZoom(1 / 1.1)
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: StandardKey.ZoomOut
            onActivated: kernel_reusable_cache.setCameraZoom(1.1)
        }

    }

}
