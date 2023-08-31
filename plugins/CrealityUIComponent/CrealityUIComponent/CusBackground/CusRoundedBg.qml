import QtQuick 2.12
import QtGraphicalEffects 1.12
import QtQml 2.3

Item{
    property alias radius: canvas.radius
    property alias borderColor: canvas.strokeStyle
    property alias borderWidth: canvas.lineWidth

    property bool allRadius: false
    property bool leftTop: allRadius
    property bool rightTop: allRadius
    property bool rightBottom: allRadius
    property bool leftBottom: allRadius

    property bool shadowEnabled: false  //是否启用阴影
    property color color

    id:roundBtn

    onColorChanged: {
        canvas.requestPaint()
    }

    onBorderColorChanged:{
        canvas.requestPaint()
    }

    Component.onCompleted: canvas.requestPaint()

    layer.enabled: shadowEnabled
    layer.effect:
        DropShadow {
        verticalOffset: 2
        //        samples: 5
        color:Constants.dropShadowColor
    }


    Canvas {
        id: canvas
        width: parent.width
        height: parent.height
        antialiasing: true

        property int radius : 5 //: roundBtn.radius
        property int rectx: 0
        property int recty: 0
        property int rectWidth: width
        property int rectHeight: height
        property color strokeStyle:  Qt.darker(fillStyle, 1.0)
        property color fillStyle: roundBtn.color
        property int lineWidth: 1
        property bool fill: true
        property bool stroke: true
        property real alpha: 1.0

        onLineWidthChanged:requestPaint();
        onFillChanged:requestPaint();
        onStrokeChanged:requestPaint();
        onRadiusChanged:requestPaint();

        onPaint: {
            var ctx = getContext("2d");
            ctx.save();
            ctx.clearRect(0,0,canvas.width, canvas.height);
            ctx.strokeStyle = canvas.strokeStyle;
            ctx.lineWidth = canvas.lineWidth
            ctx.fillStyle = canvas.fillStyle
            ctx.globalAlpha = canvas.alpha
            ctx.beginPath();
            ctx.moveTo(rectx+radius,recty);                 // top side
            ctx.lineTo(rectx+rectWidth-radius*roundBtn.rightTop,recty);
            // draw top right corner
            if(roundBtn.rightTop)
                ctx.arcTo(rectx+rectWidth,recty,rectx+rectWidth,recty+radius,radius);
            ctx.lineTo(rectx+rectWidth,recty+rectHeight-radius*roundBtn.rightBottom);    // right side
            // draw bottom right corner
            if(roundBtn.rightBottom)
                ctx.arcTo(rectx+rectWidth,recty+rectHeight,rectx+rectWidth-radius,recty+rectHeight,radius);
            ctx.lineTo(rectx+radius*roundBtn.leftBottom,recty+rectHeight);              // bottom side
            // draw bottom left corner
            if(roundBtn.leftBottom)
                ctx.arcTo(rectx,recty+rectHeight,rectx,recty+rectHeight-radius,radius);
            ctx.lineTo(rectx,recty+radius*roundBtn.leftTop);                 // left side
            // draw top left corner
            if(roundBtn.leftTop)
                ctx.arcTo(rectx,recty,rectx+radius,recty,radius);
            ctx.closePath();
            if (canvas.fill)
                ctx.fill();
            if (canvas.stroke)
                ctx.stroke();
            ctx.restore();
        }
    }
}
