import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "../components"
import "../qml"
import "../secondqml"
DockItem {
    id: idDialog
    width: 730 * screenScaleFactor
    height: (Constants.languageType == 0 ? 428 : 378) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Temperature calibration")


    signal sigTemp(var start,var end,var step,var temp)

    ListModel{
        id: _aModel
        ListElement{key:qsTr("Starting temperature:");text: 230;value: 230}
        ListElement{key:qsTr("End temperature:");text: 190;value: 190}
        ListElement{key:qsTr("Temperature step:");text: 5;value: 5}
        ListElement{key:qsTr("Hot Plate Template");text: "50";value: "50";unit:"°C";min:1;max:200}
    }
    onVisibleChanged:{
        if(!visible) return
        const machine = kernel_parameter_manager.currentMachineObject
        const material_index = machine.extruderObject(0).materialIndex
        const material = machine.materialObject(material_index)
        const hotPlateTemp = material.basicDataModel().getDataItem('hot_plate_temp').value
        _aModel.setProperty(3,"text",hotPlateTemp+"")
        _aModel.setProperty(3,"value",hotPlateTemp+"")
    }
    

    property string emptyStr: ""
    UploadMessageDlg2 {
        id: notEmpty
        objectName: "flowWarning"
        property var receiver
        visible: false
        messageType:0
        cancelBtnVisible: false
        msgText: emptyStr + qsTr("cannot be empty")
        onSigOkButtonClicked:notEmpty.visible = false
    }


    function setValue(index)
    {
        const defaultValue = [[230,190,5],[270,230,5],[260,230,5],[240,210,5],[320,280,5],[320,280,5]]
            _aModel.setProperty(0,"text", defaultValue[index][0])
            _aModel.setProperty(1,"text", defaultValue[index][1])
            _aModel.setProperty(2,"text", defaultValue[index][2])
            _aModel.setProperty(0,"value", defaultValue[index][0])
            _aModel.setProperty(1,"value", defaultValue[index][1])
            _aModel.setProperty(2,"value", defaultValue[index][2])
    }


    Column
    {
        y:60* screenScaleFactor
        width: parent.width
        height: parent.height-20* screenScaleFactor
        spacing:15* screenScaleFactor
        BasicGroupBox{
            implicitWidth: 680* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            implicitHeight:100* screenScaleFactor
            title: qsTr("Material type")
            backRadius: 5
            borderWidth: 1
            borderColor: Constants.dialogItemRectBgBorderColor
            defaultBgColor: "transparent"
            textBgColor: Constants.themeColor
            contentItem:BasicScrollView{
                vpolicyVisible: materialList.height > parent.height
                anchors.fill: parent
                anchors.margins: 20*screenScaleFactor
                Grid{
                    id:materialList
                    width: parent.width
                    columns: 5
                    columnSpacing: 30*screenScaleFactor
                    rowSpacing: 10*screenScaleFactor
                    height: 100 // 设置列表项高度
                    Repeater{
                        model: ["PLA", "ABS", "PETG","TPU", "PA-CF","PET-CF"]
                        delegate:RadioDelegate{
                            id: materialRadioControl
                            implicitWidth: 100*screenScaleFactor
                            implicitHeight: 20*screenScaleFactor
                            checked: index ===0
                            indicator: Rectangle{
                                implicitHeight: 10*screenScaleFactor
                                implicitWidth: 10*screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter
                                radius: 5*screenScaleFactor
                                color:materialRadioControl.checked ? "#00a3ff" : "transparent"
                                border.color: (materialRadioControl.checked || materialRadioControl.hovered)? "#00a3ff" : "#cbcbcc"
                                border.width: 1* screenScaleFactor

                                Rectangle {
                                    width: 4*screenScaleFactor
                                    height: 4*screenScaleFactor
                                    anchors.centerIn: parent
                                    radius: 2*screenScaleFactor
                                    color: "#ffffff"
                                    visible: materialRadioControl.checked
                                }

                            }
                            background: Rectangle {
                                color: "transparent"
                                Text{
                                    text: modelData
                                    opacity: enabled ? 1.0 : 0.3
                                    color:(materialRadioControl.checked || materialRadioControl.hovered)? "#00a3ff" :  Constants.manager_printer_switch_default_color
                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                    x:18*screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            onClicked:
                            {
                                setValue(index)
                                console.log("index =",index)
                            }
                        }
                    }
                }

            }
        }

        BasicGroupBox{
            implicitWidth: 680* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            implicitHeight:180* screenScaleFactor
            title: qsTr("Set up")
            backRadius: 5
            borderWidth: 1
            borderColor: Constants.dialogItemRectBgBorderColor
            defaultBgColor: "transparent"
            textBgColor:  Constants.themeColor
            contentItem:  Column{
                anchors.fill: parent
                anchors.margins: 20*screenScaleFactor
                id: parentCol
                spacing: 5*screenScaleFactor
                Repeater{
                    model:_aModel

                    delegate: RowLayout{
                        width: parentCol.width
                        StyledLabel{
                            activeFocusOnTab: false
                            Layout.preferredWidth: 200* screenScaleFactor
                            Layout.preferredHeight: 30* screenScaleFactor
                            text:model.key
                            font.pointSize:Constants.labelFontPointSize_10
                            color: Constants.themeTextColor
                        }

                        Item{
                            Layout.fillWidth: true
                        }

                        BasicDialogTextField{
                            id: proField
                            Layout.preferredWidth: 260* screenScaleFactor
                            Layout.preferredHeight: 28* screenScaleFactor
                            radius: 5* screenScaleFactor
                            text:model.text
                            readOnly: index === 2
                            unitChar:  qsTr("°C")
                         //   validator: RegExpValidator { regExp:  textEditRegExp?textEditRegExp:/.*/}
                            onTextChanged: {
                                 _aModel.setProperty(index,"value", text)
                            }
                        }
                    }
                }
            }
        }

        BasicDialogButton {
            anchors.horizontalCenter: parent.horizontalCenter
            id: basicComButton
            width: 100*screenScaleFactor
            height: 30*screenScaleFactor
            text: qsTr("OK")
            btnRadius:15*screenScaleFactor
            btnBorderW:1* screenScaleFactor
            defaultBtnBgColor: Constants.leftToolBtnColor_normal
            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            onSigButtonClicked:
            {
                for(let i =0;i<_aModel.count;i++)
                {
                    if(!_aModel.get(i).value){
                        emptyStr = _aModel.get(i).key
                        notEmpty.visible = true
                        return
                    }

                }
                let startTemp = +_aModel.get(0).value
                let endTemp = +_aModel.get(1).value
                let step = +_aModel.get(2).value
                let temp = +_aModel.get(3).value
                if(startTemp<=endTemp){
                    tempWarning.visible = true
                    return
                }
                close()
                sigTemp(startTemp,endTemp,step,temp)
                idDialog.visible = false

            }
        }
        UploadMessageDlg2 {
              id: tempWarning
              objectName: "tempWarning"
              property var receiver
              visible: false
              messageType:0
              cancelBtnVisible: false
              msgText: qsTr("The end temperature cannot exceed the start temperature")
              onSigOkButtonClicked:tempWarning.visible = false
          }

    }
}
