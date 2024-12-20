import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Controls.Styles 1.4
import "../secondqml"

LeftPanelDialog {
    //    width: 190
    //    height: 100
    id : idRootRect
    width: 300* screenScaleFactor
    height: 260 * screenScaleFactor
    title: qsTr("Split")
    property var control
    property var curSplitType: 2
    property bool isSelectPart: kernel_model_selector.selectedPartsNum > 0

    onIsSelectPartChanged: {
        if (isSelectPart)
        {
            __cutParts.checked = true
            __cutParts.enabled = false
            if (control)
                control.bCutToParts = true
        }
        else 
        {
            __cutParts.enabled = true
        }
    }

    onCurSplitTypeChanged: {
        if(control) {
            //control.offsetVal = 0
            control.changeAxisType(idRootRect.curSplitType)
        }
    }
    function resetPos(){
        curSplitType = 2
        control.offsetVal = 0
        gapvalue.realValue = 0
        control.modelGap(0)

        __indicateCheck.checked = false
        control.bCutToParts = false
        control.indicateEnabled(false)
    }
    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
        onClicked:
        {
            focus = true
        }
    }

    Item {
        property alias z_pos: z_pos
        anchors.fill: idRootRect.panelArea
        Column
        {
            anchors.top: parent.top
            anchors.topMargin: 20* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5* screenScaleFactor
            StyledLabel {
                color: Constants.textColor
                text: qsTr("Position:")
                font.family: Constants.labelFontFamily
            }
            Row {
                bottomPadding: 10 * screenScaleFactor
                width : parent.width
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 5*screenScaleFactor
                Repeater{
                    model:ListModel{

                        ListElement{imgSource: "qrc:/UI/photo/splitSvg/splitCenterX.svg"}
                        ListElement{imgSource: "qrc:/UI/photo/splitSvg/splitCenterY.svg"}
                        ListElement{imgSource: "qrc:/UI/photo/splitSvg/splitCenterZ.svg"}
                        ListElement{imgSource: "qrc:/UI/photo/splitSvg/splitCenterXYZ.svg"}
                    }
                    delegate: __splitBtnType
                }

            }

            Row
            {
                spacing: 10* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    color: Constants.textColor
                    width: 50 * screenScaleFactor
                    text: qsTr("Offset") + ":"
//                    width: 15* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    font.pointSize: Constants.labelFontPointSize_9
                    font.family: Constants.labelFontFamily

                    opacity: z_pos.opacity
                }


                CusStyledSpinBox {
                    id: z_pos

                    enabled: kernel_model_selector.selectedNum > 0

                    width: 200* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    stepSize: 0.1 * Math.pow(10, decimals)
                    from: -9999 * Math.pow(10, decimals)
                    to: 9999 * Math.pow(10, decimals)
                    decimals: 2
                    unitchar: "mm"
                    realValue: control.offsetVal.toFixed(2)
                    font.pointSize: Constants.labelFontPointSize_9
                    textValidator: RegExpValidator {
                        regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
                    }

                    onTextContentChanged: {
                        if (realValue != result)
                        realValue = result
                        
                    }

                    onRealValueChanged: {
                        if (realValue != control.offsetVal)
                            control.offsetVal = realValue
                    }

                    Connections {
                        target: control

                        onOffsetValChanged: {
                            let offset = control.offsetVal.toFixed(2)
                            if (z_pos.realValue != offset)
                                z_pos.realValue = offset
                        }
                    }
                }
            }
            Row
            {
                spacing: 10* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    color: Constants.textColor
                    width: 50 * screenScaleFactor
                    text: qsTr("Gap") + ":"
                    anchors.verticalCenter: parent.verticalCenter
                    font.pointSize: Constants.labelFontPointSize_9
                    font.family: Constants.labelFontFamily
                }

                CusStyledSpinBox {
                    id: gapvalue

                    width: 200* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    stepSize: 1 * Math.pow(10, decimals)
                    from: -999 * Math.pow(10, decimals)
                    to: 999 * Math.pow(10, decimals)
                    decimals: 2
                    unitchar: "mm"
                    realValue: control.gap.toFixed(2)
                    font.pointSize: Constants.labelFontPointSize_9
                    textValidator: RegExpValidator {
                        regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
                    }

                    onTextContentChanged: {
                        if (realValue != result)
                            realValue = result
                        
                    }

                    onRealValueChanged: {
                        control.modelGap(realValue)
                    }

                }

            }
            Row {
                StyleCheckBox
                {
                    id : __indicateCheck
                    width: 120 * screenScaleFactor
                    height: 20 * screenScaleFactor
                    checked :  false
                    text : qsTr("Indicate")
                    onClicked:
                    {
                        focus = true
                        control.indicateEnabled(checked)
                    }
                }

                StyleCheckBox
                {
                    id : __cutParts
                    width: 120 * screenScaleFactor
                    height: 20 * screenScaleFactor
                    checked : control ? control.bCutToParts : false
                    text : qsTr("Cut to parts")
//                    visible: kernel_global_const.isDebug
                    onClicked:
                    {
                        focus = true
                        control.bCutToParts = checked
                    }
                }
            }
            Row
            {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10* screenScaleFactor
                BasicButton {
                    id: reset
                    width: 125* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("Reset")
                    btnRadius:14* screenScaleFactor
                    btnBorderW:1
                    pointSize: Constants.imageButtomPointSize
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        resetPos()


                    }

                }

                BasicButton {
                    id: slice
                    width: 125* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("Start Split")
                    btnRadius:14* screenScaleFactor
                    btnBorderW:1
                    pointSize:  Constants.imageButtomPointSize
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        control.split();
                    }
                }
            }
        }
        Component
        {
            id: __splitBtnType
            CusImglButton
            {
                width: 61* screenScaleFactor
                height: 39* screenScaleFactor
                shadowEnabled : false
                opacity : enabled ? 1 : 0.7
                defaultBtnBgColor: Constants.themeColor_secondary
                hoveredBtnBgColor:  Constants.themeColor_secondary
                borderWidth: hovered ? 2 :1
                hoverBorderColor: Constants.themeGreenColor
                selectedBtnBgColor: Constants.themeGreenColor
                selectBorderColor:"transparent"
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                enabledIconSource: imgSource
                highLightIconSource: imgSource
                pressedIconSource: imgSource
                bottonSelected : idRootRect.curSplitType === index
                onClicked:
                {

                    idRootRect.curSplitType = index
                    control.offsetVal = 0
                }
            }
        }
    }
}
