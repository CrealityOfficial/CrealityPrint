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
    property var parameterSetting: kernel_ui_parameter.processOverrideContext.dataModel
    property var supportAngleItem
    property var supportEnableItem
    property var supportStructureItem

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
        supportEnableItem = parameterSetting.getDataItem(apdateEngine("support_enable"))
        supportStructureItem = parameterSetting.getDataItem(apdateEngine("support_structure"))

        idOverHangs.value = parseInt(supportAngleItem.value)
        idOverHangsEnable.checked = com.overHangsEnable

        let type = supportStructureItem.value
        if (type == apdateEngine("normal")) {
            idSupportType.currentIndex = 0
        } else if (type == apdateEngine("thomastree")) {
            idSupportType.currentIndex = 1
        } else if (type == apdateEngine("normal_manual")) {
            idSupportType.currentIndex = 2
        } else if (type == apdateEngine("thomastree_manual")) {
            idSupportType.currentIndex = 3
        }

        idEnableSupport.checked = (supportEnableItem.value == apdateEngine("true"))

        idToolConfigPanel.update();
        com.setOperateModeEnable(idEnableSupport.checked)

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

        let structureItem = idRoot.supportStructureItem
        let type = structureItem.value
        if (type == apdateEngine("normal")) {
            idSupportType.currentIndex = 0
        } else if (type == apdateEngine("thomastree")) {
            idSupportType.currentIndex = 1
        } else if (type == apdateEngine("normal_manual")) {
            idSupportType.currentIndex = 2
        } else if (type == apdateEngine("thomastree_manual")) {
            idSupportType.currentIndex = 3
        }

        let enableItem = idRoot.supportEnableItem
        idEnableSupport.checked = (enableItem.value == apdateEngine("true"))
        if (com)
            com.setOperateModeEnable(idEnableSupport.checked)
    }

    onParameterSettingChanged: {
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

    Connections
    {
        target: supportEnableItem

        function onValueChanged() {
            updateParameterSettting()
        }
    }

    Connections
    {
        target: supportStructureItem

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
        visible: idRoot.visible && (com ? com.colorMethod == 1 : false) && idEnableSupport.checked
    }

    Item {
        anchors.fill: parent.panelArea
        anchors.margins: 20

        StyleCheckBox {
            id: idEnableSupport
            // height: 16 * screenScaleFactor
            anchors.top: parent.top
            height: 30 * screenScaleFactor
            width: 145 * screenScaleFactor
            textColor: Constants.textColor
            text: qsTr("Enable Support")
            font.pointSize: Constants.labelFontPointSize_9

            onCheckedChanged: {
                let value = ""
                if (checked)
                    value = apdateEngine("true")
                else
                    value = apdateEngine("false")

                let enableItem = idRoot.supportEnableItem
                // console.log("set support enable *************")
                // console.log(enableItem)
                // console.log("old: " + enableItem.value)
                // console.log("new: " + value)

                if (enableItem.value != value) {
                    enableItem.value = value
                }

                com.setOperateModeEnable(checked)
            }

            Component.onCompleted: {
                checked = (parameterSetting.value(apdateEngine("support_enable")) == apdateEngine("true"))
                if (com)
                    com.setOperateModeEnable(checked)
            }

        }

        Item {
            id: item2
            anchors.top: idEnableSupport.bottom
            anchors.topMargin: 10
            width: parent.width
            height: 28 * screenScaleFactor
            enabled: idEnableSupport.checked

            CusText {
                id: idSupportTypeTitle
                anchors.left: parent.left
                anchors.right: idSupportType.left
                anchors.rightMargin: 10
                height: parent.height
                hAlign: 0
                fontPointSize: Constants.labelFontPointSize_9
                fontText: qsTr("Support Type")
                fontColor: Constants.textColor
            }

            CustomComboBox
            {
                id: idSupportType
                width: 150 * screenScaleFactor
                height: parent.height
                anchors.right: parent.right
                anchors.rightMargin: 55
                font.pointSize: Constants.labelFontPointSize_9
                model: [
                    qsTranslate("ParameterComponent", "normal(auto)"),
                    qsTranslate("ParameterComponent", "tree(auto)"),
                    qsTranslate("ParameterComponent", "normal(manual)"),
                    qsTranslate("ParameterComponent", "tree(manual)")
                ]

                onCurrentIndexChanged: {
                    let value = ""
                    if (currentIndex == 0) {
                        value = apdateEngine("normal")
                    } else if (currentIndex == 1) {
                        value = apdateEngine("thomastree")
                    } else if (currentIndex == 2) {
                        value = apdateEngine("normal_manual")
                    } else if (currentIndex == 3) {
                        value = apdateEngine("thomastree_manual")
                    }

                    let structureItem = idRoot.supportStructureItem
                    if (structureItem.value != value) {
                        structureItem.value = value
                    }
                }
            }
        }

        ToolSeletor {
            id: idToolSeletor
            anchors.top: item2.bottom
            width: parent.width
            height: 45 * screenScaleFactor
            anchors.topMargin: 20
            enabled: idEnableSupport.checked

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
            enabled: idEnableSupport.checked

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
            strToolTip: qsTr("Drawing only takes effect on the selected faces in the highlighted and suspended area")
            text: qsTr("On overhangs only")
            textColor: Constants.textColor

            enabled: idEnableSupport.checked

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
            enabled: idEnableSupport.checked
        }

        SliderWithSpinBox {
            id: idSectionView
            anchors.top: idToolConfigPanel.bottom
            width: parent.width
            anchors.topMargin: 12 * screenScaleFactor
            height: 30 * screenScaleFactor
            title: qsTr("Section")
            enabled: idEnableSupport.checked

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
            enabled: idEnableSupport.checked

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
            enabled: idEnableSupport.checked

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
