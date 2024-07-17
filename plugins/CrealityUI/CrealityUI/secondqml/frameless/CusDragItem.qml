import QtQuick 2.9
import QtQuick.Controls 2.2

Item {
    id: root

    property alias containsMouse: mouseArea.containsMouse
    property int posType: Qt.ArrowCursor
    //5.10之前, qml是不能定义枚举的，用只读的int属性代替一下。
    readonly property int posLeftTop: Qt.SizeFDiagCursor
    readonly property int posLeft: Qt.SizeHorCursor
    readonly property int posLeftBottom: Qt.SizeBDiagCursor
    readonly property int posTop: Qt.SizeVerCursor
    readonly property int posBottom: Qt.SizeVerCursor
    readonly property int posRightTop: Qt.SizeBDiagCursor
    readonly property int posRight: Qt.SizeHorCursor
    readonly property int posRightBottom: Qt.SizeFDiagCursor

    signal posChange(int xOffset, int yOffset)

    implicitWidth: 12
    implicitHeight: 12

    MouseArea {
        id: mouseArea

        property int lastX: 0
        property int lastY: 0

        anchors.fill: parent
        hoverEnabled: true
        onContainsMouseChanged: {
            if (containsMouse)
                cursorShape = posType;
            else
                cursorShape = Qt.ArrowCursor;
        }
        onPressedChanged: {
            if (containsPress) {
                lastX = mouseX;
                lastY = mouseY;
            }
        }
        onPositionChanged: {
            if (pressed)
                posChange(mouseX - lastX, mouseY - lastY);

        }
    }

}
