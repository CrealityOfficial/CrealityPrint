import CrealityUI 1.0
import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import "qrc:/CrealityUI"

Item {
    property var com: ""

    width: 102 * screenScaleFactor
    height: 48 * screenScaleFactor

    RowLayout {
        anchors.fill: parent
        spacing: 6

        CusImglButton {
            id: autoBtn

            Layout.preferredWidth: 48 * screenScaleFactor
            width: 48 * screenScaleFactor
            height: 48 * screenScaleFactor
            imgWidth: 28 * screenScaleFactor
            imgHeight: 33 * screenScaleFactor
            enabledIconSource: Constants.currentTheme ?  "qrc:/UI/photo/cToolBar/faceFlatAuto_light_default.svg" : "qrc:/UI/photo/cToolBar/faceFlatAuto_dark_default.svg"
            pressedIconSource: enabledIconSource
            highLightIconSource:enabledIconSource
            defaultBtnBgColor: "transparent"
            hoverBorderColor: Constants.themeGreenColor
            hoveredBtnBgColor: "transparent"
            state: "imgOnly"
            borderWidth: 1 * screenScaleFactor
            borderColor: Constants.leftbar_btn_border_color
            shadowEnabled: Constants.currentTheme
            onClicked: {
                if (com) {
                    com.autoPickBottom();
                }
            }
        }

    }

}
