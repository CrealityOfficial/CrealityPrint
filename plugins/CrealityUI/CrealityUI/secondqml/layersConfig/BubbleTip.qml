import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "../../qml"
import "../../components"
    
Rectangle {
    id: root
    color: "transparent"
    radius: 5
    clip: true

    property var tailAnchor: "top" //"top", "bottom", "center"
    property var bubbleColor: "black"

    function requestPaint() {
        canvas.requestPaint()
    }

    onTailAnchorChanged: requestPaint()
    onBubbleColorChanged: requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        property int tailWidth: 15
        property int tailHeight: 10
        property int radius: root.radius

        function paintRect(ctx, _width, _height, radius) {
            var x = 0
            var y = 0;
            var w = _width
            var h = _height

            //console.log("paint(" + _width + ", " + _height + ", " + radius + ")")

            ctx.beginPath();
            ctx.moveTo(x + radius, y);
            ctx.lineTo(x + w - radius, y);
            ctx.arcTo(x + w, y, x + w, y + radius, radius);
            ctx.lineTo(x + w, y + h - radius);
            ctx.arcTo(x + w, y + h, x + w - radius, y + h, radius);
            ctx.lineTo(x + radius, y + h);
            ctx.arcTo(x, y + h, x, y + h - radius, radius);
            ctx.lineTo(x, y + radius);
            ctx.arcTo(x, y, x + radius, y, radius);
            ctx.closePath();
            ctx.fill();
            ctx.stroke();
        }

        onPaint: {
            var context = getContext("2d")
            context.clearRect(0, 0, width, height)
            context.fillStyle = root.bubbleColor
            context.lineWidth = 0
            context.strokeStyle = "transparent"

            paintRect(context, width - tailWidth, height, radius)

            if (root.tailAnchor == "top") {
                context.beginPath()
                context.moveTo(width, 0)
                context.lineTo(width - radius - tailWidth, 0)
                context.lineTo(width - radius - tailWidth, tailHeight)
                context.closePath()
                context.fill()

            } else if (root.tailAnchor == "bottom") {
                context.beginPath()
                context.moveTo(width, height)
                context.lineTo(width - radius - tailWidth, height)
                context.lineTo(width - radius - tailWidth, height - tailHeight)
                context.closePath()
                context.fill()

            } else if (root.tailAnchor == "center") {
                context.beginPath()
                context.moveTo(width, height / 2)
                context.lineTo(width - radius - tailWidth, (height + tailHeight) / 2)
                context.lineTo(width - radius - tailWidth, (height - tailHeight) / 2)
                context.closePath()
                context.fill()

            } 
        }
    }
}