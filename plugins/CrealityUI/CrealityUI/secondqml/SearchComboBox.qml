import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"

CustomComboBox {
    property alias searchText: text_field.text
    property alias placeholderText: text_field.placeholderText
    property bool indicatorVisible: true

    id: root

    editable: true
    radius: height / 2
    maximumVisibleCount: 15

    indicator: Image {
        visible: root.indicatorVisible
        width: 14 * screenScaleFactor
        height: width

        anchors.left: root.left
        anchors.leftMargin: 8 * screenScaleFactor
        anchors.verticalCenter: root.verticalCenter

        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter

        fillMode: Image.PreserveAspectFit
        source: Constants.currentTheme ? "qrc:/UI/photo/search_light.svg"
                                       : "qrc:/UI/photo/search_dark.svg"
    }

    contentItem: TextField {
        id: text_field

        anchors.top: root.background.top
        anchors.bottom: root.background.bottom
        anchors.left: root.indicatorVisible ? root.indicator.right : root.background.left
        anchors.right: root.background.right

        background: Item {}

        topPadding: root.textTopPadding
        bottomPadding: root.textBottomPadding
        leftPadding: root.textLeftPadding
        rightPadding: root.textRightPadding
        verticalAlignment: root.textVerticalAlignment
        horizontalAlignment: root.textHorizontalAlignment

        font: Constants.font
        color: Constants.right_panel_text_default_color

        selectByMouse: true
        selectionColor: Constants.themeColor_New
        selectedTextColor: "#000000"
        placeholderTextColor: Constants.manager_printer_button_text_color

        onReleased: event => {
            root.popup.visible = !root.popup.visible
        }
    }
}
