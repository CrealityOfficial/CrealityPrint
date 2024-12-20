import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "./"
import "../"
import "../../qml"
import "../../components"

Canvas {
    property var printerSettings
    property var layers: printerSettings ? printerSettings.specificExtruderHeights : []
    property var totalLayers: 1
    property var radius: 0
    property var control 

    onLayersChanged: {
        requestPaint()
    }

    function paintRect(ctx, length, radius) {
        var x = 0
        var y = 0;
        var w = width
        var h = length

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

    function paintRectWithBottomRadius(ctx, length, r) {
        ctx.beginPath();
        // ctx.moveTo(width, 0)
        // ctx.arcTo(width, 0, width, length, r)
        // ctx.arcTo(width, length, 0, length, r)
        // ctx.arcTo(0, length, 0, 0, r)
        // ctx.lineTo(0, 0)
        // ctx.lineTo(width, 0)
        
        ctx.moveTo(0, length)
        ctx.lineTo(width, length)
        ctx.lineTo(width, 0)
        ctx.lineTo(0, 0)
        ctx.closePath();
        ctx.fill();
        ctx.stroke();
    }

    function paintRectWithTopRadius(ctx, length, r) {
        ctx.beginPath();
        // ctx.moveTo(0, length)
        // ctx.arcTo(0, length, 0, 0, r)
        // ctx.arcTo(0, 0, width, 0, r)
        // ctx.arcTo(width, 0, width, length, r)

        ctx.moveTo(0, length)
        ctx.lineTo(width, length)
        ctx.lineTo(width, 0)
        ctx.lineTo(0, 0)
        ctx.closePath();
        ctx.fill();
        ctx.stroke();
    }

    onPaint: {
        if (!printerSettings)
            return 
        
        var ctx = getContext("2d");
        ctx.lineWidth = 0
        ctx.strokeStyle = "transparent"

        ctx.fillStyle = printerSettings.defaultColor()
        paintRect(ctx, height, radius)

        for (let i = 0, count = layers.length; i < count; ++i) {
            let height = layers[i].toFixed(2)
            let layer = printerSettings.heightLayer(height); 
            let length = control.locationMap[layer]
            ctx.fillStyle = printerSettings.layerColor(layer)
            paintRect(ctx, length, radius)

        }

    }
}
