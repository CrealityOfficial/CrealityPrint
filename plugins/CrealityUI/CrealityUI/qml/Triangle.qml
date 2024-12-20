import QtQuick 2.5

Canvas {
    id: canvasId
    property color triangleColor: "#474747"
    property color borderColor: "#00000000"
    property var hideEdge: false
    width: parent.width; height: parent.height
   // contextType: "2d"

   Rectangle{
       color: triangleColor
       width:2
       height:canvasId.height -  2
       anchors.top: canvasId.top
       anchors.topMargin:1
       anchors.right: canvasId.right
       anchors.rightMargin:-1
       visible: hideEdge
   }

    onPaint: {
        var context = getContext("2d")
        context.lineWidth = 1
        context.strokeStyle = borderColor//"#00000000"
        context.fillStyle = triangleColor
        context.beginPath();
        context.moveTo(0, canvasId.height/2)
        context.lineTo(canvasId.width, 0);
        context.lineTo(canvasId.width, canvasId.height);
        context.lineTo(0, canvasId.height/2);
        context.closePath();
        context.fill()
        context.stroke();
    }
}
