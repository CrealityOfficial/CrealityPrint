import ".."
import "../components"
import "../qml"
import Qt.labs.platform 1.1
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Flow {
    id: root

    property alias itemCount: colorRecRepeater.count
    property alias checkedButton: btnGroup.checkedButton
    property var currentMachine
    property string extruder0Material

    padding: 0
    spacing: 0

    ButtonGroup {
        id: btnGroup
    }

    Repeater {
        id: colorRecRepeater
        model: currentMachine.extrudersModel
        delegate: Button {
            id: wrapperRec

            property color adaptColor
            property bool isHover: false
            property alias color: colorPickBtn1.color
            property alias isExpand: materialTypeItem.isExpend

            function setColor(itemColor) {
                currentMachine.setExtruderColor(model.index, itemColor);
            }

            function setTextColor() {
                let bgColor = [model.extruderColor.r * 255, model.extruderColor.g * 255, model.extruderColor.b * 255];
                let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
                console.log("whiteContrast = ", whiteContrast);
                let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
                console.log("blackContrast = ", blackContrast);
                if (whiteContrast > blackContrast) {
                    colorText.color = "#ffffff";
                    materialTypeText.color = "#ffffff";
                    adaptColor = "#ffffff";
                    materialTypeItem.isBlack = false;
                } else {
                    colorText.color = "#000000";
                    materialTypeText.color = "#000000";
                    adaptColor = "#000000";
                    materialTypeItem.isBlack = true;
                }
                setBorderColor();
            }

            function setBorderColor() {
                let bgColor = [model.extruderColor.r * 255, model.extruderColor.g * 255, model.extruderColor.b * 255];
                let whiteContrast = Constants.getContrast(bgColor, [Constants.right_panel_bgColor.r*255, Constants.right_panel_bgColor.g*255, Constants.right_panel_bgColor.b*255]);
                if(whiteContrast < 1.04){
                    colorPickBtn1.border.width = 1 * screenScaleFactor
                }else{
                    colorPickBtn1.border.width = 0 * screenScaleFactor
                }
            }

            checkable: true
            ButtonGroup.group: btnGroup
            padding: 0
            height: 48 * screenScaleFactor
            width: colorRecRepeater.count <= 8 ? height * 2 + root.spacing + 8* screenScaleFactor :
                                                 height + 5* screenScaleFactor

            background: Rectangle {
                radius: 5 * screenScaleFactor
                color: "transparent"
                border.width: 2 * screenScaleFactor
                border.color: wrapperRec.checked ? Constants.themeColor_New : "transparent"
            }

            contentItem: Rectangle {
                id: colorPickBtn1

                property alias background: wrapperRec.background
                property color btnColor: model.extruderColor

                anchors.fill: background
                anchors.margins: background.border.width + 1 * screenScaleFactor
                color: btnColor
                radius: background.radius

                border.color: "black"
                border.width: 0 * screenScaleFactor

                onColorChanged: {
                    wrapperRec.setTextColor();
                }

                Rectangle {
                    id: topRec

                    property bool isHovered: false

                    radius: 5 * screenScaleFactor
                    color: "transparent"
                    width: parent.width
                    height: parent.height / 2
                    anchors.top: parent.top
                    border.width: topRec.isHovered ? 1 : 0
                    border.color: wrapperRec.adaptColor

                    Text {
                        id: colorText

                        text: model.index + 1
                        font.family: Constants.labelFontFamily
                        anchors.centerIn: parent
                    }

                    MouseArea {
                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: {
                            wrapperRec.checked = true;
                            var color = kernel_kernelui.showColorDialog(wrapperRec.color, frameLessView);
                            if(color==wrapperRec.color)
                            {
                                return;
                            }
                            wrapperRec.setColor(color);
                            //                            wrapperRec.color = color;
                            wrapperRec.setTextColor();
                            let colorList = kernel_parameter_manager.currentMachineObject.extrudersModel.colorList();
                            currentMachine.setFilamentColor(colorList);
                        }
                        onExited: {
                            topRec.isHovered = false;
                            wrapperRec.isHover = false;
                        }
                        onEntered: {
                            topRec.isHovered = true;
                            wrapperRec.isHover = true;
                            console.log("+++index = ", model.index);
                        }
                    }

                }

                Rectangle {
                    id: materialTypeItem

                    property bool isExpend: false
                    property bool isHovered: false
                    property bool isBlack: false
                    property string upIndicatorImg: isBlack ? "qrc:/UI/photo/rightDrawer/upBtn_black.svg" : "qrc:/UI/photo/rightDrawer/upBtn_white.svg"
                    property string downIndicatorImg: isBlack ? "qrc:/UI/photo/lanPrinterSource/downBtn_black.svg" : "qrc:/UI/photo/lanPrinterSource/downBtn_white.svg"
                    property string indicatorImg: isExpend ? upIndicatorImg : downIndicatorImg

                    color: "transparent"
                    radius: 5 * screenScaleFactor
                    width: parent.width
                    height: parent.height / 2
                    anchors.bottom: parent.bottom
                    border.width: materialTypeItem.isHovered ? 1 : 0
                    border.color: wrapperRec.adaptColor

                    Row{
                        anchors.centerIn: parent
                        spacing: 3 * screenScaleFactor
                        Text {
                            property real maxWidth: materialTypeItem.width - 10 * screenScaleFactor
                            id: materialTypeText
                            elide: Text.ElideRight
                            //                            width: contentWidth
                            anchors.verticalCenter: parent.verticalCenter
                            text: model.curMaterialObj.materialType
                            horizontalAlignment:Text.AlignRight
                            font.family: Constants.labelFontFamily
                            Component.onCompleted: {
                                wrapperRec.setTextColor();
                            }

                            onContentWidthChanged: {
                                if(contentWidth > maxWidth)
                                    width = maxWidth
                            }

                            Connections{
                                target: colorRecRepeater
                                onCountChanged:{
                                    if(colorRecRepeater.count > 8){
                                        if(materialTypeText.contentWidth > materialTypeText.maxWidth)
                                            materialTypeText.width = materialTypeText.maxWidth
                                    }else{
                                    }
                                }
                            }

                            BasicTooltip {
                                id: idTooptip
                                text: materialTypeText.text
                                visible: false
                            }
                        }

                        Image {
                            id: foldImg
                            anchors.verticalCenter: parent.verticalCenter
                            source: materialTypeItem.indicatorImg
                        }
                    }

                    MouseArea {
                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: {
                            materialTypeItem.isExpend = !materialTypeItem.isExpend;
                            wrapperRec.checked = true;
                            chosedMaterial.colorBtn = wrapperRec;
                            chosedMaterial.extruderObj = model.extruderObj;
                            chosedMaterial.extruderIndex = model.index;
                            chosedMaterial.visible = materialTypeItem.isExpend;
                        }
                        onEntered: {
                            materialTypeItem.isHovered = true;
                            idTooptip.visible = true
                        }
                        onExited: {
                            materialTypeItem.isHovered = false;
                            idTooptip.visible = false
                        }
                    }

                }

            }

        }

    }
    Popup {
        id: chosedMaterial

        property alias materialIndex: idExtruder11.currentIndex
        property alias materialListModel: idExtruder11.model
        property int extruderIndex: 0
        property var colorBtn
        property var button: parent.checkedButton
        property var extruderObj: null

        y: button ? button.y + button.height : parent.height
        width: parent.width
        height: 24 * screenScaleFactor
        visible: false
        onVisibleChanged: {
            if (!visible) {
                colorBtn.checked = false;
                colorBtn.isExpand = false;
            }else{
                currentMachine.materialsModel.refresh()
            }
        }

        background: Rectangle {
            radius: 5 * screenScaleFactor
            border.width: 1 * screenScaleFactor
            border.color: Constants.themeGreenColor
            color: Constants.right_panel_item_default_color
        }

        contentItem: RowLayout {
            anchors.fill: parent
            spacing: -1

            CXComboBox {
                id: idExtruder11
                currentIndex: chosedMaterial.extruderObj.materialIndex
                showCount: 20
                radius: 0
                clip: true
                sectionProperty: "defaultOrSync"
                textRole: "name"
                valueRole: "materialColor"
                itemBorderColor: "transparent"
                Layout.fillWidth: true
                Layout.preferredHeight: 24 * screenScaleFactor
                model: !currentMachine ? null : currentMachine.materialsModel
                onActivated: {
                    currentMachine.setExtruderMaterial(chosedMaterial.extruderIndex, currentIndex);
                    checkParam();
                }

                onCurrentTextChanged: {
                    extruder0Material = currentText;
                    kernel_ui_parameter.focusedMaterialName = idExtruder11.currentText + "-1.75"
                    //                    extruderTempText.fontText = currentMachine.materialObject(kernel_ui_parameter.focusedMaterialName).extruderTemp + "℃"
                    //                    bedTempText.fontText = currentMachine.materialObject(kernel_ui_parameter.focusedMaterialName).bedTemp + "℃"
                }

                slotItem: RowLayout {
                    spacing: 2

                    CusButton {
                        id: addBtn

                        Layout.fillWidth: true
                        Layout.preferredHeight: 24 * screenScaleFactor
                        radius: 5
                        txtContent: qsTr("Add") + "/" + qsTr("Delete")
                        txtColor: enabled ? Constants.textColor : Qt.lighter(Constants.textColor, 0.8)
                        enabled: !currentMachine.isImportedMachine()
                        onClicked: {
                            idExtruder11.popup.visible = false;
                            let materialName = currentMachine.materialsNameList()[idExtruder11.currentIndex];
                            showAddMaterial(chosedMaterial.extruderIndex, materialName, this);
                            chosedMaterial.close()
                        }
                    }

                }
            }

            Rectangle {
                Layout.preferredWidth: 1 * screenScaleFactor
                Layout.fillHeight: true
                color: Constants.themeGreenColor
            }

            Item {
                Layout.preferredWidth: 80 * screenScaleFactor
                Layout.fillHeight: true

                Row {
                    anchors.centerIn: parent
                    spacing: 5 * screenScaleFactor

                    Image {
                        id: name

                        source: !Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/extruderTemp.svg" : "qrc:/UI/photo/rightDrawer/extruderTemp_black.svg"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    CusText {
                        id: extruderTempText
                        fontText: {
                            let materialObj = currentMachine.materialObject(idExtruder11.currentText + "-1.75");
                            return materialObj.extruderTemp + "℃"
                        }
                        hAlign: 0
                        fontColor: Constants.textColor
                        fontFamily: Constants.labelFontFamily
                        isDefault: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1 * screenScaleFactor
                Layout.fillHeight: true
                color: Constants.themeGreenColor
            }

            Item {
                Layout.preferredWidth: 80 * screenScaleFactor
                Layout.fillHeight: true

                Row {
                    anchors.centerIn: parent
                    spacing: 5 * screenScaleFactor

                    Image {
                        id: bedTempImg

                        source: !Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/bedTemp.svg" : "qrc:/UI/photo/rightDrawer/bedTemp_black.svg"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    CusText {
                        id: bedTempText
                        hAlign: 0
                        fontText:  {
                            let materialObj = currentMachine.materialObject(idExtruder11.currentText + "-1.75");
                            let type = kernel_parameter_manager.currBedType()
                            return materialObj.bedTemp + "℃";
                        }
                        fontColor: Constants.textColor
                        fontFamily: Constants.labelFontFamily
                        isDefault: true
                        anchors.verticalCenter: parent.verticalCenter
                    }

                }

            }

            Rectangle {
                Layout.preferredWidth: 2 * screenScaleFactor
                Layout.fillHeight: true
                color: Constants.themeGreenColor
            }

            CusButton {
                Layout.preferredWidth: 22 * screenScaleFactor
                Layout.preferredHeight: 22 * screenScaleFactor
                normalColor: Constants.right_panel_button_default_color
                hoveredColor: Constants.right_panel_border_default_color
                pressedColor: Constants.right_panel_border_default_color
                toolTipText: qsTr("Edit")
                radius: 5 * screenScaleFactor
                allRadius: false
                rightTop: true
                rightBottom: true
                borderWidth: 1
                onClicked: {
                    if (!materialManagerLoader.active)
                        materialManagerLoader.active = true;

                    materialManagerLoader.item.focusedMaterialIndex = idExtruder11.currentIndex;
                    materialManagerLoader.item.focusedProcessIndex = process_combo_box.currentIndex;
                    materialManagerLoader.item.curExtruderObj = chosedMaterial.extruderObj
                    materialManagerLoader.item.show();
                    chosedMaterial.visible = false
                }

                Image {
                    anchors.centerIn: parent
                    source: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/edit_light_default.svg" : "qrc:/UI/photo/rightDrawer/profile_editBtn.svg"
                }

            }

            Item {
                Layout.preferredWidth: 2 * screenScaleFactor
                Layout.fillHeight: true
            }

        }

    }

}
