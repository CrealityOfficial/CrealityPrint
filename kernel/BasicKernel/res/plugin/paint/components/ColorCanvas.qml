import QtQuick 2.0
import QtQuick.Controls 2.5

Item {
    id: root
    property var penSize: 50
    property var color: Qt.rgba(1.0, 0.5, 0.0, 1.0)
    property var cursorPos
    property var screenScale: 1
    property int realPenSize: penSize / screenScale
    
    function update() {
        // circle.requestPaint()
    }

    onRealPenSizeChanged: {
        circle.requestPaint()
    }

    onColorChanged: {
        circle.requestPaint()
    }

    Canvas {
        id: circle
        width: realPenSize
        height: realPenSize
        visible: true
        property int radius: realPenSize / 2
        property int dash: radius < 10 ? 50 : 5
        x: cursorPos ? cursorPos.x - radius : 0
        y: cursorPos ? cursorPos.y - radius : 0


        onPaint: {
            var ctx = getContext("2d")
            ctx.strokeStyle = root.color
            ctx.lineWidth = 2
            ctx.setLineDash([dash, dash])

            var r = radius
            if (radius < 2)
                r = 2
            
            var centerX = r
            var centerY = r
            var inRadius = r - ctx.lineWidth / 2

            // console.log("paint circle: ", radius, ", screenScale ", screenScale, ", penSize ", penSize)

            ctx.beginPath()
            ctx.arc(centerX, centerY, Math.floor(inRadius), 0, 2 * Math.PI)
            ctx.closePath()

            ctx.stroke()
        }
    }
}