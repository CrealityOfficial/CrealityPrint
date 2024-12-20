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
    title: qsTr("圆弧拟合")
    property string version:""
    property string website: ""


    signal sigGenerate(var start,var end,var step)

    ListModel{
        id: _aModel
        ListElement{key:qsTr("圆弧起始值");text: "5";value: "5"}
        ListElement{key:qsTr("圆弧结束值");text: "15";value: "15"}
        ListElement{key:qsTr("圆弧拟合增值");text: "1";value: "1"}
    }
    UploadMessageDlg2 {
        id: flowWarning
        objectName: "flowWarning"
        property var receiver
        visible: false
        messageType:0
        cancelBtnVisible: false
        msgText: qsTr("The starting flow rate cannot exceed the ending flow rate")
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
        msgText: emptyStr + "不能为空"
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
                            unitChar:  qsTr("mm³/s")
                            onTextChanged:{
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
                let startTemp = _aModel.get(0).value
                let endTemp = _aModel.get(1).value
                if(+startTemp>+endTemp){
                    flowWarning.visible = true
                    return
                }
                close()
                sigGenerate(_aModel.get(0).value,_aModel.get(1).value,_aModel.get(2).value)
            }
        }

    }
}
