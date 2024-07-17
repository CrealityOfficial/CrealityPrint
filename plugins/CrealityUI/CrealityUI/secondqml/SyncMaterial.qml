import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import "qrc:/CrealityUI"
import ".."
import "../components"
import "../qml"
DockItem {
    id: idDialog
    width: 730 * screenScaleFactor
    height: (Constants.languageType == 0 ? 510 : 560) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Synchronize material information from printer")
    property string version:""
    property string website: ""
    property bool syncEnable:false
    onVisibleChanged: {
        printerListView.model = null
        printerListView.model = printerListModel
        idDialog.syncEnable = false
    }
    Column
    {
        y:60* screenScaleFactor
        width: parent.width
        height: parent.height-20* screenScaleFactor

        ScrollView{
            width:  690* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            height: 380* screenScaleFactor
            clip: true
            ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            Column{
                id: parentCol
                spacing: 10*screenScaleFactor
                anchors.fill: parent
                ButtonGroup { id: idMaterialGroup }
                Repeater{
                    id:printerListView
                    delegate: Rectangle{
                        property var hoverBorderColor: Constants.currentTheme ? Constants.themeGreenColor :"#6e6e73"
                        property var defaultBorderColor: Constants.currentTheme ? "#CBCBCC" :"#545458"
                        width: parent.width
                        height: flowLayout.height>0?  flowLayout.height + 20*screenScaleFactor :60*screenScaleFactor
                        border.width: radioBtn.checked?2:isHover ? 2: 1
                        border.color:radioBtn.checked? Constants.themeGreenColor: isHover? hoverBorderColor: defaultBorderColor
                        color: Constants.currentTheme? "#FFFFFF" :"#545458"
                        radius: 5* screenScaleFactor
                        visible: printerStatus == 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        property var isHover: false
                        MouseArea{
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.isHover = true
                            onExited: parent.isHover = false
                            onClicked: {
                                radioBtn.checked = true
                                idDialog.syncEnable = true

                            }

                        }

                        RowLayout{
                            anchors.fill: parent
                            RowLayout{
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                BasicRadioButton {
                                    property var printerModel:model
                                    property var printerId: printerListModel.getDeviceObject(model.index).pcPrinterID
                                    property int materialModelIndex:model.index
                                    id:radioBtn
                                    checked: idMaterialGroup.checkedButton === this
                                    ButtonGroup.group: idMaterialGroup
                                    padding: 0
                                    Layout.leftMargin: 6*screenScaleFactor

                                }
                                CusText{
                                    fontText: kernel_add_printer_model.getModelNameByCodeName(mItem.pcPrinterModel).trim()+ "/"+ ipAddr
                                    Layout.alignment: Qt.AlignVCenter|Qt.AlignLeft
                                    fontColor: Constants.themeTextColor
                                    fontPointSize:Constants.labelFontPointSize_9
                                    font.weight: Font.ExtraBold
                                    hAlign:0
                                    isDefault: true

                                }
                            }

                            Item{
                                Layout.fillWidth: true
                            }
                            Item {
                                Layout.preferredWidth : flowLayout.width
                                Layout.preferredHeight: flowLayout.height
                                Layout.alignment: Qt.AlignVCenter
                                Flow{
                                    id:flowLayout
                                    width: Math.min((kernel_parameter_manager.currentMachineObject.extrudersModel.rowCount())*45+5, 365)* screenScaleFactor
                                    spacing: 5*screenScaleFactor
                                    Repeater{
                                        model: materialList
                                        delegate:Rectangle{
                                            function setTextColor(){
                                                let bgColor = [color.r * 255, color.g * 255, color.b * 255]
                                                let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
                                                let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
                                                if(whiteContrast > blackContrast)
                                                    colorText.fontColor = "#ffffff"
                                                else
                                                    colorText.fontColor =  "#000000"
                                            }
                                            width: 40*screenScaleFactor
                                            height:  40*screenScaleFactor
                                            radius: 3*screenScaleFactor
                                            color: materialColor
                                            Component.onCompleted: setTextColor()
                                            Column{
                                                spacing: 2
                                                anchors.centerIn: parent
                                                width: colorText.width + materialTypeText.width
                                                height: colorText.height + materialTypeText.height +spacing
                                                CusText{
                                                    id:colorText
                                                    fontText: index + 1
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    fontPointSize:Constants.labelFontPointSize_9
                                                }
                                                CusText{
                                                    id:materialTypeText
                                                    fontColor:colorText.fontColor
                                                    fontText: materialType
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    fontPointSize:Constants.labelFontPointSize_8
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

        Rectangle{
            height: 20*screenScaleFactor
            width: parent.width
            color: "transparent"
        }
        RowLayout{
            Layout.preferredHeight: 30*screenScaleFactor
            Layout.preferredWidth: 230*screenScaleFactor
            Layout.alignment: Qt.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            // BasicDialogButton {
            //     Layout.preferredHeight: 30*screenScaleFactor
            //     Layout.preferredWidth: 100*screenScaleFactor

            //     text: qsTr("Synchronize")
            //     btnRadius:15*screenScaleFactor
            //     btnBorderW:1* screenScaleFactor
            //     defaultBtnBgColor: Constants.leftToolBtnColor_normal
            //     hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            //     onSigButtonClicked:
            //     {

            //         close()

            //     }
            // }
            BasicDialogButton {
                Layout.preferredHeight: 30*screenScaleFactor
                Layout.preferredWidth: 100*screenScaleFactor
                text: qsTr("Resync")
                btnRadius:15*screenScaleFactor
                btnBorderW:1* screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                enabled: idDialog.syncEnable
                onSigButtonClicked:
                {
                    let curIndex = idMaterialGroup.checkedButton.materialModelIndex
                    let printerModel = printerListModel.getDeviceObject(curIndex)
                    let materialColorModel = printerModel.materialBoxList

                    var idList = []
                    var colorList = []
                    for(let i =0;i<materialColorModel.rowCount();i++){
                        let materialType = materialColorModel.getMaterialType(printerModel.pcPrinterID,i)
                        let materialId = materialColorModel.getRfid(printerModel.pcPrinterID,i)
                        let materialColor = materialColorModel.getMaterialColor(printerModel.pcPrinterID,i)
                        idList.push(materialId)
                        colorList.push(materialColor)
                    }
                    kernel_parameter_manager.currentMachineObject.syncExtruders(idList, colorList)
                    close()

                }
            }
            BasicDialogButton {
                Layout.preferredHeight: 30*screenScaleFactor
                Layout.preferredWidth: 100*screenScaleFactor
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
