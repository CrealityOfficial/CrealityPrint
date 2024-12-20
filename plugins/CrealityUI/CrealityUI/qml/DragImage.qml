import QtQuick 2.0
import QtQuick 2.13
import QtQuick.Shapes 1.13
Rectangle {
    id:rectRoi
    property var oldX
    property var oldY
    property var oldWidth
    property var oldHeight
    property var oldRotation
	property  var imageName: "def"
	property  var imageW: 0
	property  var imageH: 0
    property var dragType:"image"
    property var threshold: 127
    property var rulersSize: 9
    property var densityValue: 9
    property var reverse: false
    property var m4mode: false
    property var originalShow: false
    property var contrastValue: 50
    property var brightnessValue: 50
    property var whiteValue: 255
    property var filpModelValue: 0
    transformOrigin: Item.Center;
    property var selected:false
    property alias imageurl: idDrawImage.source
    property var sizelock: Constants.bIsLaserSizeLoced
    property var hasError: false
    signal select(var sender)
    signal ctrlSelect(var ctrlSender)
    signal objectChanged(var obj, var oldX, var oldY, var oldWidth, var oldHeight, var oldRotation, var newX, var newY, var newWidth, var newHeight, var newRotation)
    antialiasing: true
    opacity: 0.4

    function setAnimation(value)
    {
        //console.log("animation ...")
        idAnimatedImage.visible = value
    }
    function showWarningColor(hasShow)
    {
        hasError = hasShow
        idRect.visible = hasShow;
    }

    border {
        width: selected?1:0
        color: Constants.textColor
    }
    color: "transparent"
    Image {
        id:idDrawImage
        width: parent.width
        height: parent.height
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        source: imageurl
        mipmap:true
        cache: false
    }
    Rectangle{
        id: idRect
        width: parent.width
        height: parent.height
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: "red"
        opacity: 0.5
        visible: false;
    }
    AnimatedImage{
        anchors{
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }
        id: idAnimatedImage
        width: 20
        height: 20
        visible: false
        z: idDrawImage.z + 1
        source: "qrc:/UI/photo/loading.gif"
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
        visible: selected
        border {
            width: 1
            color: Constants.textColor
        }
        anchors {
            top: parent.top;
            margins: -100;
            horizontalCenter: parent.horizontalCenter;
        }

        MouseArea{
            anchors.fill: parent;
            cursorShape: Qt.PointingHandCursor
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
        color: Constants.laserLineBorderColor
        width: 1
        height: 100-handle.height
        visible: selected
        antialiasing: true
    }
    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: Constants.laserLineBorderColor
        }
        antialiasing: true
        visible: selected
        anchors.horizontalCenter: parent.left
        anchors.verticalCenter: parent.top
        id: selComp
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeFDiagCursor
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

                    if(sizelock)
                    {
                        var factor =  rectRoi.width/oldWidth
                        rectRoi.height = oldHeight*factor
                    }
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active){
                    var newHeight = rectRoi.height - mouseY;

                    if (newHeight < 30 || sizelock)
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
        color: "transparent"
        border {
            width: 1
            color: Constants.laserLineBorderColor
        }
        antialiasing: true
        visible: selected
        anchors.horizontalCenter: parent.left
        anchors.verticalCenter: parent.bottom

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeBDiagCursor
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

                    if(sizelock)
                    {
                        var factor =  rectRoi.width/oldWidth
                        rectRoi.height = oldHeight*factor
                    }
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active){

                    var newHeight = rectRoi.height + mouseY;

                    if (newHeight < 30 || sizelock)
                        return
                    rectRoi.height = newHeight
                }
            }
        }
    }

    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: Constants.laserLineBorderColor
        }
        antialiasing: true
        visible: selected
        anchors.horizontalCenter: parent.right
        anchors.verticalCenter: parent.bottom
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeFDiagCursor
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

                    if(sizelock)
                    {
                        var factor =  rectRoi.width/oldWidth
                        rectRoi.height = oldHeight*factor
                    }
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active){

                    var newHeight = rectRoi.height + mouseY;

                    if (newHeight < 30 || sizelock)
                        return
                    rectRoi.height = newHeight
                }
            }
        }
    }

    Rectangle {
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: Constants.laserLineBorderColor
        }
        antialiasing: true
        visible: selected
        anchors.horizontalCenter: parent.right
        anchors.verticalCenter: parent.top
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeBDiagCursor
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

                    if(sizelock)
                    {
                        var factor =  rectRoi.width/oldWidth
                        rectRoi.height = oldHeight*factor
                    }
                }
            }
            drag{ target: parent; axis: Drag.YAxis }
            onMouseYChanged: {
                if(drag.active) {
                    var newHeight = rectRoi.height - mouseY;
                    if (newHeight < 30 || sizelock)
                        return
                    rectRoi.height = newHeight
                    rectRoi.y = rectRoi.y + mouseY
                }
            }
        }
    }
}
