import QtQuick 2.0
import QtQuick.Window 2.2

Item {
    property int enableSize: 4
    property bool isPressed: false
    property point customPoint
    property bool cursorEnable : true
    //左上角
    Item {
        id: leftTop
        width: enableSize
        height: enableSize
        anchors.left: parent.left
        anchors.top: parent.top
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeFDiagCursor
            onPressed: press(mouse)
//            onEntered: enter(1)
            onReleased: release()
            onPositionChanged: positionChange(mouse, -1, -1)
        }
    }

    //Top
    Item {
        id: top
        height: enableSize
        anchors.left: leftTop.right
        anchors.right: rightTop.left
        anchors.top: parent.top
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeVerCursor
            onPressed: press(mouse)
//            onEntered: enter(2)
            onReleased: release()

            onMouseYChanged: positionChange(Qt.point(customPoint.x, mouseY), 1, -1)
        }
    }

    //右上角
    Item {
        id: rightTop
        width: enableSize
        height: enableSize
        anchors.right: parent.right
        anchors.top: parent.top
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeBDiagCursor
            onPressed: press(mouse)
//            onEntered: enter(3)
            onReleased: release()
            onPositionChanged: positionChange(mouse, 1, -1)
        }
    }

    //Left
    Item {
        id: left
        width: enableSize
        anchors.left: parent.left
        anchors.top: leftTop.bottom
        anchors.bottom: leftBottom.top
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeHorCursor
            onPressed: press(mouse)
//            onEntered: enter(4)
            onReleased: release()

            onMouseXChanged: positionChange(Qt.point(mouseX, customPoint.y), -1, 1)
        }
    }

    //Center - 5
    Item {
        id: center
        anchors.left: left.right
        anchors.right: right.left
        anchors.top: top.bottom
        anchors.bottom: bottom.top
        MouseArea {
            anchors.fill: parent
            cursorShape:Qt.ArrowCursor
            property point clickPos: "0,0"

            onPressed: clickPos = Qt.point(mouse.x, mouse.y)

            onPositionChanged: {
                if(pressed && standaloneWindow.visibility !== Window.Maximized && standaloneWindow.visibility !== Window.FullScreen)
                {
                    var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
                    standaloneWindow.x += delta.x
                    standaloneWindow.y += delta.y
                }
            }
        }
    }

    //Right
    Item {
        id: right
        width: enableSize
        anchors.right: parent.right
        anchors.top: rightTop.bottom
        anchors.bottom: rightBottom.top
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeHorCursor
            onPressed: press(mouse)
//            onEntered: enter(6)
            onReleased: release()
            onMouseXChanged: positionChange(Qt.point(mouseX, customPoint.y), 1, 1)
        }
    }

    //左下角
    Item {
        id: leftBottom
        width: enableSize
        height: enableSize
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeBDiagCursor
            onPressed: press(mouse)
//            onEntered: enter(7)
            onReleased: release()

            onPositionChanged: positionChange(mouse, -1, 1)
        }
    }

    //bottom
    Item {
        id: bottom
        height: enableSize
        anchors.left: leftBottom.right
        anchors.right: rightBottom.left
        anchors.bottom: parent.bottom
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            cursorShape:Qt.SizeVerCursor
            onPressed: press(mouse)
//            onEntered: enter(8)
            onReleased: release()

            onMouseYChanged: positionChange(Qt.point(customPoint.x, mouseY), 1, 1)
        }
    }

    //右下角
    Item {
        id:rightBottom
        width: enableSize
        height: enableSize
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: cursorEnable
        z: 1000
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape:Qt.SizeFDiagCursor
            onPressed: press(mouse)
//            onEntered: enter(9)
            onReleased: release()

            onPositionChanged: positionChange(mouse,1,1)
        }
    }

    function enter(direct) {
        switch(direct)
        {
        case 1 : return  Qt.IBeamCursor;

        }
    }

    function press(mouse) {
        isPressed = true
        customPoint = Qt.point(mouse.x, mouse.y)
    }

    function release() {
        isPressed = false
        //customPoint = undefined
    }

    function positionChange(newPosition, directX/*x轴方向*/, directY/*y轴方向*/) {
        if(!isPressed) return

        var delta = Qt.point(newPosition.x-customPoint.x, newPosition.y-customPoint.y)
        var tmpW,tmpH

        if(directX >= 0)
            tmpW = standaloneWindow.width + delta.x
        else
            tmpW = standaloneWindow.width - delta.x

        if(directY >= 0)
            tmpH = standaloneWindow.height + delta.y
        else
            tmpH = standaloneWindow.height - delta.y

        if(tmpW < standaloneWindow.minimumWidth) {
            if(directX < 0)
                standaloneWindow.x += (standaloneWindow.width - standaloneWindow.minimumWidth)
            standaloneWindow.width = standaloneWindow.minimumWidth
        }
        else {
            standaloneWindow.width = tmpW
            if(directX < 0)
                standaloneWindow.x += delta.x
        }

        if(tmpH < standaloneWindow.minimumHeight) {
            if(directY < 0)
                standaloneWindow.y += (standaloneWindow.height - standaloneWindow.minimumHeight)
            standaloneWindow.height = standaloneWindow.minimumHeight
        }
        else {
            standaloneWindow.height = tmpH
            if(directY < 0)
                standaloneWindow.y += delta.y
        }
    }
}
