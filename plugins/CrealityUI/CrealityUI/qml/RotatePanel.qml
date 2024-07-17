import QtQuick 2.10
import QtQuick.Controls 2.0
import "qrc:/CrealityUI"
import "../secondqml"

LeftPanelDialog {//by TCJ
    id:control
    width: 300* screenScaleFactor
    height: 240* screenScaleFactor
    title:qsTr("Rotate")
    property var mrotate
    readonly property  int  g_nButHeight: 32

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }



    Item {
        property alias x_angle: x_angle
        property alias y_angle: y_angle
        property alias z_angle: z_angle

        anchors.fill: control.panelArea

        Column{
            spacing:20* screenScaleFactor
            anchors.top: parent.top
            anchors.topMargin: 20* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter

            Row {
                spacing:10* screenScaleFactor

                BasicButton{
                    width: 56* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("-45°")
                    pointSize: Constants.labelFontPointSize_9
                    btnRadius:14
                    btnBorderW:1
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mrotate.setQmlRotate((x_angle.realValue - 45 )%360,0)
                    }
                }

                StyledLabel {
                    color: "#FB0202"
                    text: "X:"
                    width:12* screenScaleFactor
                    height: 28* screenScaleFactor
                    font.family: Constants.labelFontFamily
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignHCenter
                }

                LeftToolSpinBox {
                    id:x_angle
                    width: 115 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    realStepSize:5
                    realFrom:-9999
                    realTo:9999
                    realValue: mrotate ? mrotate.rotate.x%360 : 0
                    textValidator: RegExpValidator {
                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                    }
                    property var tmpVal: mrotate ? mrotate.rotate.x%360 : 0
                    unitchar: qsTr("° ")

//                    onTextPressed: {
//                        mrotate.startRotate()
//                    }
                    onValueEdited:{
                        mrotate.setQmlRotate(realValue,0)
                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }

                BasicButton{
                    width: 56* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : "+45°"
                    btnRadius:14
                    btnBorderW:1
                    pointSize: Constants.labelFontPointSize_9
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mrotate.setQmlRotate((x_angle.realValue + 45 )%360,0)
                    }
                }
            }
            Row {
                spacing:10* screenScaleFactor
                BasicButton{
                    width: 56* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("-45°")
                    btnRadius:14
                    btnBorderW:1
                    pointSize: Constants.labelFontPointSize_9
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mrotate.setQmlRotate((y_angle.realValue - 45 )%360,1)
                    }
                }

                StyledLabel {
                    color: "#00FD00"
                    text: "Y:"
                    width:12* screenScaleFactor
                    height: 28* screenScaleFactor
                    font.family: Constants.labelFontFamily
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignHCenter
                }
                LeftToolSpinBox {
                    id:y_angle
                    width:115* screenScaleFactor
                    height: 28 * screenScaleFactor
                    realStepSize:5
                    realFrom:-9999
                    realTo:9999
                    realValue: mrotate ? mrotate.rotate.y%360 : 0
                    textValidator: RegExpValidator {
                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                    }
                    //                       realValue: mrotate.rotate.y
                    unitchar: qsTr("° ")
                    property var tmpVal: mrotate ? mrotate.rotate.y%360 : 0

//                    onTextPressed: {
//                        mrotate.startRotate()
//                    }
                    onValueEdited:{
                        mrotate.setQmlRotate(realValue,1)
                    }

                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }

                BasicButton{
                    width: 56* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : "+45°"
                    btnRadius:14
                    btnBorderW:1
                    pointSize: Constants.labelFontPointSize_9
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mrotate.setQmlRotate((y_angle.realValue + 45 )%360,1)
                    }
                }
            }
            Row {
                spacing:10* screenScaleFactor
                BasicButton{
                    width: 56* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("-45°")
                    btnRadius:14
                    btnBorderW:1
                    pointSize: Constants.labelFontPointSize_9
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mrotate.setQmlRotate((z_angle.realValue - 45 )%360,2)
                    }
                }

                StyledLabel {
                    color: "#008FFD"
                    text: "Z:"
                    width:12* screenScaleFactor
                    height: 28* screenScaleFactor
                    font.pointSize: Constants.labelFontPointSize_9
                    font.family: Constants.labelFontFamily
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignHCenter
                }
                LeftToolSpinBox {
                    id:z_angle
                    width:115* screenScaleFactor
                    height: 28 * screenScaleFactor
                    realStepSize:5
                    realFrom:-9999
                    realTo:9999
                    realValue: mrotate ? mrotate.rotate.z%360 : 0

                    textValidator: RegExpValidator {
                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                    }
                    //                       realValue: mrotate.rotate.z
                    unitchar: qsTr("° ")
                    property var tmpVal: mrotate ? mrotate.rotate.z%360 : 0

//                    onTextPressed: {
//                        mrotate.startRotate()
//                    }
                    onValueEdited:{
                       mrotate.setQmlRotate(realValue,2)
                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }

                BasicButton{
                    width: 56* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : "+45°"
                    btnRadius:14
                    btnBorderW:1
                    pointSize: Constants.labelFontPointSize_9
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mrotate.setQmlRotate((z_angle.realValue + 45 )%360,2)
                    }
                }
            }

            BasicButton{
                width: 258* screenScaleFactor
                height: 28* screenScaleFactor
                text : qsTr("Reset")
                btnRadius:14
                btnBorderW:1
                pointSize: Constants.labelFontPointSize_9
                anchors.horizontalCenter: parent.horizontalCenter
                borderColor: Constants.lpw_BtnBorderColor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                    mrotate.reset()
                }
            }
        }
    }
}
