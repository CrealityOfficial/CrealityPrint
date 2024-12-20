import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"
import "components"

LeftPanelDialog {
    id: idRoot
    width: 317 * screenScaleFactor
    // height: 380 * screenScaleFactor
    visible: !kernel_kernel.isDefaultVisScene
    height: (355 + idToolConfigPanel.hasConfig * 25) * screenScaleFactor + idToolConfigPanel.height 
    title: qsTr("Color")
    property var com
    z: 10

    onTitleChanged: {
        idBottom.updateCopywriting()
    }

    function execute() {
        // com.colorsList = idColorSeletor.colorsList
        idToolSeletor.method = com.colorMethod
        idColorSeletor.setSelectedColorIndex(com.colorIndex)
        // idCanvas.color = idColorSeletor.selectedColor 
        idSectionView.value = com.sectionRate

        idToolConfigPanel.update();
        idCanvas.update();
        idColorSeletor.update();
        idToolSeletor.update();
        idBottom.updateCopywriting()
    }

    Connections
    {
        target: com ? com : null

        onScreenRadiusChanged: {
            if (idCanvas.screenRadius != com.screenRadius) {
                idCanvas.screenRadius = com.screenRadius
                idCanvas.update();
            }
        }

        onSectionRateChanged: {
            idSectionView.value = com.sectionRate;
        }

        onColorIndexChanged: {
            idColorSeletor.setSelectedColorIndex(com.colorIndex)
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
    }

    ColorCanvas {
        id : idCanvas
        parent: gGlScene
        anchors.fill: parent
        cursorPos: com ? com.pos : { x: 0, y: 0}
        screenScale: com ? com.scale : 1
        visible: idRoot.visible && (com ? com.colorMethod == 1 : false)
    }

    Item {
        anchors.fill: parent.panelArea
        anchors.margins: 20

        ColorSeletor {
            id: idColorSeletor
            anchors.top: parent.top
            width: parent.width
            height: 100 * screenScaleFactor

            onSelectedColorChanged: {
                idCanvas.color = selectedColor 
            }

            onSelectedColorIndexChanged: {
                if (com && com.colorIndex != selectedColorIndex){
                    com.colorIndex = selectedColorIndex
                    // idCanvas.color = selectedColor 
                    // idCanvas.update()
                }
            }

            // onColorsListChanged: {
            //     console.log("colorsList changed")
            //     if (com)
            //         com.colorsList = colorsList
            // }

            onUpdateColorsList: {
                if (com && com.isRunning) {
                    console.log("qml color list set empty")
                    com.colorsList = []
                }
            }
        }

        ToolSeletor {
            id: idToolSeletor
            anchors.top: idColorSeletor.bottom
            width: parent.width
            height: 70 * screenScaleFactor

            onMethodChanged: {
                if (!com || com.colorMethod == method)
                    return

                com.colorMethod = method        
                idCanvas.update()
                idBottom.updateCopywriting()
            }
        }


        ToolConfigPanel {
            id: idToolConfigPanel
            anchors.top: idToolSeletor.bottom
            width: parent.width
            com: idRoot.com
            forceOpenSmartFill: false
            property var hasConfig: com && com.colorMethod != 0
        }

        SliderWithSpinBox {
            id: idSectionView
            anchors.top: idToolConfigPanel.bottom
            width: parent.width
            anchors.topMargin: idToolConfigPanel.hasConfig ?  25 * screenScaleFactor : 0
            height: 30 * screenScaleFactor
            title: qsTr("Section")

            from: 0
            to: 1
            value: 1
            stepSize: 0.05

            onValueChanged: {
                var rate = value
                com.sectionRate = rate
            }
            onSliderPressedChanged: {
                if (!sliderPressed) {
                    idRoot.focus = true
                }
            }
        }

        BasicButton {
            id : idResetDirection
            anchors.top: idSectionView.bottom
            anchors.left: idSectionView.left
            anchors.topMargin: 2
            height: idBottom.height
            width: (text.length > 5 ? 95 : 60) * screenScaleFactor
            text : qsTr("Reset Direction")
            btnRadius: height / 2
            btnBorderW:1
            pointSize: Constants.labelFontPointSize_9
            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.lpw_BtnColor
            hoveredBtnBgColor: Constants.lpw_BtnHoverColor

            onSigButtonClicked: {
                com.resetDirection();
            }
        }

        ColorPanelBottom {
            id: idBottom
            anchors.top: idSectionView.bottom
            width: parent.width
            // anchors.topMargin: 20
            anchors.topMargin: 56
            height: 28 * screenScaleFactor
            com: idRoot.com

            contextList: []

            function updateCopywriting() {
                let method = idToolSeletor.method
                if (method == 0) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Paint"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 1) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Paint"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Ctrl + Wheel") + ":",
                        qsTr("Pen size"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 4) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Paint"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Ctrl + Wheel") + ":",
                        qsTr("Pen size"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 2) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Paint"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Ctrl + Wheel") + ":",
                        qsTr("Smart Fill Angle"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 5) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Paint"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Ctrl + Wheel") + ":",
                        qsTr("Height Range"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 3) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Fill gap"),
                        qsTr("Ctrl + Wheel") + ":",
                        qsTr("Gap Area"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } 
            }
        }
    }

}
