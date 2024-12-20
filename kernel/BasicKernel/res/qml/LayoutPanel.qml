import CrealityUI 1.0
import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.13
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

LeftPanelDialog {
    id: control

    property var com

    title: qsTr("Layout")
    width: 300 * screenScaleFactor
    height: 240 * screenScaleFactor
    //    property var com : com //: HalotContext.obj("Command.LayoutPanel")
    property real minModelVal: 1
    property real maxModelVal: 50

    property real extruder_clearance_radius_Val: 45

    property var print_sequence_obj
    property var extruder_radius_obj
    property var curPlatformObj

    Component.onCompleted: {
        //        initObjectData()
        //        platformParameter()
    }
    onComChanged:
    {
        if(com)
        {
            initObjectData()
            platformParameter()
        }
    }
    //捕获鼠标点击空白地方的事件
    MouseArea {
        anchors.fill: parent
    }

    Item {
        anchors.fill: parent.panelArea

        ColumnLayout {
            spacing: 10 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 21
            anchors.right: parent.right
            anchors.rightMargin: 21

            RowLayout {
                spacing: 20 * screenScaleFactor
                Layout.topMargin: 10 * screenScaleFactor

                BasicRadioButton {
                    id: __allModels

                    text: qsTr("All models")
                    checked: com ? com.type === 0 : true
                    font.pointSize: Constants.labelFontPointSize_11
                    font.weight: Font.bold
                    font.family: Constants.labelFontFamilyBold
                    onClicked: {
                        com.type = 0;
                        //                        platformParameter();
                        do_gloabl_setting();
                        console.log("All models");
                    }
                }

                BasicRadioButton {
                    id: __select

                    text: qsTr("Selected models")
                    checked: com ? com.type === 1 : false
                    font.pointSize: Constants.labelFontPointSize_11
                    font.weight: Font.bold
                    font.family: Constants.labelFontFamilyBold
                    onClicked: {
                        com.type = 1;
                        platformParameter();
                    }
                }

            }

            RowLayout {
                width: parent.width

                StyledLabel {
                    text: qsTr("Model interval")
                    color:  Constants.textColor
                    Layout.fillWidth: true
                }

                CusStyledSpinBox {
                    Layout.preferredWidth: 120 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    decimals: 0
                    from: minModelVal * Math.pow(10, decimals)
                    to: maxModelVal * Math.pow(10, decimals)
                    value: com ? com.modelSpacing * Math.pow(10, decimals) : 0
                    stepSize: 1 * Math.pow(10, decimals)
                    unitchar: "mm"
                    onTextContentChanged: {
                        com.modelSpacing = result;
                    }


                    textValidator: DoubleValidator {
                        bottom: minModelVal
                        top: maxModelVal
                        notation: DoubleValidator.StandardNotation
                        decimals: decimals
                    }
                }

            }

            RowLayout {
                width: parent.width

                StyledLabel {
                    text: qsTr("Plate Margin")
                    color: Constants.textColor
                    Layout.fillWidth: true
                }

                CusStyledSpinBox {
                    Layout.preferredWidth: 120 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    decimals: 0
                    from: 1 * Math.pow(10, decimals)
                    to: 20 * Math.pow(10, decimals)
                    value: com ? com.platformSpacing * Math.pow(10, decimals) : 0
                    stepSize: 1 * Math.pow(10, decimals)
                    unitchar: "mm"

                    onTextContentChanged: {
                         com.platformSpacing = result;
                    }

                    textValidator: RegExpValidator {
                        regExp: /(\d{1,3})([.,]\d{1,1})?$/
                    }

                }

            }

            RowLayout {
                width: parent.width

                StyleCheckBox {
                    Layout.preferredHeight: 28 * screenScaleFactor
                    Layout.preferredWidth: parent.width
                    checked: com ? com.allowRotate : true
                    text: qsTr("Allow rotation for Auto-Layout")
                    onClicked: {
                        com.allowRotate = checked;
                    }
                }

            }

            Row {
                width: parent.width
                spacing: 10 * screenScaleFactor

                BasicDialogButton
                {
                    width: parent.width //  / 2 - 5
                    height: 28 * screenScaleFactor

                    text: qsTr("Comply")
                    btnRadius: height/2
                    opacity: enabled ? 1 : 0.8
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        com.autoLayout(0);
                    }
                }
            }

        }

    }
    Connections
    {
        target: kernel_reusable_cache.printerManager
        function onSignalDidSelectPrinter(printer)
        {
            curPlatformObj = kernel_reusable_cache.printerManager.selectedPrinter
            platformParameter()
        }
    }
    function  calculateRadius()
    {
        extruder_clearance_radius_Val = +extruder_radius_obj.value
    }

    function initObjectData()
    {
        let curPrinter = kernel_parameter_manager.currentMachineObject
        var curProfile = curPrinter.currentProfileObject
        var dataMode = curProfile.dataModel;
        var dataItem = dataMode.getDataItem("print_sequence")

        var machineDataModel = curPrinter.dataModel
        var machineDataItem = machineDataModel.getDataItem("extruder_clearance_radius")

        print_sequence_obj = dataItem
        extruder_radius_obj = machineDataItem


        var curPatform = kernel_reusable_cache.printerManager.selectedPrinter
        curPlatformObj = curPatform


        print_sequence_obj.valueChanged.connect((_)=>
                                                {
                                                    console.log("***changed dataitem.val =",dataItem.value)
                                                    console.log("***changed  extruder_clearance_radius value =",machineDataItem.value)
                                                    calculateRadius()
                                                    setMinAndMax(dataItem.value)
                                                })


        extruder_radius_obj.valueChanged.connect((_)=>{
                                                     console.log("**machineDataItem_changed dataitem.val =",dataItem.value)
                                                     console.log("**machineDataItem_changed  extruder_clearance_radius value =",machineDataItem.value)
                                                     setMinAndMax(dataItem.value,machineDataItem.value)
                                                 })
    }

    function do_gloabl_setting()
    {
        calculateRadius()
        setMinAndMax(print_sequence_obj.value)
    }

    function platformParameter()
    {
        var data= curPlatformObj.parameter("print_sequence")
        if(data == "by object" && (com.type ===1))
        {
            calculateRadius()
            setMinAndMax(data)
        }
        else
        {
            do_gloabl_setting()
        }
        curPlatformObj.parameterChanged.disconnect(updatePlatformData)
        curPlatformObj.parameterChanged.connect(updatePlatformData)
    }
    function updatePlatformData(key,val)
    {
        platformParameter()
    }
    function setMinAndMax(sequence)
    {
        console.log("setMinAndMax(sequence) =",sequence)
        if(sequence === "by object")
        {
            minModelVal = extruder_clearance_radius_Val + 5
            maxModelVal = minModelVal + 20
//            if(com)
//                com.modelSpacing = minModelVal
        }
        else
        {
            minModelVal = 1
            maxModelVal = 50
//            if(com)
//                com.modelSpacing = minModelVal
        }
    }

}
