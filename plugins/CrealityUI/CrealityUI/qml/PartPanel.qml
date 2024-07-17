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
	
    onCurSplitTypeChanged: {
        if(control) {
            control.offsetVal = 0
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
                }
                LeftToolSpinBox {
                    id:z_pos
                    width: 200* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    realStepSize:0.01
                    realFrom:-9999
                    realTo:9999
                    font.pointSize: Constants.labelFontPointSize_9
                    realValue: control.offsetVal.toFixed(2)
                    decimals : 2
                    bResetOtgValue : true
                    orgValue : control.offsetVal.toFixed(2)
                    property var posVal : control ? control.offsetVal.toFixed(2) : 0
                    onValueEdited:{
                        if(control)
                        {
                            control.offsetVal = (realValue)
                        }

                    }
                    onPosValChanged:
                    {
                         realValue = posVal
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
                LeftToolSpinBox {
                    id:gapvalue
                    width: 200* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    realStepSize:1
                    realFrom:-999
                    realTo:999
                    font.pointSize: Constants.labelFontPointSize_9
                    realValue: 0
                    orgValue : 0
                    bResetOtgValue : true
                    onValueEdited:{
                        if(control)
                        {
                            control.modelGap(realValue)
                        }
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
                    visible: kernel_global_const.isDebug
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
