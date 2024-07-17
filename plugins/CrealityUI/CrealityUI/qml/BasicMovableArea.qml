import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5

Item{
    property Item rec:new Item()
    property string pointColor: "transparent"
    property real minimumWidth: 10
    property real minimumHeight: 10

    anchors.fill: parent
    focus: true
    MouseArea{
        property real pressX: 0
        property real pressY: 0

        anchors.fill: parent
        onPressedChanged: {
            if(pressed){
                pressX = mouseX
                pressY = mouseY
            }
            else{
                pressX = 0
                pressY = 0
            }
        }

        onMouseXChanged: {
            parent.x = mouseX - pressX
            parent.y = mouseY - pressY
        }
    }

    Rectangle{
        id:rightRec
        color:"transparent"
        width: 1
        height: parent.height/2
        border.width: 1
        border.color: pointColor
        anchors{
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        MouseArea{
            property real previousX: 0
            property real previousY: 0
            anchors.centerIn: parent
            width: parent.width+10
            height:parent.height+10
            cursorShape:Qt.SplitHCursor

            onPressedChanged: {
                if(pressed)
                {
                    previousX = mouseX
                    previousY = mouseY
                }
            }

            onMouseXChanged: {
                 if((rec.width < rec.maxWidth && mouse.x > previousX) || (rec.width > rec.minWidth && mouse.x < previousX)){
                    rec.width += mouse.x - previousX
                }
            }
        }
    }
}
