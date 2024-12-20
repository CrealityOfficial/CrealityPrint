import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5

import "../qml"

Rectangle {
    id: laserPanelRect

    property var control: null
    property alias settingsModel: idSliceParameters.settingsModel
    property var selShape: null

    Layout.fillWidth: true

    objectName: "objLaserPanel"
    color: "transparent"

    signal showInfopanle(var bShow, var imageName, var imageW, var imageH)
    signal delObjects()

    onSelShapeChanged: {
      idImageSettings.visible = false
      idFontSettings.visible = false

      if (selShape == null) {
        return
      }

      if (selShape.dragType === "image") {
          idImageSettings.visible = true
          showInfopanle(true, selShape.imageName, selShape.imageW, selShape.imageH)
      } else if (selShape.dragType === "text") {
          idFontSettings.visible = true
          idfontSetting.fontText = selShape.txt.text
          showInfopanle(false, 0,0,0)
      } else {
          showInfopanle(false, 0,0,0)
      }
    }

    function initImageSettings() {
        laserImage.initState()
        idLaserShapeSettings.initImagePos()
    }

    function updateImageInfo() {
        if (selShape != null && selShape.dragType === "image") {
            idImageSettings.visible = true
            showInfopanle(true, selShape.imageName, selShape.imageW, selShape.imageH)
        } else {
            idImageSettings.visible = false
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

                        LaserShapeSettings{
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
                    width: parent.width
                    title: qsTr("Working Parameters")

                    accordionContent: Item {
                        id: idSliceParameters

                        property var settingsModel

                        width: parent.width
                        height: 120 * screenScaleFactor

                        Column {
                            spacing: 5

                            Rectangle {
                                width: parent.width
                                height: 10 * screenScaleFactor
                                color: "transparent"
                            }

                            Row {
                                spacing:5
                                //visible: false

                                Rectangle{
                                    width: 13 * screenScaleFactor
                                    height: parent.height
                                    color: "transparent"
                                }

                                Label {
                                    width: 110 * screenScaleFactor
                                    height: 28 * screenScaleFactor
                                    font.pointSize: Constants.labelFontPointSize_9
                                    font.family: Constants.labelFontFamily
                                    text: qsTr("Run Count")
                                    color: Constants.textColor
                                    verticalAlignment: Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft
                                }

                                BasicDialogTextField {
                                    width: 120 * screenScaleFactor
                                    height: 28 * screenScaleFactor
                                    horizontalAlignment: Text.AlignLeft
                                    text: settingsModel.laserTotalNum
                                    onTextChanged: {
                                        settingsModel.laserTotalNum = parseInt(text);
                                    }
                                }
                            }

                            Row {
                                spacing:5

                                Rectangle {
                                    width: 13* screenScaleFactor
                                    height: parent.height
                                    color: "transparent"
                                }

                                Label {
                                    width: 110* screenScaleFactor
                                    height: 28* screenScaleFactor
                                    verticalAlignment: Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft

                                    font.pointSize:Constants.labelFontPointSize_9
                                    font.family: Constants.labelFontFamily
                                    text:qsTr("Power Rate")
                                    color: Constants.textColor
                                }

                                BasicDialogTextField {
                                    width: 120 * screenScaleFactor
                                    height: 28 * screenScaleFactor
                                    horizontalAlignment: Text.AlignLeft

                                    unitChar:"%"
                                    text: settingsModel.laserPowerRate

                                    onTextChanged: {
                                        if(parseInt(text) > 100) {
                                            text = 100
                                        }
                                        settingsModel.laserPowerRate = parseInt(text);
                                    }
                                }
                            }

                            Row {
                                spacing:5
                                visible: false

                                Rectangle{
                                    width: 13 * screenScaleFactor
                                    height: parent.height
                                    color: "transparent"
                                }

                                Label {
                                    width: 110* screenScaleFactor
                                    height: 28* screenScaleFactor
                                    font.pointSize:Constants.labelFontPointSize_9
                                    font.family: Constants.labelFontFamily

                                    text: qsTr("Speed Rate")
                                    color: Constants.textColor
                                    verticalAlignment: Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft
                                }

                                BasicDialogTextField {
                                    width: 120 * screenScaleFactor
                                    height: 28 * screenScaleFactor
                                    horizontalAlignment: Text.AlignLeft

                                    unitChar: "%"
                                    text: settingsModel.laserSpeedRate
                                    onTextChanged: {
                                        settingsModel.laserSpeedRate = parseInt(text);
                                    }
                                }

                            }

                            Row {
                                spacing:5
                                visible: false

                                Rectangle {
                                    width: 13 * screenScaleFactor
                                    height: parent.height
                                    color: "transparent"
                                }

                                Label {
                                    width: 110* screenScaleFactor
                                    height: 28* screenScaleFactor
                                    font.pointSize: Constants.labelFontPointSize_9
                                    font.family: Constants.labelFontFamily

                                    text:qsTr("Travel Speed")
                                    color: Constants.textColor
                                    verticalAlignment: Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft
                                }

                                BasicDialogTextField {
                                    width: 120* screenScaleFactor
                                    height: 28* screenScaleFactor
                                    horizontalAlignment: Text.AlignLeft

                                    unitChar: "mm/min"
                                    text: settingsModel.laserJogSpeed
                                    onTextChanged: {
                                        settingsModel.laserJogSpeed = parseInt(text);
                                    }
                                }
                            }

                            Row {
                                spacing:5
                                //visible: false

                                Rectangle {
                                    width: 13 * screenScaleFactor
                                    height: parent.height
                                    color: "transparent"
                                }

                                Label {
                                    width: 110* screenScaleFactor
                                    height: 28* screenScaleFactor
                                    verticalAlignment: Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft

                                    font.pointSize: Constants.labelFontPointSize_9
                                    font.family: Constants.labelFontFamily
                                    text:qsTr("Work Speed")
                                    color: Constants.textColor
                                }

                                BasicDialogTextField {
                                    width: 120* screenScaleFactor
                                    height: 28* screenScaleFactor
                                    horizontalAlignment: Text.AlignLeft

                                    unitChar:"mm/min"
                                    text: settingsModel.laserWorkSpeed

                                    onTextChanged: {
                                        if(parseInt(text) > 6000) {
                                            text = 6000
                                        }
                                        settingsModel.laserWorkSpeed = parseInt(text);
                                    }
                                }
                            }
                        }
                    }
                }

                LaserFoldItem {
                    id:idImageSettings
                    visible: false

                    title: qsTr("Processing Mode")
                    width: parent.width

                    accordionContent: Item {
                        width: parent.width
                        height: 300 * screenScaleFactor

                        LaserImageSettings {
                            id: laserImage

                            anchors.rightMargin: 0
                            anchors.bottomMargin: 0
                            anchors.leftMargin: 18
                            anchors.topMargin: 0
                            anchors.fill: parent

                            onVectorSelected: {
                                control.vectorImage(selShape)
                            }

                            onBlackSelected: {
                                control.blackImage(selShape)
                            }

                            onGraySelected: {
                                control.grayImage(selShape)
                            }

                            onDitheringImage: {
                                control.DitheringImage(selShape)
                            }

                            onThresholdChanged: function(value) {
                                if(selShape) {
                                    //selShape.threshold=value
                                    control.imageThresholdChanged(selShape,value)
                                }
                            }

                            onOriginalImageShow: {
                                control.imageOriginalShowChanged(selShape,value)
                            }

                            onM4modeChanged: {
                                control.m4modeChanged(selShape,value)
                            }

                            onReverseImage: {
                                control.imageReverseChanged(selShape,value)
                            }

                            onContrastValueChanged: {
                                control.imageContrastChanged(selShape,value)
                            }

                            onBrightnessValueChanged: {
                                control.imageBrightnessChanged(selShape,value)
                            }

                            onWhiteValueChanged: {
                                control.imageWhiteValueChanged(selShape,value)
                            }

                            onFlipModelValueChanged: {
                                control.imageFlipModelValueChanged(selShape,value)
                            }
                        }
                    }
                }

                LaserFoldItem {
                    id: idFontSettings
                    visible:false

                    title: qsTr("Word")
                    width: parent.width

                    accordionContent: Item {
                        width: parent.width
                        height: 130 * screenScaleFactor

                        LaserFontSettings {
                            id: idfontSetting

                            anchors.rightMargin: 0
                            anchors.bottomMargin: 0
                            anchors.leftMargin: 18
                            anchors.topMargin: 0
                            anchors.fill: parent

                            onFontFamilyChanged: {
                                selShape.txt.font.family = fontFamily
                                selShape.txt.font.styleName = fontStyle
                                selShape.txt.font.pointSize = fontSize
                            }

                            onFontTextChanged: {
                                selShape.txt.text = fontText
                            }

                            onFontSizeChanged: {
                                selShape.txt.font.family = fontFamily
                                selShape.txt.font.styleName = fontStyle
                                selShape.txt.font.pointSize = fontSize
                            }

                            onFontStyleChanged: {
                                selShape.txt.font.family = fontFamily
                                selShape.txt.font.styleName = fontStyle
                                selShape.txt.font.pointSize = fontSize
                            }
                        }
                    }
                }
            }
        }
    }
}
