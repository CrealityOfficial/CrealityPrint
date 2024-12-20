import CrealityUI 1.0
import QtQml 2.0
import QtQml 2.3
import QtQuick 2.0
import QtQuick.Controls 2.12
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/cxcloud"

MenuBar {
    id: idMenuBar

    property var controlEnabled: kernel_kernel.currentPhase === 0
    property var isFDMPrinter: kernel_parameter_manager.functionType === 0
    property var isLaserPrinter: kernel_parameter_manager.functionType === 1
    property var isCNCPrinter: kernel_parameter_manager.functionType === 2
    property bool logined: cxkernel_cxcloud.accountService.logined
    property var cloudContext: standaloneWindow.cloudContext

    signal barItemTriggered()

    function validLogin(cb, showUserDialog = true) {
        if (!logined)
            return cloudContext.loginDialog.show();

        cb();
        if (showUserDialog)
            cloudContext.userInfoDialog.show();

    }

    function indexOfMenu(menu) {
        let menu_list = idMenuBar.menus;
        for (let index = 0; index < menu_list.length; ++index) {
            if (menu_list[index] === menu)
                return index;

        }
        return -1;
    }

    function showFDMMenuBar() {
    }

    function onlyShowOtherMenuBar() {
    }

    function excuteOpt(optName) {
        Qt.callLater(function(opt_name) {
            actionCommands.getOpt(opt_name).execute();
        }, optName);
    }

    // property color backgroundColor: "white"
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: 40 * screenScaleFactor /*Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)*/
    Component.onCompleted: {
        if (!kernel_global_const.cxcloudEnabled)
            takeMenu(indexOfMenu(modelLibraryMenu));

    }

    BasicMenu {
        id: fileMenu

        title: qsTr("File(&F)")
        maxWidth: Math.max(implicitContentWidth, 280)
        enabled: controlEnabled && isFDMPrinter

        BasicMenuItem {
            id: __openfile

            actionName: qsTr("Open File")
            //                enabled : controlEnabled
            actionShortcut: "Ctrl+O"
            onTriggered: {
                excuteOpt("Open File");
            }
        }

        BasicMenuItem {
            id: __openProject

            actionName: qsTr("Open Project")
            //                enabled : controlEnabled
            separatorVisible: true
            onTriggered: {
                excuteOpt("Open Project");
            }
        }

        BasicSubMenu {
            //                enabled : controlEnabled

            id: __fileHistory

            separator: false
            title: cxTr("Recent Files")
            mymodel: actionCommands.getOpt("Recently files").subMenuActionmodel
        }

        BasicSubMenu {
            //                enabled : controlEnabled

            id: __projectHistory

            title: qsTr("Recently Opened Project")
            mymodel: projectSubmodel.subMenuActionmodel
        }

        BasicMenuItem {
            id: __saveSTL

            separatorVisible: true
            actionName: qsTr("Save STL")
            //                enabled : HalotContext.obj("ModelSelector").nocxbinSelectionNum > 0
            onTriggered: {
                excuteOpt("Save STL");
            }
        }

        BasicMenuItem {
            id: __saveProject

            separatorVisible: true
            actionName: qsTr("Save As Project")
            onTriggered: {
                excuteOpt("Save As Project");
            }
        }

        BasicSubMenu {
            id: __standardModel

            title: qsTr("Standard Model")
            mymodel: actionCommands.getOpt("Standard Model").subMenuActionmodel
        }

        BasicSubMenu {
            id: __testdModel

            title: qsTr("Test Model")
            mymodel: actionCommands.getOpt("Test Model").subMenuActionmodel
        }

        BasicMenuItem {
            id: __importPresetConfig

            separatorVisible: true
            actionName: qsTr("Import Preset Config")
            onTriggered: {
                excuteOpt("Import Preset");
            }
        }

        BasicMenuItem {
            id: __exportPresetConfig

            separatorVisible: true
            actionName: qsTr("Export Preset Config")
            onTriggered: {
                excuteOpt("Export Preset");
            }
        }

        BasicMenuItem {
            id: __load3MF

            separatorVisible: true
            actionName: qsTr("Load 3MF")
            //                enabled : HalotContext.obj("ModelSelector").nocxbinSelectionNum > 0
            onTriggered: {
                excuteOpt("Load 3MF");
            }
        }

        BasicMenuItem {
            id: __save3MF

            separatorVisible: true
            actionName: qsTr("Save 3MF")
            //                enabled : HalotContext.obj("ModelSelector").nocxbinSelectionNum > 0
            onTriggered: {
                excuteOpt("Save 3MF");
            }
        }

        BasicMenuItem {
            id: __closeProject

            separatorVisible: true
            actionName: qsTr("Close")
            actionShortcut: "Ctrl+Q"
            onTriggered: {
                excuteOpt("Close");
            }
        }
        // ...

    }

    BasicMenu {
        id: editMenu

        title: qsTr("Edit") + "(&E)"

        BasicMenuItem {
            actionName: qsTr("Undo")
            enabled: controlEnabled && isFDMPrinter
            actionShortcut: "Ctrl+Z"
            onTriggered: {
                excuteOpt("Undo");
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Redo")
            enabled: controlEnabled && isFDMPrinter
            actionShortcut: "Ctrl+Y"
            onTriggered: {
                excuteOpt("Redo");
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Copy")
            enabled: controlEnabled
            actionShortcut: "Ctrl+C "
            onTriggered: {
                kernel_ui.onCopy();
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Paste")
            enabled: controlEnabled
            actionShortcut: "Ctrl+V "
            onTriggered: {
                kernel_ui.onPaste();
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Delete Model")
            enabled: controlEnabled
            actionShortcut: "Ctrl+D"
            onTriggered: {
                excuteOpt("Delete Model");
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Clear All")
            enabled: controlEnabled
            actionShortcut: "Ctrl+Shift+D"
            onTriggered: {
                excuteOpt("Clear All");
            }
        }

        BasicMenuItem {
            //                excuteOpt("Clone Model")

            separatorVisible: true
            actionName: qsTr("Clone Model")
            enabled: controlEnabled
            actionShortcut: enabled ? "Ctrl+Shift+C" : ""
            onTriggered: {
                kernel_kernelui.otherCommandIndex = 0;
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Split Model")
            enabled: controlEnabled
            actionShortcut: "Alt+S"
            onTriggered: {
                excuteOpt("Split Model");
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Merge Model")
            enabled: controlEnabled
            actionShortcut: "Alt+Shift+M"
            onTriggered: {
                excuteOpt("Merge Model");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Merge Model Locations")
            enabled: controlEnabled && isFDMPrinter
            actionShortcut: "Ctrl+M"
            onTriggered: {
                excuteOpt("Unit As One");
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Select All Model")
            enabled: controlEnabled
            actionShortcut: "Ctrl+A"
            onTriggered: {
                excuteOpt("Select All Model");
            }
        }

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("Reset All Model")
            enabled: controlEnabled
            actionShortcut: "Ctrl+Shift+R"
            onTriggered: {
                excuteOpt("Reset All Model");
            }
        }

    }

    BasicMenu {
        id: viewMenu

        title: qsTr("View") + "(&V)"
        // ...
        enabled: controlEnabled && isFDMPrinter

        BasicMenuItem {
            separatorVisible: true
            actionName: qsTr("ShowModelLine")
            enabled: controlEnabled
            onTriggered: {
                excuteOpt("ShowModelLine");
            }
        }

        BasicMenuItem {
            actionName: qsTr("ShowModelFace")
            enabled: controlEnabled
            onTriggered: {
                excuteOpt("ShowModelFace");
            }
        }

        BasicMenuItem {
            actionName: qsTr("ShowModelFaceLine")
            enabled: true
            onTriggered: {
                excuteOpt("ShowModelFaceLine");
            }
        }
        //        BasicMenuItem{

        //            actionName: qsTr("Merge Model Locations")
        //            enabled : true
        //            onTriggered: {
        //                excuteOpt("Unit As One")
        //            }
        //        }
        Column {
            width: viewMenu.width

            Repeater {
                id: idMenuItems

                model: actionCommands.getOpt("View Show").subMenuActionmodel

                delegate: BasicMenuItem {
                    width: viewMenu.width
                    actionName: actionNameRole
                    enabled: true
                    onTriggered: {
                        actionItem.execute();
                        viewMenu.close();
                    }
                }

            }

        }

    }

    BasicMenu {
        id: toolMenu

        title: qsTr("Tool") + "(&T)"

        // ...
        BasicSubMenu {
            separator: false
            title: qsTr("Language")
            mymodel: actionCommands.getOpt("Language").subMenuActionmodel
        }

        BasicSubMenu {
            separator: false
            title: qsTr("Theme color change")
            mymodel: actionCommands.getOpt("Theme color change").subMenuActionmodel
        }

//        BasicMenuItem {
//            visible: Qt.platform.os !== "windows"
//            actionName: qsTr("Model Repair")
//            enabled: controlEnabled && isFDMPrinter
//            onTriggered: {
//                excuteOpt("Model Repair");
//            }
//        }

        BasicMenuItem {
            actionName: qsTr("Manage Printer")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Manage Printer");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Manage Filaments")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                //                excuteOpt("Manage Material")
                standaloneWindow.showManageMaterialDialog();
            }
        }

        BasicMenuItem {
            actionName: qsTr("Preferences")
            enabled: true
            onTriggered: {
                excuteOpt("Preferences");
            }
        }

        Loader {
            active: kernel_global_const.sliceHeightSettingEnabled

            sourceComponent: BasicMenuItem {
                enabled: kernel_modelspace.modelNNum > 0
                actionName: "切片高度设置"
                onTriggered: {
                    toolMenu.close();
                    kernel_ui.requestQmlDialog("slice_height_setting_dialog");
                }
            }

        }

        Loader {
            active: kernel_global_const.partitionPrintEnabled

            sourceComponent: BasicMenuItem {
                enabled: kernel_modelspace.modelNNum > 0
                actionName: "分区打印"
                onTriggered: {
                    toolMenu.close();
                    kernel_ui.requestQmlDialog("partition_print_dialog");
                }
            }

        }

    }

    BasicMenu {
        id: modelLibraryMenu

        title: qsTr("Creality Cloud") + "(&M)"
        enabled: isFDMPrinter

        BasicMenuItem {
            actionName: qsTr("Model Library")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Models");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Download Manage")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: validLogin(() => {
                return cloudContext.downloadManageDialog.show();
            }, false)
        }

        BasicMenuItem {
            actionName: qsTr("My Model")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: validLogin(() => {
                return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_MODEL);
            })
        }

        BasicMenuItem {
            actionName: qsTr("My Slice")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: validLogin(() => {
                return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_SLICE);
            })
        }

        BasicMenuItem {
            actionName: qsTr("My Devices")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: validLogin(() => {
                return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.PRINTER);
            })
        }

    }

    BasicMenu {
        id: calibrationMenu

        title: qsTr("Calibration(&C)")

        // ...
        BasicMenuItem {
            actionName: qsTr("Temperature")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Temperature");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Coarse tune")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                kernel_slice_calibrate.lowflow();
                excuteOpt("Coarse tune");
            }
        }

        BasicMenuItem {
            // excuteOpt("FlowFineTuning")

            actionName: qsTr("Fine tune")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                kernel_slice_calibrate.highflow(1);
            }
        }

        BasicMenuItem {
            actionName: qsTr("PA")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("PA");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Retraction test")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Retraction");
            }
        }

        BasicMenuItem {
            actionName: cxTr("Maximum volume flow")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("MaxFlowVolume");
            }
        }

        BasicMenuItem {
            actionName: qsTr("VFA")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("VFA");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Tutorial")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Tutorial");
            }
        }

    }

    BasicMenu {
        id: helpMenu

        title: qsTr("Help") + "(&H)"

        BasicMenuItem {
            property string modelData: "About Us"

            actionName: qsTr(modelData)
            enabled: true
            onTriggered: {
                excuteOpt(modelData);
            }
        }

        Loader {
            active: kernel_global_const.upgradeable

            sourceComponent: BasicMenuItem {
                property string modelData: "Update"

                actionName: qsTr(modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        Loader {
            active: kernel_global_const.teachable

            sourceComponent: BasicMenuItem {
                property string modelData: "Use Course"

                actionName: qsTr(modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        Loader {
            active: kernel_global_const.feedbackable

            sourceComponent: BasicMenuItem {
                property string modelData: "User Feedback"

                actionName: qsTr(modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        BasicMenuItem {
            actionName: qsTr("Log View")
            enabled: true
            onTriggered: {
                excuteOpt("Log View");
            }
        }

        BasicMenuItem {
            actionName: qsTr("Mouse Operation Function")
            enabled: true
            onTriggered: {
                idAllMenuDialog.mosueInfo.visible = true;
            }
        }

    }

    delegate: BasicMenuBarItemStyle {
        leftPadding: 10 * screenScaleFactor
        rightPadding: 10 * screenScaleFactor
        //        topPadding : 5
        //        width: 67* screenScaleFactor
        //        padding:20* screenScaleFactor
        height: idMenuBar.height
        onTriggered: {
            barItemTriggered();
        }
    }

}
