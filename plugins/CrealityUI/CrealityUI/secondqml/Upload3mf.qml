import "../components"
import "../qml"
import "../secondqml"
import "qrc:/CrealityUI/cxcloud"
import QtGraphicalEffects 1.12
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtQuick.Window 2.13

DockItem{
    id: root
    width: 902* screenScaleFactor
    height: 737* screenScaleFactor
    title: qsTr("Upload project files to Creality Cloud")
    property string textColor: Constants.currentTheme ? "#333333" : "#ffffff"
    property string edit:  "qrc:/UI/photo/upload3mf/edit_name_dark.svg"
    property string slice:  "qrc:/UI/photo/upload3mf/slice_dark.svg"
    property string empty:  "qrc:/UI/photo/upload3mf/empty_dark.svg"
    property string plate:  "qrc:/UI/photo/upload3mf/plate_dark.svg"
    property string printer:  "qrc:/UI/photo/upload3mf/printer_dark.svg"
    property string profile:  "qrc:/UI/photo/upload3mf/profile_dark.svg"
    property string slice_msg:  "qrc:/UI/photo/upload3mf/slice_msg_dark.svg"
    property string material:  "qrc:/UI/photo/upload3mf/material_dark.svg"
    property string logo:  "qrc:/UI/photo/upload3mf/logo.svg"
    property string no_slice:  "qrc:/UI/photo/upload3mf/no_slice.svg"
    property var extruder_list : []

    function setTextColor(color) {
        let bgColor = [color.r * 255, color.g * 255, color.b * 255];
        let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
        let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
        let calc;
        if (whiteContrast > blackContrast) {
            calc = "#ffffff";

        } else {
            calc = "#000000";
        }
        return calc;
    }
    function findNumberForMultipleOfThree(n) {
        let sum = 0;
        let temp = n;
        while (temp !== 0) {
            sum += temp % 10;
            temp = Math.floor(temp / 10);
        }
        let m = 3 - (sum % 3);
        if (m === 3) {
            m = 0; 
        }
        return m;
    }

    function getEmptyModel(){        
        let count=root.rowCount;
        if(count===1) return 0
        if(count<=6&&count>1){
            return 6 - count
        }else{
            return root.findNumberForMultipleOfThree(count)
        }
    }

    property int rowCount
    onVisibleChanged:{
        if (!root.visible) {
            return
        }
        kernel_slice_flow.onPictureUpdated()
        rowCount =  kernel_slice_flow.slicePreviewListModel.rowCount()
        extruder_list = kernel_printer_manager.usedExtruders()
        empty_repeater.model = getEmptyModel()
    }

    ColumnLayout{
        height: root.height - 60* screenScaleFactor
        width: parent.width
        anchors.top: parent.top
        anchors.topMargin: 20* screenScaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 20* screenScaleFactor
        spacing: 5* screenScaleFactor

        RowLayout {
            Layout.preferredHeight: 20 * screenScaleFactor
            Layout.preferredWidth: idGcodeFileName.width + 20 * screenScaleFactor
            Layout.topMargin: 20* screenScaleFactor
            TextField {
                id: idGcodeFileName

                property var editGcodeReadOnly: true

                readOnly: editGcodeReadOnly
                background: null
                Layout.preferredWidth: contentWidth + 30 * screenScaleFactor
                Layout.preferredHeight: 30 * screenScaleFactor
                text: {
                    if (!root.visible) {
                        return ""
                    }

                    return kernel_project_ui.getProjectNameNoSuffix()
                }
                maximumLength: 100
                selectByMouse: true
                selectionColor: Constants.currentTheme ? "#98DAFF" : "#1E9BE2"
                selectedTextColor: color
                leftPadding: 10 * screenScaleFactor
                rightPadding: 10 * screenScaleFactor
                verticalAlignment: TextInput.AlignVCenter
                horizontalAlignment: TextInput.AlignLeft
                color: Constants.currentTheme ? "#333333" : "#CBCBCC"
                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                Keys.onEnterPressed: editGcodeReadOnly = true
                Keys.onReturnPressed: editGcodeReadOnly = true
                activeFocusOnPress: {
                    if (!activeFocus)
                        editGcodeReadOnly = true;

                }

                validator: RegExpValidator {
                    regExp: /^.{100}$/
                }

            }

            Rectangle {
                id: editGroupRect

                color: "transparent"
                width: 14 * screenScaleFactor
                height: 14 * screenScaleFactor
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.centerIn: parent
                    source:edit
                }

                MouseArea {
                    id: editGroupArea

                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (idGcodeFileName.editGcodeReadOnly) {
                            idGcodeFileName.editGcodeReadOnly = false;
                            idGcodeFileName.forceActiveFocus();
                        }
                    }
                }

            }

        }

        ScrollView{
            Layout.preferredWidth: parent.width - 40* screenScaleFactor
            Layout.preferredHeight: 410* screenScaleFactor
            ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            clip:true
            Flow{
                id:flowId
                spacing: 10* screenScaleFactor
                anchors.fill:parent
               
                Repeater{
                    id: printer_repeater
                    model: kernel_slice_flow.slicePreviewListModel
                    delegate: Rectangle{
                        readonly property var modelItem: model

                        width: rowCount==1? parent.width: 280* screenScaleFactor
                        height: rowCount==1? parent.height: 200* screenScaleFactor
                        radius: 5* screenScaleFactor
                        color: Constants.currentTheme ? "#FFFFFF" : "#424244"

                        Image {
                            anchors.centerIn:parent
                            cache:false
                            width:90* screenScaleFactor
                            height:80* screenScaleFactor
                            fillMode: model.isEmpty ? Image.Pad : Image.PreserveAspectFit
                            source: model.imageSource//model.isSliced ? model.imageSource : model.isEmpty ? plate : no_slice
                        }

                        Text {
                            text: model.index + 1
                            color: textColor
                            font.weight: Font.ExtraBold
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_10
                            anchors.left: parent.left
                            anchors.leftMargin: 10* screenScaleFactor
                            anchors.top: parent.top
                            anchors.topMargin: 10* screenScaleFactor
                        }

                        CusImglButton {
                            visible: model.isSliced || model.isEmpty
                            width: 30 * screenScaleFactor
                            height: 30 * screenScaleFactor
                            imgWidth: 14 * screenScaleFactor
                            imgHeight: 14 * screenScaleFactor
                            defaultBtnBgColor: "transparent"
                            hoveredBtnBgColor: "transparent"
                            selectedBtnBgColor: "transparent"
                            opacity: 1
                            hoverBorderColor: Constants.themeGreenColor
                            borderWidth: 0
                            text: model.isSliced ? qsTr("Sliced Plate") : model.isEmpty ? qsTr("Empty Plate") : qsTr("Not Sliced")
                            cusTooltip.position: BasicTooltip.Position.TOP
                            shadowEnabled: Constants.currentTheme
                            enabledIconSource: model.isSliced ? slice : empty
                            pressedIconSource: enabledIconSource
                            highLightIconSource: enabledIconSource
                            anchors.right: parent.right
                            anchors.rightMargin: 10* screenScaleFactor
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 10* screenScaleFactor
                        }
                    }
                }
                Repeater{
                    id: empty_repeater
                    model:{}
                    delegate: Rectangle{
                        width: 280* screenScaleFactor
                        height: 200* screenScaleFactor
                        radius: 5* screenScaleFactor
                        color: Constants.currentTheme ? "#FFFFFF" : "#424244"
                        Image {
                            anchors.centerIn:parent
                            width:90* screenScaleFactor
                            height:80* screenScaleFactor
                            fillMode: Image.PreserveAspectFit
                            source: logo
                        }
                    }
                }
                
            }
        }

   

        ColumnLayout{
            spacing: 10* screenScaleFactor
            Layout.topMargin: 10* screenScaleFactor

            RowLayout{
                Image {
                    width: 14* screenScaleFactor
                    height: 14* screenScaleFactor
                    source: printer
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("Printer:")
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    id: printerName
                    text: kernel_parameter_manager.currentMachineObject.name()
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout{
                Image {
                    width: 14* screenScaleFactor
                    height: 14* screenScaleFactor
                    source: material
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("Filament:")
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                RowLayout{
                   Repeater{
                    model:kernel_parameter_manager.currentMachineObject.extrudersModel 
                    delegate: Rectangle{
                        width: 42* screenScaleFactor
                        height: 42* screenScaleFactor
                        radius: 5* screenScaleFactor
                        color:extruderColor
                        visible: extruder_list.includes(extruderIndex-1)
                        Column{
                            spacing: 2
                            anchors.centerIn: parent
                            width: colorText.width + materialTypeText.width
                            height: colorText.height + materialTypeText.height +spacing
                            CusText{
                                id:colorText
                                fontText:  index + 1
                                fontColor:setTextColor(extruderColor)
                                anchors.horizontalCenter: parent.horizontalCenter
                                fontPointSize:Constants.labelFontPointSize_9
                            }
                            CusText{
                                id:materialTypeText
                                fontColor:colorText.fontColor
                                fontText:  model.curMaterialObj.materialType
                                anchors.horizontalCenter: parent.horizontalCenter
                                fontPointSize:Constants.labelFontPointSize_8
                            }

                        }
                    }
                   }
                }
            }

            RowLayout{
                Image {
                    Layout.preferredWidth: 14* screenScaleFactor
                    Layout.preferredHeight: 14* screenScaleFactor
                    source: root.profile
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("Process:")
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    property var curObj:kernel_parameter_manager.currentMachineObject.currentProfileObject
                    id: profile
                    text: curObj.name+"("+ curObj.dataModel.getDataItem('layer_height').value+"mm layer,"+curObj.dataModel.getDataItem('wall_loops').value+" walls,"+curObj.dataModel.getDataItem('sparse_infill_density').value+"% infill)"
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout{
                Image {
                    Layout.preferredWidth: 14* screenScaleFactor
                    Layout.preferredHeight: 14* screenScaleFactor
                    source: root.plate
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("Plates:")
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    id: plate
                    text: kernel_printer_manager.printersCount
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }

            RowLayout{
                Image {
                    Layout.preferredWidth: 14* screenScaleFactor
                    Layout.preferredHeight: 14* screenScaleFactor
                    source:slice_msg
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("Sliced Plates:")
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    id: slicePlate

                    // Connections {
                    //     target: root
                    //     function onVisibleChanged() {
                    //         if (!root.visible) {
                    //             return
                    //         }

                    //         let count = 0
                    //         for (let index = 0; index < printer_repeater.count; ++index) {
                    //             if (printer_repeater.itemAt(index).isSliced) {
                    //                 ++count
                    //             }
                    //         }

                    //         slicePlate.text = count
                    //     }
                    // }

                    // text: ""
                    text: {
                        if (!root.visible) {
                            return ""
                        }

                        let count = 0
                        for (let index = 0; index < printer_repeater.count; ++index) {
                            if (printer_repeater.itemAt(index).modelItem.isSliced) {
                                ++count
                            }
                        }

                        return String(count)
                    }
                    color: textColor
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout{
            id:btn
            Layout.alignment: Qt.AlignHCenter
            width: 210 * screenScaleFactor
            spacing: 10* screenScaleFactor
            anchors.bottom: root.bottom
            anchors.bottomMargin: 20* screenScaleFactor
            BasicDialogButton {
                Layout.preferredHeight: 30 * screenScaleFactor
                Layout.preferredWidth: 100 * screenScaleFactor
                text: qsTr("Export And Upload")
                btnRadius: 15 * screenScaleFactor
                btnBorderW: 1 * screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    project_kernel.saveAs3mf(idGcodeFileName.text)
                    // Qt.openUrlExternally(cxkernel_cxcloud.webUrl + "/create-model?source=100")
                }
            }
            BasicDialogButton {
                Layout.preferredHeight: 30 * screenScaleFactor
                Layout.preferredWidth: 100 * screenScaleFactor
                text: qsTr("Cancel")
                btnRadius: 15 * screenScaleFactor
                btnBorderW: 1 * screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: root.visible = false
            }
        }
    }
}
