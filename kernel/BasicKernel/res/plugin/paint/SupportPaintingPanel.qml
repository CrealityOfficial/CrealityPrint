import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"
import "components"

LeftPanelDialog {
    id: idRoot
    width: (317 * screenScaleFactor) + (idOverHangs.title.length > 6 ? (43 * screenScaleFactor) : 0)
    visible: (!kernel_kernel.isDefaultVisScene) || (com ? com.isRunning : false)
    height: idBottom.y + idBottom.height + 68 * screenScaleFactor

    title: qsTr("Support Painting")
    property var com
    property var parameterTreeModel: kernel_ui_parameter.processTreeModel
    property var parameterSetting: kernel_ui_parameter.processOverrideContext.dataModel
    property var supportAngleItem

    onTitleChanged: {
        idBottom.updateCopywriting()
    }

    function execute() {
        idToolSeletor.setMethodEnable(0, false)
        idToolSeletor.setMethodEnable(3, false)

        idToolSeletor.method = com.colorMethod
        idSectionView.value = com.sectionRate
        idCanvas.screenRadius = com.screenRadius

        supportAngleItem = parameterSetting.getDataItem(apdateEngine("support_angle"))

        idOverHangs.value = parseInt(supportAngleItem.value)
        idOverHangsEnable.checked = com.overHangsEnable

        idToolConfigPanel.update();
        com.setOperateModeEnable(enable_support.dataModel.checked)

        idCanvas.update();
        idToolSeletor.update();
    }

    function apdateEngine(key) {
        let type = kernel_global_const.getEngineIntType()
        if (type == 1) { //ORCA
            if (key == "support_angle") {
                key = "support_threshold_angle"
            } else if (key == "support_enable") {
                key = "enable_support"
            } else if (key == "support_structure") {
                key = "support_type"
            } else if (key == "normal") {
                key = "normal(auto)"
            } else if (key == "thomastree") {
                key = "tree(auto)"
            } else if (key == "normal_manual") {
                key = "normal(manual)"
            } else if (key == "thomastree_manual") {
                key = "tree(manual)"
            } else if (key == "true") {
                key = "1"
            } else if (key == "false") {
                key = "0"
            }
        }
        return key
    }

    function updateParameterSettting() {
        if (!idRoot.visible)
            return;

        idOverHangs.value = parseInt(supportAngleItem.value)
        idOverHangsEnable.checked = com.overHangsEnable
    }

    onParameterSettingChanged: {
        supportAngleItem = parameterSetting.getDataItem(apdateEngine("support_angle"))
        updateParameterSettting()
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

    Connections
    {
        target: supportAngleItem

        function onValueChanged() {
            updateParameterSettting()
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
        // visible: idRoot.visible && (com ? com.colorMethod == 1 : false) && idEnableSupport.checked
        visible: idRoot.visible && (com ? com.colorMethod == 1 : false) && enable_support.dataModel.checked
    }

    ParameterComponent {
        id: parameter_component
        toolTipEnabled: false
        labelTextColor: Constants.textColor
    }

    Item {
        anchors.fill: parent.panelArea
        anchors.margins: 20

        Loader {
            id: enable_support

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            height: 30 * screenScaleFactor

            readonly property var uiModel: parameterTreeModel.findNodeObjectRecursived('/support/support/enable_support')
            readonly property var dataModel: parameterSetting.getDataItem('enable_support')

            sourceComponent: parameter_component.parameter

            Connections {
                target: enable_support.dataModel

                function onDefaultValueChanged() {
                    com.setOperateModeEnable(enable_support.dataModel.checked)
                }

                function onValueChanged() {
                    com.setOperateModeEnable(enable_support.dataModel.checked)
                }
            }
        }

        Loader {
            id: support_type

            anchors.top: enable_support.bottom
            anchors.topMargin: 5 * screenScaleFactor
            anchors.left: parent.left
            anchors.right: parent.right

            height: 28 * screenScaleFactor

            readonly property var uiModel: parameterTreeModel.findNodeObjectRecursived('/support/support/support_type')
            readonly property var dataModel: parameterSetting.getDataItem('support_type')

            sourceComponent: parameter_component.parameter
        }

        ToolSeletor {
            id: idToolSeletor
            anchors.top: support_type.bottom
            width: parent.width
            height: 45 * screenScaleFactor
            anchors.topMargin: 10 * screenScaleFactor
            enabled: enable_support.dataModel.checked

            onMethodChanged: {
                if (!com || com.colorMethod == method)
                    return

                com.colorMethod = method
                idCanvas.update()
                idBottom.updateCopywriting()
            }
        }

        SliderWithSpinBox {
            id: idOverHangs
            anchors.top: idToolSeletor.bottom
            width: parent.width
            height: 30 * screenScaleFactor
            anchors.topMargin: 12 * screenScaleFactor
            title: qsTr("Highlight overhangs")
            // unit: '°'
            enabled: enable_support.dataModel.checked

            from: 0
            to: 90
            value: 0

            onValueChanged: {
                com.highlightOverhangsDeg = idOverHangs.value
            }

            onSliderPressedChanged: {
                const value =  Math.floor(idOverHangs.value)
                if (!sliderPressed && supportAngleItem.value != value) {
                    supportAngleItem.value = value
                }
            }
        }

        StyleCheckBox {
            id: idOverHangsEnable
            anchors.top: idOverHangs.bottom
            height: 16 * screenScaleFactor
            strToolTip: qsTr("Allows painting only on facets selected by:'Highlight overhang areas'")
            text: qsTr("On overhangs only")
            textColor: Constants.textColor

            enabled: enable_support.dataModel.checked

            onCheckedChanged: {
                com.overHangsEnable = checked
            }
        }

        ToolConfigPanel {
            id: idToolConfigPanel
            anchors.top: idOverHangsEnable.bottom
            anchors.topMargin: 12 * screenScaleFactor
            width: parent.width
            com: idRoot.com
            enabled: enable_support.dataModel.checked
        }

        SliderWithSpinBox {
            id: idSectionView
            anchors.top: idToolConfigPanel.bottom
            width: parent.width
            anchors.topMargin: 12 * screenScaleFactor
            height: 30 * screenScaleFactor
            title: qsTr("Section")
            enabled: enable_support.dataModel.checked

            from: 0
            to: 1
            value: 1
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
            width: (text.length > 5 ? 95 : 70) * screenScaleFactor
            text : qsTr("Reset Direction")
            enabled: enable_support.dataModel.checked

            btnRadius: height / 2
            btnBorderW:1
            pointSize: Constants.labelFontPointSize_9

            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.leftToolBtnColor_normal
            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered

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
            enabled: enable_support.dataModel.checked

            // contextList: [
            //     qsTr("Left mouse button") + ":",
            //     qsTr("Enforce supports"),
            //     qsTr("Right mouse button") + ":",
            //     qsTr("Block supports"),
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
                        qsTr("Enforce supports"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block supports"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 1) {
                    contextList = [
                        qsTr("Left mouse button") + ":",
                        qsTr("Enforce supports"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block supports"),
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
                        qsTr("Enforce supports"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block supports"),
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
                        qsTr("Enforce supports"),
                        qsTr("Right mouse button") + ":",
                        qsTr("Block supports"),
                        qsTr("Shift + Left mouse button") + ":",
                        qsTr("Erase"),
                        qsTr("Ctrl + Wheel") + ":",
                        qsTr("Height Range"),
                        qsTr("Alt + Wheel") + ":",
                        qsTr("Section view")
                    ]
                } else if (method == 3) {
                    contextList = [
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
