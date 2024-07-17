import QtQuick 2.0
import QtQuick.Controls 2.12
//import QtQuick.Layouts 1.3
import QtQuick.Window 2.0
import QtQml 2.3
import QtGraphicalEffects 1.12
import "../qml"
import "../secondqml"

CusRoundedBg {
    id: eo_askDialog
    property alias cloader: contentLoader
    property real shadowWidth: 0
    property real titleHeight: 30* screenScaleFactor
    property string title: ""
    property Component bdContentItem
    property alias titleTextColor: closeBtn.fontColor
    property alias closeBtnVis: closeBtn.closeBtnVis
    property alias titleColor: closeBtn.color

    borderColor: Constants.dialogBorderColor

    width: 300
    height: 200
    radius: 5
    clip: true
    color: Constants.lpw_titleColor
    signal closed()

    Rectangle{
        id: bgRec
        anchors.fill: parent
        anchors.margins: shadowWidth
        color: parent.color
        border.width: 0
        clip: true
        radius: eo_askDialog.radius
        //标题栏
        CusPopViewTitle {
            id:closeBtn
            width: parent.width
            height: titleHeight
            clip: true
            title: eo_askDialog.title
            color: Constants.lpw_titleColor
            fontColor: Constants.themeTextColor
            borderColor : "transparent"
            closeBtnNColor:  "transparent"//color
            closeBtnWidth: 8
            closeBtnHeight: 8
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            radius: eo_askDialog.radius
            leftTop: true
            rightTop: true
            clickedable: false
            maxBtnVis: false
            closeBtnVis: false
            onCloseClicked:{
                closed()
                eo_askDialog.visible = false
            }
        }

//        MouseArea{//用来捕获鼠标事件，使点击在框内不影响模型的选中状态
//            anchors.top: closeBtn.bottom
//            anchors.horizontalCenter: closeBtn.horizontalCenter
//            width: parent.width
//            height: parent.height - closeBtn.height
            Loader{
                id: contentLoader
                anchors.top: closeBtn.bottom
                anchors.horizontalCenter: closeBtn.horizontalCenter
                width: parent.width
                height: parent.height - closeBtn.height
                sourceComponent: bdContentItem
            }
        }
//    }

    //    DropShadow {
    //        anchors.fill: bgRec
    //        radius: 8
    //        spread: 0.2
    //        samples: 17
    //        source: bgRec
    //        color:Constants.dropShadowColor
    //    }
}
