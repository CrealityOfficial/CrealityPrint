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
    // height: 380 * screenScaleFactor
    visible: (!kernel_kernel.isDefaultVisScene) || (com ? com.isRunning : false)
    // height: 346 * screenScaleFactor + 36 * screenScaleFactor * idPenSize.visible
    height: 436 * screenScaleFactor + 36 * screenScaleFactor * idPenSize.visible
    title: qsTr("Support Painting")
    property var com
    property var parameterSetting: kernel_parameter_manager.currentMachineObj.currentProfileObj.profileParameterModel(true)
	property var currentProfile: kernel_parameter_manager.currentMachineObj.currentProfileObj

    function execute() {
        idToolSeletor.setMethodEnable(0, false)
        
        idToolSeletor.method = com.colorMethod
        idPenSize.value = com.colorRadius * 100.0
        idSectionView.value = com.sectionRate * 100.0
        idCanvas.penSize = idPenSize.value * 2
        idOverHangs.value = parseInt(parameterSetting.value("support_angle")) * 100.0
        idOverHangsEnable.checked = com.overHangsEnable
        idEnableSupport.checked = (parameterSetting.value("support_enable") == "true")
        com.setSupportEnabled(idEnableSupport.checked)

        idCanvas.update();
        idToolSeletor.update();
    }

    function updateParameterSettting() {
        let type = parameterSetting.value("support_structure")
        if (type == "normal") {
            idSupportType.currentIndex = 0
        } else if (type == "thomastree") {
            idSupportType.currentIndex = 1
        } else if (type == "normal_manual") {
            idSupportType.currentIndex = 2
        } else if (type == "thomastree_manual") {
            idSupportType.currentIndex = 3
        }

        idEnableSupport.checked = (parameterSetting.value("support_enable") == "true")
        if (com)
            com.setSupportEnabled(idEnableSupport.checked)
        console.log("value changed " + parameterSetting.value("support_enable") + " " + parameterSetting.value("support_structure"))
    }

    onParameterSettingChanged: {
        updateParameterSettting()
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

    Connections
    {
        target: parameterSetting

        onDataChanged: {
            updateParameterSettting()
        }
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

        CusCheckBox {
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
                    value = "true"
                else  
                    value = "false"

                if (parameterSetting.value("support_enable") != value)
                    parameterSetting.setValue("support_enable", value)

                com.setSupportEnabled(checked)
            }

            Component.onCompleted: {
                checked = (parameterSetting.value("support_enable") == "true")
                if (com)
                    com.setSupportEnabled(checked)
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
            
            BasicCombobox
            {
                id: idSupportType
                width: 150 * screenScaleFactor
                height: parent.height
                anchors.right: parent.right
                anchors.rightMargin: 55
                font.pointSize: Constants.labelFontPointSize_9
                model: [
                    qsTr("normal(auto)"),
                    qsTr("tree(auto)"),
                    qsTr("normal(manual)"),
                    qsTr("tree(manual)")
                ]

                onCurrentIndexChanged: {  
                    let value = ""
                    if (currentIndex == 0) {
                        value = "normal"
                    } else if (currentIndex == 1) {
                        value = "thomastree"
                    } else if (currentIndex == 2) {
                        value = "normal_manual"
                    } else if (currentIndex == 3) {
                        value = "thomastree_manual"
                    }
                    if (parameterSetting.value("support_structure") != value)
                        parameterSetting.setValue("support_structure", value)
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
                if (com)
                    com.colorMethod = method        
                idCanvas.update();
            }
        }

        CusCheckBox {
            id: idOverHangsEnable
            anchors.top: idToolSeletor.bottom
            anchors.topMargin: 12
            height: 16 * screenScaleFactor  
            strToolTip: qsTr("Drawing only takes effect on the selected faces in the highlighted and suspended area")
            text: qsTr("On overhangs only")
            textColor: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_9
            enabled: idEnableSupport.checked

            onCheckedChanged: {
                com.overHangsEnable = checked
            }
        }

        SliderWithSpinBox {
            id: idPenSize
            anchors.top: idOverHangsEnable.bottom
            width: parent.width
            height: 30 * screenScaleFactor
            anchors.topMargin: 30 * screenScaleFactor
            title: qsTr("Pen Size")
            visible: com ? com.colorMethod == 1 : false
            enabled: idEnableSupport.checked
            
            from: 10
            to: 800
            value: 100

            onValueChanged: {
                idCanvas.penSize = idPenSize.value * 2
                com.colorRadius = idPenSize.value / 100
            }
        }

        SliderWithSpinBox {
            id: idOverHangs
            anchors.top: idPenSize.visible ? idPenSize.bottom : idOverHangsEnable.bottom
            width: parent.width
            height: 30 * screenScaleFactor
            anchors.topMargin: (idPenSize.visible ? 6 : 30) * screenScaleFactor
            title: qsTr("Highlight overhangs")
            enabled: idEnableSupport.checked
            
            from: 0
            to: 9000
            value: 0

            onValueChanged: {
                com.highlightOverhangsDeg = idOverHangs.value / 100
            }
        }

        SliderWithSpinBox {
            id: idSectionView
            anchors.top: idOverHangs.bottom
            width: parent.width
            anchors.topMargin: 30 * screenScaleFactor
            // anchors.topMargin: 30
            height: 30 * screenScaleFactor
            title: qsTr("Section")
            enabled: idEnableSupport.checked

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

            contextList: [
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
        }
    }

}
