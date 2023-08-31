import QtQml 2.13
import QtQuick 2.0
import QtQuick.Layouts 1.3
//import QtQuick.Controls 2.12
//import QtQuick.Controls 1.4 as Control14
//import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 2.5
import Qt.labs.platform 1.1
import "../qml"

DockItem {
    id: root
    property var currentMachineObject : null
    property var currentPrinterName : ""
    signal delMachine(var machineObject)

    width: 1130 * screenScaleFactor
    height: 613 * screenScaleFactor
    titleHeight : 30 * screenScaleFactor

    title : qsTr("Manage Printer")//by TCJ
    objectName: "printerManagerDlg"

    onVisibleChanged: {
        if (root.visible) {
            updateData()
        }
        else{
            kernel_parameter_manager.applyCurrentMachine()
        }
    }

    function updateData(){
        var currentIndex = kernel_parameter_manager.curMachineIndex
        console.log("++++++++++++++++currentMachineIndex = ", kernel_parameter_manager.curMachineIndex)
        var machineObject = kernel_parameter_manager.machineObject(currentIndex)

        if(!machineObject){
            close()
            return
        }

        idLeftListView.currentIndex = currentIndex
        setCurrentMachine(machineObject)
    }

    function setCurrentMachine(machine){
        console.log("=============setCurrentMachine == ", machine, "currentMachineObject", currentMachineObject)
        currentMachineObject = machine
        if(currentMachineObject)
            currentPrinterName = currentMachineObject.name()
        idFdmGroupBox.updateFromMachine(machine)
    }

    function resetFunc() {
        currentMachineObject.reset()
        for(var i = 0; i < currentMachineObject.extruderCount(); ++i){
            var extruder = currentMachineObject.extruderObject(i)
            extruder.reset("base", idFdmGroupBox.isAdvanced)
            extruder.reset("line", idFdmGroupBox.isAdvanced)
            extruder.reset("retraction", idFdmGroupBox.isAdvanced)
        }
        warningDlg.visible = false
    }
    function deleteFunc() {
        kernel_parameter_manager.removeMachine(root.currentMachineObject)
        updateData()
        warningDlg.visible = false
        if(kernel_parameter_manager.machinesCount === 0)
        {
            root.visible = false
        }
    }

    UploadMessageDlg{
        id: warningDlg
    }

    ColumnLayout {
        id: rectangle

        anchors.fill: parent
        anchors.topMargin: 40 * screenScaleFactor + root.titleHeight
        anchors.bottomMargin: 20 * screenScaleFactor
        anchors.leftMargin: 5 * screenScaleFactor
        anchors.rightMargin: 20 * screenScaleFactor

        spacing: 20 * screenScaleFactor

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 10 * screenScaleFactor
            //            Control14.ExclusiveGroup { id: tabPositionGroup }
            ButtonGroup { id: tabPositionGroup }
            Item{
                Layout.preferredWidth: 210* screenScaleFactor
                Layout.fillHeight: true
                Layout.topMargin: 0
                ListView{
                    id: idLeftListView
                    spacing: 5* screenScaleFactor
                    anchors.fill: parent
                    currentIndex: 0
//                    contentHeight: idLeftListView.count*20*screenScaleFactor
                    model: kernel_parameter_manager.machineNameList
                    clip: true
                    ScrollBar.vertical: ScrollBar {
                    }
                    delegate:CheckDelegate{
                        id: control
                        width: idLeftListView.width
                        height: 20*screenScaleFactor
                        checked: idLeftListView.currentIndex === model.index
                        implicitHeight: 30*screenScaleFactor//checkLabel.height
                        indicator:Item{
                        }
                        ButtonGroup.group: tabPositionGroup
                        background:Item{
                            Row{
                                spacing: 10* screenScaleFactor
                                Item{
                                    width: 10* screenScaleFactor
                                    height: 10* screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    Rectangle {
                                        implicitWidth: 10* screenScaleFactor
                                        implicitHeight: 10* screenScaleFactor
                                        anchors.centerIn: parent
                                        radius: 5* screenScaleFactor
                                        color:control.checked ? "#00a3ff" : "transparent"
                                        border.color: (control.checked || control.hovered)? "#00a3ff" : "#cbcbcc"
                                        border.width: 1* screenScaleFactor
                                        Rectangle {
                                            width: 4* screenScaleFactor
                                            height: 4* screenScaleFactor
                                            anchors.centerIn: parent
                                            visible: control.checked
                                            color: "#ffffff"
                                            radius: 2* screenScaleFactor
                                            anchors.margins: 4* screenScaleFactor
                                        }
                                    }
                                }

                                Label {
                                    id: checkLabel
                                    text: modelData
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: control.checked || control.hovered
                                           ? Constants.manager_printer_switch_checked_color
                                           : Constants.manager_printer_switch_default_color
                                }
                            }
                        }
                        onClicked: {
                            if(checked){
                                //切换机型刷新itemsModel
                                setCurrentMachine(kernel_parameter_manager.machineObject(index))
                                idLeftListView.currentIndex = model.index
                            }
                        }
                    }
                }
            }

            //            Rectangle {
            //                width: 1 * screenScaleFactor
            //                Layout.fillHeight: true
            //                color: Constants.splitLineColor
            //            }

            ManagerFdmGroupShow {
                id: idFdmGroupBox
                Layout.fillWidth: true
                Layout.fillHeight: true
                Row{
                    anchors.top: parent.top
                    anchors.right: parent.right
                    spacing: 5*screenScaleFactor
                    anchors.topMargin: -20*screenScaleFactor
                    StyledLabel {
                        height: 25* screenScaleFactor
                        color: "#cbcbcc"
                        text: qsTr("Advanced")
                    }

                    CusSwitchBtn{
                        id : idAdvance
                        width: 50*screenScaleFactor
                        height: 32*screenScaleFactor
                        btnHeight: 26*screenScaleFactor
                        onCheckedChanged: {
                            idFdmGroupBox.isAdvanced = checked
                            idFdmGroupBox.updateExtruder(currentMachineObject)
                        }
                    }
                }
                objectName: "fdmMachineParemeter"
            }
        }

        FileDialog {
            id: inputDialog
            title: qsTr("Import")
            fileMode: FileDialog.OpenFile
            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
            nameFilters: ["Profiles (*.cxprinter)", "All files (*)"]
            onAccepted: {
                importPrinterDlg.filePath = inputDialog.currentFile
                importPrinterDlg.machineName = kernel_parameter_manager.getMachineName(inputDialog.currentFile)
                importPrinterDlg.userMachineName = kernel_parameter_manager.getFileNameByPath(inputDialog.currentFile)
                importPrinterDlg.visible = true
            }
        }

        FileDialog {
            id: saveDialog
            title: qsTr("Export")
            fileMode: FileDialog.SaveFile
            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
            nameFilters: ["Profiles (*.cxprinter)", "All files (*)"]
            defaultSuffix : "cxprinter"
            onAccepted: {
                currentMachineObject.exportPrinterFromQml(saveDialog.currentFile)
            }
        }

        BasicDialogV4{
            id: addPrinterDlg
            property string addString: "@copy" //重复的时候补充的字符串
            property int nozzleNum: 1
            width: 600*screenScaleFactor
            height: 256*screenScaleFactor
            title: qsTr("Custom Printer")
            maxBtnVis: false

            bdContentItem:Item{
                GridLayout{
                    anchors.top: parent.top
                    anchors.topMargin: 20*screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2
                    StyledLabel{
                        text : qsTr("Printer Name")+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicDialogTextField{
                        id: userPrinterText
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        onlyPositiveNum: false
                        radius: 5
                        limitEnable: false
                        text : templateComb.currentText.split('_')[0] + addPrinterDlg.addString
                        font.pointSize:Constants.labelFontPointSize_10
                        onEditingFinished: {

                        }
                        onEditingFieldFinished: {
                            console.log("onEditingFieldFinished")
                        }
                        onTextChanged: {
                            console.log("onTextChanged")
                            var newPrinterName = userPrinterText.text
                            var nozzleString = templateComb.currentText.split('_')[0] === "Sermoon D3 Pro" ?
                                        nozzle1Comb.currentText + "-" + nozzle2Comb.currentText : nozzle1Comb.currentText
                            let isRept = kernel_parameter_manager.isMachineNameRepeat(newPrinterName+'_'+nozzleString)
                            errorLabel.visible = isRept
                        }
                    }

                    StyledLabel{
                        text : (addPrinterDlg.nozzleNum > 1 ? qsTr("Nozzle 1 Diameter"): qsTr("Nozzle Diameter"))+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicCombobox{
                        id: nozzle1Comb
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        model: kernel_parameter_manager.extruderSupportDiameters(templateComb.currentText.split('_')[0], 0)
                        Component.onCompleted:{
                        }

                        onStyleComboBoxIndexChanged:{
                        }
                    }

                    StyledLabel{
                        visible: addPrinterDlg.nozzleNum > 1
                        text : (addPrinterDlg.nozzleNum > 1 ? qsTr("Nozzle 2 Diameter"): qsTr("Nozzle Diameter"))+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicCombobox{
                        id: nozzle2Comb
                        visible: addPrinterDlg.nozzleNum > 1
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        model: kernel_parameter_manager.extruderSupportDiameters(templateComb.currentText.split('_')[0], 1)
                        Component.onCompleted:{
                        }

                        onStyleComboBoxIndexChanged:{
                        }
                    }

                    StyledLabel{
                        text : qsTr("Chose Model")+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicCombobox{
                        id: templateComb
                        currentIndex: idLeftListView.currentIndex
                        model: kernel_parameter_manager.machineNameList
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        Component.onCompleted:{
                        }

                        onStyleComboBoxIndexChanged:{
                            var machineObject = kernel_parameter_manager.machineObject(currentIndex)
                            addPrinterDlg.nozzleNum = machineObject.extruderCount()
                        }

                        onCurrentTextChanged: {
                            nozzle1Comb.model = kernel_parameter_manager.extruderSupportDiameters(templateComb.currentText.split('_')[0], 0)
                            nozzle2Comb.model = kernel_parameter_manager.extruderSupportDiameters(templateComb.currentText.split('_')[0], 1)
                        }
                    }

                    StyledLabel{
                        //                        text : qsTr("Warning")
                    }

                    StyledLabel{
                        id: errorLabel
                        visible: true
                        text : qsTr("Have the same printer Name!")
                        color:"red"
                        Layout.preferredWidth: 120*screenScaleFactor
                    }
                }

                Row{
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin:20*screenScaleFactor
                    spacing: 20*screenScaleFactor
                    BasicDialogButton {
                        text: qsTr("Create")
                        width: 120*screenScaleFactor
                        height: 28*screenScaleFactor
                        btnRadius: height / 2
                        btnBorderW: 1 * screenScaleFactor
                        btnTextColor: Constants.manager_printer_button_text_color
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                        onSigButtonClicked: {
                            var oldPrinterName = templateComb.currentText.split('_')[0]
                            var newPrinterName = userPrinterText.text
                            var nozzleString = templateComb.currentText.split('_')[0] === "Sermoon D3 Pro" ?
                                        nozzle1Comb.currentText + "-" + nozzle2Comb.currentText : nozzle1Comb.currentText

                            if(kernel_parameter_manager.machinesNames().indexOf(newPrinterName+'_'+nozzleString) === -1){
                                kernel_parameter_manager.addUserMachine(oldPrinterName, nozzleString, newPrinterName)
                                updateData()
                            }else{
                            }

                            addPrinterDlg.close()
                        }
                    }

                    BasicDialogButton {
                        text: qsTr("Cancel")
                        width: 120*screenScaleFactor
                        height: 28*screenScaleFactor
                        btnRadius: height / 2
                        btnBorderW: 1 * screenScaleFactor
                        btnTextColor: Constants.manager_printer_button_text_color
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                        onSigButtonClicked: {
                            addPrinterDlg.close()
                        }
                    }
                }
            }
        }

        BasicDialogV4{
            id: importPrinterDlg
            property string filePath
            property string machineName
            property string userMachineName
            property int nozzleNum: 1

            width: 600*screenScaleFactor
            height: 256*screenScaleFactor
            title: qsTr("Import Printer")
            maxBtnVis: false

            onVisibleChanged: {
                if(visible){
                    console.log("+++++++++++++++++++")
                    importPrinterDlg.cloader.item.nozzle1bcb.model = kernel_parameter_manager.extruderSupportDiameters(machineName.split('_')[0], 0)
                    importPrinterDlg.cloader.item.nozzle2bcb.model = kernel_parameter_manager.extruderSupportDiameters(machineName.split('_')[0], 1)
                    importPrinterDlg.nozzleNum = kernel_parameter_manager.getMachineExtruderCount(filePath)
                }
            }

            bdContentItem:Item{
                property alias nozzle1bcb: nozzle1bcb
                property alias nozzle2bcb: nozzle2bcb
                GridLayout{
                    anchors.top: parent.top
                    anchors.topMargin: 20*screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2
                    StyledLabel{
                        text : qsTr("Printer Name")+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicDialogTextField{
                        id: userPrinterName
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        onlyPositiveNum: false
                        radius: 5
                        limitEnable: false
                        text : importPrinterDlg.userMachineName
                        font.pointSize:Constants.labelFontPointSize_10
                        onEditingFinished: {
                        }

                        onTextChanged: {
                            console.log("onTextChanged")
                            var newPrinterName = userPrinterName.text
                            var nozzleString = machineNameTF.text.split('_')[0] === "Sermoon D3 Pro" ?
                                        nozzle1bcb.currentText + "-" + nozzle2bcb.currentText : nozzle1bcb.currentText
                            let isRept = kernel_parameter_manager.isMachineNameRepeat(newPrinterName+'_'+nozzleString)
                            warningLabel.visible = isRept
                        }
                    }

                    StyledLabel{
                        text : (importPrinterDlg.nozzleNum > 1 ? qsTr("Nozzle 1 Diameter"): qsTr("Nozzle Diameter"))+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicCombobox{
                        id: nozzle1bcb
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        model: kernel_parameter_manager.extruderSupportDiameters(machineNameTF.text.split('_')[0], 0)
                        Component.onCompleted:{
                        }

                        onStyleComboBoxIndexChanged:{
                        }
                    }

                    StyledLabel{
                        visible: importPrinterDlg.nozzleNum > 1
                        text : (importPrinterDlg.nozzleNum > 1 ? qsTr("Nozzle 2 Diameter"): qsTr("Nozzle Diameter"))+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicCombobox{
                        id: nozzle2bcb
                        visible: importPrinterDlg.nozzleNum > 1
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        model: kernel_parameter_manager.extruderSupportDiameters(machineNameTF.text.split('_')[0], 1)
                        Component.onCompleted:{
                        }

                        onStyleComboBoxIndexChanged:{
                        }
                    }

                    StyledLabel{
                        text : qsTr("Chose Model")+": "
                        Layout.preferredWidth: 120*screenScaleFactor
                    }

                    BasicDialogTextField{
                        id: machineNameTF
                        enabled: false
                        Layout.preferredWidth: 405*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        onlyPositiveNum: false
                        radius: 5
                        text : importPrinterDlg.machineName
                        font.pointSize:Constants.labelFontPointSize_10
                        onEditingFinished: {
                        }
                    }

                    StyledLabel{
                    }

                    StyledLabel{
                        id: warningLabel
                        visible: false
                        color:"red"
                        text : qsTr("Have the same printer Name!")
                        Layout.preferredWidth: 120*screenScaleFactor
                    }
                }

                Row{
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 20*screenScaleFactor
                    spacing: 20*screenScaleFactor
                    BasicDialogButton {
                        text: qsTr("Import")
                        width: 120*screenScaleFactor
                        height: 28*screenScaleFactor
                        btnRadius: height / 2
                        enabled: !warningLabel.visible
                        btnBorderW: 1 * screenScaleFactor
                        btnTextColor: Constants.manager_printer_button_text_color
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                        onSigButtonClicked: {
                            console.log("onTextChanged")
                            var newPrinterName = userPrinterName.text
                            var nozzleString = machineNameTF.text.split('_')[0] === "Sermoon D3 Pro" ?
                                        nozzle1bcb.currentText + "-" + nozzle2bcb.currentText : nozzle1bcb.currentText
                            let isRept = kernel_parameter_manager.isMachineNameRepeat(newPrinterName+'_'+nozzleString)
                            warningLabel.visible = isRept

                            if(!isRept){
                                kernel_parameter_manager.importUserMachine(importPrinterDlg.filePath, userPrinterName.text, machineNameTF.text, nozzleString)
                                updateData()
                                importPrinterDlg.close()
                            }else{
                                return;
                            }
                        }
                    }

                    BasicDialogButton {
                        text: qsTr("Cancel")
                        width: 120*screenScaleFactor
                        height: 28*screenScaleFactor
                        btnRadius: height / 2
                        btnBorderW: 1 * screenScaleFactor
                        btnTextColor: Constants.manager_printer_button_text_color
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                        onSigButtonClicked: {
                            importPrinterDlg.close()
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumHeight: 28 * screenScaleFactor
            Layout.maximumHeight: Layout.minimumHeight
            Layout.alignment: Qt.AlignCenter
            spacing: 10 * screenScaleFactor

            BasicDialogButton {
                text: qsTr("Create")

                Layout.minimumWidth: 120 * screenScaleFactor
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter

                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor
                onSigButtonClicked: {
                    //                    root.close()
                    //                    idFdmGroupBox.saveMachineParam()
                    addPrinterDlg.visible = true
                }
            }

            BasicDialogButton {
                text: qsTr("Reset")

                Layout.minimumWidth: 120 * screenScaleFactor
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter

                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                onSigButtonClicked: {
                    warningDlg.msgText = qsTr("Do you want to reset the selected printer parameters?")
                    warningDlg.visible = true
                    warningDlg.sigOkButtonClicked.disconnect(resetFunc)
                    warningDlg.sigOkButtonClicked.disconnect(deleteFunc)
                    warningDlg.sigOkButtonClicked.connect(resetFunc)
                }
            }
            BasicDialogButton {
                text: qsTr("Delete")

                Layout.minimumWidth: 120 * screenScaleFactor
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter

                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                onSigButtonClicked: {
                    warningDlg.msgText = qsTr("Do you want to delete the selected printer?")
                    warningDlg.visible = true
                    warningDlg.sigOkButtonClicked.disconnect(deleteFunc)
                    warningDlg.sigOkButtonClicked.disconnect(resetFunc)
                    warningDlg.sigOkButtonClicked.connect(deleteFunc)

                    //                    deleteFunc();
                }
            }

            BasicDialogButton {
                text: qsTr("Import")
                Layout.minimumWidth: 120 * screenScaleFactor
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter

                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                onSigButtonClicked: {
                    inputDialog.open()
                }
            }

            BasicDialogButton {
                text: qsTr("Export")

                Layout.minimumWidth: 120 * screenScaleFactor
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter

                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                onSigButtonClicked: {
                    saveDialog.open()
                }
            }

            BasicDialogButton {
                text: qsTr("Cancel")
                visible: false
                Layout.minimumWidth: 120 * screenScaleFactor
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter

                btnRadius: height / 2
                btnBorderW: 1 * screenScaleFactor
                btnTextColor: Constants.manager_printer_button_text_color
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                selectedBtnBgColor: Constants.lpw_BtnBorderHoverColor

                onSigButtonClicked: {
                    root.close()
                    kernel_parameter_manager.clearMachineCache(root.currentPrinterName)
                }
            }
        }
    }
}
