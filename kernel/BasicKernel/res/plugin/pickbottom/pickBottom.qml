import QtQuick 2.12
import QtQuick.Controls 2.5

import CrealityUI 1.0
import "qrc:/CrealityUI"

Item
{
    width: 48 * screenScaleFactor
    height: 48 * screenScaleFactor

    property var com: ""
	
    CusImglButton {
        id: openFileBtn
        width: 48 * screenScaleFactor
        height: 48 * screenScaleFactor
        imgWidth: 24 * screenScaleFactor
        imgHeight: 24 * screenScaleFactor

        enabledIconSource: Constants.leftbar_pick_btn_icon_default
        pressedIconSource: Constants.leftbar_pick_btn_icon_pressed
        highLightIconSource: Constants.leftbar_pick_btn_icon_hovered

        text: qsTr("AUTO")
        state: "imgBottom"
        borderWidth: 1 * screenScaleFactor
        borderColor: Constants.leftbar_btn_border_color
        shadowEnabled: Constants.currentTheme

        onClicked: if(com) com.maxFaceBottom()
    }
}
