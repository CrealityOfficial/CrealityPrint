import "../components"
import "../secondqml"
import QtQuick 2.13
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.13
import QtGraphicalEffects 1.15

DockItem {
    id: control

    property var currentMachine: kernel_parameter_manager.currentMachineObject
    property var type: "all"
    property var receiver
    property alias model: idContentView.model
    property var msgOtherChange: qsTr("You have changed some settings of preset \"{1}\". Would you like to keep these changed settings (new value) after swittching presets?")
    property var msgAllChange: qsTr("You have changed some preset settings. Would you like to keep these changed settings (new value) after switching presets?")
    property var profileName: ""

    signal save()
    signal cancel()
    signal transfer()
    signal saved(var index)

    function getModelData() {
        var modelDataStr = kernel_parameter_manager.getModifyModelDataFromType(type);
        //        console.log(modelDataStr);
        return JSON.parse(modelDataStr);
    }

    function showSaveDialog() {
        saveParamSecond.visible = true;
    }

    function onSave(index) {
        visible = false;
        saved(index);
    }

    width: 620 * screenScaleFactor
    height: 618 * screenScaleFactor
    title: qsTr("Save Tips")
    onTypeChanged: {
        model = getModelData();
    }

    SaveParameterSecond {
        id: saveParamSecond

        visible: false
        type: control.type
        receiver: control
    }

    Item {
        anchors.fill: parent
        anchors.topMargin: 30 * screenScaleFactor
        anchors.margins: 20 * screenScaleFactor

        ColumnLayout {
            spacing: 2
            anchors.fill: parent

            Row {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 20 * screenScaleFactor

                Image {
                    sourceSize.width: 20 * screenScaleFactor
                    sourceSize.height: 20 * screenScaleFactor
                    source: "qrc:/UI/photo/lanPrinterSource/addPrinter_warning.png"
                }

                StyledLabel {
                    id: cloneNum

                    wrapMode: Text.WordWrap
                    width: 570 * screenScaleFactor
                    //Layout.fillWidth: true
                    text: type == "all" ? msgAllChange : msgOtherChange.replace("{1}", profileName)
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                }

            }

            Rectangle {
                color: "transparent"
                border.width: 1* screenScaleFactor
                border.color: Constants.darkThemeColor_secondary
                radius: 5 * screenScaleFactor
                Layout.preferredWidth: 570 * screenScaleFactor
                Layout.preferredHeight: 420 * screenScaleFactor
                //Layout.alignment: Qt.AlignHCenter
                layer.enabled: true
                layer.effect: OpacityMask{
                    maskSource: Rectangle{
                        width: 570 * screenScaleFactor
                        height: 420 * screenScaleFactor
                        radius: 5* screenScaleFactor
                    }
                }
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    RowLayout {
                        id: idTableRowTitle

                        Layout.fillWidth: true
                        spacing: 0
                        height: 20 * screenScaleFactor

                        Rectangle {
                            color: "#6B6B6B"
                            Layout.preferredWidth: 190 * screenScaleFactor
                            Layout.preferredHeight: 20 * screenScaleFactor

                            CusText {
                                hAlign: 0
                                fontElide: Text.ElideRight
                                fontColor: Constants.themeTextColor
                                anchors.centerIn: parent
                                fontText: qsTr("Settings")
                            }

                        }

                        Rectangle {
                            color: "#6B6B6B"
                            Layout.preferredWidth: 190 * screenScaleFactor
                            Layout.preferredHeight: 20 * screenScaleFactor

                            CusText {
                                hAlign: 0
                                fontElide: Text.ElideRight
                                fontColor: Constants.themeTextColor
                                anchors.centerIn: parent
                                fontText: qsTr("Old value")
                            }

                        }

                        Rectangle {
                            color: "#6B6B6B"
                            Layout.preferredWidth: 190 * screenScaleFactor
                            Layout.preferredHeight: 20 * screenScaleFactor
                            Layout.fillWidth: true

                            CusText {
                                hAlign: 0
                                fontElide: Text.ElideRight
                                fontColor: Constants.themeTextColor
                                anchors.centerIn: parent
                                fontText: qsTr("New value")
                            }

                        }

                    }

                    Rectangle {
                        color: "#EEEEEE"
                        Layout.preferredHeight: 20 * screenScaleFactor
                        Layout.fillWidth: true
                        visible: false //currentMachine.isCurrentProcessModified()

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Process")
                        }

                    }

                    ListView {
                        id: idContentView

                        clip: true
                        spacing: -1
                        Layout.fillWidth: true
                        Layout.preferredHeight: 400 * screenScaleFactor

                        delegate: ItemDelegate{
                            id: level1Com

                            width: parent.width
                            padding: 0
                            contentItem:ColumnLayout {
                                property int level: 1
                                spacing: -2

                                Rectangle {
                                    color: "transparent"
                                    border.width: 1 * screenScaleFactor
                                    border.color: Constants.darkThemeColor_secondary
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 20 * screenScaleFactor

                                    CusText {
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10 * 1 * screenScaleFactor
                                        hAlign: 0
                                        fontColor: Constants.themeTextColor
                                        fontElide: Text.ElideRight
                                        fontText: modelData.label
                                        font.weight: Font.Bold
                                    }

                                }

                                Repeater {
                                    id: level2List

                                    model: modelData.groups

                                    delegate: Item {
                                        id: level2Com

                                        Layout.fillWidth: true
                                        Layout.preferredHeight: level3List.height + 20 * screenScaleFactor

                                        ColumnLayout {
                                            property int level: 2

                                            anchors.fill: parent
                                            spacing: -1

                                            Rectangle {
                                                color: "transparent"
                                                border.width: 1 * screenScaleFactor
                                                border.color: Constants.darkThemeColor_secondary
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 20 * screenScaleFactor

                                                CusText {
                                                    anchors.left: parent.left
                                                    anchors.leftMargin: 10 * 2* screenScaleFactor
                                                    hAlign: 0
                                                    fontColor: Constants.themeTextColor
                                                    width: parent.width
                                                    fontElide: Text.ElideRight
                                                    fontText: modelData.label
                                                    font.weight: Font.Bold
                                                }

                                            }

                                            ListView {
                                                id: level3List

                                                model: modelData.children
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: count * 20 * screenScaleFactor

                                                delegate: Rectangle{
                                                    id: level3Com

                                                    property int level: 3
                                                    color:"transparent"
                                                    border.width: 1 * screenScaleFactor
                                                    border.color: Constants.darkThemeColor_secondary
                                                    width: parent.width
                                                    height: 20 * screenScaleFactor

                                                    RowLayout {
                                                        anchors.fill: parent
                                                        spacing: -1

                                                        Item {
                                                            Layout.preferredWidth: 190 * screenScaleFactor
                                                            Layout.preferredHeight: 20 * screenScaleFactor

                                                            CusText {
                                                                hAlign: 0
                                                                fontColor: Constants.themeTextColor
                                                                anchors.left: parent.left
                                                                anchors.leftMargin: 10 * 3* screenScaleFactor
                                                                width: parent.width
                                                                fontElide: Text.ElideRight
                                                                fontText: modelData.label
                                                                aliasText.maximumLineCount: 1
                                                            }

                                                        }

                                                        Item {
                                                            Layout.preferredWidth: 190 * screenScaleFactor
                                                            Layout.preferredHeight: 20 * screenScaleFactor

                                                            CusText {
                                                                hAlign: 0
                                                                fontColor: Constants.themeTextColor
                                                                width: parent.width
                                                                fontElide: Text.ElideRight
                                                                anchors.centerIn: parent
                                                                fontText: modelData.OlderValue
                                                                aliasText.maximumLineCount: 1
                                                            }

                                                        }

                                                        Item {
                                                            Layout.preferredWidth: 190 * screenScaleFactor
                                                            Layout.preferredHeight: 20 * screenScaleFactor

                                                            CusText {
                                                                hAlign: 0
                                                                fontColor: Constants.themeTextColor
                                                                width: parent.width
                                                                fontElide: Text.ElideRight
                                                                anchors.centerIn: parent
                                                                fontText: modelData.NewValue
                                                                aliasText.maximumLineCount: 1
                                                            }

                                                        }

                                                    }

                                                }

                                            }

                                        }

                                    }

                                }

                            }

                        }

                    }

                    Item {
                        Layout.fillHeight: true
                    }

                }

            }

            Row {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                BasicButton {
                    visible: type == "process"
                    width: 80 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    text: qsTr("Transfer")
                    btnRadius: 15 * screenScaleFactor
                    btnBorderW: 1* screenScaleFactor
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        transfer();
                    }
                }

                BasicButton {
                    width: 80 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    text: qsTr("Discard")
                    btnRadius: 15 * screenScaleFactor
                    btnBorderW: 1* screenScaleFactor
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        cancel();
                    }
                }

                BasicButton {
                    width: 80 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    text: qsTr("Save")
                    btnRadius: 15 * screenScaleFactor
                    btnBorderW: 1* screenScaleFactor
                    borderColor: Constants.lpw_BtnBorderColor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: {
                        save();
                    }
                }

            }

        }

    }

}
