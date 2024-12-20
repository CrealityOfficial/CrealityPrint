import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"
import "components"

LeftPanelDialog {
    id: idRoot
    width: 295 * screenScaleFactor
    // height: 380 * screenScaleFactor
    visible: !kernel_kernel.isDefaultVisScene
    height: 308 * screenScaleFactor
    title: qsTr("Seam Painting")
    property var com

    onTitleChanged: {
        idBottom.updateCopywriting()
    }

    function execute() {
        idToolSeletor.setMethodEnable(0, false)
        idToolSeletor.setMethodEnable(2, false)
        idToolSeletor.setMethodEnable(3, false)
        idToolSeletor.setMethodEnable(4, false)

        idToolSeletor.method = com.colorMethod
        idSectionView.value = com.sectionRate
        idCanvas.screenRadius = com.screenRadius 

        idToolConfigPanel.update();
        idCanvas.update();
        idToolSeletor.update();
    }

    Connections
    {
        target: com

        onScreenRadiusChanged: {
            idCanvas.screenRadius = com.screenRadius
            idCanvas.update();
        }

        onSectionRateChanged: {
            idSectionView.value = com.sectionRate;
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

        ToolSeletor {
            id: idToolSeletor
            anchors.top: parent.top
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
        }

        SliderWithSpinBox {
            id: idSectionView
            anchors.top: idToolConfigPanel.bottom
            width: parent.width
            anchors.topMargin: idToolConfigPanel.visible * 25
            // anchors.topMargin: 30
            height: 30 * screenScaleFactor
            title: qsTr("Section")

            from: 0
            to: 1
            value: com ? com.sectionRate : 1
            stepSize: 0.05

            onValueChanged: {
                var rate = value
                com.sectionRate = rate
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
            
            // contextList: [
            //     qsTr("Left mouse button") + ":",
            //     qsTr("Enforce seam"),
            //     qsTr("Right mouse button") + ":",
            //     qsTr("Block seam"),
            //     qsTr("Shift + Left mouse button") + ":",
            //     qsTr("Erase"),
            //     qsTr("Ctrl + Wheel") + ":",
            //     qsTr("Pen size"),
            //     qsTr("Alt + Wheel") + ":",
            //     qsTr("Section view")
            // ]

            
            contextList: []

            function updateCopywriting() {
                let method = idToolSeletor.method
                if (method == 0) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Enforce seam"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block seam"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),

                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 1) {
                    contextList = [
                        
                        qsTr("Left mouse button") + ":",
                        qsTr("Enforce seam"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block seam"),
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
                        qsTr("Enforce seam"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block seam"),
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
                        qsTr("Enforce seam"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block seam"),
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
                        qsTr("Enforce seam"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block seam"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
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
