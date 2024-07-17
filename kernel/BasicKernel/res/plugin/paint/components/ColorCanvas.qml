import QtQuick 2.0
import QtQuick.Controls 2.5

Item {
    id: root
    property var penSize: 50
    property var color: Qt.rgba(1.0, 0.5, 0.0, 1.0)
    property var cursorPos
    property var screenScale: 1
    property var realPenSize: penSize * 100.0 / screenScale
    property var screenRadius: 10
    property int lineWidth: 2
    property int inRadius: Math.floor(screenRadius - lineWidth / 2)
    
    function update() {
        // circle.requestPaint()
    }

    onInRadiusChanged: {
        circle.width = inRadius * 2 + 2
        circle.height = inRadius * 2 + 2
        circle.x = cursorPos.x - inRadius - 1 
        circle.y = cursorPos.y - inRadius - 1
        circle.requestPaint()
    }

    onCursorPosChanged: {
        circle.x = cursorPos.x - inRadius - 1
        circle.y = cursorPos.y - inRadius - 1
    }

    onColorChanged: {
        circle.requestPaint()
    }

    Canvas {
        id: circle
        visible: radius != 0
        property int radius: screenRadius
        property int dash: radius < 10 ? 50 : 5

        onPaint: {
            if (radius == 0)
                return

            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height);
            ctx.strokeStyle = root.color
            ctx.lineWidth = lineWidth
            ctx.setLineDash([dash, dash])

            var centerX = root.inRadius + 1
            var centerY = root.inRadius + 1

            // console.log("paint circle: ", radius, ", screenScale ", screenScale, ", penSize ", penSize)
            ctx.beginPath()
            ctx.arc(centerX, centerY, root.inRadius, 0, 2 * Math.PI)
            ctx.closePath()

            ctx.stroke()
        }
    }
}