import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "../secondqml"
import "../components"
import "../qml"
DockItem {
    id: idDialog
    width: 730 * screenScaleFactor
    height: (Constants.languageType == 0 ? 260 : 210) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Max acceleration")
    property string version:""
    property string website: ""


    signal sigGenerate(var start,var end,var step,var speed, var temp)

    ListModel{
        id: _aModel
        ListElement{key:qsTr("Start acceleration");text: "500";value: "500";unit:"mm/s2";min:0;max:12000}
        ListElement{key:qsTr("End acceleration");text: "30000";value: "30000";unit:"mm/s2";min:1;max:100000}
        ListElement{key:qsTr("Step");text: "1000";value: "1000";unit:"mm/s2";min:1;max:1000}
        ListElement{key:qsTr("Speed");text: "300";value: "1000";unit:"mm/s2";min:0;max:9999999}
        ListElement{key:qsTr("Hot Plate Template");text: "50";value: "50";unit:"°C";min:1;max:200}
    }
     onVisibleChanged:{
        if(!visible) return
        const machine = kernel_parameter_manager.currentMachineObject
        const material_index = machine.extruderObject(0).materialIndex
        const material = machine.materialObject(material_index)
        const hotPlateTemp = material.basicDataModel().getDataItem('hot_plate_temp').value
        _aModel.setProperty(4,"text",hotPlateTemp+"")
        _aModel.setProperty(4,"value",hotPlateTemp+"")
    }
    UploadMessageDlg2 {
        id: flowWarning
        objectName: "flowWarning"
        property var receiver
        visible: false
        messageType:0
        cancelBtnVisible: false
        msgText: qsTr("Please enter a valid value") + "\n"+ qsTr("Starting value")+ 
        ":>=0;\n"+ qsTr("End value")+":>"+qsTr("Starting value")+";\n"+
        qsTr("End value")+":>"+qsTr("Starting value")+qsTr("Step")+";\n"+qsTr("Step")+":>1"
        onSigOkButtonClicked:flowWarning.visible = false
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


    Column
    {
        y:60* screenScaleFactor
        width: parent.width
        height: parent.height-20* screenScaleFactor
        spacing:15* screenScaleFactor
        Rectangle{
            implicitWidth: 690* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            implicitHeight:100* screenScaleFactor
            color: "transparent"
            Column{
                anchors.fill: parent
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
                            unitChar:model.unit  //qsTr("mm³/s")
                            validator:IntValidator {bottom: model.min; top: model.max;}
                            onTextChanged:{
                                if(min&&+text < min) text=""
                                if(+text > max) text = text.slice(0, text.length - 1);
                                _aModel.setProperty(index,"value",text)
                             }
                        }
                    }
                }
            }
        }
        Rectangle{
            height: 15*screenScaleFactor
            width: parent.width
            color: "transparent"
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
                let speed = +_aModel.get(3).value
                let temp = +_aModel.get(4).value
                if(startTemp>endTemp||endTemp<=(startTemp+step)||step<1||startTemp<0){
                    flowWarning.visible = true
                    return
                }
                sigGenerate(startTemp,endTemp,step,speed,temp)
                 close()
            }
        }

    }
}
