import "../qml"
import QtGraphicalEffects 1.12
import QtQml 2.3
import QtQuick 2.0
import QtQuick.Controls 2.12
//import QtQuick.Layouts 1.3
import QtQuick.Window 2.0

Popup {
    id: eo_askDialog

    property real shadowWidth: 0
    property string title: "basic dialog"
    property var titleIcon: "" //"qrc:/scence3d/res/logo.png"
    property var closeIcon: "qrc:/UI/photo/closeBtn.svg"
    property var closeIcon_d: "qrc:/UI/photo/closeBtn_d.svg"
    property string titleBackground
    property alias cloader: contentLoader
    property color titleColor: "black"
    property var titleHeight: 30 * screenScaleFactor
    property Component bdContentItem
    property alias maxBtnVis: titleCom.maxBtnVis
    //flags: Qt.FramelessWindowHint | Qt.Window | Qt.WindowMinimizeButtonHint
    property var centerToWindow: true

    //modality: Qt.ApplicationModal
    //color: "transparent"
    signal closing()
    signal afterClose()

    function show() {
        if (centerToWindow) {
            x = Math.round((standaloneWindow.width - width) / 2) - parent.mapToItem(null, 0, 0).x;
            y = Math.round((standaloneWindow.height - height) / 2) - parent.mapToItem(null, 0, 0).y;
        }
        open();
    }

    modal: true
    closePolicy: Popup.NoAutoClose
    width: 300 * screenScaleFactor
    height: 200 * screenScaleFactor
    //parent:standaloneWindow
    x: Math.round((standaloneWindow.width - width) / 2) - parent.mapToItem(null, 0, 0).x
    y: Math.round((standaloneWindow.height - height) / 2) - parent.mapToItem(null, 0, 0).y

    Rectangle {
        id: bgRec

        anchors.fill: parent
        anchors.margins: shadowWidth
        color: Constants.themeColor
        radius: 5 * screenScaleFactor

        //标题栏
        CusPopViewTitle {
            id: titleCom

            color: Constants.dialogTitleColor
            width: parent.width
            height: titleHeight
            leftTop: true
            rightTop: true
            borderColor: Constants.dialogBorderColor
            fontColor: Constants.dialogTitleTextColor
            closeBtnNColor: Constants.dialogTitleColor
            shadowEnabled: true
            clickedable: false
            title: eo_askDialog.title
            closeIcon: eo_askDialog.closeIcon
            closeIcon_d: eo_askDialog.closeIcon_d
            moveItem: eo_askDialog
            onCloseClicked: {
                closing();
                close();
            }
        }

        Loader {
            id: contentLoader

            anchors.top: titleCom.bottom
            anchors.horizontalCenter: titleCom.horizontalCenter
            width: parent.width
            height: parent.height - titleHeight - bgRec.radius / 2
            sourceComponent: bdContentItem
        }

    }

    DropShadow {
        anchors.fill: bgRec
        radius: 8
        spread: 0.2
        samples: 17
        source: bgRec
        color: Constants.dropShadowColor
    }

    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

}
