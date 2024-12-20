import "../components"
import "../qml"
import "../calibration"
import "../secondqml"
import QtQuick 2.0
import QtQuick.Controls 2.0
import com.cxsw.SceneEditor3D 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/secondqml/printer"
import "../unittool"
Loader {
    id: idAllMenuDialog

    property var receiver
    property var menuObjName

    signal sigShowLoginPanel()
    signal dialogShowed(string name)

    function openSaveAs(machine) {
        sourceComponent = idSaveDialog;
        idAllMenuDialog.menuObjName = machine;
    }

    function requestDialog(name) {
        sourceComponent = null;
        idAllMenuDialog.menuObjName = name;
        if (name === "idModelUnfitMessageDlg")
            sourceComponent = idModelUnfitMessageDlg;
        else if (name === "idModelUnfitMessageDlg_small")
            sourceComponent = idModelUnfitSmallMessageDlg;
        else if (name === "idWarningLoginDlg")
            sourceComponent = idWarningLoginDlg;
        else if (name === "idUploadModelFailedDialog")
            sourceComponent = idUploadModelFailedDialog;
        else if (name === "idAddPrinterDlgNew")
            sourceComponent = idAddPrinterDlgNew;
        else if (name === "slice_height_setting_dialog")
            sourceComponent = slice_height_setting_dialog;
        else if (name === "partition_print_dialog")
            sourceComponent = partition_print_dialog;
        else if (name === "mosueInfo")
            sourceComponent = idMosueInfo;
        else if (name === "idUpdateDlg")
            sourceComponent = idUpdateDlg;
        else if (name === "idReleaseNote")
            sourceComponent = idReleaseNote;
        else if (name === "idStartFirstConfig")
            sourceComponent = idStartFirstConfig;
        else if (name === "idOutPlatform")
            sourceComponent = idOutPlatform;
        else if (name === "saveParemDialog")
            sourceComponent = saveParemDialog;
        else if (name === "saveParamSecondDialog")
            sourceComponent = saveParamSecondDialog;
        else if (name === "switchDownModelPathDialog")
            sourceComponent = swichDownloadPathDialog;
        else if(name === "newProjectDialog")
            sourceComponent = __newProjectDialog
        else if(name === "openProjectDialog")
            sourceComponent = __openProjectDialog
        else if(name === "saveProjectDlg")
            sourceComponent = __saveProjectDialog
        else if(name === "migrateSuccessful")
        {

            sourceComponent = idMigrateDialog
            item.dialogType = 0
        }
        else if(name === "modifySuccessful")
        {
            sourceComponent = idMigrateDialog
            item.dialogType = 1
        }
        else if(name === "unitTestDialog")
        {
            sourceComponent = _unitTestDlg
        }
        else if(name === "cacheToGcodeDlg")
        {
            sourceComponent = _cacheToGcodeDlg
        }
        dialogShowed(name);
    }

    function requestMenuDialog(receiver, menuObjName) {
        idAllMenuDialog.receiver = receiver;
        idAllMenuDialog.menuObjName = menuObjName;
        sourceComponent = null;
        switch (menuObjName) {
        case "languageObj":
            sourceComponent = idLanguage;
            break;
        case "newProjectObj":
            sourceComponent = idNewProject;
            break;
        case "cloneNumObj":
            sourceComponent = idCloneNumDlg;
            break;
        case "aboutUsObj":
            sourceComponent = idAboutUs;
            break;
        case "updateObj":
            standaloneWindow.checkupgrade();
            return ;
            break;
        case "closeProfile":
            sourceComponent = idClosePro;
            break;
        case "closeProfile1":
            sourceComponent = idClosePro1;
            break;
        case "messageDlg":
            sourceComponent = idConfirmMessageDlg;
            break;
        case "deletemessageDlg":
            sourceComponent = idDoubleMessageDlg;
            break;
        case "messageSingleBtnDlg":
            sourceComponent = idConfirmMessageDlg;
            break;
        case "importimageDlg":
            sourceComponent = idImportImageConfigDlg;
            break;
        case "projectComformDlg":
            sourceComponent = idProjectComformDlg;
            break;
        case "modelRepairMessageDlg":
            sourceComponent = idModelRepairMessageDlg;
            break;
        case "sliceSuccessDlg":
            sourceComponent = idSelectPrint;
            break;
        case "laserSaveSuccessDlg":
            sourceComponent = idLaserSelectPrint;
            break;
        case "deleteSupportDlg":
            sourceComponent = idSupportConfirmMessageDlg;
            break;
        case "lanPrintErrorDlg":
            sourceComponent = idLanPrintErrorDlg;
            break;
        case "openDefaultCx3d":
            sourceComponent = idOpenDefaultProjectDlg;
            break;
        case "openRecentlyFileDlg":
            sourceComponent = idOpenRecentlyFileDlg;
            break;
        case "repaircmdDlg":
            sourceComponent = idRepairdlg;
            break;
        case "messageNoModelDlg":
            sourceComponent = idMessageNoBtn;
            break;
        case "TemperatureObj":
            sourceComponent = idTemperature;
            break;
        case "PAObj":
            sourceComponent = idPA;
            break;
        case "VFAObj":
            sourceComponent = idVFA;
            break;
        case "RetractionObj":
            sourceComponent = idRetraction;
            break;
        case "FlowFineTuningObj":
            sourceComponent = idFlowFineTuning;
            break;
        case "MaxFlowVolumeObj":
            sourceComponent = idMaxFlowVolume;
            break;
        case "RetractionDistanceObj":
            sourceComponent = idRetractionDistance;
            break;
        case "RetractionSpeedObj":
            sourceComponent = idRetractionSpeed;
            break;
        case "FanSpeedObj":
            sourceComponent = idFanSpeed;
        break;
        case "JitterSpeedObj":
            sourceComponent = idJitterSpeed;
        break;
        case "MaxSpeedObj":
            sourceComponent = idMaxSpeed;
        break;
        case "SpeedTowerObj":
            sourceComponent = idSpeedTower;
        break;
        case "MaxAccelerationObj":
            sourceComponent = idMaxAcceleration;
        break;
        case "AccelerationTowerObj":
            sourceComponent = idAccelerationTower;
        break;
        case "SlowAccelerationObj":
            sourceComponent = idSlowAcceleration;
        break;

        case "ArcFittingObj":
            sourceComponent = idArcFitting;
        break;
        case "idPrinterSettingsDialog":
            sourceComponent = idPrinterSettingsDialog;
            break;
        case "idPrinterNameEditorDialog":
            sourceComponent = idPrinterNameEditorDialog;
            break;
        case "PreferencesObj":
            sourceComponent = idPreferences;
            break;
        case "saveParemDialog":
            sourceComponent = saveParemDialog;
            break;
        case "saveParamSecondDialog":
            sourceComponent = saveParamSecondDialog;
            break;
        case "idDeletePreset":
            sourceComponent = idDeletePreset;
            break;
        case "ManagePrinterObj":
            sourceComponent = idManagePrinter;
            break;
        case "model_setting_tip_dialog":
            sourceComponent = model_setting_tip_dialog;
            break;
        case "idNoMaterialWarning":
            sourceComponent = idNoMaterialWarning;
            break;
        case "parameter_update_dialog":
            sourceComponent = parameter_update_dialog;
            break;
         case"newProjectDialog":
            sourceComponent = __newProjectDialog
             break;
        default:
        }
    }

    function requestTipDialog(message) {
        idAllMenuDialog.menuObjName = message;
        sourceComponent = null;
        sourceComponent = tip_dialog;
    }

    onLoaded: {
        console.log("loaded:" + menuObjName);
        if (item.hasOwnProperty("receiver"))
            item.receiver = idAllMenuDialog.receiver;

        item.show();
    }

    Component {
        id: model_setting_tip_dialog

        ModelSettingTipDialog {
            Component.onCompleted: {
                confirmButtonClicked.connect(receiver.onTipDialogFinished);
            }
        }

    }

    Component {
        id: parameter_update_dialog

        ParameterUpdateDialog {
            //confirm.connect(receiver.onConfirmUpdate);
            //cancel.connect(receiver.onCancelUpdate);

            Component.onCompleted: {
                model = receiver.updateProfileModel;
            }
        }

    }

    Component {
        id: idUploadModelFailedDialog

        UploadMessageDlg {
            msgText: qsTr("Failed to upload model!")
            cancelBtnVisible: false
            visible: false
        }

    }
    Component {
        id: idMigrateDialog

        UploadMessageDlg {
            property real dialogType: 0
            msgText: dialogType ===0 ?  qsTr("Path modification and model migration successful") + "!"
                                     :  qsTr("Path modified successfully") + "!"
            cancelBtnVisible: false
            visible: false
        }

    }

    Component {
        id: idOutPlatform

        //模型超出打印范围的提示弹窗
        UploadMessageDlg {
            visible: false
            centerToWindow: false
            // cancelBtnVisible: false
            messageType: 0
            msgText: qsTr("The model has exceeded the printing range, do you want to continue slicing?")
            onSigOkButtonClicked: {
                close();
                kernel_kernel.setKernelPhase(1);
            }
            onSigCancelButtonClicked: {
                kernel_kernel.setKernelPhase(0);
            }
        }

    }

    Component {
        id: idNoMaterialWarning

        //模型超出打印范围的提示弹窗
        UploadMessageDlg {
            visible: false
            width: 500 * screenScaleFactor
            height: 160 * screenScaleFactor
            messageType: 0
            msgText: qsTr("Choose at least one material.\n Do you wish to use the default material list?")
            onSigOkButtonClicked: {
                receiver.onOk();
            }
            onSigCancelButtonClicked: {
                receiver.onCancel();
            }
        }

    }

    Component {
        id: idLanguage

        LanguagePreferDlg {
            visible: false
            objectName: "languageObj"
            onSaveLanguageConfig: {
                receiver.setCurrentMinutes(timeInterval);
                receiver.setMoneyCurrentIndex(cmbMoneyCurrentIndex);
                receiver.saveLanguageConfig();
            }
            Component.onCompleted: {
                idLanguage.timeInterval = receiver.getMinutes().toFixed(1);
                idLanguage.cmbMoneyCurrentIndex = receiver.currentMoneyIndex();
            }
        }

    }

    Component {
        id: idMaterial

        ManageMaterialDlg {
            visible: false
            objectName: "managematerialObj"
        }

    }

    Component {
        id: idNewProject

        AddNewProjectDlg {
            property var receiver

            visible: false
            objectName: "newProjectObj"
            onSigProject: receiver.saveProject(name)
        }

    }

    Component {
        id: idCloneNumDlg

        CloneNumDlg {
            property var receiver

            visible: false
            objectName: "cloneNumObj"
            onSigNumber: receiver.clone(clonenum)
        }

    }

    Component {
        id: idAboutUs

        AboutUsDlg {
            visible: false
            objectName: "aboutUsObj"
        }

    }

    Component {
        id: idTemperature

        TemperatureDlg {
            property var receiver

            visible: false
            objectName: "temperatureObj"
            onSigTemp: kernel_slice_calibrate.temp(start, end, step,temp)
        }

    }

    Component {
        id: idRetraction

        RetractionDlg {
            objectName: "RetractionObj"
            onSigRetraction: kernel_slice_calibrate.retractionTower(start, end, step,temp)
        }

    }

    Component {
        id: idPA

        PADlg {
            visible: false
            objectName: "PAObj"
            onSigPaTower: kernel_slice_calibrate.setPA(start, end, step, type,temp)
        }

    }

    Component {
        id: idMaxFlowVolume

        MaxFlowVolumeDlg {
            property var receiver

            visible: false
            objectName: "MaxFlowVolumeObj"
            onSigGenerate: kernel_slice_calibrate.maxFlowValume(start, end, step,temp)
        }

    }

    Component {
        id: idPreferences

        PreferencesDlg {
            property var receiver

            visible: false
            objectName: "PreferencesObj"
        }

    }

    Component {
        id: idManagePrinter

        MachineManagerPanel_New {
            property var receiver

            visible: false
            objectName: "ManagePrinterObj"
        }

    }

    Component {
        id: idManageMaterial

        MaterialManagerPanel_New {
            property var receiver

            visible: false
            objectName: "ManageMaterialObj"
        }

    }

    Component {
        id: idVFA

        VFADlg {
            property var receiver

            visible: false
            objectName: "VFAObj"
            onSigGenerate: kernel_slice_calibrate.setVFA(start, end, step,temp)
        }

    }

    Component {
        id: idFlowFineTuning

        FlowFineTuningDlg {
            property var receiver

            visible: false
            objectName: "FlowFineTuningObj"
            onSigGenerate: kernel_slice_calibrate.highflow(flowValue)
            onGetTuningTutorial: receiver ? receiver.getTuningTutorial() : ""
        }

    }

    Component {
        id: idRetractionDistance

        RetractionDistanceDlg {
            property var receiver

            visible: false
            objectName: "RetractionDistanceObj"
            onSigGenerate: kernel_slice_calibrate.retractionTower(start, end, step,temp)

        }

    }

    Component {
        id: idRetractionSpeed

        RetractionSpeedDlg {
            property var receiver

            visible: false
            objectName: "RetractionSpeedObj"
            onSigGenerate: kernel_slice_calibrate.retractionTower_speed(start, end, step,temp)
        }

    }

    Component {
        id: idFanSpeed

        FanSpeedDlg {
            property var receiver

            visible: false
            objectName: "FanSpeedObj"
            onSigGenerate: kernel_slice_calibrate.fanSpeed(start, end, step,temp)
        }
    }
    Component {
        id: idJitterSpeed

        JitterSpeedDlg {
            property var receiver

            visible: false
            objectName: "JitterSpeedObj"
            onSigGenerate: kernel_slice_calibrate.x_y_Jerk(start, end, step,temp)
        }
    }

     Component {
        id: idMaxSpeed

        MaxSpeedDlg {
            property var receiver

            visible: false
            objectName: "MaxSpeedObj"
            onSigGenerate: kernel_slice_calibrate.limitSpeed(start, end, step,speed,temp)
        }
    }

    Component {
        id: idSpeedTower

        SpeedTowerDlg {
            property var receiver

            visible: false
            objectName: "SpeedTowerObj"
            onSigGenerate: kernel_slice_calibrate.speedTower(start, end, step,temp)
        }
    }

    Component {
        id: idMaxAcceleration

        MaxAccelerationDlg {
            property var receiver

            visible: false
            objectName: "MaxAccelerationObj"
            onSigGenerate: kernel_slice_calibrate.limitAcceleration(start, end, step,speed,temp)
        }
    }
    Component {
        id: idAccelerationTower

        AccelerationTowerDlg {
            property var receiver

            visible: false
            objectName: "AccelerationTowerObj"
            onSigGenerate: kernel_slice_calibrate.accelerationTower(start, end, step,temp)
        }
    }
    Component {
        id: idSlowAcceleration

        SlowAccelerationDlg {
            property var receiver

            visible: false
            objectName: "SlowAccelerationObj"
            onSigGenerate: kernel_slice_calibrate.accel2decel(start, end, step, highstep,temp)
        }
    }

      Component {
        id: idArcFitting

        ArcFittingDlg {
            property var receiver

            visible: false
            objectName: "ArcFittingObj"
            onSigGenerate: kernel_slice_calibrate.arc2lerance(start, end, step)
        }
    }


    Component {
        id: idMessageNoBtn

        BasicDialog {
            width: 400
            height: 200
            titleHeight: 30
            title: qsTr("Message")

            StyledLabel {
                anchors.centerIn: parent
                text: qsTr("No model selected to save")
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
            }

        }

    }

    Component {
        id: idWarningLoginDlg

        UploadMessageDlg {
            msgText: qsTr("Not logged in to Creality Cloud, Unable to current operation, Do you want to log in?")
            visible: false
            messageType: 0
            onSigOkButtonClicked: {
                close();
                sigShowLoginPanel();
            }
        }

    }

    Component {
        id: idRepairdlg

        RepairPanelDlg {
            property var receiver

            function updateErrorInfo() {
                faceSize = receiver ? receiver.getFacesizeAll() : 0;
                errorNormals = receiver ? receiver.getErrorNormalsAll() : 0;
                errorEdges = receiver ? receiver.getErrorEdgesAll() : 0;
                verticessize = receiver ? receiver.getVerticessizeAll() : 0;
                shells = receiver ? receiver.getshellsAll() : 0;
                errorHoles = receiver ? receiver.getholesAll() : 0;
                errorIntersects = receiver ? receiver.getIntersectsAll() : 0;
            }

            objectName: "repaircmdDlg"
            onAccept: {
                if (receiver)
                    receiver.acceptRepair();

            }
            onCancel: {
                if (receiver)
                    receiver.cancel();

            }
        }

    }

    Component {
        id: idClosePro

        UploadMessageDlg {
            property var receiver

            objectName: "closeProfile"
            msgText: qsTr("Save Project Before Exiting?")
            visible: false
            ignoreBtnVisible: true
            okBtnText: qsTr("Save")
            ignoreBtnText: qsTr("Exit")
            cancelBtnText: qsTr("Cancel")
            labelPointSize: Constants.labelFontPointSize_9
            messageType: 1

            onSigOkButtonClicked: {
                close();
                receiver.saveFile(true);
                Qt.callLater(Qt.quit);
            }
            onSigIgnoreButtonClicked: {
                close();
                if (kernel_parameter_manager.isAnyModified()) {
                   standaloneWindow.saveParemDialog();
                } else {
                    receiver.clearScreen();
                    Qt.callLater(Qt.quit);
                }
            }
        }

    }

    Component {
        id: idClosePro1

        UploadMessageDlg2 {
            property var receiver

            objectName: "closeProfile"
            msgText: qsTr("Save Project Before Exiting?")
            visible: false
            ignoreBtnVisible: true
            okBtnText: qsTr("Save")
            ignoreBtnText: qsTr("Exit")
            cancelBtnText: qsTr("Cancel")
            labelPointSize: Constants.labelFontPointSize_9
            messageType: 1

            onSigOkButtonClicked: {
                close();
                receiver.saveFile(true);
                Qt.callLater(Qt.quit);
            }
            onSigIgnoreButtonClicked: {
                close();
                if (kernel_parameter_manager.isAnyModified()) {
                   standaloneWindow.saveParemDialog();
                } else {
                    receiver.clearScreen();
                    Qt.callLater(Qt.quit);
                }
            }
        }

    }

    Component {
        id: idOpenDefaultProjectDlg

        UploadMessageDlg {
            property var receiver

            objectName: "openDefaultProjectDlg"
            visible: false
            messageType: 0
            //cancelBtnVisible: false
            msgText: qsTr("The software closes abnormally, whether to open the saved temporary file?")
            onSigOkButtonClicked: {
                if (receiver)
                    receiver.accept();

                visible = false;
            }
            onSigCancelButtonClicked: {
                if (receiver)
                    receiver.reject();

                visible = false;
            }
        }

    }

    Component {
        id: idOpenRecentlyFileDlg

        UploadMessageDlg {
            id: idOpenRecentlyFileDlg

            objectName: "openRecentlyFileDlg"
            visible: false
            cancelBtnVisible: false
            messageType: 1
            msgText: qsTr("The file does not exist or the path has changed, the file cannot be accessed!")
            onSigOkButtonClicked: {
                idOpenRecentlyFileDlg.visible = false;
            }
        }

    }

    Component {
        id: idDoubleMessageDlg

        BasicMessageDialog {
            property var receiver
            property var typename

            objectName: "openMessageDlg"
            width_x: 480
            isInfo: typename === "deletemessageDlg" ? false : true
            onAccept: {
                if (receiver) {
                    receiver.accept();
                    if (typename === "deletemessageDlg") {
                        if (checkedFlag)
                            receiver.writeReg(false);

                    }
                }
            }
            onCancel: {
                if (receiver)
                    receiver.cancel();

            }
            onDialogClosed: {
                if (receiver)
                    receiver.cancel();

            }
            Component.onCompleted: {
                showDoubleBtn();
                showCheckBox(true);
                receiver = idAllMenuDialog.receiver;
                typename = idAllMenuDialog.menuObjName;
                messageText = receiver.getMessageText();
            }
        }

    }

    Component {
        id: idLanPrintErrorDlg

        UploadMessageDlg {
            property var receiver

            messageType: 1
            cancelBtnVisible: false
            msgText: qsTr("Some Print Error,Reset Printer?")
            okBtnText: qsTr("Reset")
            onSigOkButtonClicked: {
                close();
                if (receiver)
                    receiver.accept();

            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
            }
        }

    }

    Component {
        id: idSupportConfirmMessageDlg

        UploadMessageDlg {
            property var receiver

            isInfo: true
            cancelBtnVisible: true
            msgText: qsTr("Whether to remove  all supports")
            onSigOkButtonClicked: {
                close();
                if (receiver)
                    receiver.accept();

            }
            onSigCancelButtonClicked: {
                close();
                if (receiver)
                    receiver.cancel();

            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
            }
        }

    }

    Component {
        id: idConfirmMessageDlg

        UploadMessageDlg {
            property var receiver

            isInfo: true
            cancelBtnVisible: true

            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
                if (!receiver) {
                    return
                }

                msgText = receiver.getMessageText();

                if (typeof receiver.isAcceptable == "function") {
                    acceptBtnVisible = receiver.isAcceptable()
                }

                if (typeof receiver.isCancelable == "function") {
                    cancelBtnVisible = receiver.isCancelable()
                }

                if (typeof receiver.isIgnoreable == "function") {
                    ignoreBtnVisible = receiver.isIgnoreable()
                }
            }

            onSigOkButtonClicked: {
                close()

                if (receiver) {
                    if (typeof receiver.accept == "function") {
                        receiver.accept()
                    }

                    if (typeof receiver.onAccepted == "function") {
                        receiver.onAccepted()
                    }
                }
            }

            onSigCancelButtonClicked: {
                close()

                if (receiver) {
                    if (typeof receiver.cancel == "function") {
                        receiver.cancel()
                    }

                    if (typeof receiver.onCanceled == "function") {
                        receiver.onCanceled()
                    }
                }
            }

            onSigIgnoreButtonClicked: {
                close()

                if (receiver) {
                    if (typeof receiver.ignore == "function") {
                        receiver.ignore()
                    }

                    if (typeof receiver.onIgnored == "function") {
                        receiver.onIgnored()
                    }
                }
            }
        }

    }
    Component {
        id: __saveProjectDialog

        UploadMessageDlg {
            property var receiver
            msgText: qsTranslate("LaserScene","Save Finished, Open Local Folder")
            isInfo: true
            cancelBtnVisible: true
            onSigOkButtonClicked: {
                close();
                cxkernel_io_manager.openLastSaveFolder()
            }
            onSigCancelButtonClicked: {
                close();

            }
        }

    }



    Component {
        id: idModelUnfitMessageDlg

        UploadMessageDlg {
            ignoreBtnVisible: false
            messageType: 0
            okBtnText: qsTr("Yes")
            cancelBtnText: qsTr("No")
            msgText: qsTr("Model's size exceeds the printer's maximum build volume,apply auto-scale?")
            onSigOkButtonClicked: {
                close();
                kernel_mesh_loader_center.adaptTempModels();
            }
            onSigCancelButtonClicked: {
                close();
                kernel_mesh_loader_center.ignoreTempModels();
            }
        }

    }

    Component {
        id: idModelUnfitSmallMessageDlg

        UploadMessageDlg {
            ignoreBtnVisible: false
            messageType: 0
            okBtnText: qsTr("Yes")
            cancelBtnText: qsTr("No")
            msgText: qsTr("Model's size too small and mabye in inches or meters measurement, do you want to scale to millimeters?")
            onSigOkButtonClicked: {
                close();
                kernel_mesh_loader_center.adaptTempModels();
            }
            onSigCancelButtonClicked: {
                close();
                kernel_mesh_loader_center.ignoreTempModels();
            }
        }

    }

    Component {
        id: swichDownloadPathDialog

        UploadMessageDlg {
            ignoreBtnVisible: false
            messageType: 0
            okBtnText: qsTr("Yes")
            cancelBtnText: qsTr("No")
            msgText: qsTranslate("CusPrefernceObj", "Whether it is necessary to migrate the files in the original folder") + "?"
            onSigOkButtonClicked: {
                close();
                kernel_preference_manager.acceptRepalceCache();
            }
            onSigCancelButtonClicked: {
                close();
                requestDialog("modifySuccessful")
            }
        }

    }

    Component {
        id: __newProjectDialog

        UploadMessageDlg {
            property var receiver
            ignoreBtnVisible: false
            messageType: 0
            okBtnText: qsTr("Yes")
            cancelBtnText: qsTr("No")
            msgText: qsTranslate("CustomObject","A new project is about to be opened. Do you want to save the existing project before opening it?")
            onSigOkButtonClicked: {
//                project_kernel.saveCX3D(true)
                close();

                receiver.onOkClicked()
            }
            onSigCancelButtonClicked: {
//                project_kernel.cancelSave()
                close();

                receiver.onCancelClicked()
            }
        }

    }

    Component {
        id: __openProjectDialog

        UploadMessageDlg {
            ignoreBtnVisible: false
            messageType: 0
            okBtnText: qsTr("Yes")
            cancelBtnText: qsTr("No")
            msgText: qsTranslate("CustomObject","The current project has unsaved changes, save it before continue?")
            onSigOkButtonClicked: {
                project_kernel.saveCX3D(true)
                close();
            }
            onSigCancelButtonClicked: {
                close();
                project_kernel.openCX3D()
            }
        }

    }


    Component {
        id: idModelRepairMessageDlg

        BasicMessageDialog {
            property var receiver

            objectName: "modelRepairMessageDlg"
            width_x: 480
            isInfo: false
            messageText: qsTr("There are exceptions or errors in the model. What should be done?")
            yesBtnText: qsTr("Repair model")
            noBtnText: qsTr("Delete model")
            ignoreBtnText: qsTr("Ignore")
            onAccept: {
                if (receiver)
                    receiver.repairModel();

            }
            onCancel: {
                if (receiver)
                    receiver.delSelectedModel();

            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
                showTripleBtn();
            }
        }

    }

    Component {
        id: idImportImageConfigDlg

        ImportImageDlg {
            property var receiver

            objectName: "importimageDlg"
            onAccept: {
                console.log("parseFloat(myBase).toFixed(2) =" + parseFloat(myBase).toFixed(2));
                console.log("parseFloat(myHeight).toFixed(2) =" + parseFloat(myHeight).toFixed(2));
                console.log("parseFloat(myWidth).toFixed(2) =" + parseFloat(myWidth).toFixed(2));
                receiver.setBaseHeight(parseFloat(myBase).toFixed(2));
                receiver.setMaxHeight(parseFloat(myHeight).toFixed(2));
                receiver.setMeshWidth(parseFloat(myWidth).toFixed(2));
                receiver.setLighterOrDarker(myCmbCurrentIndex);
                receiver.setBlur(mySmothing);
                receiver.accept();
            }
            onCancel: {
                receiver.cancel();
            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
                initTextVelue();
            }
        }

    }
    Component {
        id: idProjectComformDlg
        ProjectComformDlg {
            property var receiver
            objectName: "projectComformDlg"
            onAccept: function(option){
                console.log("option:"+option);
                receiver.onOK(option);
                close();
            }
            onCancel: {
                receiver.onCancel();
                close();
            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
            }
        }
    }
    Component {
        id: idSelectPrint

        ComPrinterSelectDlg {
            property var receiver
            property var slice: receiver ? receiver.getSlicePlugin() : ""

            myTableModel: receiver ? receiver.getTableModel() : ""
            onSearchWifi: receiver.startSearchWifi()
            onSendFileFunction: receiver.sendSliceFile()
            onAccept: idSelectPrint.close()
            onCancel: close()
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
            }
        }

    }

    Component {
        id: idLaserSelectPrint

        ComLaserPrinterSelectDlg {
            property var receiver
            property var laserscene: receiver ? receiver.getLaserScene() : ""

            myTableModel: receiver ? receiver.getTableModel() : ""
            onSearchWifi: {
                receiver.startSearchWifi();
            }
            onSendFileFunction: {
                receiver.sendSliceFile();
            }
            onAccept: {
                laserscene.saveLocalGcodeFile();
                close();
            }
            onCancel: {
                close();
            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
            }
        }

    }

    Component {
        id: idAddPrinterDlgNew

        AddPrinterDlgNew {
            property var isFristStart: "0"

            visible: false
            objectName: "AddPrinterDlg"
            // context: dock_context
            autoClose: false
            onCloseButtonClicked: {
                showLeastOnePrinterMsg();
            }
        }

    }

    Component {
        id: tip_dialog

        UploadMessageDlg {
            msgText: ""
            cancelBtnVisible: false
            ignoreBtnVisible: false
            onSigOkButtonClicked: {
                this.visible = false;
            }
            onVisibleChanged: {
                msgText = idAllMenuDialog.menuObjName;
            }
        }

    }

    Component {
        id: slice_height_setting_dialog

        SliceHeightSettingDialog {
        }

    }

    Component {
        id: partition_print_dialog

        PartitionPrintDialog {
        }

    }

    Component {
        id: addMaterialDlg

        AddMaterialDlg {
            property var eItem: null

            Component.onCompleted: {
                eItem = idAllMenuDialog.receiver.eItem;
                startShowMaterialDialog(idAllMenuDialog.receiver.state, idAllMenuDialog.receiver.extruderIndex, idAllMenuDialog.receiver.materialName, idAllMenuDialog.receiver.currentMachine);
            }
        }

    }

    Component {
        id: manageMaterialDlg

        ManageMaterialDlg {
            Component.onCompleted: {
                startShowMaterialDialog(idAllMenuDialog.receiver.state, idAllMenuDialog.receiver.extruderIndex, idAllMenuDialog.receiver.materialName, idAllMenuDialog.receiver.currentMachine);
            }
        }

    }

    Component {
        id: idParameterProfile

        ParameterProfilePanel {
            //if (!visible)
            //   currentMachine.profilesModel().notifyReset();

            property var currentMachine

            visible: false
            onRemoveProfile: {
                currentMachine.removeProfile(profileObject);
            }
            onVisibleChanged: {
            }
            Component.onCompleted: {
                currentMachine = idAllMenuDialog.receiver.currentMachine;
                startEditProfile(currentMachine, idAllMenuDialog.receiver.addOrEdit, idAllMenuDialog.receiver.extruderCount);
            }
        }

    }

    Component {
        id: idUploadGcodeDialog

        UploadGcodeDlg {
            objectName: "idUploadGcodeDialog"
            onSigCancelBtnClicked: {
                idAllMenuDialog.sourceComponent = null;
            }
            onSigConfirmBtnClicked: {
                if (uploadDlgType) {
                    visible = false;
                    __LANload.wifiSwitch = true;
                    __LANload.curGcodeFileName = curGcodeFileName + ".gcode";
                    var currentIndex = kernel_parameter_manager.curMachineIndex;
                    var machineObject = kernel_parameter_manager.machineObject(currentIndex);
                    let currentPrinterName = machineObject.inheritBaseName();
                    printerListModel.setCurrentDevice(currentPrinterName);
                    kernel_kernel.setKernelPhase(2);
                } else {
                    curWindowState = UploadGcodeDlg.WindowState.State_Progress;
                    cxkernel_cxcloud.sliceService.uploadSlice(curGcodeFileName);
                }
            }
            Component.onCompleted: {
                showUploadDialog(idAllMenuDialog.receiver.type);
            }
        }

    }

    Component {
        id: idSaveDialog

        SaveDialog {
            property var currentMachine: idAllMenuDialog.menuObjName

            title: qsTranslate("ExpertParamPanel", "Save Presets")
            clabel.text: qsTranslate("ExpertParamPanel", "Process Save As") + ":"
            onOkBtnClicked: {
                close();
                if (!currentMachine)
                    return ;

                var curName = currentMachine.currentProfileObject.uniqueName();
                if (curName == cText)
                    currentMachine.saveCurrentProfile();
                else
                    currentMachine.addProfile(cText, currentMachine.currentProfileObject.uniqueName());
            }
            onCancelBtnClicked: {
                close();
            }
        }

    }

    Component {
        id: idMosueInfo

        MouseInfo {
        }

    }

    Component {
        id: idStartFirstConfig

        StartFirstConfig {
            visible: false
        }

    }

    Component {
        id: idReleaseNote

        ReleaseNote {
        }

    }

    Component {
        id: idPrinterSettingsDialog

        PrinterSettingsDialog {
            Component.onCompleted: {
                printer = idAllMenuDialog.receiver;
                init();
            }
        }

    }

    Component {
        id: idPrinterNameEditorDialog

        PrinterNameEditorDialog {
            Component.onCompleted: {
                printer = idAllMenuDialog.receiver;
            }
        }

    }

    Component {
        id: idUpdateDlg

        DockItem {
            width: 400 * screenScaleFactor
            height: 200 * screenScaleFactor
            titleHeight: 30 * screenScaleFactor
            title: qsTr("Version Update")

            Rectangle {
                anchors.centerIn: parent
                width: parent.width / 2
                height: parent.height / 2
                color: "transparent"

                Text {
                    id: name

                    anchors.centerIn: parent
                    width: idUpdateDlg.width - 20 * screenScaleFactor
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    text: qsTr("You are currently on the latest version and do not need to update")
                    font.pointSize: Constants.labelFontPointSize_10
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    color: Constants.textColor
                }

            }

            StyledLabel {
                id: urlLabel

                property string openUrl: cxkernel_cxcloud.serverType ? "https://wiki.creality.com/en/software/update-released/release-notes" : "https://wiki.creality.com/zh/software/update-released/release-notes"

                anchors.left: parent.left
                anchors.leftMargin: 20 * screenScaleFactor
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10 * screenScaleFactor
                text: qsTranslate("ReleaseNote", "Check out the changelog")
                color: Constants.leftBtnBgColor_selected
                font.underline: mouseArea.containsMouse

                MouseArea {
                    id: mouseArea

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: (containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor)
                    onClicked: Qt.openUrlExternally(urlLabel.openUrl)
                }

            }

        }

    }

    Component {
        id: idAddProfileDialog

        AddEditProfile {
            id: idAddProfile

            visible: false
            onSigAddProfile: {
                root.currentMachine.addProfile(newProfileName, templateObject);
                showEditProfile(0);
            }
        }

    }

    Component {
        id: idUploadModelDialog

        UploadModelDlg {
            function uploadModelError() {
                uploadModelGroupFailed();
                hide();
                idAllMenuDialog.requestDialog("idUploadModelFailedDialog");
            }

            objectName: "idUploadModelDialog"
            onViewMyModel: {
                kernel_ui.invokeCloudUserInfoFunc("setUserInfoDlgShow", "myModel");
                idUploadModelDialog.initData();
                idUploadModelDialog.visible = false;
            }
            Component.onCompleted: {
            }

            Connections {
                // onUploadSelectModelsFailed: function() {
                //     idUploadModelDialog.uploadModelError()
                // }

                target: cxkernel_cxcloud.modelService
                onUploadModelGroupFailed: function() {
                    idUploadModelDialog.uploadModelError();
                }
            }

        }

    }

    Component {
        id: saveParemDialog

        SaveParameterFirst {
            id: saveParem

            //parent: standaloneWindow
            visible: false
            onTransfer: {
                receiver.onTransfer();
                close();
            }
            onCancel: {
                receiver.onIgnore();
                close();
            }
            onSave: {
                //requestMenuDialog(receiver2, "saveParamSecondDialog");
                showSaveDialog();
            }
            onSaved: function(index) {
                receiver.onSaved(index);
            }
            onCloseButtonClicked: {
                receiver.onCloseButtonClicked();
            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
                type = receiver.type;
                profileName = receiver.profileName;
            }
            onVisibleChanged:
            {
                if(!visible)
                {
                    receiver.onVisibleFalse()
                }
            }
        }

    }

    Component {
        id: saveParamSecondDialog

        SaveParameterSecond {
            id: saveParamSecond

            visible: false
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
                type = receiver.type;
            }
        }
    }

    Component {
        id: addLeastOnePrinterTipDialog

        UploadMessageDlg {
            id: addLeastOnePrinterTip

            msgText: qsTr("Please add at least one printer")
            cancelBtnVisible: false
            ignoreBtnVisible: false
            onSigOkButtonClicked: {
                this.visible = false;
            }
        }

    }

    Component {
        id: idDeletePreset

        UploadMessageDlg2 {
            property var receiver

            msgText: qsTr("Do you want to delete the preset?")
            ignoreBtnVisible: false
            onSigOkButtonClicked: {
                receiver.exec();
            }
            Component.onCompleted: {
                receiver = idAllMenuDialog.receiver;
            }
        }

    }
    Component {
        id: _unitTestDlg
        UnitTestDialog {

        }
    }
    Component {
        id: _cacheToGcodeDlg
        CacheToGCodeDialog {

        }
    }
}
