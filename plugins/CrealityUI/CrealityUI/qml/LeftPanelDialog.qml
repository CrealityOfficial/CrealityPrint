import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.Controls 2.12
import QtQml 2.3
import QtGraphicalEffects 1.0
import "./"
import "../secondqml"

Rectangle {
    id: root

    property string title: ""
    property alias titleHeight: title.height
    property alias panelArea: panel_area
    property alias closeButtonVisible: close_button.visible
    signal closeButtonClicked()

    radius: 5 * screenScaleFactor
    color: Constants.currentTheme ? "#D6D6DC": "#6E6E73"   //Constants.lpw_titleColor
    border.width: 1 * screenScaleFactor
    border.color: color

    MouseArea {
        id: wheelFilter
        anchors.fill: parent
        propagateComposedEvents: false
        onWheel: {}
    }

    Rectangle {
        id: title

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: 24 * screenScaleFactor

        color: "transparent"

        Text {
            id: title_text

            anchors.verticalCenter: title.verticalCenter
            anchors.left: title.left
            anchors.leftMargin: 10 * screenScaleFactor

            text: root.title
            color: Constants.dialogTitleTextColor
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_9
        }

        Button {
            id: close_button

            anchors.top: title.top
            anchors.right: title.right
            anchors.bottom: title.bottom

            width: height

            visible: false

            background: CusRoundedBg {
                anchors.fill: parent
                anchors.topMargin: root.border.width
                anchors.rightMargin: root.border.width
                radius: 5 * screenScaleFactor
                rightTop: true
                color: close_button.hovered ? Constants.left_model_list_close_button_hovered_color : "transparent"
            }

            Image {
                anchors.centerIn: parent
                width: 10 * screenScaleFactor
                height: 10 * screenScaleFactor
                source: close_button.hovered ? "qrc:/UI/photo/closeBtn_d.svg" : "qrc:/UI/photo/closeBtn.svg"
            }

            onClicked: {
                root.closeButtonClicked()
            }
        }
    }

    Rectangle {
        id: split_line
        anchors.top: title.bottom
        anchors.left: root.left
        anchors.right: root.right
        height: root.border.width
        color: Constants.currentTheme == 0 ? "#56565C" : "#DDDDDD"
    }

    CusRoundedBg{
        id: panel_top

        anchors.top: split_line.bottom
        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: root.bottom
        color: Constants.themeColor

        rightBottom: true
        leftBottom: true
    }

    Item {
        id: panel_area

        anchors.top: panel_top.top
        anchors.left: panel_top.left
        anchors.right: panel_top.right
        anchors.bottom: parent.bottom
    }

    DropShadow {
        anchors.fill: panel_top
        radius: 8
        spread: 0.2
        samples: 17
        source: panel_top
        verticalOffset:5
        horizontalOffset: 0
        color:  Constants.dropShadowColor
    }
}
