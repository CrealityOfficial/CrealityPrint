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
import "qrc:/CrealityUI/secondqml"

QQC2.ApplicationWindow {
    id: standaloneWindow

    property int tmpWidth: 0
    property int defultRightPanelHeight: 630 * screenScaleFactor
    property var imageSource: (Constants.currentTheme === 0) ? "qrc:/UI/photo/bgImg_black.png" : "qrc:/UI/photo/bgImg.png"
    property bool isWindow: Qt.platform.os === "windows" ? true : false
    property alias leftToolBar: leftToolBar
    property alias statusBarItem: idStatusBar
    property alias g_rightPanel: swipeView
    property alias dockContext: dock_context
    property alias zSlider: layerSliderRange
    property alias resizeItem: resize_item
    property alias tipsItem: tipsItem
    property alias menuDialog: idAllMenuDialog
    property alias gGlScene: editorContent
    property int controlEnabled: kernel_kernel.currentPhase === 0
    property int isFDMPrinter: kernel_parameter_manager.functionType === 0
    property int isLaserPrinter: kernel_parameter_manager.functionType === 1
    property int isCNCPrinter: kernel_parameter_manager.functionType === 2

    signal sigInitAdvanceParam()

    function startSlice() {
        var ret = kernel_modelspace.haveModelsOutPlatform();
        if (ret)
            idOutPlatform.visible = true;
        else
            doSlice();
    }

    function doSlice() {
        kernel_kernel.setKernelPhase(1);
        kernel_slice_flow.slice();
    }

    function showReleaseNote() {
        idReleaseNote.exec();
    }

    function changeThemeColor(themeType) {
        Constants.currentTheme = themeType;
    }

    //lisugui 2020-10-29
    function beforeCloseWindow() {
        kernel_kernelui.beforeCloseWindow();
    }

    function showFdmPmDlg(filePath) {
        idFdmPmDlg.visible = true;
        idFdmPmDlg.filePath = filePath;
    }

    function showWizardDlg() {
        sigInitAdvanceParam();
        __LANload.sourceComponent = __printerComponet;
    }

    function excuteOpt(optName) {
        Qt.callLater(function(opt_name) {
            actionCommands.getOpt(opt_name).execute();
        }, optName);
    }

    function showManagePrinterDialog() {
        idManagerPrinterWizard.visible = true;
    }

    function showManageMaterialDialog() {
        //        idManagerPrinterWizard.visible = true
        idSlicePanel.showEditMaterialDlg();
    }

    function doMinimized() {
        if (!isWindow)
            flags = Qt.Window | Qt.WindowFullscreenButtonHint | Qt.CustomizeWindowHint | Qt.WindowMinimizeButtonHint;
        else
            flags = Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint;
        visibility = Window.Minimized;
    }

    flags: Qt.platform.os !== "osx" ? Qt.FramelessWindowHint | Qt.Window | Qt.WindowMinimizeButtonHint : Qt.Window | Qt.WindowFullScreen | Qt.WindowFullscreenButtonHint
    visibility: Window.FullScreen
    title: qsTr("Creative3D")
    visible: false
    color: "white"
    minimumHeight: (720 * screenScaleFactor) > Screen.desktopAvailableHeight ? Screen.desktopAvailableHeight : (720 * screenScaleFactor) //400
    minimumWidth: (1280 * screenScaleFactor) > Screen.desktopAvailableWidth ? Screen.desktopAvailableWidth : (1280 * screenScaleFactor)
    onClosing: {
        var a = kernel_kernel.checkUnsavedChanges();
        close.accepted = a;
    }
    Component.onCompleted: {
        kernel_kernelui.leftToolbar = leftToolBar;
        kernel_kernelui.appWindow = standaloneWindow;
        kernel_kernelui.footer = idStatusBar;
        kernel_kernelui.glMainView = editorContent;
        kernel_kernelui.topbar = idMenuBar;
        kernel_kernelui.fontList = Constants.fontList;
        //kernel_kernel.setGLQuickItem(editorContent)
        kernel_kernel.setScene3DWrapper(editorContent);
        //bind wizard
        Constants.screenScaleFactor = WizardUI.getScreenScaleFactor();
        WizardUI.setRootObjet(standaloneWindow);
        kernel_kernel.invokeFromQmlWindow();
        leftToolBar.mymodel = kernel_kernelui.lMainModel;
        leftToolBar.otherModel = kernel_kernelui.lOtherModel;
        kernel_kernelui.rightPanel = g_rightPanel;
        idStatusBar.bind(cxkernel_job_executor);
        kernel_kernel.processCommandLine();
        showMaximized();
    }

    DockContext {
        // 留 1 个单位的边界, 避免子窗口最大化时覆盖主窗口导致主窗口无法拖放大小

        id: dock_context

        tabBarX: standaloneWindow.x + 1
        tabBarY: Qt.platform.os !== "osx" ? standaloneWindow.y + idMenuBar.height + topToolBar.height + 1 : standaloneWindow.y + topToolBar.height + 1
        tabBarWidth: standaloneWindow.width - 2
        tabBarHeight: 30 * screenScaleFactor
        adsorbedItemX: tabBarX
        adsorbedItemY: tabBarY + tabBarHeight
        adsorbedItemWidth: tabBarWidth
        adsorbedItemHeight: Qt.platform.os !== "osx" ? standaloneWindow.height - idMenuBar.height - topToolBar.height - tabBarHeight - 2 : standaloneWindow.height - topToolBar.height - tabBarHeight - 2
    }

    UploadMessageDlg {
        id: idOutPlatform

        visible: false
        // cancelBtnVisible: false
        messageType: 0
        msgText: qsTr("The model has exceeded the printing range, do you want to continue slicing?")
        onSigOkButtonClicked: {
            idOutPlatform.visible = false;
            doSlice();
        }
        onSigCancelButtonClicked: {
            kernel_kernel.setKernelPhase(0);
        }
    }

    Loader {
        id: __LANload

        property bool wifiSwitch: false
        property string curGcodeFileName: ""

        z: 1000
        anchors.fill: parent
        anchors.topMargin: Qt.platform.os !== "osx" ? idMenuBar.height + topToolBar.height : topToolBar.height
        //sourceComponent: __printerComponet
        visible: kernel_kernel.currentPhase == 2
        onVisibleChanged: {
            if (visible) {
                __LANload.item.setRealEntryPanel(wifiSwitch);
                if (wifiSwitch)
                    __LANload.item.setCurGcodeFileName(curGcodeFileName);

            } else {
                wifiSwitch = false;
                printerListModel.setCurrentDevice("");
            }
        }
    }

    Component {
        id: __printerComponet

        LanPrinterPanel {
        }

    }

    DropArea {
        id: idDropArea

        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls)
                kernel_io_manager.openWithUrls(drop.urls);

        }
    }

    Connections {
        target: kernel_kernelui
        //Triggered when switching language,Handle menu bar switching separately
        onSigChangeMenuLanguage: {
        }
        onSigCloseAction: {
            standaloneWindow.close();
        }
        onSigOpenglOld: {
            dlgTimer.start();
        }
    }

    Loader {
        active: Qt.platform.os == "osx"
        sourceComponent: menuBarCom
    }

    Component {
        id: menuBarCom

        MacMenu.MenuBar {
            MacMenu.Menu {
                // MacMenu.MenuItem{
                //     id : __closeProject
                //     text: qsTr("Close")
                //     shortcut : "Ctrl+Q"
                //     onTriggered:
                //     {
                //         excuteOpt("Close")
                //     }
                // }

                id: fileMenu

                title: qsTranslate("BasicMenuBarStyle", "File(&F)")

                MacMenu.MenuItem {
                    id: __openfile

                    text: qsTranslate("BasicMenuBarStyle", "Open File")
                    shortcut: "Ctrl+O"
                    onTriggered: {
                        excuteOpt("Open File");
                    }
                }

                MacMenu.MenuItem {
                    id: __openProject

                    text: qsTranslate("BasicMenuBarStyle", "Open Project")
                    onTriggered: {
                        excuteOpt("Open Project");
                    }
                }

                MacMenu.Menu {
                    id: __fileHistory

                    title: qsTranslate("BasicMenuBarStyle", "Recently files")

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

                    title: qsTranslate("BasicMenuBarStyle", "Recently Opened Project")

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

                MacMenu.MenuItem {
                    id: __saveSTL

                    text: qsTranslate("BasicMenuBarStyle", "Save STL")
                    onTriggered: {
                        excuteOpt("Save STL");
                    }
                }

                MacMenu.MenuItem {
                    id: __saveProject

                    text: qsTranslate("BasicMenuBarStyle", "Save As Project")
                    onTriggered: {
                        excuteOpt("Save As Project");
                    }
                }

                MacMenu.Menu {
                    id: __standardModel

                    title: qsTranslate("BasicMenuBarStyle", "Standard Model")

                    Instantiator {
                        id: __standardModelSubMenu

                        model: actionCommands.getOpt("Standard Model").subMenuActionmodel
                        onObjectAdded: __standardModel.insertItem(0, object)
                        onObjectRemoved: __standardModel.removeItem(object)

                        delegate: MacMenu.MenuItem {
                            text: model.actionNameRole
                            iconSource: "qrc:/UI/photo/configTabBtn2_d.png"
                            onTriggered: model.actionItem.execute()
                        }

                    }

                }

            }

            MacMenu.Menu {
                id: editMenu

                title: qsTranslate("BasicMenuBarStyle", "Edit") + "(&E)"

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Undo")
                    enabled: controlEnabled && isFDMPrinter
                    shortcut: "Ctrl+Z"
                    onTriggered: {
                        excuteOpt("Undo");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Redo")
                    enabled: controlEnabled && isFDMPrinter
                    shortcut: "Ctrl+Shift+Z"
                    onTriggered: {
                        excuteOpt("Redo");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Copy")
                    enabled: controlEnabled
                    shortcut: "Ctrl+C "
                    onTriggered: {
                        kernel_ui.onCopy();
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Paste")
                    enabled: controlEnabled
                    shortcut: "Ctrl+V "
                    onTriggered: {
                        kernel_ui.onPaste();
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Delete Model")
                    enabled: controlEnabled
                    shortcut: "Ctrl+D"
                    onTriggered: {
                        excuteOpt("Delete Model");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Clear All")
                    enabled: controlEnabled
                    shortcut: "Ctrl+Shift+D"
                    onTriggered: {
                        excuteOpt("Clear All");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Clone Model")
                    enabled: controlEnabled
                    shortcut: "Ctrl+Shift+C"
                    onTriggered: {
                        excuteOpt("Clone Model");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Split Model")
                    enabled: controlEnabled
                    shortcut: "Alt+S"
                    onTriggered: {
                        excuteOpt("Split Model");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Merge Model")
                    enabled: controlEnabled
                    shortcut: "Alt+Shift+M"
                    onTriggered: {
                        excuteOpt("Merge Model");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Merge Model Locations")
                    enabled: true
                    shortcut: "Ctrl+M"
                    onTriggered: {
                        excuteOpt("Unit As One");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Select All Model")
                    enabled: controlEnabled
                    shortcut: "Ctrl+A"
                    onTriggered: {
                        excuteOpt("Select All Model");
                    }
                }
                /* MacMenu.MenuItem{
                    text: qsTranslate("BasicMenuBarStyle","Reset All Model")
                    enabled : controlEnabled
                    shortcut : "Ctrl+Shift+R"
                    onTriggered: {
                        excuteOpt("Reset All Model")
                    }
                }*/

            }

            MacMenu.Menu {
                id: viewMenu

                title: qsTranslate("BasicMenuBarStyle", "View") + "(&V)"
                // ...
                enabled: controlEnabled && isFDMPrinter

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "ShowModelLine")
                    enabled: controlEnabled
                    onTriggered: {
                        excuteOpt("ShowModelLine");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "ShowModelFace")
                    enabled: controlEnabled
                    onTriggered: {
                        excuteOpt("ShowModelFace");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "ShowModelFaceLine")
                    enabled: true
                    onTriggered: {
                        excuteOpt("ShowModelFaceLine");
                    }
                }

                Instantiator {
                    id: idMenuItems

                    model: actionCommands.getOpt("View Show").subMenuActionmodel
                    onObjectAdded: viewMenu.insertItem(0, object)
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

                title: qsTranslate("BasicMenuBarStyle", "Tool") + "(&T)"

                // ...
                MacMenu.Menu {
                    id: languageMenu

                    title: qsTranslate("BasicMenuBarStyle", "Language")

                    Instantiator {
                        model: actionCommands.getOpt("Language").subMenuActionmodel
                        onObjectAdded: languageMenu.insertItem(0, object)
                        onObjectRemoved: languageMenu.removeItem(object)

                        delegate: MacMenu.MenuItem {
                            text: model.actionNameRole
                            onTriggered: model.actionItem.execute()
                        }

                    }

                }

                MacMenu.Menu {
                    id: themeMenu

                    title: qsTranslate("BasicMenuBarStyle", "Theme color change")

                    Instantiator {
                        model: actionCommands.getOpt("Theme color change").subMenuActionmodel
                        onObjectAdded: themeMenu.insertItem(0, object)
                        onObjectRemoved: themeMenu.removeItem(object)

                        delegate: MacMenu.MenuItem {
                            text: model.actionNameRole
                            onTriggered: model.actionItem.execute()
                        }

                    }

                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Model Repair")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("Model Repair");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Manage Printer")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("Manage Printer");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Manage Materials")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        //                excuteOpt("Manage Material")
                        standaloneWindow.showManageMaterialDialog();
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Log View")
                    enabled: true
                    onTriggered: {
                        excuteOpt("Log View");
                    }
                }

                Loader {
                    active: kernel_global_const.sliceHeightSettingEnabled

                    sourceComponent: MacMenu.MenuItem {
                        enabled: kernel_modelspace.modelNNum > 0
                        text: "切片高度设置"
                        onTriggered: {
                            toolMenu.close();
                            kernel_ui.requestQmlDialog("slice_height_setting_dialog");
                        }
                    }

                }

                Loader {
                    active: kernel_global_const.partitionPrintEnabled

                    sourceComponent: MacMenu.MenuItem {
                        enabled: kernel_model_selector.selectedNum === 1
                        text: "分区打印"
                        onTriggered: {
                            toolMenu.close();
                            kernel_ui.requestQmlDialog("partition_print_dialog");
                        }
                    }

                }

            }

            MacMenu.Menu {
                id: modelLibraryMenu

                title: qsTranslate("BasicMenuBarStyle", "Models(&M)")
                enabled: isFDMPrinter

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Models")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("Models");
                    }
                }

            }

            MacMenu.Menu {
                id: calibrationMenu

                title: qsTranslate("BasicMenuBarStyle", "Calibration(&C)")

                // ...
                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Temperature")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("Temperature");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Coarse tune")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        kernel_slice_calibrate.lowflow();
                        excuteOpt("Coarse tune");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Fine tune")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        // kernel_slice_calibrate.highflow()
                        excuteOpt("FlowFineTuning");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "PA")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("PA");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Maximum volume flow")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("MaxFlowVolume");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "VFA")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("VFA");
                    }
                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Tutorial")
                    enabled: controlEnabled && isFDMPrinter
                    onTriggered: {
                        excuteOpt("Tutorial");
                    }
                }

            }

            MacMenu.Menu {
                id: helpMenu

                title: qsTranslate("BasicMenuBarStyle", "Help") + "(&H)"

                MacMenu.MenuItem {
                    property string modelData: "About Us"

                    text: qsTranslate("BasicMenuBarStyle", modelData)
                    enabled: true
                    onTriggered: {
                        excuteOpt(modelData);
                    }
                }

                Loader {
                    active: kernel_global_const.upgradeable

                    sourceComponent: MacMenu.MenuItem {
                        property string modelData: "Update"

                        text: qsTranslate("BasicMenuBarStyle", modelData)
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

                        text: qsTranslate("BasicMenuBarStyle", modelData)
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

                        text: qsTranslate("BasicMenuBarStyle", modelData)
                        enabled: true
                        onTriggered: {
                            excuteOpt(modelData);
                        }
                    }

                }

                MacMenu.MenuItem {
                    text: qsTranslate("BasicMenuBarStyle", "Mouse Operation Function")
                    enabled: true
                    onTriggered: {
                        idAllMenuDialog.mosueInfo.visible = true;
                    }
                }

            }

        }

    }

    ResizeItem {
        id: resize_item

        enableSize: 8
        anchors.fill: parent
        focus: true
        cursorEnable: standaloneWindow.visibility !== Window.Maximized && standaloneWindow.visibility !== Window.FullScreen

        BasicMenuBar {
            id: idMenuBar

            visible: Qt.platform.os !== "osx"
            width: parent.width
            //myLoad:content
            height: 32 * screenScaleFactor
            mainWindow: standaloneWindow
            onCloseWindow: {
                beforeCloseWindow();
                standaloneWindow.close();
            }
            onChangeServer: {
                idLoginBtn.setCurrentServer(serverIndex);
            }
        }

        Rectangle {
            id: depRec

            width: parent.width
            height: 0
            anchors.top: Qt.platform.os !== "osx" ? idMenuBar.bottom : parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            color: Constants.headerBackgroundColorEnd
        }

        Connections {
            target: kernel_slice_flow
            onSliceAttainChanged: {
                //切片数据变动时，统一通知  idFDMPreview 和 idFDMFooter
                idFDMPreview.update();
                idFDMFooter.update();
            }
        }

        RightBtns {
            enabled: true
            anchors.horizontalCenter: layerSliderRange.horizontalCenter
            anchors.bottom: layerSliderRange.top
            anchors.bottomMargin: 0 * screenScaleFactor
            visible: kernel_parameter_manager.functionType === 0
            z: 3
        }

        QQC25.Control {
            id: topDialog

            width: parent.width
            height: parent.height
            padding: 0
            visible: cxkernel_job_executor.visible
            z: 20

            contentItem: Item {
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log("clicked blue");
                        mouse.accepted = true;
                    }

                    CusStyledStatusBar {
                        id: idStatusBar

                        objectName: "StatusBarObj"
                        x: 0
                        y: 15 * screenScaleFactor
                        width: standaloneWindow.width
                        height: 500 * screenScaleFactor
                        currentPhase: kernel_kernel.currentPhase
                        onSigJobStart: {
                        }
                        onSigJobEnd: {
                        }
                    }

                }

            }

            background: Rectangle {
                color: "#1c1c1d"
                opacity: 0.7
            }

        }

        ZSliderRange {
            id: layerSliderRange

            property int currentPhase: kernel_kernel.currentPhase
            property string labelTextBegin: (currentPhase == 0 ? "Z" : qsTr("Layer")) + ": "
            property int labelDecimal: currentPhase == 0 ? 1 : 0

            function resetValue() {
                first.value = Qt.binding(function() {
                    return from;
                });
                second.value = Qt.binding(function() {
                    return kernel_modelspace.modelNNum <= 0 ? from : to;
                });
            }

            z: 3
            width: 18 * screenScaleFactor
            height: 360 * screenScaleFactor
            anchors.right: currentPhase === 0 ? parent.right : idFDMPreview.left
            anchors.rightMargin: (currentPhase === 0 ? Constants.rightPanelWidth : 0) + 65 * screenScaleFactor
            anchors.verticalCenter: idFDMPreview.verticalCenter
            anchors.verticalCenterOffset: 35 * screenScaleFactor
            orientation: Qt.Vertical
            visible: currentPhase != 2 && kernel_parameter_manager.functionType == 0
            enabled: currentPhase == 0 ? kernel_modelspace.modelNNum > 0 : true
            from: (currentPhase == 0 && currentPhase != 2)  ? 0 : 1
            to: (currentPhase == 0 && currentPhase != 2) ? (plugin_info_sliderinfo.maxLayer + 0.01) : (kernel_slice_model.layers)
            stepSize: currentPhase == 0 ? 0.1 : 1
            first.value: from
            second.value: kernel_modelspace.modelNNum <= 0 ? from : to
            animation: idFDMFooter.startAnimation
            firstControl: currentPhase == 0
            firstLabelText: labelTextBegin + first.value.toFixed(labelDecimal)
            secondLabelText: labelTextBegin + second.value.toFixed(labelDecimal)

            first.onValueChanged: {
                if (currentPhase == 0) 
                    plugin_info_sliderinfo.setBottomCurrentLayer(first.value);
            }
            second.onValueChanged: {
                if (currentPhase == 0) {
                    plugin_info_sliderinfo.setTopCurrentLayer(second.value);
                } else {
                    if (!idFDMFooter.startAnimation)
                        kernel_slice_model.setAnimationState(0);

                    if (idFDMFooter.onlyLayer) {
                        kernel_slice_model.setCurrentLayer(second.value, idFDMFooter.randomstep);
                        kernel_slice_model.showOnlyLayer(second.value);
                    } else {
                        kernel_slice_model.setCurrentLayer(second.value, idFDMFooter.randomstep);
                    }
                }
            }
        }

        FDMPreviewPanel {
            id: idFDMPreview

            objectName: "FDMPreviewPanel"
            z: 3
            anchors.top: topToolBar.bottom
            anchors.right: parent.right
            anchors.topMargin: 10 * screenScaleFactor
            anchors.rightMargin: 10 * screenScaleFactor
            fdmMainObj: editorContent
        }

        TipsManageCom {
            id: tipsItem

            z: 100
            visible: false
            width: 450 * screenScaleFactor
            anchors.bottom: idFDMFooter.top
            anchors.bottomMargin: 20 * screenScaleFactor
            anchors.horizontalCenter: idFDMFooter.horizontalCenter
        }

        FDMFooterPanel {
            id: idFDMFooter

            z: 3
            anchors.bottom: parent.bottom
            anchors.left: leftToolBar.right
            anchors.right: idFDMPreview.left
            anchors.bottomMargin: 10 * screenScaleFactor
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.rightMargin: 10 * screenScaleFactor
            visible: idFDMPreview.visible
            previewSlider: layerSliderRange
            stepMax: kernel_slice_model.steps
            currentStep: kernel_slice_model.steps
            onVisibleChanged: {
                if (visible)
                    kernel_slice_model.setCurrentLayerFocused(false);

            }
            onStepSliderChange: {
                kernel_slice_model.setCurrentStep(value);
                idFDMPreview.aFDMRightPanel.setLayerGCodesIndex(kernel_slice_model.findViewIndexFromStep(value));
            }

            Connections {
                target: kernel_slice_model
                onStepsChanged: {
                    idFDMFooter.stepMax = kernel_slice_model.steps;
                    idFDMFooter.currentStep = kernel_slice_model.steps;
                }
            }

        }

        FDMPreviewButtonBox {
            id: idFDMButtonBox

            z: 3
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10 * screenScaleFactor
            anchors.bottomMargin: 10 * screenScaleFactor
            onUsbButtonClicked: {
                if (kernel_slice_flow.sliceAttain)
                    plugin_usb_printer.startUSBPrint(kernel_slice_flow.sliceAttain.sourceFileName(), kernel_slice_flow.sliceAttain.printTime());

            }
            onWifiButtonClicked: {
                idUploadGcodeDialog.showUploadDialog(1);
            }
            onUploadButtonClicked: {
                if (cxkernel_cxcloud.accountService.logined)
                    idUploadGcodeDialog.showUploadDialog(0);
                else
                    kernel_ui.requestQmlDialog("idWarningLoginDlg");
            }
            onExportButtonClicked: {
                kernel_slice_flow.saveGCodeLocal();
            }
        }

        ReleaseNote {
            id: idReleaseNote

            onRequestNoNewVersionTip: {
                idUpdateDlg.show();
            }
        }

        Timer {
            id: dlgTimer

            interval: 1000
            running: false
            repeat: false
            onTriggered: {
                idConfirmMessageDlg.visible = true;
                idConfirmMessageDlg.msgText = qsTr("The OpenGL version is too low to start Creality Print, please upgrade the graphics card driver!");
            }
        }

        Item {
            id: rightItem

            anchors.top: depRec.bottom
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.right: parent.right

            QuickScene3DWrapper {
                id: editorContent

                property bool needFirstConfig: false
                property bool isPrepared: false

                function requestFirstConfig() {
                    if (isPrepared)
                        idStartFirstConfig.visible = true;
                    else
                        needFirstConfig = true;
                }

                anchors.fill: parent
                objectName: "gLQuickItem"
                focus: true
                Component.onCompleted: {
                    isPrepared = true;
                    if (needFirstConfig)
                        idStartFirstConfig.visible = true;

                }

                StartFirstConfig {
                    id: idStartFirstConfig

                    visible: false
                }

            }

            Scene3D {
                id: scene3d

                z: -1
                anchors.fill: parent
                opacity: (kernel_kernel.currentPhase != 2 && kernel_parameter_manager.functionType == 0) ? 1 : 0
                enabled: Constants.bGLViewEnabled
                //focus: true
                aspects: ["input", "logic"]
                Component.onCompleted: {
                    editorContent.bindScene3D(scene3d);
                }
            }

            LaserScene {
                id: idLaserView

                anchors.fill: parent
                anchors.topMargin: topToolBar.height
                objectName: "laserView"
                visible: kernel_parameter_manager.functionType == 1
                winWidth: standaloneWindow.width
                winHeight: standaloneWindow.height
            }

            PlotterScene {
                id: idPlotterView

                anchors.fill: parent
                anchors.topMargin: topToolBar.height
                objectName: "plotterView"
                visible: kernel_parameter_manager.functionType == 2
                winWidth: standaloneWindow.width
                winHeight: standaloneWindow.height
            }

            Item {
                id: idRIghtItem

                visible: kernel_kernel.currentPhase == 0
                width: 300 * screenScaleFactor
                height: 522 * screenScaleFactor
                x: parent.width - idRIghtItem.width - 10
                y: topToolBar.height + 10
                enabled: Constants.bRightPanelEnabled
                onXChanged: {
                    Constants.rightPanelWidth = idRIghtItem.parent.width - idRIghtItem.x;
                }

                Column {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 4
                    width: parent.width

                    CusTabBar {
                        id: idTabBar

                        radius: 5 * screenScaleFactor
                        width: parent.width
                        height: 30 * screenScaleFactor
                        btnWidth: idTabBar.width / tabModel.length
                        color: Constants.right_panel_tab_background_color
                        allRadius: true
                        btnNormalColor: Constants.right_panel_tab_button_default_color
                        btnHoveredColor: Constants.right_panel_tab_button_default_color
                        btnPressedColor: Constants.right_panel_tab_button_checked_color
                        txtColorNormal: Constants.right_panel_tab_text_default_color
                        txtColorPressed: Constants.right_panel_tab_text_checked_color
                        btnBorderColor: Constants.right_panel_tab_button_default_color
                        btnBorderColorHovered: Constants.rectBorderColor
                        currentIndex: 0
                        tabModel: curType
                        onCurrentIndexChanged: {
                            kernel_parameter_manager.setFunctionType(currentIndex);
                            console.log("++++currentIndex = ", currentIndex);
                        }
                    }

                    Connections {
                        //                            kernel_parameter_manager.setFunctionType(0)

                        target: kernel_parameter_manager
                        onCurMachineIndexChanged: {
                            var machineType = kernel_parameter_manager.curMachineType();
                            if (machineType === 1)
                                idTabBar.tabModel = [qsTr("FDM")];
                            else if (machineType === 2)
                                idTabBar.tabModel = [qsTr("FDM"), qsTr("Laser")];
                            else
                                idTabBar.tabModel = [qsTr("FDM"), qsTr("Laser"), qsTr("CNC")];
                            idTabBar.visible = machineType !== 1;
                            idPrinterComboBox.currentIndex = kernel_parameter_manager.curMachineIndex;
                            idTabBar.currentIndex = 0;
                        }
                    }

                    Rectangle {
                        id: wrapperRec

                        width: 300 * screenScaleFactor
                        height: 500 * screenScaleFactor
                        radius: 5
                        color: Constants.right_panel_menu_border_color

                        QQC25.Button {
                            id: drawerKey

                            property bool isClosed: false

                            width: 12 * screenScaleFactor
                            height: 77 * screenScaleFactor
                            anchors.right: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            onClicked: {
                                drawerKey.isClosed ? aniXOut.start() : aniXIn.start();
                                drawerKey.isClosed = !drawerKey.isClosed;
                            }

                            background: Image {
                                source: Constants.drawerBgImg

                                Image {
                                    id: indicatorImg

                                    property string in_imgPath: drawerKey.hovered ? "qrc:/UI/photo/rightDrawer/in_hover.svg" : "qrc:/UI/photo/rightDrawer/in_normal.svg"
                                    property string out_imgPath: drawerKey.hovered ? "qrc:/UI/photo/rightDrawer/out_hover.svg" : "qrc:/UI/photo/rightDrawer/out_normal.svg"

                                    anchors.centerIn: parent
                                    width: 4 * screenScaleFactor
                                    height: 7 * screenScaleFactor
                                    source: drawerKey.isClosed ? out_imgPath : in_imgPath
                                }

                            }

                        }

                        Image {
                            id: downKey

                            property bool isClosed: false

                            visible: true
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 3
                            source: "qrc:/UI/photo/rightDrawer/rightPanelDown.svg"
                            width: 19 * screenScaleFactor
                            height: 4 * screenScaleFactor

                            MouseArea {
                                property point clickPos: "0,0"

                                anchors.fill: parent
                                cursorShape: Qt.SizeVerCursor
                                onPressed: {
                                    clickPos = Qt.point(mouse.x, mouse.y);
                                    wrapperRec.color = Constants.right_panel_tab_button_checked_color;
                                }
                                onReleased: {
                                    if (wrapperRec.height < 500 * screenScaleFactor)
                                        wrapperRec.height = 500 * screenScaleFactor;
                                    else if (wrapperRec.height > 838 * screenScaleFactor)
                                        wrapperRec.height = 838 * screenScaleFactor;
                                    wrapperRec.color = Constants.right_panel_menu_border_color;
                                }
                                onPositionChanged: {
                                    //鼠标偏移量motai
                                    var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y);
                                    if (wrapperRec.height < 500 * screenScaleFactor)
                                        return ;
                                    else if (wrapperRec.height > 838 * screenScaleFactor)
                                        return ;
                                    wrapperRec.height = wrapperRec.height + delta.y;
                                }
                            }

                        }

                        Rectangle {
                            id: idPrinterInternal

                            color: Constants.right_panel_menu_background_color
                            anchors.fill: parent
                            anchors.topMargin: 4 * screenScaleFactor
                            anchors.bottomMargin: 9 * screenScaleFactor
                            anchors.leftMargin: 1 * screenScaleFactor
                            anchors.rightMargin: 1 * screenScaleFactor

                            MouseArea {
                                id: printerWheelFilter

                                anchors.fill: parent
                                propagateComposedEvents: false
                                onWheel: {
                                }
                            }

                            Item {
                                id: rightPanelView

                                anchors.fill: parent

                                Column {
                                    //                                    BasicCombobox_Metarial
                                    //                                    {
                                    //                                        id : idPrinterComboBox
                                    //                                        width: 260 * screenScaleFactor
                                    //                                        height: 28 * screenScaleFactor
                                    //                                        Layout.alignment: Qt.AlignHCenter
                                    //                                        model: kernel_parameter_manager.machineNameList
                                    //                                        anchors.horizontalCenter: parent.horizontalCenter
                                    //                                        clip: true
                                    //                                        onAddMaterialClick:{
                                    //                                            kernel_ui.requestQmlDialog("idAddPrinterDlgNew")
                                    //                                        }
                                    //                                        onSigManagerClick:{
                                    //                                            if(kernel_parameter_manager.machinesCount > 0)
                                    //                                                idManagerPrinterWizard.visible = true
                                    //                                        }

                                    id: printCol

                                    anchors.top: parent.top
                                    anchors.topMargin: 20 * screenScaleFactor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    spacing: 8 * screenScaleFactor

                                    PanelComponent {
                                        imgUrl: "qrc:/UI/photo/printerPanel.svg"
                                        title: qsTr("Printer")
                                        sourceSize.width: 11 * screenScaleFactor
                                        sourceSize.height: 12 * screenScaleFactor
                                    }

                                    //                                        onActivated:{
                                    //                                            kernel_parameter_manager.setCurrentMachine(currentIndex)
                                    //                                            idSlicePanel.checkParam()
                                    //                                        }
                                    //                                    }
                                    CXComboBox {
                                        id: idPrinterComboBox

                                        width: 260 * screenScaleFactor
                                        height: 28 * screenScaleFactor
                                        Layout.alignment: Qt.AlignHCenter
                                        model: kernel_parameter_manager.machineNameList
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        clip: true
                                        onActivated: {
                                            kernel_parameter_manager.setCurrentMachine(currentIndex);
                                            idSlicePanel.checkParam();
                                        }

                                        slotItem: Row {
                                            spacing: 2
                                            anchors.left: parent.left
                                            anchors.leftMargin: 2

                                            CusButton {
                                                width: idPrinterComboBox.width / 2 - 4 * screenScaleFactor
                                                height: 24 * screenScaleFactor
                                                radius: 5
                                                txtContent: qsTr("Add")
                                                enabled: true
                                                onClicked: {
                                                    idPrinterComboBox.popup.visible = false;
                                                    kernel_ui.requestQmlDialog("idAddPrinterDlgNew");
                                                }
                                            }

                                            CusButton {
                                                width: idPrinterComboBox.width / 2 - 4 * screenScaleFactor
                                                height: 24 * screenScaleFactor
                                                radius: 5
                                                txtContent: qsTr("Manage")
                                                enabled: true
                                                onClicked: {
                                                    idPrinterComboBox.popup.visible = false;
                                                    if (kernel_parameter_manager.machinesCount > 0)
                                                        idManagerPrinterWizard.visible = true;

                                                }
                                            }

                                        }

                                    }

                                }

                                Rectangle {
                                    id: sepRec

                                    anchors.top: printCol.bottom
                                    anchors.topMargin: 15 * screenScaleFactor
                                    width: parent.width
                                    height: 1
                                    color: Constants.right_panel_menu_split_line_color
                                }

                                StackLayout {
                                    id: swipeView

                                    anchors.top: sepRec.bottom
                                    width: parent.width
                                    height: wrapperRec.height - 110 * screenScaleFactor
                                    currentIndex: idTabBar.currentIndex

                                    HalotBoxTotalRightUIPanel {
                                        id: idSlicePanel

                                        visible: kernel_kernel.currentPhase == 0
                                        width: 280 * screenScaleFactor
                                    }

                                    LaserPanel {
                                        id: idLaserPanel

                                        objectName: "objLaserItem"
                                        color: idPrinterInternal.color
                                        control: laserScene
                                        settingsModel: laserScene.laserSettings()
                                        onVisibleChanged: {
                                            if (idLaserPanel.visible)
                                                idLaserPanel.updateImageInfo();

                                        }
                                    }

                                    PlotterPanel {
                                        id: idPlotterPanel

                                        objectName: "objPlotterItem"
                                        color: idPrinterInternal.color
                                        onVisibleChanged: {
                                            if (idPlotterPanel.visible)
                                                idPlotterPanel.updateImageInfo();

                                        }
                                    }

                                }

                            }

                        }

                    }

                }

                NumberAnimation on x {
                    id: aniXIn

                    from: rightItem.width - idRIghtItem.width - 10
                    to: rightItem.width
                    duration: 500
                    running: false
                }

                NumberAnimation on x {
                    id: aniXOut

                    from: rightItem.width
                    to: rightItem.width - idRIghtItem.width - 10
                    duration: 500
                    running: false
                }

            }

        }

        QQC25.Control {
            id: topToolBar

            width: parent.width
            height: Qt.platform.os == "osx" ? 110 * screenScaleFactor : 60 * screenScaleFactor
            anchors.top: depRec.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                id: topToolBarWheelFilter

                anchors.fill: parent
                propagateComposedEvents: false
                onWheel: {
                }
            }

            Item {
                anchors.fill: parent

                CusText {
                    fontText: kernel_global_const.bundleName
                    fontPointSize: 18
                    fontBold: true
                    fontColor: Constants.currentTheme === 0 ? "#ababab" : "#7d7d82"
                    //                fontWeight: Font.Bold
                    fontFamily: Constants.labelFontFamilyBold
                    fontWidth: 300 * screenScaleFactor
                    //                    opacity: 0.5
                    hAlign: 0
                    anchors.left: parent.left
                    anchors.leftMargin: 5 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 10 * screenScaleFactor
                    visible: kernel_parameter_manager.functionType === 0

                    CusButton {
                        id: prepairBtn

                        anchors.verticalCenter: parent.verticalCenter
                        width: 100 * screenScaleFactor
                        height: 32 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        checkable: true
                        isChecked: kernel_kernel.currentPhase == 0
                        normalColor: "transparent"
                        hoveredColor: Constants.topBtnBgColor_hovered
                        pressedColor: Constants.topBtnBgColor_normal
                        txtColor: isChecked ? Constants.topBtnTextColor_checked : isHovered ? Constants.topBtnTextColor_hovered : Constants.topBtnTextColor_normal
                        //                    txtWeight: Font.Bold
                        txtPointSize: Constants.labelFontPointSize_12
                        txtFamily: Constants.labelFontFamilyBold
                        txtBold: true
                        txtContent: qsTr("Prepare")
                        onClicked: {
                            kernel_kernel.setKernelPhase(0);
                            editorContent.focus = true;
                        }
                    }

                    CusButton {
                        id: previewBtn

                        anchors.verticalCenter: parent.verticalCenter
                        width: 100 * screenScaleFactor
                        height: 32 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        enabled: kernel_modelspace.modelNNum > 0 || kernel_slice_flow.sliceAttain != null
                        checkable: true
                        isChecked: kernel_kernel.currentPhase == 1
                        normalColor: "transparent"
                        hoveredColor: Constants.topBtnBgColor_hovered
                        pressedColor: Constants.topBtnBgColor_normal
                        txtColor: isChecked ? Constants.topBtnTextColor_checked : isHovered ? Constants.topBtnTextColor_hovered : Constants.topBtnTextColor_normal
                        //                    txtWeight: Font.Bold
                        txtPointSize: Constants.labelFontPointSize_12
                        txtFamily: Constants.labelFontFamilyBold
                        txtBold: true
                        txtContent: qsTr("Preview")
                        onClicked: {
                            startSlice();
                            dock_context.closeAllItem();
                        }
                    }

                    CusButton {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 100 * screenScaleFactor
                        height: 32 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        enabled: __LANload.status == Loader.Ready
                        checkable: true
                        isChecked: kernel_kernel.currentPhase == 2
                        normalColor: "transparent"
                        hoveredColor: Constants.topBtnBgColor_hovered
                        pressedColor: Constants.topBtnBgColor_normal
                        txtColor: isChecked ? Constants.topBtnTextColor_checked : isHovered ? Constants.topBtnTextColor_hovered : Constants.topBtnTextColor_normal
                        txtPointSize: Constants.labelFontPointSize_12
                        txtFamily: Constants.labelFontFamilyBold
                        txtBold: true
                        txtContent: qsTr("Device")
                        onClicked: {
                            kernel_kernel.setKernelPhase(2);
                            dock_context.closeAllItem();
                        }
                    }

                }

                RowLayout {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: spacing
                    spacing: 10 * screenScaleFactor
                    visible: kernel_parameter_manager.functionType === 0

                    QQC25.Button {
                        id: undoBtn

                        enabled: kernel_undo_proxy.canUndo
                        visible: kernel_kernel.currentPhase == 0
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 40 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor
                        onClicked: {
                            //添加模型是否有添加支撑的判断，给出提示
                            var hasSupport = kernel_model_list_data.hasSupport();
                            console.log("****hasSupport = ", hasSupport);
                            if (hasSupport)
                                idClosePro.visible = true;
                            else
                                kernel_undo_proxy.undo();
                        }

                        BasicTooltip {
                            id: undoTooptip

                            visible: undoBtn.hovered && String(text).length
                            position: BasicTooltip.Position.BOTTOM
                            text: qsTr("Undo")
                        }

                        UploadMessageDlg {
                            id: idClosePro

                            property var receiver

                            msgText: qsTr("When performing an undo operation, the added support will be deleted. Do you want to continue?")
                            visible: false
                            labelPointSize: Constants.labelFontPointSize_9
                            messageType: 1
                            onSigOkButtonClicked: {
                                kernel_undo_proxy.undo();
                                supportUICpp.removeAll();
                                idClosePro.visible = false;
                            }
                            onSigIgnoreButtonClicked: {
                                idClosePro.visible = false;
                            }
                        }

                        background: Rectangle {
                            color: parent.hovered ? (Constants.currentTheme ? "#FFE0E1E5" : "#80000000") : "transparent"
                            radius: parent.width / 2

                            Image {
                                anchors.centerIn: parent
                                width: 16 * screenScaleFactor
                                height: 15 * screenScaleFactor
                                source: (undoBtn.hovered && !Constants.currentTheme) ? "qrc:/UI/photo/undo_hover.svg" : "qrc:/UI/photo/undo_normal.svg"
                            }

                        }

                    }

                    QQC25.Button {
                        id: redoBtn

                        enabled: kernel_undo_proxy.canRedo
                        visible: kernel_kernel.currentPhase == 0
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 40 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor
                        onClicked: kernel_undo_proxy.redo()

                        BasicTooltip {
                            id: redoTooptip

                            visible: redoBtn.hovered && String(text).length
                            position: BasicTooltip.Position.BOTTOM
                            text: qsTr("Redo")
                        }

                        background: Rectangle {
                            color: parent.hovered ? (Constants.currentTheme ? "#FFE0E1E5" : "#80000000") : "transparent"
                            radius: parent.width / 2

                            Image {
                                anchors.centerIn: parent
                                width: 16 * screenScaleFactor
                                height: 15 * screenScaleFactor
                                source: (redoBtn.hovered && !Constants.currentTheme) ? "qrc:/UI/photo/redo_hover.svg" : "qrc:/UI/photo/redo_normal.svg"
                            }

                        }

                    }

                    DownloadManageButton {
                        id: download_manage_button

                        visible: kernel_global_const.cxcloudEnabled
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 40 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor
                        downloadTaskCount: download_manage_dialog.downloadTaskCount
                        finishedTaskCount: download_manage_dialog.finishedTaskCount
                        onClicked: {
                            download_manage_dialog.visible = true;
                        }

                        BasicTooltip {
                            id: downLoadTooptip

                            visible: download_manage_button.hovered && String(text).length
                            position: BasicTooltip.Position.BOTTOM
                            text: qsTr("Download Manager")
                        }

                    }

                    LoginBtn {
                        id: idLoginBtn

                        objectName: "loginBtn"
                        visible: kernel_global_const.cxcloudEnabled
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 40 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

                        BasicTooltip {
                            id: loginTooptip

                            visible: idLoginBtn.hovered && String(text).length
                            position: BasicTooltip.Position.BOTTOM
                            text: cxkernel_cxcloud.accountService.logined ? qsTr("Personal Center") : qsTr("Login")
                        }

                    }

                }

            }

            background: Rectangle {
                opacity: 1

                gradient: Gradient {
                    GradientStop {
                        position: 0
                        color: Constants.topToolBarColor
                    }

                    GradientStop {
                        position: 1
                        color: "transparent"
                    }

                }

            }

        }

        StackLayout {
            id: btnStack

            property var tempSliceEnable: kernel_modelspace.modelNNum

            width: 300 * screenScaleFactor
            height: 58 * screenScaleFactor
            visible: kernel_kernel.currentPhase == 0
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10 * screenScaleFactor
            anchors.bottomMargin: 10 * screenScaleFactor
            currentIndex: idTabBar.currentIndex
            onTempSliceEnableChanged: sliceBtnsModel.set(0, {
                "isEnabled": kernel_modelspace.modelNNum > 0 ? true : false
            })

            Repeater {

                model: ListModel {
                    id: sliceBtnsModel

                    ListElement {
                        textContent: qsTr("Slice")
                        isEnabled: false
                    }

                    ListElement {
                        textContent: qsTr("Generate GCode")
                        isEnabled: false
                    }

                    ListElement {
                        textContent: qsTr("Generate GCode")
                        isEnabled: false
                    }

                }

                delegate: Item {
                    implicitWidth: 300 * screenScaleFactor
                    implicitHeight: 58 * screenScaleFactor

                    Rectangle {
                        anchors.fill: parent
                        color: Constants.currentTheme === 0 ? "#000000" : "#ffffff"
                        opacity: Constants.currentTheme === 0 ? 0.1 : 1
                        radius: 5
                    }

                    CusButton {
                        id: idSliceBtn

                        width: 260 * screenScaleFactor
                        height: 36 * screenScaleFactor
                        anchors.centerIn: parent
                        normalColor: enabled ? Constants.right_panel_slice_button_default_color : Constants.right_panel_slice_button_disable_color
                        hoveredColor: Constants.right_panel_slice_button_hovered_color
                        pressedColor: Constants.right_panel_slice_button_checked_color
                        radius: 5
                        enabled: isEnabled
                        shadowEnabled: false
                        txtColor: enabled ? Constants.right_panel_slice_text_default_color : Constants.right_panel_slice_text_disable_color
                        txtFamily: Constants.panelFontFamily
                        txtContent: textContent
                        txtPointSize: Constants.labelFontPointSize_12
                        onClicked: {
                            switch (model.index) {
                            case 0:
                                startSlice();
                                break;
                            case 1:
                                idLaserView.generateGCode();
                                break;
                            case 2:
                                idPlotterView.generateGCode();
                                break;
                            }
                        }

                        Connections {
                            target: idLaserView
                            onEnableGenerateChanged: {
                                if (model.index !== 1)
                                    return ;

                                sliceBtnsModel.set(1, {
                                    "isEnabled": idLaserView.enableGenerate
                                });
                            }
                        }

                        Connections {
                            target: idPlotterView
                            onEnableGenerateChanged: {
                                if (model.index !== 2)
                                    return ;

                                sliceBtnsModel.set(2, {
                                    "isEnabled": idPlotterView.enableGenerate
                                });
                            }
                        }

                    }

                }

            }

        }

        CusImglButton {
            id: idModelRecommendButton

            width: 50 * screenScaleFactor
            height: 50 * screenScaleFactor
            imgWidth: 24 * screenScaleFactor
            imgHeight: 24 * screenScaleFactor
            visible: kernel_global_const.cxcloudEnabled && kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType == 0
            anchors.top: topToolBar.bottom
            anchors.topMargin: 10 * screenScaleFactor
            anchors.horizontalCenter: leftToolBar.horizontalCenter
            text: qsTr("Models")
            cusTooltip.position: BasicTooltip.Position.RIGHT
            shadowEnabled: Constants.currentTheme ? true : false
            bottonSelected: idModelRecommendDialog.visible
            borderColor: Constants.leftbar_btn_border_color
            hoverBorderColor: Constants.leftbar_btn_border_color
            selectBorderColor: Constants.leftbar_btn_border_color
            enabledIconSource: Constants.leftbar_rocommand_btn_icon_default
            pressedIconSource: Constants.leftbar_rocommand_btn_icon_pressed
            highLightIconSource: Constants.leftbar_rocommand_btn_icon_hovered
            onClicked: {
                if (!kernel_global_const.cxcloudEnabled)
                    return ;

                if (!idModelRecommendDialog.visible && !idModelRecommendDialog.visibleFlag) {
                    if (idModelLibraryInfoDialog.visible)
                        idModelLibraryInfoDialog.refreshData();
                    else
                        actionCommands.getOpt("Models").execute();
                } else {
                    idModelRecommendDialog.hide();
                }
            }
        }

        CusImglButton {
            id: openFileBtn

            width: 50 * screenScaleFactor
            height: 50 * screenScaleFactor
            imgWidth: 24 * screenScaleFactor
            imgHeight: 24 * screenScaleFactor
            visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType == 0
            anchors.top: idModelRecommendButton.visible ? idModelRecommendButton.bottom : topToolBar.bottom
            anchors.topMargin: idModelRecommendButton.visible ? 8 * screenScaleFactor : 10 * screenScaleFactor
            anchors.horizontalCenter: leftToolBar.horizontalCenter
            text: qsTr("Open File")
            cusTooltip.position: BasicTooltip.Position.RIGHT
            shadowEnabled: Constants.currentTheme ? true : false
            borderColor: Constants.leftbar_btn_border_color
            hoverBorderColor: Constants.leftbar_btn_border_color
            selectBorderColor: Constants.leftbar_btn_border_color
            enabledIconSource: Constants.leftbar_open_btn_icon_default
            pressedIconSource: Constants.leftbar_open_btn_icon_pressed
            highLightIconSource: Constants.leftbar_open_btn_icon_hovered
            onClicked: {
                kernel_kernel.openFile();
            }
        }

        LeftToolBar {
            id: leftToolBar

            property bool enabledState: true

            objectName: "leftToolObj"
            topLeftButton: kernel_global_const.cxcloudEnabled ? idModelRecommendButton : openFileBtn
            enabled: kernel_model_selector.selectedNum > 0
            visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType === 0
            anchors.top: openFileBtn.bottom
            anchors.topMargin: 8 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
        }

        ModelListCombo {
            id: mlcBtn

            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10 * screenScaleFactor
            width: 50 * screenScaleFactor
            height: 50 * screenScaleFactor
            visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType == 0
        }

    }

    Item {
        id: idRightClickMenu

        property var rightMenu

        function requstShow() {
            if (rightMenu)
                rightMenu.destroy();

            var component = Qt.createComponent("qrc:/CrealityUI/qml/BasicRClickMenu.qml");
            if (component.status === Component.Ready) {
                rightMenu = component.createObject(standaloneWindow, {
                    "menuItems": kernel_rclick_menu_data.getCommandsList(),
                    "nozzleObj": kernel_rclick_menu_data.getNozzleModel(),
                    "uploadObj": kernel_rclick_menu_data.getUploadModel()
                });
                rightMenu.popup();
            }
        }

        objectName: "idRightClickMenu"
    }

    DockItem {
        id: idUpdateDlg

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

            property string openUrl: cxkernel_cxcloud.serverType ? "https://www.crealitycloud.com/post-detail/6454af4f58f57701c94a56f9" : "https://www.crealitycloud.cn/post-detail/6454a92b716e5ffc532cec31"

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

    Timer {
        id: idModelALwaysShowDlgTimer

        interval: 200
        repeat: false
        onTriggered: {
            idModelRecommendDialog.visibleFlag = false;
        }
    }

    BasicMessageDialog {
        id: idDoubleMessageDlg

        property var receiver
        property var machineObject
        property var messageDlgType

        width_x: 480 * screenScaleFactor
        onAccept: {
            console.log("messageDlgType = ", messageDlgType);
            if ("deleteSelectMachine" === messageDlgType) {
                console.log("=111111111111");
                if (checkedFlag)
                    kernel_global_const.writeRegister("UIparam/Machine/isPop", false);

                kernel_parameter_manager.removeMachine(machineObject);
                idManagerPrinterWizard.updateData();
                console.log("2222222222222");
            }
        }
        onCancel: {
            close();
        }
    }

    ManagerPrinterDlg {
        id: idManagerPrinterWizard

        visible: false
        objectName: "managerPrinterDlg"
    }

    UploadMessageDlg {
        id: idConfirmMessageDlg

        cancelBtnVisible: false
        msgText: qsTr("At least one printer should be added!")
        onSigOkButtonClicked: {
            idConfirmMessageDlg.close();
        }
    }

    UploadGcodeDlg {
        id: idUploadGcodeDialog

        objectName: "idUploadGcodeDialog"
        onSigCancelBtnClicked: {
            visible = false;
        }
        onSigConfirmBtnClicked: {
            if (uploadDlgType) {
                visible = false;
                __LANload.wifiSwitch = true;
                __LANload.curGcodeFileName = gcodeFileName + ".gcode";
                var currentIndex = kernel_parameter_manager.curMachineIndex;
                var machineObject = kernel_parameter_manager.machineObject(currentIndex);
                let currentPrinterName = machineObject.baseName();
                printerListModel.setCurrentDevice(currentPrinterName);
                kernel_kernel.setKernelPhase(2);
            } else {
                curWindowState = UploadGcodeDlg.WindowState.State_Progress;
                cxkernel_cxcloud.sliceService.uploadSlice(curGcodeFileName);
            }
        }
    }

    UploadModelDlg {
        id: idUploadModelDialog

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

    AllMenuDialog {
        id: idAllMenuDialog

        objectName: "idAllMenuDialog"
        onSigShowLoginPanel: idLoginBtn.showLoginPanel()
        onDialogShowed: function(name) {
            if (name == "slice_height_setting_dialog")
                zSlider.resetValue();

        }
    }

    ModelAlwaysShowPopupDlg {
        id: idModelRecommendDialog

        property var visibleFlag: false

        function showRecommendModels(jsonStr) {
            setRecommendModelsData(jsonStr);
            show(idModelRecommendButton.x + idModelRecommendButton.width + 10, idModelRecommendButton.y);
        }

        objectName: "idModelRecommendDialog"
        visible: false
        onVisibleChanged: {
            if (idModelRecommendDialog.visible === false) {
                idModelALwaysShowDlgTimer.restart();
                idModelRecommendDialog.visibleFlag = true;
            }
        }
        onClickItem: {
            idModelLibraryInfoDialog.currentGroupId = id;
            idModelLibraryInfoDialog.show();
        }
        onClickMoreItem: {
            idModelLibraryInfoDialog.show();
        }

        Connections {
            target: cxkernel_cxcloud.modelLibraryService
            onLoadRecommendModelGroupListSuccessed: function(json_string, page_index) {
                enabled = false;
                if (kernel_ui.firstStart)
                    return ;

                if (!kernel_global_const.cxcloudEnabled)
                    return ;

                idModelRecommendDialog.showRecommendModels(json_string);
            }
        }

    }

    ModelLibraryInfoDlg {
        id: idModelLibraryInfoDialog

        property var collectId: ""
        property var modelId: ""
        property var modelNum: 0
        property var modelGroupId: ""
        property var modelDetailId: ""
        property var modelDetailName: ""
        property var modelDetailLink: ""
        property int langIndex: -1

        objectName: "idModelLibraryInfoDialog"
        context: standaloneWindow.dockContext
        visible: false
        onRequestLogin: {
            kernel_ui.requestQmlDialog("idWarningLoginDlg");
        }
        onRequestToShowDownloadTip: {
            download_manage_button.showTip();
            download_manage_dialog.show();
        }
        onVisibleChanged: {
            if (visible) {
                var curLang = kernel_global_const.readRegister("cloud_service/server_type");
                if (langIndex !== curLang) {
                    console.log("idModelLibraryInfoDialog onVisibleChanged");
                    langIndex = curLang;
                    idModelLibraryInfoDialog.onServerChange();
                }
            }
        }
    }

    DownloadManageDialog {
        id: download_manage_dialog

        visible: false
        onRequestToOpenModelLibrary: {
            idModelLibraryInfoDialog.visible = true;
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: Constants.mainBackgroundColor
    }

}
