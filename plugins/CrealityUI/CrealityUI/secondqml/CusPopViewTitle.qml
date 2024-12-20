import QtQuick 2.12
import "../qml"

CusRoundedBg{
    id: crb
    property alias title: ct.fontText
    property alias fontColor: ct.fontColor
    property real closeBtnWidth: 8 * screenScaleFactor
    property real closeBtnHeight: 8 * screenScaleFactor
    property real titleLeftMargin: 8 * screenScaleFactor
    property color closeBtnNColor: "#56565c"
    property string closeIcon: Constants.currentTheme == 0 ? "qrc:/UI/photo/close_normal_dark.png"
                                                           : "qrc:/UI/photo/close_normal.png"
    property string closeIcon_d: "qrc:/UI/photo/close_press.png"
    property bool isMaxed: false
    property bool maxBtnVis: true
    property bool closeBtnVis: true
    property var moveItem
    signal closeClicked()
    signal normalClicked()
    signal maxClicked()
    clickedable: false
    radius: 5
    borderColor: "transparent"

    CusText{
        id: ct
        anchors.left: parent.left
        anchors.leftMargin: titleLeftMargin
        anchors.verticalCenter: parent.verticalCenter
        fontPointSize: Constants.labelFontPointSize_9
        fontColor: "#ffffff"
        hAlign: 0
    }

    MouseArea{
        id: mouseControler
        anchors.fill: parent
        property point clickPos: "0,0"

        onPressed: clickPos = Qt.point(mouse.x, mouse.y)

        onPositionChanged: {
           // var cursorPos = WizardUI.cursorGlobalPos()
           
           var x = moveItem.x - clickPos.x + mouse.x;
           var y = moveItem.y - clickPos.y + mouse.y;
           var centerx = moveItem.parent.width;
           var centery = moveItem.parent.height;
           if(moveItem.centerToWindow)
           {
                centerx = standaloneWindow.width - moveItem.parent.mapToItem(null,0,0).x;
                centery = standaloneWindow.height - moveItem.parent.mapToItem(null,0,0).y;
           }
           if((x+moveItem.width)<=centerx)
           {
                moveItem.x = x;
           
           }else{
                moveItem.x =  centerx - moveItem.width ;
           }
           if((y+moveItem.height)<=centery)
           {
                 moveItem.y = y;
           }else{
                moveItem.y = centery - moveItem.height ;
           }
            //moveItem.x = cursorPos.x - clickPos.x
           // moveItem.y = cursorPos.y - clickPos.y
        }
    }

    TitleBarBtn{
        id:maxBtn
        visible: maxBtnVis
        width: parent.height - 4
        height: parent.height - 4
        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.right: parent.right
        anchors.rightMargin: 35
        normalIconSource: crb.isMaxed ?"qrc:/UI/photo/normalBtn.svg" : "qrc:/UI/photo/maxBtn.svg" //"qrc:/UI/images/maximized.png"
        onClicked: {
            if(crb.isMaxed)
            {
                crb.isMaxed=false
                moveItem.showNormal()
                normalIconSource =  "qrc:/UI/photo/maxBtn.svg"
            }
            else{
                crb.isMaxed=true
                moveItem.showMaximized()
                normalIconSource =  "qrc:/UI/photo/normalBtn.svg"
            }
        }

    }

    CusButton{
        id:closeBtn
        visible: closeBtnVis
        width: parent.height -2
        height: parent.height -2
        rightTop: true
        allRadius: false
        radius: crb.radius
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        normalColor:  closeBtnNColor
        hoveredColor:  Constants.left_model_list_close_button_hovered_color
        pressedColor: Constants.left_model_list_close_button_hovered_color
        borderWidth: 0
        Image{
            anchors.centerIn: parent
            sourceSize.width: closeBtnWidth
            sourceSize.height: closeBtnHeight
            source: closeBtn.isHovered ? closeIcon_d : closeIcon
        }

        onClicked: {
            crb.closeClicked()
        }
    }

   Rectangle{
       width: parent.width
       height: 1
       border.width: 0
       color: Constants.currentTheme == 0 ? "#56565C" : "#DDDDDD"
       anchors.bottom: parent.bottom
       anchors.horizontalCenter: parent.horizontalCenter
   }
}
