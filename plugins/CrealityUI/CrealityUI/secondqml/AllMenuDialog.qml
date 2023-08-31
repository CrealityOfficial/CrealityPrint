import QtQuick 2.0
import QtQuick.Controls 2.0
import com.cxsw.SceneEditor3D 1.0
import "qrc:/CrealityUI"
import "../qml"

Item {
    property alias mosueInfo: mosueInfo
    signal sigShowLoginPanel()
    signal dialogShowed(string name)

    function showDiag() {
        idMaterial.show()
    }

    function requestDialog(name) {
        if(name === "idModelUnfitMessageDlg")
        {
//            idModelUnfitMessageDlg.ignoreBtnVisible = true
            idModelUnfitMessageDlg.okBtnText = qsTr("Yes")
//            idModelUnfitMessageDlg.ignoreBtnText = qsTr("Delete model")
            idModelUnfitMessageDlg.cancelBtnText = qsTr("No")

            idModelUnfitMessageDlg.msgText = qsTr("Model's size exceeds the printer's maximum build volume,apply auto-scale?")
            idModelUnfitMessageDlg.show()
        }else if(name === "idModelUnfitMessageDlg_small")
        {
//            idModelUnfitMessageDlg.ignoreBtnVisible = true
            idModelUnfitMessageDlg.okBtnText = qsTr("Yes")
//            idModelUnfitMessageDlg.ignoreBtnText = qsTr("Delete model")
            idModelUnfitMessageDlg.cancelBtnText = qsTr("No")

            idModelUnfitMessageDlg.msgText = qsTr("Model's size too small and mabye in inches or meters measurement, do you want to scale to millimeters?")
            idModelUnfitMessageDlg.show()
        }
        else if(name === "idWarningLoginDlg")
        {
            idWarningLoginDlg.show()
        }
        else if(name === "idUploadModelFailedDialog")
        {
            idUploadModelFailedDialog.show()
        }
        else if(name === "idAddPrinterDlgNew")
        {
            idAddPrinterDlgNew.show()
        } else if (name === "slice_height_setting_dialog") {
            slice_height_setting_dialog.show()
        } else if (name === "partition_print_dialog") {
            partition_print_dialog.show()
        }

        dialogShowed(name)
    }

    function requestMenuDialog(receiver,menuObjName) {
        //console.log("requestMenuDialog success")
        if(menuObjName === "languageObj")
        {
            idLanguage.show()
            idLanguage.receiver = receiver
            idLanguage.timeInterval = receiver.getMinutes().toFixed(1)
            idLanguage.cmbMoneyCurrentIndex = receiver.currentMoneyIndex()
            //idLanguage.currentLanguageIndex = receiver.currentLanguageIndex()
        }
        else if(menuObjName === "newProjectObj")
        {
            idNewProject.show()
            idNewProject.receiver = receiver
        }
        else if(menuObjName === "cloneNumObj")//by TCJ
        {
            idCloneNumDlg.show()
            idCloneNumDlg.receiver = receiver
            //console.log("display cloneN")
        }
        else if(menuObjName === "aboutUsObj")
        {
            idAboutUs.show()
        }
        else if(menuObjName === "updateObj")
        {
            showReleaseNote()
        }
        else if(menuObjName === "closeProfile")
        {
            idClosePro.ignoreBtnVisible = true
            idClosePro.okBtnText = qsTr("Save")
            idClosePro.ignoreBtnText = qsTr("Exit")
            idClosePro.cancelBtnText = qsTr("Cancel")
            idClosePro.show()
            idClosePro.receiver = receiver
        }
        else if(menuObjName === "messageDlg")
        {
            idConfirmMessageDlg.receiver = receiver
            idConfirmMessageDlg.cancelBtnVisible = true
            idConfirmMessageDlg.isInfo = true
            idConfirmMessageDlg.msgText = receiver.getMessageText()
            idConfirmMessageDlg.show()
        }
        else if(menuObjName === "deletemessageDlg")
        {
            idDoubleMessageDlg.showDoubleBtn()
            idDoubleMessageDlg.receiver = receiver
            idDoubleMessageDlg.showCheckBox(true)
            idDoubleMessageDlg.typename = menuObjName
            idDoubleMessageDlg.messageText = receiver.getMessageText()
            idDoubleMessageDlg.show()
        }
        else if(menuObjName === "messageSingleBtnDlg")
        {
            idConfirmMessageDlg.isInfo = true
            idConfirmMessageDlg.cancelBtnVisible = false
            idConfirmMessageDlg.receiver = receiver
            idConfirmMessageDlg.msgText = receiver.getMessageText()
            idConfirmMessageDlg.show()
        }
        else if(menuObjName === "importimageDlg")
        {
            idImportImageConfigDlg.initTextVelue()

            idImportImageConfigDlg.show()
            idImportImageConfigDlg.receiver = receiver
        }
        else if(menuObjName === "modelRepairMessageDlg")
        {
            idModelRepairMessageDlg.showTripleBtn()
            idModelRepairMessageDlg.isInfo = false
            idModelRepairMessageDlg.receiver = receiver
            idModelRepairMessageDlg.messageText = qsTr("There are exceptions or errors in the model. What should be done?")
            idModelRepairMessageDlg.show()
        }
        else if(menuObjName === "sliceSuccessDlg")
        {
            idSelectPrint.receiver = receiver
            idSelectPrint.show()
        }
        else if(menuObjName === "laserSaveSuccessDlg")
        {
            idLaserSelectPrint.receiver = receiver
            idLaserSelectPrint.show()
        }
        else if(menuObjName === "deleteSupportDlg")
        {
            idConfirmMessageDlg.receiver = receiver
            idConfirmMessageDlg.cancelBtnVisible = true
            idConfirmMessageDlg.msgText = qsTr("Whether to remove  all supports")
            idConfirmMessageDlg.show()
        }
        else if(menuObjName === "lanPrintErrorDlg")
        {
            idLanPrintErrorDlg.messageType = 1
            idLanPrintErrorDlg.receiver = receiver
            idLanPrintErrorDlg.cancelBtnVisible = false
            idLanPrintErrorDlg.msgText = qsTr("Some Print Error,Reset Printer?")
            idLanPrintErrorDlg.show()
        }
        else if(menuObjName === "openDefaultCx3d")
        {
            idOpenDefaultProjectDlg.receiver = receiver
            idOpenDefaultProjectDlg.visible = true
        }
        else if(menuObjName === "openRecentlyFileDlg")
        {
            idOpenRecentlyFileDlg.visible = true
        }
        else if(menuObjName === "messageNoModelDlg")
        {
            idMessageNoBtn.visible = true
        }
        else if(menuObjName === "repaircmdDlg")
        {
            idRepairdlg.receiver = receiver
            idRepairdlg.updateErrorInfo()
            idRepairdlg.show()
        } else if(menuObjName === "TemperatureObj")
        {
            idTemperature.show()
            idTemperature.receiver = receiver
        } else if(menuObjName === "PAObj")
        {
            idPA.receiver = receiver
            idPA.show()
        }  else if(menuObjName === "MaxFlowVolumeObj")
        {
            idMaxFlowVolume.show()
            idMaxFlowVolume.receiver = receiver
        } else if(menuObjName === "VFAObj")
        {
            idVFA.show()
            idVFA.receiver = receiver
        }else if(menuObjName === "FlowFineTuningObj")
        {
            idFlowFineTuning.show()
            idFlowFineTuning.receiver = receiver
        }

        dialogShowed(menuObjName)
    }

    function requestTipDialog(message) {
        tip_dialog.msgText = message
        tip_dialog.visible = true
    }

    UploadMessageDlg {
        id: idUploadModelFailedDialog
        msgText: qsTr("Failed to upload model!")
        cancelBtnVisible: false
        visible: false
    }

    LanguagePreferDlg {
        id: idLanguage
        visible: false
        objectName: "languageObj"

        onSaveLanguageConfig:
        {
            receiver.setCurrentMinutes(timeInterval)
            receiver.setMoneyCurrentIndex(cmbMoneyCurrentIndex)
            receiver.saveLanguageConfig()
        }
    }

    ManageMaterialDlg {
        id: idMaterial
        visible: false
        objectName: "managematerialObj"
    }

    AddNewProjectDlg {
        id: idNewProject
        visible: false
        objectName: "newProjectObj"
        property var receiver

        onSigProject: receiver.saveProject(name)
    }

    CloneNumDlg {
        id: idCloneNumDlg
        visible: false
        objectName: "cloneNumObj"
        property var receiver

        onSigNumber: receiver.clone(clonenum)
    }

    MouseInfo{
        id: mosueInfo
        visible: false
    }

    AboutUsDlg {
        id: idAboutUs
        visible: false
        objectName: "aboutUsObj"
    }

    TemperatureDlg{
        id: idTemperature
        visible: false
        objectName: "temperatureObj"
        property var receiver

        onSigGenerate: kernel_slice_calibrate.generate(start,end,step)
    }

    PADlg{
        id: idPA
        visible: false
        objectName: "PAObj"

        onSigTestType: kernel_slice_calibrate.testType(type)
        onSigPaTower: kernel_slice_calibrate.paTower(start,end,step)
    }


    MaxFlowVolumeDlg{
        id: idMaxFlowVolume
        visible: false
        objectName: "MaxFlowVolumeObj"
        property var receiver
        onSigGenerate: kernel_slice_calibrate.maxFlowValume(start,end,step)
    }

    VFADlg{
        id: idVFA
        visible: false
        objectName: "VFAObj"
        property var receiver

        onSigGenerate: kernel_slice_calibrate.VFA(start,end,step)
    }
   FlowFineTuningDlg{
        id: idFlowFineTuning
        visible: false
        objectName: "FlowFineTuningObj"
        property var receiver
        onSigGenerate: kernel_slice_calibrate.highflow(flowValue)
        onGetTuningTutorial: receiver ? receiver.getTuningTutorial() : ""
    }




    BasicDialog {
        id: idMessageNoBtn
        width: 400
        height: 200
        titleHeight : 30
        title: qsTr("Message")

        StyledLabel{
            anchors.centerIn: parent
            text: qsTr("No model selected to save")
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment:Text.AlignLeft
        }
    }

    UploadMessageDlg {
        id: idWarningLoginDlg
        msgText: qsTr("Not logged in to Creality Cloud, Unable to current operation, Do you want to log in?")
        visible: false
        messageType: 0

        onSigOkButtonClicked:
        {
            close()
            sigShowLoginPanel()
        }
    }

    RepairPanelDlg {
        id: idRepairdlg
        objectName: "repaircmdDlg"
        property var receiver

        function updateErrorInfo()
        {
            faceSize = receiver ? receiver.getFacesizeAll() : 0
            errorNormals = receiver ? receiver.getErrorNormalsAll() : 0
            errorEdges = receiver ? receiver.getErrorEdgesAll() : 0
            verticessize = receiver ? receiver.getVerticessizeAll() : 0
            shells = receiver ? receiver.getshellsAll() : 0
            errorHoles = receiver ? receiver.getholesAll() : 0
            errorIntersects = receiver ? receiver.getIntersectsAll() : 0
        }

        onAccept: if(receiver) receiver.acceptRepair()
        onCancel: if(receiver) receiver.cancel()
    }

    UploadMessageDlg {
        id:idClosePro
        objectName: "closeProfile"
        msgText: qsTr("Save Project Before Exiting?")
        visible: false
        labelPointSize: Constants.labelFontPointSize_9
        messageType: 1
        property var receiver

        onSigOkButtonClicked:
        {
            idClosePro.close()
            receiver.saveFile()
        }
        onSigIgnoreButtonClicked:
        {
            idClosePro.close()
            receiver.noSaveFile()
            receiver.delDefaultProject()
        }
    }

    UploadMessageDlg {
        id: idOpenDefaultProjectDlg
        objectName: "openDefaultProjectDlg"
        property var receiver
        visible: false
        messageType:0
        //cancelBtnVisible: false
        msgText: qsTr("The software closes abnormally, whether to open the saved temporary file?")

        onSigOkButtonClicked:
        {
            if(receiver) receiver.accept()
            idOpenDefaultProjectDlg.visible = false
        }
        onSigCancelButtonClicked:
        {
            if(receiver) receiver.reject()
            idOpenDefaultProjectDlg.visible = false
        }
    }

    UploadMessageDlg {
        id: idOpenRecentlyFileDlg
        objectName: "openRecentlyFileDlg"
        visible: false
        cancelBtnVisible: false
        messageType: 1
        msgText: qsTr("The file does not exist or the path has changed, the file cannot be accessed!")

        onSigOkButtonClicked:
        {
            idOpenRecentlyFileDlg.visible = false
        }
    }

    BasicMessageDialog {
        id: idDoubleMessageDlg
        objectName: "openMessageDlg"
        width_x: 480
        isInfo: typename === "deletemessageDlg" ? false : true
        property var receiver
        property var typename

        onAccept:
        {
            if(receiver)
            {
                receiver.accept()
                if(typename === "deletemessageDlg")
                {
                    if(checkedFlag) receiver.writeReg(false)
                }
            }
        }
        onCancel: if(receiver) receiver.cancel()
        onDialogClosed: if(receiver) receiver.cancel()
    }

    UploadMessageDlg {
        id: idLanPrintErrorDlg
        property var receiver
        messageType: 1
        okBtnText : qsTr("Reset")
        onSigOkButtonClicked:
        {
            close()
            if(receiver) receiver.accept()
        }
    }

    UploadMessageDlg {
        id: idConfirmMessageDlg
        property var receiver

        onSigOkButtonClicked:
        {
            idConfirmMessageDlg.close()
            if(receiver) receiver.accept()
        }
        onSigCancelButtonClicked:
        {
            idConfirmMessageDlg.close()
            if(receiver) receiver.cancel()
        }
    }

    UploadMessageDlg {
        id: idModelUnfitMessageDlg
        ignoreBtnVisible: false
        messageType: 0

        onSigOkButtonClicked:
        {
            close()
            kernel_modelspace.adaptTempModels()
        }
        onSigCancelButtonClicked:
        {
            close()
            kernel_modelspace.ignoreTempModels()
        }
    }

    BasicMessageDialog {
        id: idModelRepairMessageDlg
        objectName: "modelRepairMessageDlg"
        property var receiver

        width_x: 480
        yesBtnText: qsTr("Repair model")
        noBtnText: qsTr("Delete model")
        ignoreBtnText: qsTr("Ignore")

        onAccept: if(receiver) receiver.repairModel()
        onCancel: if(receiver) receiver.delSelectedModel()
    }

    ImportImageDlg {
        id:idImportImageConfigDlg
        objectName: "importimageDlg"
        property var receiver

        onAccept:
        {
            console.log("parseFloat(myBase).toFixed(2) =" + parseFloat(myBase).toFixed(2))
            console.log("parseFloat(myHeight).toFixed(2) =" + parseFloat(myHeight).toFixed(2))
            console.log("parseFloat(myWidth).toFixed(2) =" + parseFloat(myWidth).toFixed(2))
            receiver.setBaseHeight(parseFloat(myBase).toFixed(2))
            receiver.setMaxHeight(parseFloat(myHeight).toFixed(2))
            receiver.setMeshWidth(parseFloat(myWidth).toFixed(2))
            receiver.setLighterOrDarker(myCmbCurrentIndex)
            receiver.setBlur(mySmothing)
            receiver.accept()
        }
        onCancel:
        {
            receiver.cancel()
        }
    }

    ComPrinterSelectDlg {
        id:idSelectPrint
        property var receiver
        property var slice: receiver ? receiver.getSlicePlugin() : ""
        myTableModel: receiver ?receiver.getTableModel() : ""

        onSearchWifi: receiver.startSearchWifi()
        onSendFileFunction: receiver.sendSliceFile()
        onAccept: idSelectPrint.close()
        onCancel: close()
    }

    ComLaserPrinterSelectDlg {
        id:idLaserSelectPrint
        property var receiver
        property var laserscene: receiver ? receiver.getLaserScene() : ""
        myTableModel: receiver ?receiver.getTableModel() : ""

        onSearchWifi:
        {
            receiver.startSearchWifi()
        }
        onSendFileFunction:
        {
            receiver.sendSliceFile()
        }
        onAccept:
        {
            laserscene.saveLocalGcodeFile()
            close()
        }
        onCancel:
        {
            close()
        }
    }

    AddPrinterDlgNew {
        visible: false
        id: idAddPrinterDlgNew
        objectName: "AddPrinterDlg"
        property var isFristStart: "0"

        // context: dock_context
        autoClose: false
        onCloseButtonClicked: {
            if (kernel_parameter_manager.machinesCount === 0) {
                addLeastOnePrinterTip.visible = true
                return
            }

            this.visible = false
        }
        Connections {
            target: kernel_parameter_manager
            onMachinesCountChanged: {
                if (kernel_parameter_manager.machinesCount === 0) {
                    idAddPrinterDlgNew.visible = true
                }
                else{
                    //idAddPrinterDlgNew.visible = false
                }
            }
        }
    }

    UploadMessageDlg {
        id: addLeastOnePrinterTip
        msgText: qsTr("Please add at least one printer")
        cancelBtnVisible: false
        ignoreBtnVisible: false
        onSigOkButtonClicked: {
            this.visible = false
        }
    }

    UploadMessageDlg {
        id: tip_dialog
        msgText: ""
        cancelBtnVisible: false
        ignoreBtnVisible: false
        onSigOkButtonClicked: {
            this.visible = false
        }
    }

    SliceHeightSettingDialog {
        id: slice_height_setting_dialog
    }

    PartitionPrintDialog {
        id: partition_print_dialog
    }
}
