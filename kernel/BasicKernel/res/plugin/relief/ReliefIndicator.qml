import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0

Item {
    id: root

    property var pos: Qt.point(0, 0)
    property var needIndicate: false

    function setColor(color) {
        circle.color = color
        circle.requestPaint()
        console.log("hover changed : " + circle.color)
    }

    // onColorChanged: {
    //     circle.requestPaint()
    // }

    Canvas {
        id: circle

        visible: root.needIndicate
        property var centerTo: root.pos
        property var color: "red"

        property var lineSize: 12
        property var radius: 14

        width: lineSize + radius * 2.0
        height: lineSize + radius * 2.0

        x: centerTo.x - width / 2.0
        y: centerTo.y - height / 2.0


        onPaint: {
            if (radius == 0)
                return

            var margin = lineSize / 2.0
            var borderSize = width
            var borderCenter = borderSize / 2.0

            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height);
            ctx.strokeStyle = color
            ctx.lineWidth = 2

            var centerX = radius + 1
            var centerY = radius + 1

            // console.log("paint circle: ", radius, ", screenScale ", screenScale, ", penSize ", penSize)
            ctx.beginPath()
            ctx.arc(margin + centerX, margin + centerY, radius, 0, 2 * Math.PI)


            ctx.moveTo(borderCenter, 0.0)
            ctx.lineTo(borderCenter, lineSize)

            ctx.moveTo(borderCenter, borderSize - lineSize)
            ctx.lineTo(borderCenter, borderSize)

            ctx.moveTo(0.0, borderCenter)
            ctx.lineTo(lineSize, borderCenter)

            ctx.moveTo(borderSize - lineSize, borderCenter)
            ctx.lineTo(borderSize, borderCenter)

            ctx.stroke()
        }
    }
}