import "../components"
import "../secondqml"
import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.13

DockItem {
    //    property bool isProcessChanged: currentMachine.profilesModel.get(currentIndex).object.dataModel.settingsModifyed

    id: spsDialog

    property var receiver
    property var currentMachine: kernel_parameter_manager.currentMachineObject
    property bool isMachineChanged: kernel_parameter_manager.isMachineModified()
    property bool isMaterialChanged: currentMachine.isMaterialModified()
    property bool isProcessChanged: currentMachine.isCurrentProcessModified()
    property bool isMachineNameChecked: true
    property bool isMaterialNameChecked: true
    property bool isProcessNameChecked: true
    property string errorMsg0: qsTr("Do not allow overwriting of system defaults")
    property string errorMsg0_1: qsTr("Preset already exists.") + qsTr("Please note that this preset will be replaced during the save process.")
    property string errorMsg0_2: qsTr("Name cannot be empty!")
    property string errorMsgMachine
    property string errorMsgMaterial
    property string errorMsgProcess
    property var type

    function uniqueName2Basename(uniqueName) {
        var reg = /-[0-9]\.[0-9]/;
        var pos = uniqueName.search(reg);
        if (pos > 0)
            return uniqueName.substring(0, pos);

        return uniqueName;
    }

    function checkName(isFactory, isUser, currentObjectName, changedName, type) {
        var isChacked = false;
        if (changedName === "") {
            switch (type) {
            case 0:
                errorMsgMachine = errorMsg0_2;
                break;
            case 1:
                errorMsgMaterial = errorMsg0_2;
                break;
            case 2:
                errorMsgProcess = errorMsg0_2;
                break;
            }
            return false;
        }

        changedName = changedName.trim()

        if (isFactory(currentObjectName)) {
            //当前是默认
            let res = isFactory(changedName);
            if (res) {
                switch (type) {
                case 0:
                    errorMsgMachine = errorMsg0;
                    break;
                case 1:
                    errorMsgMaterial = errorMsg0;
                    break;
                case 2:
                    errorMsgProcess = errorMsg0;
                    break;
                }
                return isChacked;
            }
            res = isUser(changedName);
            if (res) {
                switch (type) {
                case 0:
                    errorMsgMachine = errorMsg0_1;
                    break;
                case 1:
                    errorMsgMaterial = errorMsg0_1;
                    break;
                case 2:
                    errorMsgProcess = errorMsg0_1;
                    break;
                }
                return true;
            }
        } else if (isUser(currentObjectName)) {
            //当前是用户
            let res = isFactory(changedName);
            if (res) {
                switch (type) {
                case 0:
                    errorMsgMachine = errorMsg0;
                    break;
                case 1:
                    errorMsgMaterial = errorMsg0;
                    break;
                case 2:
                    errorMsgProcess = errorMsg0;
                    break;
                }
                return isChacked;
            }
            res = isUser(changedName);
            if (res) {
                switch (type) {
                case 0:
                    errorMsgMachine = errorMsg0_1;
                    break;
                case 1:
                    errorMsgMaterial = errorMsg0_1;
                    break;
                case 2:
                    errorMsgProcess = errorMsg0_1;
                    break;
                }
                return true;
            }
        }
        switch (type) {
        case 0:
            errorMsgMachine = "";
            break;
        case 1:
            errorMsgMaterial = "";
            break;
        case 2:
            errorMsgProcess = "";
            break;
        }
        isChacked = true;
        return isChacked;
    }

    width: 600 * screenScaleFactor
    height: 116 * screenScaleFactor + (isMachineChanged + isMaterialChanged + isProcessChanged) * 100 * screenScaleFactor
    title: qsTr("Save preset")
    onVisibleChanged: {
        if (visible) {
            if (type == "all") {
                isProcessChanged = currentMachine.isCurrentProcessModified();
                isMachineChanged = kernel_parameter_manager.isMachineModified();
                isMaterialChanged = currentMachine.isMaterialModified();
            }
            if (type == "machine") {
                isProcessChanged = false;
                isMaterialChanged = false;
                isMachineChanged = true;
            }
            if (type == "material") {
                isProcessChanged = false;
                isMachineChanged = false;
                isMaterialChanged = true;
            }
            if (type == "process") {
                isProcessChanged = true;
                isMachineChanged = false;
                isMaterialChanged = false;
            }
            console.log(type);
        }
    }

    ColumnLayout {
        spacing: 20
        anchors.fill: parent
        anchors.top: parent.top
        anchors.topMargin: titleHeight + 20 * screenScaleFactor
        anchors.margins: 20 * screenScaleFactor

        ColumnLayout {
            id: printerSave

            Layout.fillWidth: true
            spacing: 10 * screenScaleFactor
            visible: spsDialog.isMachineChanged

            StyledLabel {
                id: printerLabel

                Layout.preferredWidth: 60 * screenScaleFactor
                text: qsTr("Save Printer as") + ":"
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_9
                Layout.alignment: Qt.AlignVCenter
            }

            BasicDialogTextField {
                id: printerInput

                Layout.fillWidth: true
                Layout.preferredHeight: 28 * screenScaleFactor
                radius: 5
                font.pointSize: 9
                Layout.alignment: Qt.AlignVCenter
                maximumLength: 30
                text: {
                    var uname = kernel_parameter_manager.modifiedMachineName();
                    var lname = kernel_parameter_manager.getBaseNameByUniqueName(uname);
                    var name = kernel_parameter_manager.getNameByUniqueName(uname);
                    if (kernel_parameter_manager.isMachineInFactory(lname))
                        return lname + "-copy";
                    else
                        return lname;
                }
                onTextChanged: {
                    let lname = currentMachine.uniqueName();
                    isMachineNameChecked = checkName(kernel_parameter_manager.isMachineInFactory, kernel_parameter_manager.isMachineInUser, kernel_parameter_manager.modifiedMachineName(), text, 0);
                }

                validator: RegExpValidator {
                }

            }

            StyledLabel {
                id: printerErrorMsgLabel

                //visible: errorMsgProcess !== ""
                Layout.fillWidth: true
                text: errorMsgMachine
                color: Constants.warningColor
                font.pointSize: Constants.labelFontPointSize_9
                Layout.alignment: Qt.AlignVCenter
            }

            RowLayout {
                visible: false
                Layout.fillWidth: true
                spacing: 20 * screenScaleFactor

                StyleCheckBox {
                    id: userBox

                    text: qsTr("User Preset")
                    onCheckedChanged: {
                    }
                    onClicked: {
                    }
                }

                StyleCheckBox {
                    id: projectBox

                    text: qsTr("Project Inside Process")
                    onCheckedChanged: {
                    }
                    onClicked: {
                    }
                }

            }

        }

        ColumnLayout {
            id: materialSave

            Layout.fillWidth: true
            spacing: 10 * screenScaleFactor
            visible: spsDialog.isMaterialChanged

            StyledLabel {
                id: materialLabel

                Layout.preferredWidth: 60 * screenScaleFactor
                text: qsTr("Save Material as") + ":"
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_9
                Layout.alignment: Qt.AlignVCenter
            }

            BasicDialogTextField {
                id: materialInput

                Layout.fillWidth: true
                Layout.preferredHeight: 28 * screenScaleFactor
                radius: 5
                font.pointSize: 9
                Layout.alignment: Qt.AlignVCenter
                maximumLength: 60
                text: {
                    if (currentMachine.isMaterialInFactory(currentMachine.modifiedMaterialName())) {
                        var reg = /-[0-9]\.[0-9]/;
                        var name = currentMachine.modifiedMaterialName();
                        var pos = name.search(reg);
                        console.log(pos);
                        if (pos > 0)
                            return name.substring(0, pos) + "-copy";
                        else
                            return currentMachine.modifiedMaterialName() + "-copy";
                    } else {
                        return uniqueName2Basename(currentMachine.modifiedMaterialName());
                    }
                }
                onTextChanged: {
                    let lname = currentMachine.modifiedMaterialName();
                    var reg = /-[0-9]\.[0-9]/;
                    var pos = lname.search(reg);
                    var name = lname.substring(0, pos);
                    console.log("onTextChanged" + name);
                    isMaterialNameChecked = checkName(currentMachine.isMaterialInFactory, currentMachine.isMaterialInUser, name, text, 1);
                }

                validator: RegExpValidator {
                }

            }

            StyledLabel {
                id: materialErrorMsgLabel

                //visible: errorMsgProcess !== ""
                Layout.fillWidth: true
                text: errorMsgMaterial
                color: Constants.warningColor
                font.pointSize: Constants.labelFontPointSize_9
                Layout.alignment: Qt.AlignVCenter
            }

            RowLayout {
                visible: false
                Layout.fillWidth: true
                spacing: 20 * screenScaleFactor

                StyleCheckBox {
                    id: materialUserBox

                    text: qsTr("User Preset")
                    onCheckedChanged: {
                    }
                    onClicked: {
                    }
                }

                StyleCheckBox {
                    id: materialProjectBox

                    text: qsTr("Project Inside Process")
                    onCheckedChanged: {
                    }
                    onClicked: {
                    }
                }

            }

        }

        ColumnLayout {
            id: processSave

            Layout.fillWidth: true
            spacing: 10 * screenScaleFactor
            visible: spsDialog.isProcessChanged

            StyledLabel {
                id: processLabel

                Layout.preferredWidth: 60 * screenScaleFactor
                text: qsTr("Save Process as") + ":"
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_9
                Layout.alignment: Qt.AlignVCenter
            }

            BasicDialogTextField {
                id: processInput

                Layout.fillWidth: true
                Layout.preferredHeight: 28 * screenScaleFactor
                radius: 5
                font.pointSize: 9
                maximumLength: 60
                Layout.alignment: Qt.AlignVCenter
                text: {
                    let lname = currentMachine.modifiedProcessObject.uniqueName();
                    if (currentMachine.isProcessInFactory(lname))
                        return lname + "-copy";
                    else
                        return lname;
                }
                onTextChanged: {
                    let lname = currentMachine.modifiedProcessObject.uniqueName();
                    isProcessNameChecked = checkName(currentMachine.isProcessInFactory, currentMachine.isProcessInUser, lname, text, 2);
                }

                validator: RegExpValidator {
                }

            }

            StyledLabel {
                id: processErrorMsgLabel

                //visible: errorMsgProcess !== ""
                Layout.fillWidth: true
                text: errorMsgProcess
                color: Constants.warningColor
                font.pointSize: Constants.labelFontPointSize_9
                Layout.alignment: Qt.AlignVCenter
            }

            RowLayout {
                visible: false
                Layout.fillWidth: true
                spacing: 20 * screenScaleFactor

                StyleCheckBox {
                    id: processUserBox

                    text: qsTr("User Preset")
                    onCheckedChanged: {
                    }
                    onClicked: {
                    }
                }

                StyleCheckBox {
                    id: processProjectBox

                    text: qsTr("Project Inside Process")
                    onCheckedChanged: {
                    }
                    onClicked: {
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

    }

    RowLayout {
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20 * screenScaleFactor

        Item {
            Layout.fillWidth: true
        }

        BasicButton {
            Layout.preferredWidth: 80 * screenScaleFactor
            Layout.preferredHeight: 30 * screenScaleFactor
            text: qsTr("OK")
            btnRadius: 15 * screenScaleFactor
            btnBorderW: 1
            borderColor: Constants.lpw_BtnBorderColor
            defaultBtnBgColor: Constants.leftToolBtnColor_normal
            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            enabled: isMachineNameChecked && isMaterialNameChecked && isProcessNameChecked
            onSigButtonClicked: {
                if (isMachineChanged) {
                    console.log("isMachineChanged:")
                    var uname = kernel_parameter_manager.modifiedMachineName();
                    var lname = kernel_parameter_manager.getBaseNameByUniqueName(uname);
                    if (printerInput.text === lname) {
                        kernel_parameter_manager.saveMachine();
                        spsDialog.visible = false;
                        receiver.onSave();
                    } else {
                        let input = printerInput.text.trim()
                        var index = kernel_parameter_manager.saveMachineAs(uname, input);
                        spsDialog.visible = false;
                        receiver.onSave(index);
                        //return ;
                    }
                }
                if (isMaterialChanged) {
                    let input = materialInput.text.trim()
                    console.log("isMaterialChanged:", input, " ", uniqueName2Basename(currentMachine.modifiedMaterialName()))
                    if (input === uniqueName2Basename(currentMachine.modifiedMaterialName())) {
                        currentMachine.saveMaterial();
                        spsDialog.visible = false;
                        receiver.onSave();
                    } else {
                        var index = currentMachine.addUserMaterial(currentMachine.modifiedMaterialName(), input);
                        spsDialog.visible = false;
                        receiver.onSave(index);
                        //return ;
                    }
                    kernel_parameter_manager.reloadAffectedMachines(currentMachine, 0, input)
                }
                if (isProcessChanged) {
                    let input = processInput.text.trim()
                    console.log("isProcessChanged:", input, " ", currentMachine.modifiedProcessObject.uniqueName())
                    if (input === currentMachine.modifiedProcessObject.uniqueName()){
                        currentMachine.saveCurrentProfile();
                    }
                    else{
                        currentMachine.addProfile(input, currentMachine.modifiedProcessObject.uniqueName());
                    }
                    kernel_parameter_manager.reloadAffectedMachines(currentMachine, 1, input)
                    spsDialog.visible = false;
                    receiver.onSave();
                    receiver.receiver.taskQueue();
                }
            }
        }

        BasicButton {
            Layout.preferredWidth: 80 * screenScaleFactor
            Layout.preferredHeight: 30 * screenScaleFactor
            text: qsTr("Cancel")
            btnRadius: 15 * screenScaleFactor
            btnBorderW: 1
            borderColor: Constants.lpw_BtnBorderColor
            defaultBtnBgColor: Constants.leftToolBtnColor_normal
            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            onSigButtonClicked: {
                spsDialog.visible = false;
            }
        }

        Item {
            Layout.fillWidth: true
        }

    }

}
