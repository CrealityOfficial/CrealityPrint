import QtQuick 2.10
import QtQuick.Controls 2.0
//import ".."
import "../secondqml"

LeftPanelDialog {
    id: control
    title:  qsTr("Move")
    width: 300 * screenScaleFactor
    height: 213 * screenScaleFactor

    property var mtranslate
    property int labelWidth: 15* screenScaleFactor

    Item {
        property alias x_pos: x_pos
        property alias y_pos: y_pos
        property alias z_pos: z_pos

        anchors.fill: control.panelArea
        MouseArea{//捕获鼠标点击空白地方的事件
            anchors.fill: parent
            onClicked:
            {
                focus = true
            }
        }

        Column {
            anchors.top: parent.top
            anchors.topMargin: 20* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 5* screenScaleFactor

            Row {
                spacing: 10//
                Label {
                    width:labelWidth* screenScaleFactor
                    color: "#FB0202"
                    text: "X:"
                    font.family: Constants.labelFontFamily
                    font.pointSize: Constants.labelFontPointSize_9
                    anchors.verticalCenter: parent.verticalCenter
                }
                LeftToolSpinBox {
                    id:x_pos
                    width: 239* screenScaleFactor
                    height: 28* screenScaleFactor
                    realStepSize:5
                    realFrom:-9999
                    realTo:9999
                    anchors.verticalCenter: parent.verticalCenter
                    realValue: mtranslate ? mtranslate.position.x.toFixed(2) : 0
                    property var posVal : mtranslate ? mtranslate.position.x.toFixed(2) : 0
                    textValidator: RegExpValidator {
                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                    }
                    onValueEdited:{
                        if(!mtranslate)return
                        mtranslate.setQmlPosition(realValue,0)
                        console.log("----onValueEdited--",realValue)
                       // realValue = Qt.binding(function(){return mtranslate.position.x.toFixed(2) });
                    }
                    onPosValChanged:
                    {
                        realValue = posVal
                    }
                }
            }
            Row {
                spacing: 10//
                Label {
                    width:labelWidth* screenScaleFactor
                    color: "#00FD00"
                    text: "Y:"
                    font.family: Constants.labelFontFamily
                    font.pointSize: Constants.labelFontPointSize_9
                    anchors.verticalCenter: parent.verticalCenter
                }
                LeftToolSpinBox {
                    id:y_pos
                    width: 239* screenScaleFactor
                    height: 28* screenScaleFactor
                    realStepSize:5
                    realFrom:-9999
                    realTo:9999
                    anchors.verticalCenter: parent.verticalCenter
                    realValue: mtranslate ? mtranslate.position.y.toFixed(2) : 0
                    property var posVal : mtranslate ? mtranslate.position.y.toFixed(2) : 0
                    textValidator: RegExpValidator {
                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                    }
                    onValueEdited:{
                        if(!mtranslate)return
                        mtranslate.setQmlPosition(realValue,1)
                    }
                    onPosValChanged:
                    {
                        realValue = posVal
                    }
                }
            }
            Row {
                spacing: 10//
                Label {
                    width:labelWidth* screenScaleFactor
                    color: "#008FFD"
                    text: "Z:"
                    font.pointSize: Constants.labelFontPointSize_9
                    font.family: Constants.labelFontFamily
                    anchors.verticalCenter: parent.verticalCenter
                }
                LeftToolSpinBox {
                    id:z_pos
                    width: 239* screenScaleFactor
                    height: 28* screenScaleFactor
                    realStepSize:5
                    realFrom:-9999
                    realTo:9999
                    anchors.verticalCenter: parent.verticalCenter
                    realValue: mtranslate ? mtranslate.position.z.toFixed(2) : 0
                    property var posVal : mtranslate ? mtranslate.position.z.toFixed(2) : 0
                    textValidator: RegExpValidator {
                        regExp:   /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/
                    }
                    onValueEdited:{
                        if(!mtranslate)return
                        mtranslate.setQmlPosition(realValue,2)
                    }
                    onPosValChanged:
                    {
                        realValue = posVal
                    }
                }
            }

            Item{
                width: 20
                height: 15
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10
                BasicButton {
                    width: 82* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("Center")
                    btnRadius:14
                    btnBorderW:1
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mtranslate.center()
                    }
                }

                BasicButton {
                    width: 82* screenScaleFactor
                    height: 28* screenScaleFactor
                    text : qsTr("Bottom")
                    btnRadius:14
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        mtranslate.bottom()
                    }
                }

                // BasicButton {
                //     width: 82* screenScaleFactor
                //     height: 28* screenScaleFactor
                //     text : qsTr("Reset")
                //     btnRadius:14
                //     btnBorderW:1
                //     borderColor: Constants.lpw_BtnBorderColor
                //     defaultBtnBgColor: Constants.leftToolBtnColor_normal
                //     hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                //     onSigButtonClicked:
                //     {
                //         mtranslate.reset()
                //     }
                // }
            }
        }
    }
}
