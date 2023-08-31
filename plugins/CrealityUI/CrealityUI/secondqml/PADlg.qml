import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import CrealityUI 1.0
import "qrc:/CrealityUI"
import ".."
import "../qml"
DockItem {
    id: idDialog
    width: 878 * screenScaleFactor
    height: (Constants.languageType == 0 ? 388 : 338) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("PA calibration")
    property string version:""
    property string website: ""
    property var receiver
    signal sigTestType(var type)
    signal sigPaTower(var start,var end,var step)
    ListModel{
        id: _aModel
        ListElement{key:qsTr("Initial PA value");text: "0";value: "0"}
        ListElement{key:qsTr("End PA value");text: "0.1";value: "0.1"}
        ListElement{key:qsTr("PA step");text: "0.002";value: "0.002"}
    }
    function setValue(index)
    {
        const defaultValue = [[0,0.1,0.002],[0,1,0.02]]
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
        RowLayout{
            implicitWidth: 840* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10* screenScaleFactor
            BasicGroupBox{
                implicitWidth: 410* screenScaleFactor
                implicitHeight:70* screenScaleFactor
                title: qsTr("Extruder type")
                backRadius: 5
                borderWidth: 1
                borderColor: Constants.dialogItemRectBgBorderColor
                defaultBgColor: "transparent"
                textBgColor:  Constants.themeColor
                contentItem:Grid{
                    id:extruderList
                    width: parent.width
                    columns: 5
                    columnSpacing: 30*screenScaleFactor
                    rowSpacing: 10*screenScaleFactor
                    height: parent.height // 设置列表项高度
                    anchors.fill: parent
                    anchors.margins: 20*screenScaleFactor
                    Repeater{
                        model: [qsTr("Proximity extruder"), qsTr("Remote extruder")]
                        delegate:RadioDelegate{
                            id: extruderRadioControl
                            implicitWidth: 100*screenScaleFactor
                            implicitHeight: 20*screenScaleFactor
                            checked: index ===0
                            indicator: Rectangle{
                                implicitHeight: 10*screenScaleFactor
                                implicitWidth: 10*screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter
                                radius: 5*screenScaleFactor
                                color:extruderRadioControl.checked ? "#00a3ff" : "transparent"
                                border.color: (extruderRadioControl.checked || extruderRadioControl.hovered)? "#00a3ff" : "#cbcbcc"
                                border.width: 1* screenScaleFactor
                                Rectangle {
                                    width: 4*screenScaleFactor
                                    height: 4*screenScaleFactor
                                    anchors.centerIn: parent
                                    radius: 2*screenScaleFactor
                                    color: "#ffffff"
                                    visible: extruderRadioControl.checked
                                }
                            }
                            background: Rectangle {
                                color: "transparent"
                                Text{
                                    text: modelData
                                    opacity: enabled ? 1.0 : 0.3
                                    color:(extruderRadioControl.checked || extruderRadioControl.hovered)? "#00a3ff" :  Constants.manager_printer_switch_default_color
                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCente
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
            BasicGroupBox{
                implicitWidth: 410* screenScaleFactor
                implicitHeight:70* screenScaleFactor
                title: qsTr("Test methods")
                backRadius: 5
                borderWidth: 1
                borderColor: Constants.dialogItemRectBgBorderColor
                defaultBgColor: "transparent"
                textBgColor: Constants.themeColor
                contentItem:Grid{
                    id:testList
                    width: parent.width
                    columns: 5
                    columnSpacing: 30*screenScaleFactor
                    rowSpacing: 10*screenScaleFactor
                    height: parent.height // 设置列表项高度
                    anchors.fill: parent
                    anchors.margins: 20*screenScaleFactor
                    Repeater{
                        model: [qsTr("PA tower"),qsTr("PA dash")]
                        delegate:RadioDelegate{
                            id: testRadioControl
                            implicitWidth: 100*screenScaleFactor
                            implicitHeight: 20*screenScaleFactor
                            checked: index ===0
                            indicator: Rectangle{
                                implicitHeight: 10*screenScaleFactor
                                implicitWidth: 10*screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter
                                radius: 5*screenScaleFactor
                                color:testRadioControl.checked ? "#00a3ff" : "transparent"
                                border.color: (testRadioControl.checked || testRadioControl.hovered)? "#00a3ff" : "#cbcbcc"
                                border.width: 1* screenScaleFactor
                                Rectangle {
                                    width: 4*screenScaleFactor
                                    height: 4*screenScaleFactor
                                    anchors.centerIn: parent
                                    radius: 2*screenScaleFactor
                                    color: "#ffffff"
                                    visible: testRadioControl.checked
                                }
                            }
                            background: Rectangle {
                                color: "transparent"
                                Text{
                                    text: modelData
                                    opacity: enabled ? 1.0 : 0.3
                                    color:(testRadioControl.checked || testRadioControl.hovered)? "#00a3ff" :  Constants.manager_printer_switch_default_color
                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCente
                                    horizontalAlignment: Text.AlignLeft
                                    x:18*screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                            onClicked:
                            {
                                sigTestType(index)
                                console.log("index =",index)
                            }
                        }
                    }
                }
            }
        }
        BasicGroupBox{
            implicitWidth: 840* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            implicitHeight:176* screenScaleFactor
            title: qsTr("Set up")
            backRadius: 5
            borderWidth: 1
            borderColor: Constants.dialogItemRectBgBorderColor
            defaultBgColor: "transparent"
            textBgColor: Constants.themeColor
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
                            unitChar:  qsTr("")
                            validator: RegExpValidator { regExp: /(\d{1,6})([.,]\d{1,6})?$/ }
                            onTextChanged: {
                                 _aModel.setProperty(index,"value", +text)
                            }
                        }
                    }
                }
//                CheckBox{
//                    implicitHeight: 16* screenScaleFactor
//                    implicitWidth: 16* screenScaleFactor
//                    id: control
//                    contentItem: Label{
//                        text: qsTr("Print numbers")
//                        color: Constants.themeTextColor
//                        horizontalAlignment: Text.AlignLeft
//                        verticalAlignment: Text.AlignVCenter
//                        leftPadding: control.indicator.width
//                        font.pointSize: Constants.labelFontPointSize_10
//                    }
//                    indicator:Item {
//                        implicitWidth: 16* screenScaleFactor
//                        implicitHeight: 16* screenScaleFactor
//                        anchors.verticalCenter: parent.verticalCenter
//                        Rectangle {
//                            implicitWidth: parent.width
//                            implicitHeight: parent.width
//                            anchors.verticalCenter: parent.verticalCenter
//                            radius: 3
//                            opacity: control.enabled ? 1 : 0.7
//                            border.color: control.checked || control.hovered ? Constants.textRectBgHoveredColor
//                                                                             : Constants.dialogItemRectBgBorderColor
//                            border.width: 1
//                            color: control.checked ? "#009CFF" : "transparent"
//                            Image {
//                                id: checkbox_image
//                                width: 9* screenScaleFactor
//                                height: 6* screenScaleFactor
//                                anchors.centerIn: parent
//                                source: control.checked ? "qrc:/UI/images/check2.png" : ""
//                                visible: control.checked
//                            }
//                        }
//                    }
//                }
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
            onClicked: {
                close()
                sigPaTower(_aModel.get(0).value,_aModel.get(1).value,_aModel.get(2).value)
            }
        }
    }
}
