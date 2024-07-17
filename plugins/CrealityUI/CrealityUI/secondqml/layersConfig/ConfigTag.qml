import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "../../qml"
import "../../secondqml"
import "../../components"

Rectangle {
    id: root
    width: 35*screenScaleFactor
    height: 20*screenScaleFactor
    visible: true
    color: "transparent"

    signal clicked()

    property var value
    property var type // "pause" "color" "gcode"
    property var tips

    onTypeChanged: {
        if (type == "pause") {  
            defaultIcon = "qrc:/UI/photo/slider/pause_default.svg"
            hoveredIcon = "qrc:/UI/photo/slider/pause_hover.svg"
            tips = qsTr("Pause")
        } else if (type == "color") {
            defaultIcon = "qrc:/UI/photo/slider/color_default.svg"
            hoveredIcon = "qrc:/UI/photo/slider/color_hover.svg"
            tips = qsTr("Change Filament")
        } else if (type == "gcode") {
            defaultIcon = "qrc:/UI/photo/slider/gcode_default.svg"
            hoveredIcon = "qrc:/UI/photo/slider/gcode_hover.svg"
        } 
    }

    property var defaultIcon
    property var hoveredIcon
    property bool hovered: false

    Image {
        anchors.fill: parent

        fillMode: Image.PreserveAspectFit

        source: root.hovered ? root.hoveredIcon : root.defaultIcon
    }

    MouseArea {
        width: parent.width / 2
        height: parent.height
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            root.hovered = true
        }

        onExited: {
            root.hovered = false
        }

        onClicked: {
            root.clicked()
        }
    }
    
    BasicTooltip {
        visible: text !== "" && root.hovered
        text: root.tips
        textWrap: false
    }

}
