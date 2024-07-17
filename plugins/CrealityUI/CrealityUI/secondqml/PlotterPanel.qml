import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5

import "../qml"

Rectangle {
    id: plotterPanelRect

    property var control: null
    property alias settingsModel: idSliceSettings.settingsModel
    property var selShape: null

    Layout.fillWidth: true

    objectName: "objPlotterPanel"
    color: "transparent"

    signal showInfopanle(var bShow, var imageName, var imageW, var imageH)
    signal delObjects()

    onSelShapeChanged: {
        idPlotterImageItem.visible = false

        if (selShape == null) {
            return
        }

        if (selShape.dragType === "image") {
            idPlotterImageItem.visible = true
            showInfopanle(true, selShape.imageName, selShape.imageW, selShape.imageH)
        } else {
            showInfopanle(false, 0,0,0)
        }
    }

    function initImageSettings() {
        laserImage.initState()
        idLaserShapeSettings.initImagePos()
    }

    function updateImageInfo() {
        if (selShape !== null && selShape.dragType === "image") {
            laserImage.visible = true
            showInfopanle(true, selShape.imageName, selShape.imageW, selShape.imageH)
        } else {
            showInfopanle(false, 0,0,0)
        }
    }

    Column {
        spacing: 10 * screenScaleFactor
        anchors.fill: parent

        PanelTitle {
            id: configTitle
            height: 60* screenScaleFactor
            image: "qrc:/UI/photo/paramPanel.png"
            title: qsTr("Configure")
            // topLineEnabled: true
        }

        BasicScrollView {
            x : 3 * screenScaleFactor
            width: parent.width - x * 2
            height: parent.height - configTitle.height - parent.spacing
            clip : true

            hPolicy: ScrollBar.AlwaysOff
            vPolicy: ScrollBar.AsNeeded
            Column {
                width: parent.width
                spacing: 10 * screenScaleFactor
                LaserFoldItem {
                    id: idLaserShapeItem
                    title: qsTr("Parameter")
                    width: parent.width
                    accordionContent: Item {
                        width: parent.width
                        height: 210 * screenScaleFactor

                        LaserShapeSettings {
                            id: idLaserShapeSettings

                            onObjectChanged: {
                                control.objectChanged(obj,
                                                      oldX, oldY, oldWidth, oldHeight, oldRotation,
                                                      newX, newY, newWidth, newHeight, newRotation)
                            }
                        }
                    }
                }

                LaserFoldItem {
                    id: idPlotterSlice
                    title: qsTr("Working Parameters")
                    width: parent.width
                    accordionContent: Item {
                        width: parent.width
                        height: 200 * screenScaleFactor

                        PlotterSliceSettings {
                            id:idSliceSettings

                            x: 18 * screenScaleFactor
                            height: 270 * screenScaleFactor
                            width: parent.width

                            onGenPlotterGcode: {
                                control.genLaserGcode()
                            }
                        }
                    }
                }

                LaserFoldItem {
                    id: idPlotterImageItem
                    visible:false
                    title: qsTr("Processing Mode")
                    width: parent.width
                    accordionContent: Item {
                        width: parent.width
                        height: 200 * screenScaleFactor
                        PlotterImageSettings {
                            id: laserImage

                            anchors.rightMargin: 0
                            anchors.bottomMargin: 0
                            anchors.leftMargin: 18
                            anchors.topMargin: 0
                            anchors.fill: parent

                            onOriginalImageShow: {
                                control.imageOriginalShowChanged(selShape,value)
                            }

                            onReverseImage: {
                                control.imageReverseChanged(selShape,value)
                            }

                            onFlipModelValueChanged: {
                                control.imageFlipModelValueChanged(selShape,value)
                            }

                            onThresholdChanged: function(value) {
                                if(selShape) {
                                    selShape.threshold=value
                                    control.imageThresholdChanged(selShape,value)
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: false
            width: 278 * screenScaleFactor
            height: 1
            color: Constants.right_panel_menu_split_line_color
        }

        CusButton {
            id: genLaserBTn
            x: (parent.width - width) / 2
            width: 258 * screenScaleFactor
            height: 48 * screenScaleFactor
            txtContent: qsTr("Generate GCode")
            txtColor: Constants.right_panel_slice_text_default_color
            visible: false
            normalColor: enabled ? Constants.right_panel_slice_button_default_color
                                 : Constants.right_panel_slice_button_disable_color
            hoveredColor: Constants.right_panel_slice_button_hovered_color
            pressedColor: Constants.right_panel_slice_button_checked_color

            normalBdColor: enabled ? Constants.right_panel_border_default_color
                                   : Constants.right_panel_border_disable_color
            hoveredBdColor: Constants.right_panel_border_hovered_color
            pressedBdColor: Constants.right_panel_border_checked_color

            radius: 5
            shadowEnabled: false
            enabled: true
            opacity : enabled ? 1 : 0.5
            txtPointSize: Constants.labelFontPointSize_12
            txtFamily: Constants.panelFontFamily

            onClicked: {
                if(checkObjState()) {
                    idOutPlatform.visible = true
                } else {
                    control.genLaserGcode()
                }
            }
        }
    }
}
