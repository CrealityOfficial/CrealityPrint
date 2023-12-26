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

    function execute() {
        idToolSeletor.setMethodEnable(0, false)
        idToolSeletor.setMethodEnable(2, false)

        idToolSeletor.method = com.colorMethod
        idPenSize.value = com.colorRadius * 100.0
        idSectionView.value = com.sectionRate * 100.0
        idCanvas.penSize = idPenSize.value * 2

        idCanvas.update();
        idToolSeletor.update();
    }

    Connections
    {
        target: com

        onColorRadiusChanged: {
            idPenSize.value = com.colorRadius * 100.0;
        }

        onSectionRateChanged: {
            idSectionView.value = com.sectionRate * 100.0;
        }

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
                if (com)
                    com.colorMethod = method        
                idCanvas.update();
            }
        }

        SliderWithSpinBox {
            id: idPenSize
            anchors.top: idToolSeletor.bottom
            width: parent.width
            height: 30 * screenScaleFactor
            title: qsTr("Pen Size")
            
            from: 10
            to: 800
            value: 100

            onValueChanged: {
                idCanvas.penSize = idPenSize.value * 2
                com.colorRadius = idPenSize.value / 100
            }
        }

        SliderWithSpinBox {
            id: idSectionView
            anchors.top: idPenSize.bottom
            width: parent.width
            anchors.topMargin: idPenSize.visible * 25
            // anchors.topMargin: 30
            height: 30 * screenScaleFactor
            title: qsTr("Section")

            from: 0
            to: 100
            value: 100

            onValueChanged: {
                var rate = value / 100.0
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
            
            contextList: [
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
        }
    }

}
