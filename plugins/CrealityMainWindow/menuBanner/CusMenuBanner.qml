import CrealityUIComponent 1.0
import QtQml 2.0
import QtQml 2.3
import QtQuick 2.0
import QtQuick.Controls 2.12
import "qrc:/qml/CrealityUIComponent"

MenuBar {
    id: idMenuBar

    property bool controlEnabled: true

    signal barItemTriggered()

    function indexOfMenu(menu) {
        let menu_list = idMenuBar.menus;
        for (let index = 0; index < menu_list.length; ++index) {
            if (menu_list[index] === menu)
                return index;

        }
        return -1;
    }

    function excuteOpt(optName) {
        Qt.callLater(function(opt_name) {
            actionCommands.getOpt(opt_name).execute();
        }, optName);
    }

    // property color backgroundColor: "white"
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: 40 * screenScaleFactor
    Component.onCompleted: {
    }

    CusMenu {
        id: fileMenu

        title: qsTr("File(&F)")

        CusMenuItem {
            id: __openfile

            actionName: qsTr("Open File")
            actionShortcut: "Ctrl+O"
            onTriggered: {
                excuteOpt("Open File");
            }
        }

        CusMenuItem {
            id: __openProject

            actionName: qsTr("Open Project")
            separatorVisible: true
            onTriggered: {
                excuteOpt("Open Project");
            }
        }

        CusMenu {
            id: __fileHistory

            separator: false
            title: qsTr("Recently files")
        }

        CusMenu {
            id: __projectHistory

            title: qsTr("Recently Opened Project")
        }

        CusMenuItem {
            id: __saveSTL

            separatorVisible: true
            actionName: qsTr("Save STL")
            onTriggered: {
                excuteOpt("Save STL");
            }
        }

        CusMenuItem {
            id: __saveProject

            separatorVisible: true
            actionName: qsTr("Save As Project")
            onTriggered: {
                excuteOpt("Save As Project");
            }
        }

        CusMenu {
            id: __standardModel

            title: qsTr("Standard Model")
        }

        CusMenuItem {
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

    CusMenu {
        id: editMenu

        title: qsTr("Edit") + "(&E)"

        CusMenuItem {
            actionName: qsTr("Undo")
            actionShortcut: "Ctrl+Z"
            onTriggered: {
                excuteOpt("Undo");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Redo")
            actionShortcut: "Ctrl+Y"
            onTriggered: {
                excuteOpt("Redo");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Copy")
            enabled: controlEnabled
            actionShortcut: "Ctrl+C "
            onTriggered: {
                kernel_ui.onCopy();
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Paste")
            enabled: controlEnabled
            actionShortcut: "Ctrl+V "
            onTriggered: {
                kernel_ui.onPaste();
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Delete Model")
            enabled: controlEnabled
            actionShortcut: "Ctrl+D"
            onTriggered: {
                excuteOpt("Delete Model");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Clear All")
            enabled: controlEnabled
            actionShortcut: "Ctrl+Shift+D"
            onTriggered: {
                excuteOpt("Clear All");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Clone Model")
            enabled: controlEnabled
            actionShortcut: "Alt+C"
            onTriggered: {
                excuteOpt("Clone Model");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Split Model")
            enabled: controlEnabled
            actionShortcut: "Alt+S"
            onTriggered: {
                excuteOpt("Split Model");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Merge Model")
            enabled: controlEnabled
            actionShortcut: "Alt+Shift+M"
            onTriggered: {
                excuteOpt("Merge Model");
            }
        }

        CusMenuItem {
            actionName: qsTr("Merge Model Locations")
            enabled: true
            actionShortcut: "Ctrl+M"
            onTriggered: {
                excuteOpt("Unit As One");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Select All Model")
            enabled: controlEnabled
            actionShortcut: "Ctrl+A"
            onTriggered: {
                excuteOpt("Select All Model");
            }
        }

        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("Reset All Model")
            enabled: controlEnabled
            actionShortcut: "Ctrl+Shift+R"
            onTriggered: {
                excuteOpt("Reset All Model");
            }
        }

    }

    CusMenu {
        id: viewMenu

        title: qsTr("View") + "(&V)"

        // ...
        CusMenuItem {
            separatorVisible: true
            actionName: qsTr("ShowModelLine")
            enabled: controlEnabled
            onTriggered: {
                excuteOpt("ShowModelLine");
            }
        }

        CusMenuItem {
            actionName: qsTr("ShowModelFace")
            enabled: controlEnabled
            onTriggered: {
                excuteOpt("ShowModelFace");
            }
        }

        CusMenuItem {
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

                model: 5

                delegate: CusMenuItem {
                    width: viewMenu.width
                    actionName: index
                    enabled: true
                    onTriggered: {
                        actionItem.execute();
                        viewMenu.close();
                    }
                }

            }

        }

    }

    CusMenu {
        id: toolMenu

        title: qsTr("Tool") + "(&T)"

        // ...
        CusMenu {
            separator: false
            title: qsTr("Language")
        }

        CusMenu {
            separator: false
            title: qsTr("Theme color change")
        }

        CusMenuItem {
            actionName: qsTr("Model Repair")
            onTriggered: {
                excuteOpt("Model Repair");
            }
        }

        CusMenuItem {
            actionName: qsTr("Manage Printer")
            onTriggered: {
                excuteOpt("Manage Printer");
            }
        }

        CusMenuItem {
            actionName: qsTr("Manage Materials")
            onTriggered: {
                //                excuteOpt("Manage Material")
                standaloneWindow.showManageMaterialDialog();
            }
        }

        CusMenuItem {
            actionName: qsTr("Log View")
            enabled: true
            onTriggered: {
                excuteOpt("Log View");
            }
        }

        Loader {

            sourceComponent: CusMenuItem {
                actionName: "切片高度设置"
                onTriggered: {
                    toolMenu.close();
                    kernel_ui.requestQmlDialog("slice_height_setting_dialog");
                }
            }

        }

        Loader {

            sourceComponent: CusMenuItem {
                actionName: "分区打印"
                onTriggered: {
                    toolMenu.close();
                    kernel_ui.requestQmlDialog("partition_print_dialog");
                }
            }

        }

    }

    CusMenu {
        id: modelLibraryMenu

        title: qsTr("Models(&M)")

        CusMenuItem {
            actionName: qsTr("Models")
            onTriggered: {
                excuteOpt("Models");
            }
        }

    }

    CusMenu {
        id: calibrationMenu

        title: qsTr("Calibration(&C)")

        // ...
        CusMenuItem {
            actionName: qsTr("Temperature")
            onTriggered: {
                excuteOpt("Temperature");
            }
        }

        CusMenuItem {
            actionName: qsTr("Coarse tune")
            onTriggered: {
                kernel_slice_calibrate.lowflow();
                excuteOpt("Coarse tune");
            }
        }

        CusMenuItem {
            actionName: qsTr("Fine tune")
            onTriggered: {
                // kernel_slice_calibrate.highflow()
                excuteOpt("FlowFineTuning");
            }
        }

        CusMenuItem {
            actionName: qsTr("PA")
            onTriggered: {
                excuteOpt("PA");
            }
        }

        CusMenuItem {
            actionName: qsTr("Maximum volume flow")
            onTriggered: {
                excuteOpt("MaxFlowVolume");
            }
        }

        CusMenuItem {
            actionName: qsTr("VFA")
            onTriggered: {
                excuteOpt("VFA");
            }
        }

        CusMenuItem {
            actionName: qsTr("Tutorial")
            onTriggered: {
                excuteOpt("Tutorial");
            }
        }

    }

    CusMenu {
        id: helpMenu

        title: qsTr("Help") + "(&H)"

        CusMenuItem {
            property string modelData: "About Us"

            actionName: qsTr(modelData)
            enabled: true
            onTriggered: {
                excuteOpt(modelData);
            }
        }

        Loader {

            sourceComponent: CusMenuItem {
                property string modelData: "Update"

                actionName: qsTr(modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        Loader {

            sourceComponent: CusMenuItem {
                property string modelData: "Use Course"

                actionName: qsTr(modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        Loader {

            sourceComponent: CusMenuItem {
                property string modelData: "User Feedback"

                actionName: qsTr(modelData)
                enabled: true
                onTriggered: {
                    excuteOpt(modelData);
                }
            }

        }

        CusMenuItem {
            actionName: qsTr("Mouse Operation Function")
            enabled: true
            onTriggered: {
                idAllMenuDialog.mosueInfo.visible = true;
            }
        }

    }

}
