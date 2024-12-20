import CrealityUI 1.0
import Qt.labs.platform 1.1 as MacMenu
import Qt.labs.platform 1.0
import Qt.labs.settings 1.1
import QtQml 2.13
import QtQuick 2.13
import QtQuick.Controls 2.0 as QQC2
import QtQuick.Controls 2.5 as QQC25
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

MacMenu.MenuBar {
    function excuteOpt(optName) {
        Qt.callLater(function(opt_name) {
            actionCommands.getOpt(opt_name).execute();
        }, optName);
    }
    MacMenu.Menu {
        id: fileMenu

        title: qsTranslate("CWinMenu", "File" + "(&F)")

        MacMenu.MenuItem {
            id: __newProject

            text: qsTranslate("MenuCmdObj","New Project")
            shortcut: "Ctrl+N"
            onTriggered: {
                if(kernel_undo_proxy.canUndo)
                {
                    var receiver = {
                    };
                    receiver.onOkClicked = function() {
                        project_kernel.saveCX3D(true)
                    };
                    receiver.onCancelClicked = function(){
                        project_kernel.cancelSave()
                    }
                    idAllMenuDialog.requestMenuDialog(receiver,"newProjectDialog")
                }
                else
                {
                    project_kernel.cancelSave()
                }
            }
        }

        MacMenu.MenuItem {
            id: __openProject
            text: qsTranslate("MenuCmdObj","Open Project")
            onTriggered: {
                excuteOpt("Open Project");
            }
        }
        MacMenu.MenuItem {
            text: qsTranslate("MenuCmdObj","Save Project")
            shortcut : "Ctrl+S"
            onTriggered: {
                project_kernel.saveCX3D(false)
            }
        }
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu","Save As Project")
            onTriggered: {
                excuteOpt("Save As Project")
            }
        }

        MacMenu.Menu {
            id: __fileHistory

            title:  cxTr("Recent Files")

            Instantiator {
                id: __fileHistorySubMenu

                model: actionCommands.getOpt("Recently files").subMenuActionmodel
                onObjectAdded: __fileHistory.insertItem(0, object)
                onObjectRemoved: __fileHistory.removeItem(object)

                delegate: MacMenu.MenuItem {
                    text: model.actionNameRole
                    onTriggered: model.actionItem.execute()
                }

            }

        }

        MacMenu.Menu {
            id: __projectHistory

            title: qsTranslate("CWinMenu", "Recently Opened Project")

            Instantiator {
                id: __projectHistorySubMenu

                model: actionCommands.getOpt("Recently Opened Project").subMenuActionmodel
                onObjectAdded: __projectHistory.insertItem(0, object)
                onObjectRemoved: __projectHistory.removeItem(object)

                delegate: MacMenu.MenuItem {
                    text: model.actionNameRole
                    onTriggered: model.actionItem.execute()
                }

            }

        }
        MacMenu.Menu {
            id: __testModel

            title: qsTranslate("CWinMenu", "Test Model")

            Instantiator {
                id: __testModelSubMenu

                model: actionCommands.getOpt("Test Model").subMenuActionmodel
                onObjectAdded: __testModel.insertItem(0, object)
                onObjectRemoved: __testModel.removeItem(object)

                delegate: MacMenu.MenuItem {
                    text: model.actionNameRole
                    onTriggered: model.actionItem.execute()
                }

            }
            Instantiator {
                id: __standardSubMenu

                model: actionCommands.getOpt("Standard Model").subMenuActionmodel
                onObjectAdded: __testModel.insertItem(0, object)
                onObjectRemoved: __testModel.removeItem(object)

                delegate: MacMenu.MenuItem {
                    text: model.actionNameRole
                    onTriggered: model.actionItem.execute()
                }

            }
            

        }

        Rectangle
        {
            id: idSeparator
            width:idLeftMenu.width
            height: 1
            x:36//5
            color: "#E0E0E0"//"white"
            anchors.bottom: __importMenu.top
        }
        MacMenu.Menu {
            id: __importMenu
            title: qsTranslate("MenuCmdObj", "Import")

            Instantiator {

                model: actionCommands.getOpt("Import SubMenu").subMenuActionmodel
                onObjectAdded: __importMenu.insertItem(0, object)
                onObjectRemoved: ____importMenuimport.removeItem(object)

                delegate: MacMenu.MenuItem {
                    text: model.actionNameRole
                    onTriggered: model.actionItem.execute()
                }

            }
        }
        MacMenu.Menu {
            id: __exportMenu
            title: qsTranslate("MenuCmdObj", "Export")

            Instantiator {
                model: actionCommands.getOpt("Export SubMenu").subMenuActionmodel
                onObjectAdded: __exportMenu.insertItem(0, object)
                onObjectRemoved: __exportMenu.removeItem(object)

                delegate: MacMenu.MenuItem {
                    text: model.actionNameRole
                    onTriggered: model.actionItem.execute()
                }

            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Close")
            shortcut : "Ctrl+Q"
            onTriggered: {
                excuteOpt("Close")
            }
        }

        

    }

    MacMenu.Menu {
        id: editMenu

        title: qsTranslate("CWinMenu", "Edit") + "(&E)"

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Undo")
            enabled: controlEnabled && isFDMPrinter
            shortcut: "Ctrl+Z"
            onTriggered: {
                excuteOpt("Undo");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Redo")
            enabled: controlEnabled && isFDMPrinter
            shortcut: "Ctrl+Y"
            onTriggered: {
                excuteOpt("Redo");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Copy")
            enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
            shortcut: "Ctrl+C "
            onTriggered: {
                kernel_ui.onCopy();
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Paste")
            enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
            shortcut: "Ctrl+V "
            onTriggered: {
                kernel_ui.onPaste();
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Delete Model")
            enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
            shortcut: "Ctrl+D"
            onTriggered: {
                excuteOpt("Delete Model");
            }
        }

        MacMenu.MenuItem {
            text: cxTr("Clear All Models")
            enabled: controlEnabled
            shortcut: "Ctrl+Shift+D"
            onTriggered: {
                excuteOpt("Clear All");
            }
        }


        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Split Model")
            enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
            // shortcut: "Alt+S"
            onTriggered: {
                excuteOpt("Split Model");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Merge Model")
            enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
            // shortcut: "Alt+Shift+M"
            onTriggered: {
                excuteOpt("Merge Model");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Merge Model Locations")
            enabled: true && !kernel_model_selector.wipeTowerSelected
            shortcut: "Ctrl+M"
            onTriggered: {
                excuteOpt("Unit As One");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Select All Model")
            enabled: controlEnabled
            shortcut: "Ctrl+A"
            onTriggered: {
                excuteOpt("Select All Model");
            }
        }

        // MacMenu.MenuItem {
        //     text: qsTranslate("BasicMenuBarStyle", "Reset All Model")
        //     enabled: controlEnabled && !kernel_model_selector.wipeTowerSelected
        //     shortcut: "Ctrl+Shift+R"
        //     onTriggered: {
        //         excuteOpt("Reset All Model");
        //     }
        // }

    }

    MacMenu.Menu {
        id: viewMenu

        title: qsTranslate("CWinMenu", "View") + "(&V)"
        // ...
        enabled: controlEnabled && isFDMPrinter

        // MacMenu.MenuItem {
        //     text: qsTranslate("BasicMenuBarStyle", "ShowModelLine")
        //     enabled: controlEnabled
        //     onTriggered: {
        //         excuteOpt("ShowModelLine");
        //     }
        // }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "ShowModelFace")
            enabled: controlEnabled
            onTriggered: excuteOpt("ShowModelFace")
            onVisibleChanged:{
                if(actionCommands.getOpt("ShowModelFaceLine")){
                    if(visible)
                        actionCommands.getOpt("ShowModelFaceLine").updateCheck()
                }
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "ShowModelFaceLine")
            enabled: true
            onTriggered: excuteOpt("ShowModelFaceLine")
            onVisibleChanged:{
                if(actionCommands.getOpt("ShowModelFaceLine")){
                    if(visible)
                        actionCommands.getOpt("ShowModelFaceLine").updateCheck()
                }
            }
        }

        Instantiator {
            id: idMenuItems

            model: actionCommands.getOpt("View Show").subMenuActionmodel
            onObjectAdded: viewMenu.insertItem(2, object)
            onObjectRemoved: viewMenu.removeItem(object)

            delegate: MacMenu.MenuItem {
                text: model.actionNameRole
                enabled: true
                onTriggered: {
                    model.actionItem.execute();
                    viewMenu.close();
                }
            }

        }

    }

    MacMenu.Menu {
        id: toolMenu

        title: qsTranslate("CWinMenu", "Tool") + "(&T)"
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Model Repair")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Model Repair");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Manage Printer")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                if (kernel_parameter_manager.machinesCount > 0) {
                    if (!machineManagerLoader.active)
                        machineManagerLoader.active = true;

                    machineManagerLoader.item.show();
                }
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Manage Filaments")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                //                excuteOpt("Manage Material")
                standaloneWindow.showManageMaterialDialog();
            }
        }
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Preferences")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Preferences");
            }
        }
        // MacMenu.MenuItem {
        //     text: qsTranslate("BasicMenuBarStyle", "Log View")
        //     enabled: true
        //     onTriggered: {
        //         excuteOpt("Log View");
        //     }
        // }

    }

    MacMenu.Menu {
        id: modelLibraryMenu

        title: qsTranslate("CWinMenu", "Creality Cloud") + "(&M)"
        enabled: isFDMPrinter

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Model Library")
            enabled: controlEnabled && isFDMPrinter
            shortcut : "Alt+M"
            onTriggered: {
                excuteOpt("Model Library");
            }
        }
        MacMenu.MenuItem
        {
            text: qsTranslate("CWinMenu", "Download Manage")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                cloudContext.downloadManageDialog.show()
            }

        }
        MacMenu.MenuItem
        {
            text: qsTranslate("CWinMenu", "My Model")
            enabled: controlEnabled && isFDMPrinter
            onTriggered:validLogin(() => {
                                       return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_MODEL);
                                   })

        }
        MacMenu.MenuItem
        {
            text: qsTranslate("CWinMenu", "My Slice")
            enabled: controlEnabled && isFDMPrinter
            onTriggered:validLogin(() => {
                                       return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_SLICE);
                                   })

        }
        MacMenu.MenuItem
        {
            text: qsTranslate("CWinMenu", "My Devices")
            enabled: controlEnabled && isFDMPrinter
            onTriggered:validLogin(() => {
                                       return cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.PRINTER);
                                   })

        }
    }

    MacMenu.Menu {
        id: calibrationMenu

        title: qsTranslate("CWinMenu", "Calibration(&C)")

        // ...
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Temperature")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Temperature");
            }
        }
        MacMenu.Menu {
            title: qsTranslate("CWinMenu", "Flow")
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Coarse tune")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    kernel_slice_calibrate.lowflow();
                    excuteOpt("Coarse tune");
                }
            }

            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Fine tune")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    kernel_slice_calibrate.highflow(1);
                }
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "PA")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("PA");
            }
        }
        MacMenu.Menu {
            title: qsTranslate("CWinMenu", "Retraction")
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu","Retraction Distance")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: excuteOpt("RetractionDistance")
            }
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu","Retraction Speed")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: excuteOpt("RetractionSpeed")
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Maximum volume flow")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("MaxFlowVolume");
            }
        }

        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "VFA")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("VFA");
            }
        }
        MacMenu.Menu {
            title: qsTranslate("CWinMenu", "Speed")
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Max Speed")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("MaxSpeed");
                }
            }
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Speed Tower")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("SpeedTower");
                }
            }
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Jitter Speed")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("JitterSpeed");
                }
            }
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Fan Speed")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("FanSpeed");
                }
            }
        }
        MacMenu.Menu {
            title: qsTranslate("CWinMenu", "Acceleration")
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Max Acceleration")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("MaxAcceleration");
                }
            }
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Acceleration Tower")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("AccelerationTower");
                }
            }
            MacMenu.MenuItem {
                text: qsTranslate("CWinMenu", "Slow Acceleration")
                enabled: controlEnabled && isFDMPrinter
                onTriggered: {
                    excuteOpt("SlowAcceleration");
                }
            }
        }
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Arc Fitting")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                kernel_slice_calibrate.arc2lerance(0,0,0)
            }
        }
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu", "Tutorial")
            enabled: controlEnabled && isFDMPrinter
            onTriggered: {
                excuteOpt("Tutorial");
            }
        }

    }

    MacMenu.Menu {
        id: helpMenu

        title: qsTranslate("CWinMenu", "Help") + "(&H)"

        MacMenu.MenuItem {
            property string modelData: "About Us"

            text: qsTranslate("CWinMenu", modelData)
            enabled: true
            onTriggered: {
                excuteOpt(modelData);
            }
        }

        Loader {
            active: kernel_global_const.upgradeable

            sourceComponent: MacMenu.MenuItem {
                property string modelData: "Update"

                text: qsTranslate("CWinMenu", modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        Loader {
            active: kernel_global_const.teachable

            sourceComponent: MacMenu.MenuItem {
                property string modelData: "Use Course"

                text: qsTranslate("CWinMenu", modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        Loader {
            active: kernel_global_const.feedbackable

            sourceComponent: MacMenu.MenuItem {
                property string modelData: "User Feedback"

                text: qsTranslate("CWinMenu", modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }
        MacMenu.MenuItem {
            text: qsTranslate("CWinMenu","Log View")
            enabled: true
            onTriggered: excuteOpt("Log View")
        }
        // MacMenu.MenuItem {
        //     text: qsTranslate("CWinMenu", "Mouse Operation Function")
        //     enabled: true
        //     onTriggered: {
        //         idAllMenuDialog.mosueInfo.visible = true;
        //     }
        // }

    }

}
