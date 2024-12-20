import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.5

import ".."
import "../qml"

Rectangle {
    width: 225 * screenScaleFactor
    height: 285 * screenScaleFactor
    radius: 10 * screenScaleFactor

    border.color: "#DDDDE1"
    border.width: Constants.currentTheme ? 1 : 0

//    gradient: Gradient {
//        GradientStop { position: 0.0; color: Constants.currentTheme ? upper_background_light : upper_background_dark }
//        GradientStop { position: 1.0; color: lower_background_color }
//    }

    property var baseId: ""
    property var baseName: ""
    property var baseImage: ""
    property var modelcount: ""
    property var downloadlink:""

    property int userType: -1
    property int baseType: -1 //0: mySlice 1: myModel
    property int buttonCount: printVisible + shareVisible + deleteVisible + importBtnVisible

    property bool printVisible: true
    property bool shareVisible: true
    property bool deleteVisible: true
    property bool importBtnVisible: true
    property bool isPreserveAspectCrop: false

    property color label_normal_color: "#333333"
    property color upper_background_dark: "#DDDDE1"
    property color upper_background_light: "#F2F2F6"
    property color lower_background_color: "#FFFFFF"

    signal sigPrintGcode(var id)
    signal sigShareModel(var id, var target)
    signal deleteCurrentItem(var id)

    signal importModelItem(var id, var count)
    signal importGcodeItem(var id, var name, var downloadlink)

    function setAnimatedImageStatus(value) {
        idImportBtn.visible = !value
        idAnimatedImage.visible = value
    }

    Rectangle {
        width: parent.width - 2 * parent.border.width
        height: 240 * screenScaleFactor

        anchors.top: parent.top
        anchors.topMargin: parent.border.width
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 10*screenScaleFactor
        clip: true
        Image {
            cache: false
            asynchronous: true
            anchors.fill: parent
            fillMode: isPreserveAspectCrop ? Image.PreserveAspectCrop : Image.PreserveAspectFit

            source: baseImage
            sourceSize: Qt.size(width, height)
        }
    }

    Rectangle {
        width: parent.width - 2 * parent.border.width
        height: 45 * screenScaleFactor

        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.border.width
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 10*screenScaleFactor
        color:lower_background_color
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            spacing: 7 * screenScaleFactor

            Label {
                width: (205 - buttonCount * 35) * screenScaleFactor
                height: 28 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter

                elide: Text.ElideRight
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                color: label_normal_color
                text: baseName

                MouseArea {
                    hoverEnabled: true
                    anchors.fill: parent
                    enabled: parent.truncated
                    onContainsMouseChanged: containsMouse ? tooltip.showTooltip(baseName, parent.x + parent.width / 2, parent.y) : tooltip.close()
                }
            }

            ToolTip {
                id: tooltip
                height: 28 * screenScaleFactor
                topPadding: 8 * screenScaleFactor
                leftPadding: 10 * screenScaleFactor
                rightPadding: 10 * screenScaleFactor
                bottomPadding: 8 * screenScaleFactor

                function showTooltip(text, xPos, yPos) {
                    labelTooltip.text = text
                    x = xPos - width  / 2
                    y = yPos - height - 2
                    open()
                }

                background: Rectangle {
                    color: "#FFFFFF"
                    border.color: Constants.currentTheme ? "#DDDDE1" : "#626262"
                    border.width: 1
                    radius: 5
                }

                contentItem: Label {
                    id: labelTooltip
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: label_normal_color
                }
            }

            //Mask effect
            AnimatedImage {
                id: idAnimatedImage
                visible: false
                width: 18 * screenScaleFactor
                height: 18 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter

                opacity: 0.5
                source: "qrc:/UI/photo/loading_g.gif"
            }

            Button {
                id: idImportBtn
                width: 28 * screenScaleFactor
                height: 28 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                property string tipLabel: printVisible ? qsTr("Import Gcode") : qsTr("Download Models")

                onClicked:
                {
                    //setAnimatedImageStatus(true)
                    baseType ? importModelItem(baseId, baseName) : importGcodeItem(baseId, baseName, downloadlink)
                }

                onHoveredChanged: hovered ? tooltip.showTooltip(tipLabel, x + width / 2, y) : tooltip.close()

                background: Rectangle {
                    color: parent.hovered ? "#1E9BE2" : "#E2F5FF"
                    radius: height / 2
                }

                Image {
                    anchors.centerIn: parent
                    source: parent.hovered ? "qrc:/UI/photo/userInfo_import_hover.png" : "qrc:/UI/photo/userInfo_import_normal.png"
                }
            }

            Button {
                id: idShareBtn
                visible: shareVisible
                width: 28 * screenScaleFactor
                height: 28 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                property string tipLabel: qsTr("Share The Model Group(Copy Link)")

                onClicked: {
                    sigShareModel(baseId, idShareBtn)
                }

                onHoveredChanged: hovered ? tooltip.showTooltip(tipLabel, x + width / 2, y) : tooltip.close()

                background: Rectangle {
                    color: parent.hovered ? "#1E9BE2" : "#E2F5FF"
                    radius: height / 2
                }

                Image {
                    anchors.centerIn: parent
                    source: parent.hovered ? "qrc:/UI/photo/userInfo_share_hover.png" : "qrc:/UI/photo/userInfo_share_normal.png"
                }
            }

            Button {
                id: idPrintBtn
                visible: printVisible
                width: 28 * screenScaleFactor
                height: 28 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                property string tipLabel: qsTr("Print Gcode")

                onClicked: sigPrintGcode(baseId)

                onHoveredChanged: hovered ? tooltip.showTooltip(tipLabel, x + width / 2, y) : tooltip.close()

                background: Rectangle {
                    color: parent.hovered ? "#1E9BE2" : "#E2F5FF"
                    radius: height / 2
                }

                contentItem: Image {
                    anchors.centerIn: parent
                    source: parent.hovered ? "qrc:/UI/photo/userInfo_print_hover.png" : "qrc:/UI/photo/userInfo_print_normal.png"
                }
            }

            Button {
                id: idDeleteBtn
                visible: deleteVisible
                width: 28 * screenScaleFactor
                height: 28 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                property string tipLabel: (userType != 3) ? (printVisible ? qsTr("Delete Gcode") : qsTr("Delete Model")) : qsTr("UnCollect")

                onClicked: deleteCurrentItem(baseId)

                onHoveredChanged: hovered ? tooltip.showTooltip(tipLabel, x + width / 2, y) : tooltip.close()

                background: Rectangle {
                    color: parent.hovered ? "#1E9BE2" : "#E2F5FF"
                    radius: height / 2
                }

                Image {
                    anchors.centerIn: parent
                    source: parent.hovered ? "qrc:/UI/photo/userInfo_delete_hover.png" : "qrc:/UI/photo/userInfo_delete_normal.png"
                }
            }
        }
    }
}
