import ".."
import CrealityUI 1.0
import Qt.labs.platform 1.1 as MacMenu
import Qt.labs.platform 1.0
import Qt.labs.settings 1.1
import QtQml 2.0
import QtQuick 2.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.5
//import QtQuick.Controls 1.0
// import QtQuick.Controls 2.0 as QQC2
// import QtQuick.Controls 2.5 as QQC25
import QtQuick.Controls.impl 2.12
import QtQuick.Layouts 1.3
import QtQuick.Scene3D 2.13
import QtQuick.Window 2.2
import com.cxsw.SceneEditor3D 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/cxcloud"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/secondqml/frameless"
import "qrc:/CrealityUI/secondqml/printer"

Rectangle {
    id: root

    property var mainWindow: standaloneWindow
    property bool isMaxed: frameLessView.isMax
    property var controlEnabled: kernel_kernel.currentPhase === 0
    property var isFDMPrinter: kernel_parameter_manager.functionType === 0
    property var isLaserPrinter: kernel_parameter_manager.functionType === 1
    property var isCNCPrinter: kernel_parameter_manager.functionType === 2
    property bool logined: cxkernel_cxcloud.accountService.logined
    property bool sliceBtnVisible: true
    property var cloudContext
    property string dividing_line: Constants.currentTheme ? "#AAAAB0" : "#454548"
    property string menu_down_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/menu_down_light.svg" : "qrc:/UI/photo/menuImg/menu_down.svg"
    property string file_down_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/file_down_light.svg" : "qrc:/UI/photo/menuImg/file_down.svg"
    property string open_file_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/open_file_light.svg" : "qrc:/UI/photo/menuImg/open_file.svg"
    property string save_file_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/save_light.svg" : "qrc:/UI/photo/menuImg/save.svg"
    property string preferences_file_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/preferences_light.svg" : "qrc:/UI/photo/menuImg/preferences.svg"
    property string undo_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/undo_light.svg" : "qrc:/UI/photo/menuImg/undo.svg"
    property string redo_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/redo_light.svg" : "qrc:/UI/photo/menuImg/redo.svg"
    property string login_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/login_light.svg" : "qrc:/UI/photo/menuImg/login.svg"
    property string download_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/download_light.svg" : "qrc:/UI/photo/menuImg/download.svg"
    property string close_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/close_light.svg" : "qrc:/UI/photo/menuImg/close.svg"
    property string hidden_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/hidden_light.svg" : "qrc:/UI/photo/menuImg/hidden.svg"
    property string zoom_out_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/zoom_out_light.svg" : "qrc:/UI/photo/menuImg/zoom_out.svg"
    property string zoom_in_img: Constants.currentTheme ? "qrc:/UI/photo/menuImg/zoom_in_light.svg" : "qrc:/UI/photo/menuImg/zoom_in.svg"

    signal setPrepare()
    signal setPreview()
    signal setDevice()
    signal requestFirstConfig()
    signal closeWindow()
    signal changeServer(int serverIndex)

    function showLoginPanel() {
        idLoginBtn.showLoginPanel();
    }

    function excuteOpt(optName) {
        Qt.callLater(function(opt_name) {
            actionCommands.getOpt(opt_name).execute();
        }, optName);
    }

    function startWizardComp() {
        var pRoot = rootLoader;
        while (pRoot.parent !== null)pRoot = pRoot.parent
        wizardComp.createObject(pRoot, {
            "model": rootLoader.item.wizardModel
        });
    }

    function validLogin(cb, showUserDialog = true) {
        if (!logined)
            return cloudContext.loginDialog.show();

        cb();
        if (showUserDialog)
            cloudContext.userInfoDialog.show();

    }

    function startFirstConfig() {
        requestFirstConfig();
    }


    Row {
        id: idLeftMenu

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 10 * screenScaleFactor
        spacing: 10 * screenScaleFactor
        Item {
            visible: Qt.platform.os === "osx"
            width:frameLessView.isFull ? 10 * screenScaleFactor : 50* screenScaleFactor
            height: parent.height
        }
        Image {
            id: logoImage

            width: 20 * screenScaleFactor
            height: 20 * screenScaleFactor
            sourceSize: Qt.size(20, 20)
            visible: Qt.platform.os === "osx"
            source: "qrc:/UI/photo/menuImg/logo.svg"
            anchors.verticalCenter: parent.verticalCenter
        }
        Loader
        {
            active : Qt.platform.os === "windows"
            sourceComponent : _winMenuRow
        }

        Component
        {
            id : _winMenuRow
            Row
            {
                MenuBar {
                    implicitHeight: 30 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    BasicMenu {
                        // enabled: controlEnabled && isFDMPrinter
                        
                        BasicSubMenu {
                            id: editMenu

                            title: qsTr("Edit") + "(&E)"
                            // maxImplicitWidth: 280 * screenScaleFactor
                            implicitWidth: 280 * screenScaleFactor
                            enabled: controlEnabled 
                            BasicMenuItem {
                                actionName: qsTr("Undo")
                                actionShortcut: "Ctrl+Z"
                                onTriggered: excuteOpt("Undo")
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Redo")
                                actionShortcut: "Ctrl+Y"
                                onTriggered: excuteOpt("Redo")
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Copy")
                                enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
                                actionShortcut: "Ctrl+C"
                                onTriggered: kernel_ui.onCopy()
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Paste")
                                enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
                                actionShortcut: "Ctrl+V"
                                onTriggered: kernel_ui.onPaste()
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Delete Model")
                                enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
                                actionShortcut: "Delete"
                                onTriggered: excuteOpt("Delete Model")
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Clear All")
                                onTriggered: excuteOpt("Clear All")
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Split Model")
                                enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
                                onTriggered: excuteOpt("Split Model")
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Merge Model")
                                enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
                                onTriggered: excuteOpt("Merge Model")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Merge Model Locations")
                                enabled: controlEnabled && isFDMPrinter && !kernel_model_selector.wipeTowerSelected
                                actionShortcut: "Ctrl+M"
                                onTriggered: excuteOpt("Unit As One")
                            }

                            BasicMenuItem {
                                separatorVisible: true
                                actionName: qsTr("Select All Model")
                                actionShortcut: "Ctrl+A"
                                onTriggered: excuteOpt("Select All Model")
                            }

                        }

                        BasicSubMenu {
                            id: viewMenu

                            title: qsTr("View") + "(&V)"
                            enabled: controlEnabled && isFDMPrinter

                            // BasicMenuItem {
                            //     separatorVisible: true
                            //     actionName: qsTr("ShowModelLine")
                            //     enabled: controlEnabled
                            //     onTriggered: excuteOpt("ShowModelLine")
                            //     itemChecked: actionCommands.getOpt("ShowModelLine")?actionCommands.getOpt("ShowModelLine").enabled : false
                            //     onVisibleChanged:{
                            //         if(actionCommands.getOpt("ShowModelLine")&&visible){
                            //             actionCommands.getOpt("ShowModelLine").updateCheck()
                            //         }
                            //     }
                            // }

                            BasicMenuItem {
                                actionName: qsTr("ShowModelFace")
                                enabled: controlEnabled
                                itemChecked: actionCommands.getOpt("ShowModelFace")?actionCommands.getOpt("ShowModelFace").enabled : false
                                onTriggered: excuteOpt("ShowModelFace")
                                onVisibleChanged:
                                {
                                    if(actionCommands.getOpt("ShowModelFace")&&visible){
                                        actionCommands.getOpt("ShowModelFace").updateCheck()
                                    }
                                }

                            }

                            BasicMenuItem {
                                actionName: qsTr("ShowModelFaceLine")
                                enabled: true
                                onTriggered: excuteOpt("ShowModelFaceLine")
                                itemChecked:actionCommands.getOpt("ShowModelFaceLine")?actionCommands.getOpt("ShowModelFaceLine").enabled : false
                                onVisibleChanged:{
                                    if(actionCommands.getOpt("ShowModelFaceLine")&&visible){
                                        actionCommands.getOpt("ShowModelFaceLine").updateCheck()
                                    }
                                }
                            }

                            Column {
                                width: viewMenu.width

                                Repeater {
                                    id: idMenuItems

                                    model: actionCommands.getOpt("View Show").subMenuActionmodel

                                    delegate: BasicMenuItem {
                                        width: viewMenu.width
                                        actionName: actionNameRole
                                        itemChecked: actionItem ? actionItem.enabled : false
                                        onVisibleChanged:{
                                            if(actionItem){
                                                actionItem.updateCheck()
                                            }
                                        }
                                        enabled: true
                                        onTriggered: {
                                            actionItem.execute();
                                            viewMenu.close();
                                        }
                                    }

                                }

                            }

                        }

                        BasicSubMenu {
                            id: toolMenu

                            title: qsTr("Tool") + "(&T)"
                            enabled: controlEnabled 
                            BasicMenuItem {
                                actionName: qsTr("Model Repair")
                                enabled: controlEnabled && isFDMPrinter
                                onTriggered: excuteOpt("Model Repair")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Manage Printer")
                                enabled: controlEnabled && isFDMPrinter
                                onTriggered: {
                                    if (kernel_parameter_manager.machinesCount > 0) {
                                        if (!machineManagerLoader.active)
                                            machineManagerLoader.active = true;

                                        machineManagerLoader.item.show();
                                    }
                                }
                                    //excuteOpt("Manage Printer")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Manage Materials")
                                enabled: controlEnabled && isFDMPrinter
                                onTriggered: {
                                    //      if (!materialManagerLoader.active) {
                                    //     materialManagerLoader.active = true;
                                    // }
                                    // materialManagerLoader.item.focusedMaterialIndex = 0;
                                    // materialManagerLoader.item.materialListVisible = true;
                                    // materialManagerLoader.item.show();
                                    standaloneWindow.showManageMaterialDialog();
                                } // standaloneWindow.showManageMaterialDialog() //excuteOpt("Manage Materials")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Preferences")
                                enabled: true
                                onTriggered: excuteOpt("Preferences")
                            }

                        }

                        BasicSubMenu {
                            id: modelLibraryMenu

                            title: qsTr("Models(&M)")
                            enabled: controlEnabled

                            BasicMenuItem {
                                actionName: qsTr("Models")
                                onTriggered: excuteOpt("Models")
                                actionShortcut: "Alt+M"
                            }

                            BasicMenuItem {
                                actionName: qsTr("Download Manage")
                                onTriggered: {
                                    cloudContext.downloadManageDialog.show()
                                }
                            }

                            BasicMenuItem {
                                actionName: qsTr("My Model")
                                onTriggered: validLogin(() => {
                                    return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_MODEL);
                                })
                            }

                            BasicMenuItem {
                                actionName: qsTr("My Slice")
                                onTriggered: validLogin(() => {
                                    return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_SLICE);
                                })
                            }

                            BasicMenuItem {
                                actionName: qsTr("My Devices")
                                onTriggered: validLogin(() => {
                                    return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.PRINTER);
                                })
                            }

                        }

                        BasicSubMenu {
                            id: calibrationMenu

                            title: qsTr("Calibration(&C)")
                            enabled: controlEnabled && isFDMPrinter
                            BasicMenuItem {
                                actionName: qsTr("Temperature")
                                onTriggered: excuteOpt("Temperature")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Coarse tune")
                                onTriggered: {
                                    kernel_slice_calibrate.lowflow();
                                    excuteOpt("Coarse tune");
                                }
                            }

                            BasicMenuItem {
                                //  excuteOpt("FlowFineTuning")

                                actionName: qsTr("Fine tune")
                                onTriggered: {
                                    kernel_slice_calibrate.highflow(1);
                                }
                            }

                            BasicMenuItem {
                                actionName: qsTr("PA")
                                onTriggered: excuteOpt("PA")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Retraction test")
                                onTriggered: excuteOpt("Retraction")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Maximum volume flow")
                                onTriggered: excuteOpt("MaxFlowVolume")
                            }

                            BasicMenuItem {
                                actionName: qsTr("VFA")
                                onTriggered: excuteOpt("VFA")
                            }

                            BasicMenuItem {
                                actionName: qsTr("Tutorial")
                                onTriggered: excuteOpt("Tutorial")
                            }

                        }

                        BasicSubMenu {
                            id: helpMenu

                            title: qsTr("Help") + "(&H)"

                            BasicMenuItem {
                                property string modelData: "About Us"

                                actionName: qsTr(modelData)
                                enabled: true
                                onTriggered: excuteOpt(modelData)
                            }

                            Loader {
                                active: kernel_global_const.upgradeable

                                sourceComponent: BasicMenuItem {
                                    property string modelData: "Update"

                                    actionName: qsTr(modelData)
                                    enabled: true
                                    onTriggered: excuteOpt(modelData)
                                }

                            }

                            Loader {
                                active: kernel_global_const.teachable

                                sourceComponent: BasicMenuItem {
                                    property string modelData: "Use Course"

                                    actionName: qsTr(modelData)
                                    enabled: true
                                    onTriggered: excuteOpt(modelData)
                                }

                            }

                            Loader {
                                active: kernel_global_const.feedbackable

                                sourceComponent: BasicMenuItem {
                                    property string modelData: "User Feedback"

                                    actionName: qsTr(modelData)
                                    enabled: true
                                    onTriggered: excuteOpt(modelData)
                                }

                            }

                            BasicMenuItem {
                                actionName: qsTr("Log View")
                                enabled: true
                                onTriggered: excuteOpt("Log View")
                            }
                            Loader {
                                active: kernel_global_const.isAlpha

                                sourceComponent: BasicMenuItem {
                                    property string modelData: "UNIT TEST"

                                    actionName: qsTr(modelData)
                                    enabled: true
                                    onTriggered:
                                    {
                                        idAllMenuDialog.requestDialog("unitTestDialog")

                                    }
                                }

                            }
                            // BasicMenuItem {
                            //     actionName: qsTr("Mouse Operation Function")
                            //     enabled: true
                            //     onTriggered: idAllMenuDialog.requestDialog("mosueInfo")
                            // }

                        }

                    }

                    delegate: MenuBarItem {
                        id: menuControl

                        implicitWidth: 46 * screenScaleFactor
                        implicitHeight: 30 * screenScaleFactor

                        background: Rectangle {
                            color: Constants.headerBackgroundColor
                            border.width: hovered ? 1 : 0
                            border.color: Constants.themeGreenColor
                        }

                        contentItem: Item {
                            Row {
                                spacing: 4 * screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter

                                Image {
                                    id: logoImage

                                    width: 20 * screenScaleFactor
                                    height: 20 * screenScaleFactor
                                    sourceSize: Qt.size(20, 20)
                                    source: "qrc:/UI/photo/menuImg/logo.svg"
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Image {
                                    width: 10 * screenScaleFactor
                                    height: 5 * screenScaleFactor
                                    fillMode: Image.PreserveAspectFit
                                    source: root.menu_down_img
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                            }

                        }

                    }

                }

                Rectangle {
                    width: 1 * screenScaleFactor
                    height: 20 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    color: root.dividing_line
                }

                MenuBar {
                    implicitHeight: 30 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    BasicMenu {
                        readonly property string imageSrc: "qrc:/UI/photo/menuImg/file_down.svg"
                        BasicMenuItem {
                            separatorVisible: false
                            actionName: qsTranslate("MenuCmdObj","New Project")
                            actionShortcut: "Ctrl+N"
                            onTriggered:
                            {

                                if(kernel_undo_proxy.canUndo)
                                {
                                    var receiver = {
                                    };
                                    receiver.onOkClicked = function() {
                                        project_kernel.saveCX3D(true)
                                    };
                                    receiver.onCancelClicked = function(){
                                        standaloneWindow.newProjectSaveParemDialog()
                                    }
                                    idAllMenuDialog.requestMenuDialog(receiver,"newProjectDialog")
                                }
                                else
                                {
                                    standaloneWindow.newProjectSaveParemDialog()
                                }
                            }
                        }

                        BasicMenuItem {
                            separatorVisible: false
                            actionName: qsTranslate("MenuCmdObj","Open Project")
                            onTriggered: excuteOpt("Open Project")
                        }

                        BasicMenuItem {
                            actionName: qsTranslate("MenuCmdObj","Save Project")
                            separatorVisible: false
                            actionShortcut: "Ctrl+S"
                            onTriggered:
                            {
                                project_kernel.saveCX3D(false)
                            }
                        }

                        BasicMenuItem {
                            separatorVisible: false
                            actionName: qsTr("Save As Project")
                            actionShortcut: "Ctrl+Shift+S"
                            onTriggered: excuteOpt("Save As Project")
                        }

                        BasicSubMenu {
                            separator: false
                            title: qsTr("Recently files")
                            mymodel: actionCommands.getOpt("Recently files").subMenuActionmodel
                        }

                        BasicSubMenu {
                            title: qsTr("Recently Opened Project")
                            mymodel: projectSubmodel.subMenuActionmodel
                        }

                        BasicDoubleSubMenu {
                            title: qsTr("Test Model")
                            mymodel: actionCommands.getOpt("Standard Model").subMenuActionmodel
                            mysecondmodel: actionCommands.getOpt("Test Model").subMenuActionmodel
                        }

                        Rectangle
                        {
                            id: idSeparator
                            width:idLeftMenu.width
                            height: 1
                            x:36//5
                            color: "#E0E0E0"//"white"
                            anchors.bottom: __import.top
                        }
                        BasicSubMenu {
                            id : __import
                            title:  qsTranslate("MenuCmdObj","Import")
                            mymodel: actionCommands.getOpt("Import SubMenu").subMenuActionmodel
                        }

                        BasicSubMenu {
                            title: qsTranslate("MenuCmdObj","Export")
                            mymodel: actionCommands.getOpt("Export SubMenu").subMenuActionmodel
                        }

                        BasicMenuItem {
                            separatorVisible: true
                            actionName: qsTr("Close")
                            actionShortcut: "Ctrl+Q"
                            onTriggered: excuteOpt("Close")
                        }

                    }

                    delegate: MenuBarItem {
                        implicitWidth: 56 * screenScaleFactor
                        implicitHeight: 30 * screenScaleFactor

                        background: Rectangle {
                            color: Constants.headerBackgroundColor
                            border.width: hovered ? 1 : 0
                            border.color: Constants.themeGreenColor
                        }

                        contentItem: Item {
                            Row {
                                spacing: 4
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter

                                Image {
                                    width: 16 * screenScaleFactor
                                    height: 16 * screenScaleFactor
                                    sourceSize: Qt.size(16, 16)
                                    source: root.file_down_img
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Text {
                                    font.pointSize: Constants.labelFontPointSize_10
                                    text: qsTr("File")
                                    color: Constants.textColor
                                    anchors.verticalCenter: parent.verticalCenter
                                    font.weight: Font.ExtraBold
                                }

                            }

                        }

                    }

                }

            }

        }

        Rectangle {
            width: 1 * screenScaleFactor
            height: 20 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            color: root.dividing_line
        }

          CusImglButton {
            id: openFileBtn

            anchors.verticalCenter: parent.verticalCenter
            width: 30 * screenScaleFactor
            height: 30 * screenScaleFactor
            imgWidth: 20 * screenScaleFactor
            imgHeight: 17 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            hoverBorderColor: Constants.themeGreenColor
            borderWidth: hovered ? 1 : 0
            opacity: 1
            text: qsTr("Open Projects")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: Constants.currentTheme ? true : false
            enabledIconSource: root.open_file_img
            pressedIconSource: root.open_file_img
            highLightIconSource: root.open_file_img
            onClicked: excuteOpt("Open Project")
        }


        CusImglButton {
            id: saveFileBtn

            anchors.verticalCenter: parent.verticalCenter
            width: 30 * screenScaleFactor
            height: 30 * screenScaleFactor
            imgWidth: 17 * screenScaleFactor
            imgHeight: 17 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            opacity: 1
            hoverBorderColor: Constants.themeGreenColor
            borderWidth: hovered ? 1 : 0
            text: qsTr("Save the project file")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: Constants.currentTheme ? true : false
            enabledIconSource: root.save_file_img
            pressedIconSource: root.save_file_img
            highLightIconSource: root.save_file_img
            onClicked:  //actionCommands.getOpt("Save As Project").execute()
            {
                project_kernel.saveCX3D(false)
            }
        }

         CusImglButton {
            id: preferencesBtn

            anchors.verticalCenter: parent.verticalCenter
            width: 30 * screenScaleFactor
            height: 30 * screenScaleFactor
            imgWidth: 17 * screenScaleFactor
            imgHeight: 17 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            opacity: 1
            hoverBorderColor: Constants.themeGreenColor
            borderWidth: hovered ? 1 : 0
            text:  qsTr("Preferences")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: Constants.currentTheme ? true : false
            enabledIconSource: root.preferences_file_img
            pressedIconSource: root.preferences_file_img
            highLightIconSource: root.preferences_file_img
            onClicked: excuteOpt("Preferences")
           
        }

        CusImglButton {
            id: undoBtn

            anchors.verticalCenter: parent.verticalCenter
            enabled: kernel_undo_proxy.canUndo
            width: 30 * screenScaleFactor
            height: 30 * screenScaleFactor
            imgWidth: 16 * screenScaleFactor
            imgHeight: 15 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            opacity: 1
            hoverBorderColor: Constants.themeGreenColor
            borderWidth: hovered ? 1 : 0
            text: qsTr("Undo")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: Constants.currentTheme
            enabledIconSource: root.undo_img
            pressedIconSource: root.undo_img
            highLightIconSource: root.undo_img
            onClicked: kernel_undo_proxy.undo()
        }

        CusImglButton {
            anchors.verticalCenter: parent.verticalCenter
            width: 30 * screenScaleFactor
            height: 30 * screenScaleFactor
            imgWidth: 16 * screenScaleFactor
            imgHeight: 15 * screenScaleFactor
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            selectedBtnBgColor: "transparent"
            opacity: 1
            hoverBorderColor: Constants.themeGreenColor
            borderWidth: hovered ? 1 : 0
            text: qsTr("Redo")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: Constants.currentTheme
            enabledIconSource: root.redo_img
            pressedIconSource: root.redo_img
            highLightIconSource: root.redo_img
            onClicked: kernel_undo_proxy.redo()
        }

    }

    Item {
        id: blankItem

        anchors.left: idLeftMenu.right
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: sliceBtnVisible
        objectName: "blankItem"
        Component.onCompleted: {
            frameLessView.setTitleItem(idTitleBar);
            frameLessView.setMaskItem(maskBtn);
        }

        Item {
            id: idTitleBar

            anchors.fill: parent
            z: 0
        }

        Item {
            id: maskBtn

            anchors.top: parent.top
            //anchors.topMargin: 1 * screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            width: 330 * screenScaleFactor
            height: 32 * screenScaleFactor
            //color: "red"
            //spacing: 10 * screenScaleFactor
            visible: kernel_parameter_manager.functionType === 0
            z: 1

            CusButton {
                id: prepairBtn

                anchors.left: parent.left
                width: 100 * screenScaleFactor
                height: 28 * screenScaleFactor
                radius: 5 * screenScaleFactor
                checkable: true
                isChecked: kernel_kernel.currentPhase === 0
                normalColor: "transparent"
                hoveredColor: "transparent"
                pressedColor: Constants.themeGreenColor
                hoveredBdColor: Constants.themeGreenColor
                txtColor: isChecked ? Constants.topBtnTextColor_checked : isHovered ? Constants.topBtnTextColor_hovered : Constants.topBtnTextColor_normal
                txtPointSize: Constants.labelFontPointSize_13
                txtFamily: Constants.labelFontFamilyBold
                txtWeight: Font.ExtraBold
                txtBold: true
                txtContent: qsTr("Prepare")
                onClicked: setPrepare()
            }

            CusButton {
                id: previewBtn

                anchors.left: prepairBtn.right
                anchors.leftMargin: 10 * screenScaleFactor
                width: 100 * screenScaleFactor
                height: 28 * screenScaleFactor
                radius: 5 * screenScaleFactor
                checkable: true
                enabled: kernel_kernel.currentPhase !== 1 && !standaloneWindow.isErrorExist
                isChecked: kernel_kernel.currentPhase === 1
                normalColor: "transparent"
                hoveredColor: "transparent"
                pressedColor: Constants.themeGreenColor
                hoveredBdColor: Constants.themeGreenColor
                txtColor: isChecked ? Constants.topBtnTextColor_checked : isHovered ? Constants.topBtnTextColor_hovered : Constants.topBtnTextColor_normal
                txtPointSize: Constants.labelFontPointSize_13
                txtFamily: Constants.labelFontFamilyBold
                txtWeight: Font.ExtraBold
                txtBold: true
                txtContent: qsTr("Preview")
                onClicked: setPreview()
            }

            CusButton {
                anchors.leftMargin: 10 * screenScaleFactor
                anchors.left: previewBtn.right
                width: 100 * screenScaleFactor
                height: 28 * screenScaleFactor
                radius: 5 * screenScaleFactor
                checkable: true
                enabled: __LANload.status === Loader.Ready
                isChecked: kernel_kernel.currentPhase === 2
                normalColor: "transparent"
                hoveredColor: "transparent"
                pressedColor: Constants.themeGreenColor
                hoveredBdColor: Constants.themeGreenColor
                txtColor: isChecked ? Constants.topBtnTextColor_checked : isHovered ? Constants.topBtnTextColor_hovered : Constants.topBtnTextColor_normal
                txtPointSize: Constants.labelFontPointSize_13
                txtFamily: Constants.labelFontFamilyBold
                txtWeight: Font.ExtraBold
                txtBold: true
                txtContent: qsTr("Device")
                onClicked: setDevice()
            }

        }

        anchors {
            left: idLeftMenu.right
            leftMargin: 4 * screenScaleFactor
            right: idControlButtonsRow.left
            rightMargin: 4 * screenScaleFactor
            top: parent.top
            topMargin: 4 * screenScaleFactor
            bottom: parent.bottom
        }

    }

    Row {
        id: idControlButtonsRow

        anchors.right: parent.right
        anchors.rightMargin: 10 * screenScaleFactor
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 1
        spacing: 15 * screenScaleFactor
        layoutDirection: Qt.RightToLeft
        objectName: "controlButtonsRow"

        TitleBarBtn {
            id: closeBtn

            anchors.verticalCenter: parent.verticalCenter
            visible: Qt.platform.os !== "osx"
            width: 30 * screenScaleFactor
            height: 30 * screenScaleFactor
            hoverBgColor: "#FD265A"
            borderwidth: 0
            // borderwidth: hovered?1:0
            // hoveredBdColor:Constants.themeGreenColor
            //  hoverBgColor: "transparent"
            buttonRadius: 5 * screenScaleFactor
            iconWidth: 10 * screenScaleFactor
            iconHeight: 11 * screenScaleFactor
            normalIconSource: root.close_img
            onClicked: {
                //                saveParem.visible = true
                frameLessView.close();
            }
        }

        TitleBarBtn {
            id: maxBtn

            property string maxPath: root.zoom_in_img
            property string normalPath: root.zoom_out_img

            anchors.verticalCenter: parent.verticalCenter
            visible: Qt.platform.os !== "osx"
            width: closeBtn.width
            height: closeBtn.height
            borderwidth: hovered ? 1 : 0
            hoveredBdColor: Constants.themeGreenColor
            hoverBgColor: "transparent"
            buttonRadius: 5 * screenScaleFactor
            normalIconSource: isMaxed ? normalPath : maxPath
            iconWidth: 12 * screenScaleFactor
            iconHeight: 10 * screenScaleFactor
            onClicked: isMaxed ? frameLessView.showNormal() : frameLessView.showMaximized()
        }

        TitleBarBtn {
            id: minBtn

            anchors.verticalCenter: parent.verticalCenter
            visible: Qt.platform.os !== "osx"
            width: closeBtn.width
            height: closeBtn.height
            borderwidth: hovered ? 1 : 0
            hoveredBdColor: Constants.themeGreenColor
            hoverBgColor: "transparent"
            buttonRadius: 5 * screenScaleFactor
            normalIconSource: root.hidden_img
            iconWidth: 9 * screenScaleFactor
            iconHeight: 1 * screenScaleFactor
            onClicked: {
                frameLessView.showLessViewMinimized();
            }
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 20 * screenScaleFactor
            layoutDirection: Qt.RightToLeft

            Rectangle {
                visible: Qt.platform.os !== "osx"
                width: 1 * screenScaleFactor
                height: 20 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                color: root.dividing_line
            }

            LoginBtn {
                id: idLoginBtn

                objectName: "loginBtn"
                visible: kernel_global_const.cxcloudEnabled
                anchors.verticalCenter: parent.verticalCenter
                Layout.alignment: Qt.AlignRight
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor
                btnImgUrl: root.login_img

                BasicTooltip {
                    id: loginTooptip

                    visible: idLoginBtn.hovered && String(text).length
                    position: BasicTooltip.Position.BOTTOM
                    text: cxkernel_cxcloud.accountService.logined ? qsTr("Personal Center") : qsTr("Login")
                }

            }

            DownloadManageButton {
                id: download_manage_button

                btnImgUrl: root.download_img
                visible: kernel_global_const.cxcloudEnabled
                Layout.alignment: Qt.AlignRight
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                downloadTaskCount: cloudContext.downloadManageDialog.downloadTaskCount
                finishedTaskCount: cloudContext.downloadManageDialog.finishedTaskCount
                onClicked: {
                    cloudContext.downloadManageDialog.visible = true;
                }

                BasicTooltip {
                    id: downLoadTooptip

                    visible: download_manage_button.hovered && String(text).length
                    position: BasicTooltip.Position.BOTTOM
                    text: qsTr("Download Manager")
                }

            }

            CusImglButton {
                id: idModelRecommendButton

                //  visible: kernel_global_const.cxcloudEnabled && kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType === 0
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor
                imgWidth: 20 * screenScaleFactor
                imgHeight: 20 * screenScaleFactor
                text: qsTr("Models")
                cusTooltip.position: BasicTooltip.Position.BOTTOM
                shadowEnabled: false
                bottonSelected: false
                defaultBtnBgColor: "transparent"
                hoveredBtnBgColor: "transparent"
                selectedBtnBgColor: "transparent"
                borderWidth: hovered ? 1 : 0
                hoverBorderColor: Constants.themeGreenColor
                enabledIconSource: Constants.leftbar_rocommand_btn_icon_default
                pressedIconSource: Constants.leftbar_rocommand_btn_icon_pressed
                highLightIconSource: Constants.leftbar_rocommand_btn_icon_hovered
                onClicked: {
                    if (cloudContext.modelLibarayDialog.visible)
                        cloudContext.modelLibarayDialog.refreshData();
                    else
                        actionCommands.getOpt("Models").execute();
                }
            }

        }

    }

    Loader {
        id: rootLoader

        source: "../secondqml/CusWizardHome.qml"
    }

    Component {
        id: wizardComp

        CusWizard {
            id: cusWizard

            anchors.fill: parent
            currentIndex: 0
            onWizardFinished: {
                destroy(cusWizard);
                showAddPrinterDlg();
            }
        }

    }

    gradient: Gradient {
        GradientStop {
            position: 0
            color: Constants.headerBackgroundColor
        }

        GradientStop {
            position: 1
            color: Constants.headerBackgroundColorEnd
        }

    }

}
