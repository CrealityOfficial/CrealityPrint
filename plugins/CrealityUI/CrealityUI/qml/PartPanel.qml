import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Controls.Styles 1.4
import "../secondqml"

LeftPanelDialog {
    //    width: 190
    //    height: 100
    id : idRootRect
    width: 300* screenScaleFactor
    height: 230 * screenScaleFactor
    title: qsTr("Split")
    property var control
    property var curSplitType: 2
	
    onCurSplitTypeChanged: {
        if(control) {
            control.changeAxisType(idRootRect.curSplitType)
        }
    }

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
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
                spacing: 20* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    color: Constants.textColor
                    text: qsTr("Offset") + ":"
//                    width: 15* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    font.pointSize: Constants.labelFontPointSize_9
                    font.family: Constants.labelFontFamily
                }
                LeftToolSpinBox {
                    id:z_pos
                    width: 212* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    realStepSize:1
                    realFrom:-9999
                    realTo:9999
                    font.pointSize: Constants.labelFontPointSize_9
                    realValue: 0
                    textValidator: RegExpValidator {
                       regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                   }
//                    property var tmpVal: control ? control.position.z : 0
                    onValueEdited:{
                        if(control)
                        {
                            control.changeOffset(realValue)
                        }

                    }
//                    onTmpValChanged:
//                    {
//                        realValue = tmpVal
//                    }
                }
            }

//            Item{
//                width: 20* screenScaleFactor
//                height: 10* screenScaleFactor
//            }

//            StyledLabel {
//                color: Constants.textColor
//                text: qsTr("Rotate:")
//                font.pointSize: Constants.labelFontPointSize_9
//                font.family: Constants.labelFontFamily
//            }

//            Row {
//                spacing: 20* screenScaleFactor
//                anchors.horizontalCenter: parent.horizontalCenter
//                StyledLabel {
//                    color: "#FB0202"
//                    text: "X:"
//                    width: 15* screenScaleFactor
//                    anchors.verticalCenter: parent.verticalCenter
//                    font.pointSize: Constants.labelFontPointSize_9
//                    font.family: Constants.labelFontFamily
//                }
//                LeftToolSpinBox {
//                    id:x_rotate
//                    width: 212* screenScaleFactor
//                    height: 28* screenScaleFactor
//                    anchors.verticalCenter: parent.verticalCenter
//                    realStepSize:5
//                    realFrom:-9999
//                    realTo:9999
//                    font.pointSize: Constants.labelFontPointSize_9
//                    realValue: control ? control.dir.x : 0
//                    textValidator: RegExpValidator {
//                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
//                    }
//                    property var tmpVal: control ? control.dir.x : 0
//                    onValueEdited:{
//                        control.setQmlRotate(realValue,0)
//                    }
//                    unitchar: qsTr("(°)")
//                    onTmpValChanged:
//                    {
//                        realValue = tmpVal
//                    }
//                }

//            }
//            Row
//            {
//                spacing: 20* screenScaleFactor
//                anchors.horizontalCenter: parent.horizontalCenter
//                StyledLabel {
//                    color: "#00FD00"
//                    text: "Y:"
//                    width: 15* screenScaleFactor
//                    anchors.verticalCenter: parent.verticalCenter
//                    font.family: Constants.labelFontFamily
//                    font.pointSize: Constants.labelFontPointSize_9
//                }
//                LeftToolSpinBox {
//                    id:y_rotate
//                    width: 212* screenScaleFactor
//                    height: 28* screenScaleFactor
//                    anchors.verticalCenter: parent.verticalCenter
//                    realStepSize:5
//                    realFrom:-9999
//                    realTo:9999
//                    font.pointSize: Constants.labelFontPointSize_9
//                    realValue:control ? control.dir.y : 0
//                    textValidator: RegExpValidator {
//                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
//                    }
//                    property var tmpVal: control ? control.dir.y : 0
//                    onValueEdited:{
//                        control.setQmlRotate(realValue,1)
//                    }
//                    unitchar: qsTr("(°)")
//                    onTmpValChanged:
//                    {
//                        realValue = tmpVal
//                    }
//                }

//            }
//            Row
//            {
//                spacing: 20* screenScaleFactor
//                anchors.horizontalCenter: parent.horizontalCenter
//                StyledLabel {
//                    color: "#008FFD"
//                    text: "Z:"
//                    width: 15* screenScaleFactor
//                    anchors.verticalCenter: parent.verticalCenter
//                    font.family: Constants.labelFontFamily
//                    font.pointSize: Constants.labelFontPointSize_9
//                }
//                LeftToolSpinBox {
//                    id:z_rotate
//                    width: 212* screenScaleFactor
//                    height: 28* screenScaleFactor
//                    anchors.verticalCenter: parent.verticalCenter
//                    realStepSize:5
//                    realFrom:-9999
//                    realTo:9999
//                    font.pointSize: Constants.labelFontPointSize_9
//                    realValue: control ? control.dir.z : 0
//                    textValidator: RegExpValidator {
//                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
//                    }
//                    property var tmpVal: control ? control.dir.z : 0
//                    onValueEdited:{
//                        control.setQmlRotate(realValue,2)
//                    }
//                    unitchar: qsTr("(°)")
//                    onTmpValChanged:
//                    {
//                        realValue = tmpVal
//                    }
//                }
//            }

            Item{
                width: 20* screenScaleFactor
                height: 15* screenScaleFactor
            }

            Row
            {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10* screenScaleFactor
                BasicButton {
                    id: reset
                    width: 124* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("Reset")
                    btnRadius:14* screenScaleFactor
                    btnBorderW:1
                    pointSize: Constants.imageButtomPointSize
                    borderColor: Constants.lpw_BtnBorderColor
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        z_pos.realValue = 0
                        control.changeOffset(0)
                    }

                }

                BasicButton {
                    id: slice
                    width: 124* screenScaleFactor
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
//                imgWidth : 61
//                imgHeight : 39
                shadowEnabled : false
                opacity : enabled ? 1 : 0.7
                defaultBtnBgColor: Constants.themeColor_secondary
                hoveredBtnBgColor:  Constants.themeColor_secondary
                borderWidth: hovered ? 2 :1
                hoverBorderColor: "#009CFF"
                selectedBtnBgColor: "#009CFF"
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
                    z_pos.realValue = 0
                }
            }
        }
    }
}
