import QtQuick 2.15
import QtQuick.Controls 2.15

//import QtQuick.Layouts 1.3
import QtQuick.Window 2.15
import QtGraphicalEffects 1.0
import QtQml 2.15
import "../qml"

Popup {
    id: eo_askDialog
    property real shadowWidth: 0
    property string title: "baisc dialog"
    property var titleIcon: ""//"qrc:/scence3d/res/logo.png"
    property var closeIcon: "qrc:/UI/photo/closeBtn.svg"
    property var closeIcon_d: "qrc:/UI/photo/closeBtn_d.svg"
    property string titleBackground
    property alias cloader: contentLoader
    property color titleColor: "black"
    property var titleHeight: 30* screenScaleFactor
    property Component bdContentItem
    property alias maxBtnVis: titleCom.maxBtnVis
    modal: true
    closePolicy:Popup.NoAutoClose
    //parent:standaloneWindow
    x: Math.round((standaloneWindow.width - width) / 2) - parent.mapToItem(null,0,0).x
    y: Math.round((standaloneWindow.height - height) / 2) - parent.mapToItem(null,0,0).y
    //flags: Qt.FramelessWindowHint | Qt.Window | Qt.WindowMinimizeButtonHint
    property var centerToWindow:true
    //modality: Qt.ApplicationModal
    //color: "transparent"
    signal closing()
    signal afterClose()
    background:Rectangle{
        anchors.fill: parent
        color: "transparent"
    }
    function show() {
        open()
    }
    Rectangle{
        id: bgRec
        anchors.fill: parent
        anchors.margins: shadowWidth
        color: Constants.themeColor

        radius: 5* screenScaleFactor
        //标题栏
        CusPopViewTitle{
            id: titleCom
            color:  Constants.dialogTitleColor
            width: parent.width
            height: titleHeight
            leftTop: true
            rightTop: true
            borderWidth: 0
//            borderColor : Constants.dialogBorderColor
            fontColor: Constants.dialogTitleTextColor
            closeBtnNColor :  Constants.dialogTitleColor
            shadowEnabled : false
            clickedable: false
            title: eo_askDialog.title
            closeIcon: Constants.currentTheme ? "qrc:/UI/photo/menuImg/close_light.svg" : "qrc:/UI/photo/menuImg/close.svg"
            closeIcon_d: Constants.currentTheme ? "qrc:/UI/photo/menuImg/close_light.svg" : "qrc:/UI/photo/menuImg/close.svg"
            moveItem: eo_askDialog
            onCloseClicked:{
                closing()
                close()
            }
        }

        Loader{
            id: contentLoader
            anchors.top: titleCom.bottom
            anchors.horizontalCenter: titleCom.horizontalCenter
            width: parent.width
            height: parent.height - titleHeight - bgRec.radius/2
            sourceComponent: bdContentItem

        }
    }

    DropShadow {
        anchors.fill: bgRec
        radius: 5
        spread: 0
        samples: 11
        source: bgRec
        color:  Constants.dropShadowColor
        verticalOffset:5
        horizontalOffset: 0
    }
}
