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

    property var dragType:"text"
    property var threshold: 50
    property var rulersSize: 15
    transformOrigin: Item.Center;
    property var selected:false
    property alias txt: idText
    property alias fontstyle: idText.font.styleName
    property alias textWidth: idText.contentWidth
    property alias textHeight: idText.contentHeight
    property alias fontfamily: idText.font.family
    property alias fontsize: idText.font.pointSize
    property alias text: idText.text
    property var textConfig

    property var hasError: false
    //height: idText.contentHeight+10
    //property alias imageurl: idDrawImage.source
    signal select(var sender)
    signal ctrlSelect(var ctrlSender)
    signal objectChanged(var obj, var oldX, var oldY, var oldWidth, var oldHeight, var oldRotation, var newX, var newY, var newWidth, var newHeight, var newRotation)
    antialiasing: true
    opacity: 1

    function showWarningColor(hasShow)
    {
        hasError = hasShow
        if(hasShow){
            idText.color = "#FF8080"
            idText.opacity = 0.5
        }else{
            idText.color = "#FFFFFF"
            idText.opacity = 1
        }
    }

    border {
        width: selected?1:0
        color: "#FFFFFF"
    }
    onSelectedChanged: {
        if(!selected)
        {
            idText.focus=false
        }
    }
    color: "transparent"
    TextInput {
        id:idText
        font.family: textConfig.familyName
        font.pointSize: textConfig.textSize
        font.weight: Font.Normal
        text: toUtf8(textConfig.textName)
        color: "#fff600"
        opacity: 1
        readOnly: true

        function toUtf8(str) {
            return encodeURIComponent(str)
        }

        onContentWidthChanged: {
            parent.width=contentWidth;
        }
        onContentHeightChanged: {
            parent.height=contentHeight;
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
        onDoubleClicked: {
            idText.focus=true
            idText.cursorPosition = 1
        }
    }

    Rectangle {
        id: handle;
        color: "#3AC2D7";
        width: 9
        height: width;
        //radius: (width / 2);
        antialiasing: true;
        visible: selected
        border {
            width: 1
            color: "#FFFFFF"
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
                var point =  mapToItem (container, mouse.x, mouse.y);
                var diffX = (point.x - centerX);
                var diffY = -1 * (point.y - centerY);
                var rad = Math.atan (diffY / diffX);
                var deg = (rad * 180 / Math.PI);
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
        color: "#999999"
        width: 1
        height: 100-handle.height
        visible: selected
        antialiasing: true
    }
    Rectangle {
        //左上角
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: "#999999"
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
//                objectChanged(rectRoi, oldX, oldY, oldWidth, oldHeight, oldRotation, rectRoi.x, rectRoi.y, rectRoi.width, rectRoi.height, rectRoi.rotation)
            }
            onMouseXChanged: {
                if(drag.active){
                    if(mouseX>0 && idText.font.pointSize <= 20)return
                    var newWidth = rectRoi.width - mouseX
                    if (newWidth < 45)
                        return
                    if(mouseX>0)
                        idText.font.pointSize-=1
                    else
                        idText.font.pointSize+=1
                    //左上角动，右边的位置不动
                    if(oldRotation ===0)
                    {
                        rectRoi.x =  (oldWidth - rectRoi.width) + oldX
                        rectRoi.y = (oldHeight - rectRoi.height) + oldY
                    }

                }
            }

        }
    }

    Rectangle {
        //左下角
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: "#999999"
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
                     if(mouseX>0 && idText.font.pointSize <= 20)return;
                    var newWidth = rectRoi.width - mouseX
                    if (newWidth < 45)
                        return

                    if(mouseX>0)
                        idText.font.pointSize-=1
                    else
                        idText.font.pointSize+=1
                    //左上角动，右边的位置不动
                    if(oldRotation ===0)
                        rectRoi.x = oldWidth - rectRoi.width + oldX
                }
            }

        }
    }

    Rectangle {
        //右下角
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: "#999999"
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
                    if(mouseX<0 && idText.font.pointSize <= 20)return;
                    var newWidth = rectRoi.width + mouseX
                    if (newWidth < 45)
                        return
                    if(mouseX>0)
                        idText.font.pointSize+=1
                    else
                        idText.font.pointSize-=1
                }
            }

        }
    }

    Rectangle {
        //右上角
        width: rulersSize
        height: rulersSize
        color: "transparent"
        border {
            width: 1
            color: "#999999"
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
                     if(mouseX<0 && idText.font.pointSize <= 20)return;
                    var newWidth = rectRoi.width + mouseX
                    if (newWidth < 45)
                        return
                    if(mouseX>0)
                        idText.font.pointSize+=1
                    else
                        idText.font.pointSize-=1

                    //右上角，X 不动Y 动
                    if(oldRotation ===0)
                        rectRoi.y = oldHeight - rectRoi.height + oldY
                }
            }

        }
    }
}
