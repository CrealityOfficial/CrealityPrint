import QtQuick 2.0
import QtQuick 2.13
import QtQuick.Shapes 1.13
import QtQml 2.13

Rectangle {
    id:rectRoi
    property var oldX
    property var oldY
    property var oldWidth
    property var oldHeight
    property var oldRotation

    property var rulersSize: 9
    transformOrigin: Item.Center;
    property var selected:false
    property var maxX: 0
    property var maxY: 0
    property var minX: 0
    property var minY: 0
	property alias shapeX: myShape.x
	property alias shapeY: myShape.y
    signal select(var sender)
    signal ctrlSelect(var ctrlSender)
    signal objectChanged(var obj, var oldX, var oldY, var oldWidth, var oldHeight, var oldRotation, var newX, var newY, var newWidth, var newHeight, var newRotation)
    antialiasing: true
    opacity: 0.4

    onWidthChanged: {
        console.log(width)
        console.log(height)
    }
    function addPath(x,y)
    {
        myPositions.append({"x":x, "y":y})
    }
	function getPathcnt()
	{
		return myPositions.count
	}
	function setEndPos(x,y)
	{
		if(myPositions.count < 2)
		{
			myPositions.append({"x":x, "y":y})
		}
		else
		{
			myPositions.get(1).x = x
			myPositions.get(1).y = y
		}
	}
	
    function reCal()
    {
        for(var i in myPath.pathElements)
        {
            var path = myPath.pathElements[i]
            path.x = path.x - minX
            path.y = path.y - minY
        }
    }
    border {
        width: selected?2:0
        color: "black"
    }
    color: "#00F0F8FF"
    Shape
       {
           id: myShape
           width: parent.width
           height: parent.height
           ListModel {
                   id: myPositions
               }
           ShapePath {
               id: myPath
                       fillColor: "transparent"
                       strokeColor: "red"
                       capStyle: ShapePath.RoundCap
                       joinStyle: ShapePath.RoundJoin
                       strokeWidth: 3
                       strokeStyle: ShapePath.SolidLine
           }
           Instantiator {
                   model: myPositions
                   onObjectAdded: myPath.pathElements.push(object)
                   PathLine {
                       x: model.x
                       y: model.y
                   }
               }
       }
    MouseArea {
        anchors.fill: parent
        drag{
            target: rectRoi
            minimumX: 0
            minimumY: 0
            maximumX: parent.parent.width - parent.width
            maximumY: parent.parent.height - parent.height
            smoothed: true
        }
        onPressed: {
            console.log("select")
            //select(rectRoi)
             switch(mouse.modifiers){
                case Qt.ControlModifier:{
                        ctrlSelect(rectRoi)
                    }
                    break;
                case Qt.ShiftModifier:
                    break;
                default:
                    {
                        select(rectRoi)
                    }
                    break;
                }
            oldX = rectRoi.x
            oldY = rectRoi.y
            oldWidth = rectRoi.width
            oldHeight = rectRoi.height
            oldRotation = rectRoi.rotation
        }
        onReleased: {
            objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
        }
    }
    Rectangle {
        id: handle;
        color: "#3AC2D7";
        width: 9;
        height: width;
        //radius: (width / 2);
        antialiasing: true;
        visible: selected && rectRoi.opacity==1
        border {
            width: 1
            color: "black"
        }
        anchors {
            top: parent.top;
            margins: -100;
            horizontalCenter: parent.horizontalCenter;
        }

        MouseArea{
            anchors.fill: parent;
            onPositionChanged:  {
                var container = roiRectArea
                var centerX = rectRoi.x+rectRoi.width/2
                var centerY = rectRoi.y+rectRoi.height/2
                console.log(centerX,centerY)
                var point =  mapToItem (container, mouse.x, mouse.y);
                console.log(point)
                var diffX = (point.x - centerX);
                var diffY = -1 * (point.y - centerY);
                var rad = Math.atan (diffY / diffX);
                var deg = (rad * 180 / Math.PI);
                console.log(deg)
                if (diffX > 0 && diffY > 0) {
                    rectRoi.rotation = 90 - Math.abs (deg);
                }
                else if (diffX > 0 && diffY < 0) {
                    rectRoi.rotation = 90 + Math.abs (deg);
                }
                else if (diffX < 0 && diffY > 0) {
                    rectRoi.rotation = 270 + Math.abs (deg);
                }
                else if (diffX < 0 && diffY < 0) {
                    rectRoi.rotation = 270 - Math.abs (deg);
                }
            }
	    	onPressed: {
                    oldX = rectRoi.x
                    oldY = rectRoi.y
                    oldWidth = rectRoi.width
                    oldHeight = rectRoi.height
                    oldRotation = rectRoi.rotation
                }
                onReleased: {
                    objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
                }
        }
    }
    Rectangle {
        x:handle.x+handle.width/2
        y:handle.y+handle.height
        color:"#000000"
        width: 1
        height: 100-handle.height
        visible: selected && rectRoi.opacity==1
        antialiasing: true
    }
    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "white"
		visible:false
        border {
            width: 1
            color: "black"
        }
        antialiasing: true
        //visible: selected && rectRoi.opacity==1
        anchors.horizontalCenter: parent.left
        anchors.verticalCenter: parent.top
        id: selComp
        MouseArea {
            anchors.fill: parent
            drag{ target: parent; axis: Drag.XAxis }
                onPressed: {
                    oldX = rectRoi.x
                    oldY = rectRoi.y
                    oldWidth = rectRoi.width
                    oldHeight = rectRoi.height
                    oldRotation = rectRoi.rotation
                }
                onReleased: {
                    objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
                }
            onMouseXChanged: {
                if(drag.active){
                    var newWidth = rectRoi.width - mouseX
                    if (newWidth < 30)
                        return
                    rectRoi.width = newWidth
                    rectRoi.x = rectRoi.x + mouseX
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active){
                    var newHeight = rectRoi.height - mouseY;

                    if (newHeight < 30)
                        return
                    rectRoi.height = newHeight
                    rectRoi.y = rectRoi.y + mouseY
                }
            }
        }
    }

    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "white"
		visible:false
        border {
            width: 1
            color: "black"
        }
        antialiasing: true
        //visible: selected && rectRoi.opacity==1
        anchors.horizontalCenter: parent.left
        anchors.verticalCenter: parent.bottom

        MouseArea {
            anchors.fill: parent
            drag{ target: parent; axis: Drag.XAxis; }
                onPressed: {
                    oldX = rectRoi.x
                    oldY = rectRoi.y
                    oldWidth = rectRoi.width
                    oldHeight = rectRoi.height
                    oldRotation = rectRoi.rotation
                }
                onReleased: {
                    objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
                }
            onMouseXChanged: {
                if(drag.active) {
                    var newWidth = rectRoi.width - mouseX
                    if (newWidth < 30)
                        return
                    rectRoi.width = newWidth
                    rectRoi.x = rectRoi.x + mouseX
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active){

                    var newHeight = rectRoi.height + mouseY;

                    if (newHeight < 30)
                        return
                    rectRoi.height = newHeight
                }
            }
        }
    }

    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "white"
		visible:false
        border {
            width: 1
            color: "black"
        }
        antialiasing: true
        //visible: selected && rectRoi.opacity==1
        anchors.horizontalCenter: parent.right
        anchors.verticalCenter: parent.bottom
        MouseArea {
            anchors.fill: parent
            drag{ target: parent; axis: Drag.XAxis }
                onPressed: {
                    oldX = rectRoi.x
                    oldY = rectRoi.y
                    oldWidth = rectRoi.width
                    oldHeight = rectRoi.height
                    oldRotation = rectRoi.rotation
                }
                onReleased: {
                    objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
                }
            onMouseXChanged: {
                if(drag.active){

                    var newWidth = rectRoi.width + mouseX
                    if (newWidth < 30)
                        return
                    rectRoi.width = newWidth
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active){

                    var newHeight = rectRoi.height + mouseY;

                    if (newHeight < 30)
                        return
                    rectRoi.height = newHeight
                }
            }
        }
    }

    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "white"
		visible:false
        border {
            width: 1
            color: "black"
        }
        antialiasing: true
       // visible: selected && rectRoi.opacity==1
        anchors.horizontalCenter: parent.right
        anchors.verticalCenter: parent.top
        MouseArea {
            anchors.fill: parent
            drag{ target: parent; axis: Drag.XAxis; }
                onPressed: {
                    oldX = rectRoi.x
                    oldY = rectRoi.y
                    oldWidth = rectRoi.width
                    oldHeight = rectRoi.height
                    oldRotation = rectRoi.rotation
                }
                onReleased: {
                    objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
                }
            onMouseXChanged: {
                if(drag.active){

                    var newWidth = rectRoi.width + mouseX
                    if (newWidth < 30)
                        return
                    rectRoi.width = newWidth
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active) {
                    var newHeight = rectRoi.height - mouseY;
                    if (newHeight < 30)
                        return
                    rectRoi.height = newHeight
                    rectRoi.y = rectRoi.y + mouseY
                }
            }
        }
    }
}
