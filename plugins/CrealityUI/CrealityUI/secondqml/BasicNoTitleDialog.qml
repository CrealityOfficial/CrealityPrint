import QtQuick 2.0
import QtQuick.Controls 2.12
//import QtQuick.Layouts 1.3
import QtQuick.Window 2.0
import QtGraphicalEffects 1.12
import ".."
import "../qml"
Window {
    id: eo_askDialog
    width: 300
    height: 200
    property string contentBackground: Constants.dialogContentBgColor

    flags: Qt.FramelessWindowHint | Qt.Dialog
   // A model window prevents other windows from receiving input events. Possible values are Qt.NonModal (the default), Qt.WindowModal, and Qt.ApplicationModal.
    modality: Qt.ApplicationModal
    color:"transparent" //Constants.themeColor

    Rectangle
    {
    	id: mainLayout
        // 一个填满窗口的容器，Page、Rectangle 都可以
        anchors.fill: parent
		// 当窗口全屏时，设置边距为 0，则不显示阴影，窗口化时设置边距为 10 就可以看到阴影了
        radius: 5
		anchors.margins: 5
		color: contentBackground//Constants.themeColor //"transparent"
        opacity: 1
        //border.color: Constants.dialogItemRectBgBorderColor//"#061F3B"
        //border.width: 1
    }

    DropShadow {
        anchors.fill: mainLayout
        radius: 8
        samples: 17
        source: mainLayout
        color: Constants.dropShadowColor
    }
	
}
