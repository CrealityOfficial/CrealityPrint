import QtQuick 2.9
import QtQuick.Controls 2.2

Item {
    id: root

    //controller 要控制大小的目标，可以是Item，也可以是view，只要提供x、y、width、height等属性的修改
    //默认值为parent
    property var control: parent
    property int borderWidth: 12

    //左上角的拖拽
    CusDragItem {
        id: leftTopHandle

        posType: Qt.SizeFDiagCursor
        width: borderWidth
        height: borderWidth
        onPosChange: {
            //不要简化这个判断条件，化简之后不容易看懂. Qml引擎会自动简化
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;

            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;

            if (control.width - xOffset > 0)
                control.width -= xOffset;

            if (control.height - yOffset > 0)
                control.height -= yOffset;

        }
    }

    //右上角拖拽
    CusDragItem {
        id: rightTopHandle

        posType: Qt.SizeBDiagCursor
        x: parent.width - width
        width: borderWidth
        height: borderWidth
        onPosChange: {
            //向左拖动时，xOffset为负数
            if (control.width + xOffset > 0)
                control.width += xOffset;

            if (control.height - yOffset > 0)
                control.height -= yOffset;

            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;

        }
    }

    //左下角拖拽
    CusDragItem {
        id: leftBottomHandle

        posType: Qt.SizeBDiagCursor
        y: parent.height - height
        width: borderWidth
        height: borderWidth
        onPosChange: {
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;

            if (control.width - xOffset > 0)
                control.width -= xOffset;

            if (control.height + yOffset > 0)
                control.height += yOffset;

        }
    }

    //右下角拖拽
    CusDragItem {
        id: rightBottomHandle

        posType: Qt.SizeFDiagCursor
        x: parent.width - width
        y: parent.height - height
        width: borderWidth
        height: borderWidth
        onPosChange: {
            if (control.width + xOffset > 0)
                control.width += xOffset;

            if (control.height + yOffset > 0)
                control.height += yOffset;

        }
    }

    //上边拖拽
    CusDragItem {
        posType: Qt.SizeVerCursor
        width: parent.width - leftTopHandle.width - rightTopHandle.width
        height: borderWidth
        x: leftBottomHandle.width
        onPosChange: {
            if (control.y + yOffset < control.y + control.height)
                control.y += yOffset;

            if (control.height - yOffset > 0)
                control.height -= yOffset;

        }
    }

    //左边拖拽
    CusDragItem {
        posType: Qt.SizeHorCursor
        height: parent.height - leftTopHandle.height - leftBottomHandle.height
        width: borderWidth
        y: leftTopHandle.height
        onPosChange: {
            if (control.x + xOffset < control.x + control.width)
                control.x += xOffset;

            if (control.width - xOffset > 0)
                control.width -= xOffset;

        }
    }

    //右边拖拽
    CusDragItem {
        posType: Qt.SizeHorCursor
        x: parent.width - width
        height: parent.height - rightTopHandle.height - rightBottomHandle.height
        width: borderWidth
        y: rightTopHandle.height
        onPosChange: {
            if (control.width + xOffset > 0)
                control.width += xOffset;

        }
    }

    //下边拖拽
    CusDragItem {
        posType: Qt.SizeVerCursor
        x: leftBottomHandle.width
        y: parent.height - height
        width: parent.width - leftBottomHandle.width - rightBottomHandle.width
        height: borderWidth
        onPosChange: {
            if (control.height + yOffset > 0)
                control.height += yOffset;

        }
    }

}
