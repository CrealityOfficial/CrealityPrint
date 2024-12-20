import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import "qrc:/CrealityUI"
import "../secondqml"
import "../components"
import "../qml"
DockItem {
    id: idDialog
    width: 730 * screenScaleFactor
    height: (Constants.languageType == 0 ? 580 : 530) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Fine tune selection")
    property string version:""
    property string website: ""
    signal onWebsite
    property int flowSelect: -1
    signal sigGenerate()
    signal getTuningTutorial()
    ListModel{
        id: _aModel
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_10.png"
            value: 10
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_-5.png"
            value: -5
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_-20.png"
            value: -20
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_15.png"
            value: 15
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_0.png"
            value: 0
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_-15.png"
            value: -15
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_20.png"
            value: 20
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_5.png"
            value: 5
        }
        ListElement{
            img: "qrc:/UI/photo/calibration/FlowFineTune_-10.png"
            value: -10
        }
    }

    property int flowValue: -1
    Column
    {
        y:60* screenScaleFactor
        width: parent.width
        height: parent.height-20* screenScaleFactor
        spacing:15* screenScaleFactor
        RowLayout{
            width:  tipText.width + tipImg.width //160* screenScaleFactor
            height: 30* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            Text {
                id:tipText
                text: qsTr("Choose coarse one")
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_10
                font.family: Constants.labelFontFamilyBold
            }
            Image {
                id: tipImg
                width: 18* screenScaleFactor
                height: 18* screenScaleFactor
                source: "qrc:/UI/photo/calibration/FlowFineTune_tip.svg"
                MouseArea{
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: getTuningTutorial()
                }
            }
        }
        Rectangle{
            width: 500* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            height:350* screenScaleFactor
            color: "transparent"
            Grid {
                anchors.fill: parent
                columnSpacing: 37*screenScaleFactor
                rowSpacing: 5*screenScaleFactor
                columns: 3
                Repeater{
                    model:_aModel
                    delegate: CusImglButton
                    {
                        width: 144* screenScaleFactor
                        height: 115* screenScaleFactor
                        imgWidth:  112*screenScaleFactor
                        imgHeight: 91*screenScaleFactor
                        shadowEnabled : false
                        opacity : enabled ? 1 : 0.7
                        defaultBtnBgColor:"transparent"
                        hoveredBtnBgColor: "transparent"
                        borderWidth: hovered || bottonSelected ? 2 : 0
                        hoverBorderColor: "#6E6E73"
                        selectedBtnBgColor: "transparent"
                        selectBorderColor:"#00A3FF"
                        font.family: Constants.labelFontFamily
                        font.weight: Constants.labelFontWeight
                        enabledIconSource: model.img
                        highLightIconSource: model.img
                        pressedIconSource: model.img
                        bottonSelected : idDialog.flowSelect === index
                        onClicked:
                        {
                            idDialog.flowSelect = index
                            flowValue = _aModel.get(index).value
                        }
                    }
                }
            }
        }
        Rectangle{
            height: 20*screenScaleFactor
            width: parent.width
            color: "transparent"
        }

        Grid{
            width : basicComButton.width + basicComButton2.width + spacing
            height: 30*screenScaleFactor
            columns: 2
            spacing: 10*screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            BasicDialogButton {
                id: basicComButton
                width: 100*screenScaleFactor
                height: parent.height
                text: qsTr("OK")
                btnRadius:15*screenScaleFactor
                opacity: flowValue === -1 ? 0.5 : 1
                btnBorderW:1* screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                   if(flowValue === -1 ) return
                    // flowValue：传的值
                    close()
                    sigGenerate(flowValue)
                }
            }
            BasicDialogButton {
                id: basicComButton2
                width: 100*screenScaleFactor
                height:  parent.height
                text: qsTr("Cancel")
                btnRadius:15*screenScaleFactor
                btnBorderW:1* screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                     close()
                }
            }
        }



    }
}
