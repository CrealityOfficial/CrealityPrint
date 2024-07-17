import QtQml 2.15

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Scene3D 2.15
import QtQuick.Window 2.15

import Qt.labs.platform 1.1 as MacMenu
import Qt.labs.platform 1.0
import Qt.labs.settings 1.1

import Qt3D.Core 2.15
import Qt3D.Render 2.15
import Qt3D.Input 2.15
import Qt3D.Extras 2.15

import com.cxsw.SceneEditor3D 1.0

import CrealityUI 1.0

import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/cxcloud"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/secondqml/frameless"
import "qrc:/CrealityUI/secondqml/layersConfig"
import "qrc:/CrealityUI/secondqml/menu"
import "qrc:/CrealityUI/secondqml/message"
import "qrc:/CrealityUI/secondqml/printer"

CusBackground {
    id: standaloneWindow

    property int tmpWidth: 0
    property int defultRightPanelHeight: 630 * screenScaleFactor
    property var imageSource: (Constants.currentTheme === 0) ? "qrc:/UI/photo/bgImg_black.png" : "qrc:/UI/photo/bgImg.png"
    property bool isWindow: Qt.platform.os === "windows" ? true : false
    property alias topToolBar: topToolBar
    property alias statusBarItem: idStatusBar
    property alias g_rightPanel: swipeView
    property alias dockContext: dock_context
    property alias zSlider: layerSliderRange
    property alias gGlScene: editorContent
    property alias isErrorExist: messageStack.isErrorExist
    property alias cloudContext: cloud_context
    property alias menuDialog: idAllMenuDialog
    property alias parameterPanel: right_panel
    property int controlEnabled: kernel_kernel.currentPhase === 0
    property int isFDMPrinter: kernel_parameter_manager.functionType === 0
    property int isLaserPrinter: kernel_parameter_manager.functionType === 1
    property int isCNCPrinter: kernel_parameter_manager.functionType === 2
    property var manualCheck: false
    property bool initialized: false  // set by main_window_loader in main.qml

    signal sigInitAdvanceParam()

    function showWizardDlg() {
        sigInitAdvanceParam();
        __LANload.sourceComponent = __printerComponet;
    }

    function changeThemeColor(themeType) {
        Constants.currentTheme = themeType;
    }

    function showReleaseNote() {
        idAllMenuDialog.requestDialog("idReleaseNote");
    }

    function showNoUpdate() {
        idAllMenuDialog.requestDialog("idUpdateDlg");
    }

    function checkupgrade() {
        manualCheck = true;
        cxkernel_cxcloud.upgradeService.checkUpgradeable();
    }

    function showToolTip(pos, text) {
        // let winPos = standaloneWindow.mapFromGlobal(pos)
        let winPos = pos;
        idGlobalToopTip.x = winPos.x;
        idGlobalToopTip.y = winPos.y;
        idGlobalToopTip.text = qsTr(text);
        idGlobalToopTip.visible = true;
    }

    function hideToolTip() {
        idGlobalToopTip.visible = false;
    }

    function requestPrinterSettingsDialog(printer) {
        idAllMenuDialog.requestMenuDialog(printer, "idPrinterSettingsDialog");
    }

    function requestPrinterNameEditorDialog(printer) {
        editorContent.focus = false
        idAllMenuDialog.requestMenuDialog(printer, "idPrinterNameEditorDialog");
    }

    function requestModelSettingPanel(hightlight) {
        right_panel.openModelSettingView();
        if (hightlight) {
            right_panel.highlightGlobalObjectSwitch();
        }
    }

    function requestProcessSettingPanel() {
        right_panel.openProcessSettingView();
    }

    function highlightProcessTreeNode(result_node) {
        right_panel.highlightTreeNode(result_node);
    }

    function showManageMaterialDialog() {
        right_panel.showMaterialManage();
    }

    function closeWindow() {
        frameLessView.close();
    }
    function newProjectSaveParemDialog(){
        if (kernel_parameter_manager.isAnyModified()) {
            var receiver = {
            };
            receiver.type = "all";
            receiver.onIgnore = function() {
                kernel_parameter_manager.abandonAll();
                project_kernel.cancelSave()
            };
            receiver.onVisibleFalse = function(){
            }
            receiver.onSaved = function() {
               project_kernel.cancelSave()
            };
            idAllMenuDialog.requestMenuDialog(receiver, "saveParemDialog");
        } else {
            project_kernel.cancelSave()
        }
    }
    function saveParemDialog() {
        if (kernel_parameter_manager.isAnyModified()) {
            var receiver = {
            };
            receiver.type = "all";
            receiver.onIgnore = function() {
                kernel_parameter_manager.abandonAll();
                kernel_project_ui.clearSecen();
                Qt.callLater(Qt.quit);
            };
            receiver.onVisibleFalse = function(){
                //if(kernel_undo_proxy.canUndo)
                //    kernel_kernelui.beforeCloseWindow()
                //else  Qt.callLater(Qt.quit);
            }
            receiver.onSaved = function() {
                kernel_project_ui.clearSecen();
                Qt.callLater(Qt.quit);
            };
            idAllMenuDialog.requestMenuDialog(receiver, "saveParemDialog");
        } else {
            kernel_project_ui.clearSecen();
            Qt.callLater(Qt.quit);
        }
    }

    //    height: 720* screenScaleFactor
    color: Constants.mainBackgroundColor
    anchors.margins: 9 * screenScaleFactor

    Component.onCompleted: function() {
        kernel_kernelui.leftToolbar = topToolBar;
        kernel_kernelui.appWindow = standaloneWindow;
        kernel_kernelui.footer = idStatusBar;
        kernel_kernelui.glMainView = editorContent;
        kernel_kernelui.topbar = idMenuBar.item;
        kernel_kernelui.fontList = Constants.fontList;
        kernel_kernel.setScene3DWrapper(editorContent);

        //bind wizard
        Constants.screenScaleFactor = WizardUI.getScreenScaleFactor();
        WizardUI.setRootObjet(standaloneWindow);

        //topToolBar.mymodel = kernel_kernelui.topBarModel;
        kernel_kernelui.rightPanel = g_rightPanel;
        idStatusBar.bind(cxkernel_job_executor);
        // view.showMaximized()
        showWizardDlg();


        historyFileListModel.findHistoryFileList(); //历史记录

        gcodeFileListModel.cSourceModel.findGcodeFileList(); //文件列表
        videoListModel.findVideoFileList(); //延时摄影文件列表

        // materialBoxListModel.findMaterialBoxList();
        printerListModel.cSourceModel.findDeviceList(); //发现设备

        kernel_kernel.invokeFromQmlWindow();

        navBtn.active = true;
        kernel_kernelui.lMainModel.orderModel();
        kernel_kernelui.lOtherModel.orderModel();
        kernel_kernelui.topBarModel.orderModel();
        topToolBar.mainmodel = kernel_kernelui.lMainModel;
        topToolBar.othermodel = kernel_kernelui.lOtherModel;
        topToolBar.drawmodel = kernel_kernelui.topBarModel;
        timeoutJob.running = true;

        //初始化新建项目
//        project_kernel.initNewProject()
    }

    Timer {
        id: timeoutJob

        interval: 1000
        repeat: false
        running: false
        onTriggered: {
            if (cxkernel_cxcloud.profileService.autoCheckUpdateable) {
                cxkernel_cxcloud.profileService.checkOfficalProfileUpdateable()
            }
        }
    }

    Connections {
        function onCheckUpgradableFinished() {
            if (cxkernel_cxcloud.upgradeService.upgradable) {
                showReleaseNote();
            } else {
                if (manualCheck)
                    showNoUpdate();

            }
        }

        target: cxkernel_cxcloud.upgradeService
    }

    //带进度条的全局蒙版
    Control {
        id: topDialog

        width: parent.width
        height: parent.height
        padding: 0
        visible: false
        z: 20

        contentItem: Item {
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    mouse.accepted = true;
                }

                CusStyledStatusBar {
                    //                            if (idMachineManager.opened)
                    //                                idMachineManager.item.close();

                    id: idStatusBar

                    objectName: "StatusBarObj"
                    x: 0
                    y: 15 * screenScaleFactor
                    width: standaloneWindow.width
                    height: 500 * screenScaleFactor
                    currentPhase: kernel_kernel.currentPhase
                    onSigJobStart: {
                        topDialog.visible = true;
                    }
                    onSigJobEnd: {
                        topDialog.visible = false;
                        if (kernel_kernel.currentPhase === 1) {
                        }
                    }
                }

            }

        }

        background: Rectangle {
            color: "#1c1c1d"
            opacity: kernel_kernel.currentPhase == 1 ? 0.2 : 0.7
        }

    }

    //3D View
    QuickScene3DWrapper {
        id: editorContent

        function requestFirstConfig() {
            idAllMenuDialog.requestDialog("idStartFirstConfig");
        }

        z: -1
        objectName: "gLQuickItem"
        focus: true
    }

    Scene3D {
        id: scene3d

        z: -2
        opacity: (kernel_kernel.currentPhase != 2 && kernel_parameter_manager.functionType == 0) ? 1 : 0
        enabled: Constants.bGLViewEnabled
        aspects: ["input", "logic"]
        Component.onCompleted: {
           editorContent.bindScene3D(scene3d);
        }
    }

    SplitView {
        id: c3droot

        anchors.fill: parent
        anchors.topMargin: idMenuBar.height
        anchors.rightMargin: 0

        orientation: Qt.Horizontal

        handle: Item {
            implicitWidth: 2 * screenScaleFactor
        }

        Connections {
            target: swipeView

            function onWidthChanged() {
                c3droot.anchors.rightMargin = right_panel_hide_button.checked ? -swipeView.width : 0
            }
        }

        Component.onCompleted: {
            editorContent.parent = c3dItem;
            editorContent.anchors.fill = c3dItem;
            scene3d.parent = c3dItem;
            scene3d.anchors.fill = c3dItem;
        }

        Item {
            // TipsManageCom {
            //     id: tipsItem
            //     z: 20
            //     visible: true
            //     width: 450 * screenScaleFactor
            //     anchors.bottom: parent.bottom
            //     anchors.bottomMargin: 20 * screenScaleFactor
            //     anchors.left: parent.left
            // }

            id: c3dItem

            SplitView.fillWidth: true

            Loader {
                id: navBtn

                active: false
                anchors.right: parent.right
                anchors.rightMargin: 35 * screenScaleFactor
                anchors.top: parent.top
                anchors.topMargin: topToolBar.visible ? 100 * screenScaleFactor : 50 * screenScaleFactor
                //anchors.horizontalCenter: layerSliderRange.horizontalCenter
                //anchors.bottom: layerSliderRange.top
                anchors.bottomMargin: 0 * screenScaleFactor
                visible: kernel_parameter_manager.functionType === 0
                z: 3

                sourceComponent: RightBtns {
                    enabled: true

                }

            }

            StackLayout {
                id: idTopArea

                width: parent.width
                currentIndex: kernel_kernel.currentPhase == 0 ? 0 : 1

                Item {
                    id: idPrepareTopArea

                    width: parent.width

                    CToolBar {
                        //anchors.leftMargin: 10 * screenScaleFactor

                        id: topToolBar

                        property bool enabledState: true

                        width: parent.width
                        height: 60 * screenScaleFactor
                        objectName: "leftToolObj"
                        //enabled: kernel_model_selector.selectedNum > 0
                        visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType === 0
                        anchors.top: parent.top
                        anchors.topMargin: 1 * screenScaleFactor
                        anchors.left: parent.left
                        onVisibleChanged: {
                            if (visible) {
                            }
                        }
                    }

                }

                Item {
                    id: idSliceManager

                    width: parent.width
                }

            }

            PreviewLayerSlider {
                id: layerSliderRange

                property int currentPhase: kernel_kernel.currentPhase
                property int labelDecimal: 2

                z: 3
                width: 18 * screenScaleFactor
                height: (screenScaleFactor> 1 ? 300 : 360) * screenScaleFactor
                anchors.right: parent.right
                anchors.rightMargin: 65 * screenScaleFactor
                anchors.top: navBtn.bottom

                orientation: Qt.Vertical
                visible: currentPhase == 1 &&
                         kernel_parameter_manager.functionType == 0 &&
                         (kernel_slice_flow.isPreviewGCodeFile ||
                         (kernel_printer_manager.selectedPrinter.sliceCache && !kernel_slice_flow.isSlicing))

                enabled: currentPhase == 1
                from: 1
                // to: kernel_slice_model.layers
                stepSize: 1
                // first.value: from
                // second.value: to
                animation: idFDMFooter.startAnimation

                onValueChanged: {
                    if (!idFDMFooter.startAnimation)
                        kernel_slice_model.setAnimationState(0);

                    if (idFDMFooter.onlyLayer) {
                        kernel_slice_model.setCurrentLayer(value, idFDMFooter.randomstep);
                        kernel_slice_model.showOnlyLayer(value);
                    } else {
                        kernel_slice_model.setCurrentLayer(value, idFDMFooter.randomstep);
                    }
                }
            }

            Rectangle {
                width: 18 *screenScaleFactor
                height: 18 *screenScaleFactor
                anchors.horizontalCenter: layerSliderRange.horizontalCenter
                anchors.top: layerSliderRange.bottom
                // anchors.topMargin: 6
                visible: layerSliderRange.visible
                color: "transparent"
                property bool hovered: false

                Image {
                    anchors.fill: parent
                    source: parent.hovered ? "qrc:/UI/photo/slider/tips_hover.svg" : "qrc:/UI/photo/slider/tips_default.svg"
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        parent.hovered = true
                    }
                    onExited: {
                        parent.hovered = false
                    }
                }

                BasicTooltip {
                    id: layerSliderRangeTip
                    position: BasicTooltip.Position.BOTTOM
                    visible: parent.hovered
                    text: qsTr("Right click to display more actions.")
                    textWrap: false
                }
            }

            ZModelRange {
                id: layerModelRange

                property int currentPhase: kernel_kernel.currentPhase
                property string labelTextBegin: "Z: "
                property int labelDecimal: 2

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
                height: (y + 360 * screenScaleFactor) > (idSliceBtn.y - 150) ? 300 * screenScaleFactor : 360 * screenScaleFactor
                anchors.right: parent.right
                anchors.rightMargin: 65 * screenScaleFactor
                anchors.top: navBtn.bottom
                //anchors.verticalCenter: idFDMPreview.verticalCenter
                //anchors.verticalCenterOffset: 35 * screenScaleFactor
                orientation: Qt.Vertical
                visible: kernel_kernel.isDefaultVisScene && currentPhase === 0 && currentPhase != 2 && kernel_parameter_manager.functionType === 0 && kernel_kernelui.commandIndex !==32
                enabled: kernel_modelspace.modelNNum > 0
                from: 0
                to: plugin_info_sliderinfo.maxLayer + 0.01
                stepSize: 0.1
                first.value: from
                second.value: kernel_modelspace.modelNNum <= 0 ? from : to
                animation: idFDMFooter.startAnimation
                firstControl: true
                firstLabelText: labelTextBegin + first.value.toFixed(labelDecimal)
                secondLabelText: labelTextBegin + second.value.toFixed(labelDecimal)
                first.onValueChanged: plugin_info_sliderinfo.setBottomCurrentLayer(first.value)
                second.onValueChanged: plugin_info_sliderinfo.setTopCurrentLayer(second.value)
            }

            Item {
                id: layerModelRangeShortcut

                Shortcut {
                    sequence: "Up"
                    onActivated: {
                        if (kernel_kernel.isDefaultVisScene)
                            layerModelRange.second.value += 0.1;

                    }
                }

                Shortcut {
                    sequence: "Down"
                    onActivated: {
                        if (kernel_kernel.isDefaultVisScene)
                            layerModelRange.second.value -= 0.1;

                    }
                }

            }

            FDMPreviewPanel {
                id: idFDMPreview

                objectName: "FDMPreviewPanel"
                z: 3
                anchors.top: parent.top
                anchors.left: !kernel_slice_flow.isPreviewGCodeFile ? idSliceSeletor.right : parent.left
                anchors.topMargin: 10 * screenScaleFactor
                anchors.leftMargin: 10 * screenScaleFactor
                fdmMainObj: editorContent
            }

            SliceSeletorPanel {
                // anchors.bottom: parent.bottom
                // anchors.bottomMargin: 10 * screenScaleFactor

                id: idSliceSeletor

                property int maxHeight: parent.height - 10 - y

                anchors.left: parent.left
                //                anchors.leftMargin: 10 * screenScaleFactor
                anchors.top: parent.top
                anchors.topMargin: 10 * screenScaleFactor
                height: recommendedHeight > maxHeight ? maxHeight : recommendedHeight
                z: 3
                visible: idFDMPreview.visible && !kernel_slice_flow.isPreviewGCodeFile
            }

            Loader {
                id: machineManagerLoader

                active: false
                sourceComponent: __printerMangerComponet
            }

            InfosPanel {
                id: infosPanel

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 6 * screenScaleFactor
                anchors.left: parent.left
                anchors.leftMargin: 6 * screenScaleFactor
                visible: kernel_kernel.currentPhase === 0 // && kernel_model_selector.selectedNum > 0
            }

            MessageStack {
                id: messageStack

                width: 430 * screenScaleFactor
                anchors.bottom: infosPanel.visible ? infosPanel.top : parent.bottom
                anchors.bottomMargin: 10 * screenScaleFactor
                anchors.left: parent.left
                anchors.leftMargin: 6 * screenScaleFactor
                z: 10
                Component.onCompleted: {
                    kernel_kernelui.setMessageStack(messageStack);
                }
            }

            CSliceButton {
                id: idSliceBtn

                width: 216 * screenScaleFactor
                onWifiButtonClicked: {
                    kernel_slice_model.saveCustomGcode();
                    idUploadGcodeDialog.showUploadDialog(1);
                }
                onUploadButtonClicked: {
                    if (cxkernel_cxcloud.accountService.logined) {
                        kernel_slice_model.saveCustomGcode();
                        idUploadGcodeDialog.showUploadDialog(0);
                    } else {
                        kernel_ui.requestQmlDialog("idWarningLoginDlg");
                    }
                }
                onExportButtonClicked: {
                    kernel_slice_model.saveCustomGcode();
                    kernel_slice_flow.saveGCodeLocal();
                }
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.bottomMargin: -4 * screenScaleFactor
                anchors.rightMargin: {
                    const normal_margin = 6 * screenScaleFactor
                    if(screenScaleFactor > 1){
                        return normal_margin + process_group_list_view.width
                    }
                    if (frameLessView.isMax || frameLessView.isFull) {
                        return normal_margin
                    }

                    if (right_panel_hide_button.checked) {
                        return normal_margin
                    }

                    return normal_margin + process_group_list_view.width
                }
            }

            BasicTooltip {
                id: idGlobalToopTip

                visible: false
                textWrap: false
                position: BasicTooltip.Position.RIGHT
                delay: 0
            }

            FDMFooterPanel {
                id: idFDMFooter

                property bool bLock: false

                function setCurrentStep(step) {
                    bLock = true;
                    currentStep = step;
                    bLock = false;
                }

                z: 3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10 * screenScaleFactor
                //anchors.horizontalCenter: parent.horizontalCenter
                anchors.left: idSliceSeletor.right
                anchors.leftMargin: (parent.width - idSliceSeletor.width - idSliceBtn.width - width) / 2
                // panelWidth: parent.width - idSliceSeletor.width - idSliceBtn.width - 30 * screenScaleFactor
                //idSliceBtn.left
                visible: layerSliderRange.visible
                previewSlider: layerSliderRange
                stepMax: kernel_slice_model.steps
                currentStep: kernel_slice_model.steps
                onVisibleChanged: {
                    if (visible)
                        kernel_slice_model.setCurrentLayerFocused(false);

                }
                onStepSliderChange: {
                    kernel_slice_model.setCurrentStep(value);
                    if (!bLock) {
                        idFDMPreview.aFDMRightPanel.setLayerGCodesIndex(value);
                    }
                }

                Connections {
                    target: kernel_slice_model
                    onStepsChanged: {
                        idFDMFooter.stepMax = kernel_slice_model.steps;
                        idFDMFooter.currentStep = kernel_slice_model.steps;
                    }
                }

            }
        }

        //右侧工具栏
        Rectangle {
            id: swipeView

            SplitView.minimumWidth: {
                return 400 * screenScaleFactor
            }

            SplitView.maximumWidth: {
                if (frameLessView.isMax || frameLessView.isFull) {
                    return   960 * screenScaleFactor
                }

                return 640 * screenScaleFactor
            }

            color: Constants.right_panel_bgColor

            Rectangle {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: (Constants.currentTheme ? 1 : 0) * screenScaleFactor
                color: Constants.right_panel_menu_split_line_color
            }

            PrintParameterPanel {
                id: right_panel
                anchors.fill: parent
                enabled: !right_panel_hide_button.checked
                processGroupView: process_group_list_view
            }

            Button {
                id: right_panel_hide_button

                anchors.right: parent.left
                y: right_panel.objectTitle.y
                width: 12 * screenScaleFactor
                height: 74 * screenScaleFactor
                checkable: true

                NumberAnimation {
                    id: right_panel_animation
                    target: c3droot
                    property: "anchors.rightMargin"
                    duration: 150
                }

                onCheckedChanged: _ => {
                    right_panel_animation.stop()
                    right_panel_animation.to = checked ? -swipeView.width : 0
                    right_panel_animation.start()
                }

                
               Keys.onPressed: {
                        if (event.key === Qt.Key_Space) {
                            event.accepted = true;
                        }
                    }

                Shortcut {
                    context: Qt.WindowShortcut
                    sequence: "Shift+Tab"
                    onActivated: {
                        right_panel_hide_button.toggle()
                    }
                }

                background: Image {
                    source: Constants.drawerBgImg

                    Image {
                        anchors.centerIn: parent
                        width: 4 * screenScaleFactor
                        height: 7 * screenScaleFactor
                        source: {
                            if (right_panel_hide_button.checked) {
                                if (right_panel_hide_button.hovered) {
                                    return "qrc:/UI/photo/rightDrawer/out_hover.svg"
                                } else {
                                    return "qrc:/UI/photo/rightDrawer/out_normal.svg"
                                }
                            } else {
                                if (right_panel_hide_button.hovered) {
                                    return "qrc:/UI/photo/rightDrawer/in_hover.svg"
                                } else {
                                    return "qrc:/UI/photo/rightDrawer/in_normal.svg"
                                }
                            }
                        }
                    }
                }
            }

            Control {
                id: process_group_control

                readonly property real borderWidth: 1 * screenScaleFactor
                readonly property real borderRadius: 5 * screenScaleFactor

                anchors.right: parent.left
                anchors.rightMargin: -borderRadius - borderWidth

                y: right_panel.configLayout.y - borderRadius
                z: right_panel.z - 1

                width: 28 * screenScaleFactor + borderRadius + borderWidth
                height: implicitContentHeight + 2 * borderRadius

                enabled: !right_panel_hide_button.checked
                visible: enabled && right_panel.configLayout.currentIndex === 0

                background: Rectangle {
                    radius: 5 * screenScaleFactor
                    color: Constants.right_panel_bgColor
                    border.width: (Constants.currentTheme ? 1 : 0) * screenScaleFactor
                    border.color: Constants.right_panel_menu_split_line_color
                }

                contentItem: ListView {
                    id: process_group_list_view

                    anchors.fill: parent
                    anchors.topMargin: process_group_control.borderRadius
                    anchors.bottomMargin: process_group_control.borderRadius
                    anchors.leftMargin: process_group_control.borderWidth
                    anchors.rightMargin: process_group_control.borderRadius

                    implicitHeight: contentHeight
                    spacing: process_group_control.borderWidth
                    interactive: false

                    model: kernel_ui_parameter.processTreeModel.childNodes
                    currentIndex: 0

                    delegate: ItemDelegate {
                        id: process_group_item

                        implicitWidth: 28 * screenScaleFactor
                        implicitHeight: 28 * screenScaleFactor
                        highlighted: ListView.isCurrentItem

                        onClicked: {
                            ListView.view.currentIndex = model.index
                        }

                        BasicTooltip {
                            visible: parent.hovered
                            timeout: -1
                            position: BasicTooltip.Position.LEFT
                            font.pointSize: Constants.labelFontPointSize_10
                            text: qsTranslate("ParameterComponent", model_node.label)
                        }

                        background: Canvas {
                            readonly property real radius: process_group_control.borderRadius
                            readonly property color borderColor: hovered
                                    ? Constants.right_panel_border_hovered_color : "transparent"
                            readonly property color backgroundColor: highlighted
                                    ? Constants.themeGreenColor : Constants.right_panel_bgColor

                            contextType: "2d"

                            onBorderColorChanged: {
                                requestPaint()
                            }

                            onBackgroundColorChanged: {
                                requestPaint()
                            }

                            onPaint: {
                                const context = getContext("2d")
                                context.clearRect(0, 0, width, height)

                                context.beginPath()
                                context.moveTo(width, 0)
                                context.lineTo(width, height)
                                context.lineTo(radius, height)
                                context.arcTo(0, height, 0, height - radius, radius, radius)
                                context.lineTo(0, radius)
                                context.arcTo(0, 0, radius, 0, radius, radius)
                                context.closePath()

                                context.fillStyle = backgroundColor
                                context.fill()

                                context.strokeStyle = borderColor
                                context.stroke()
                            }
                        }

                        contentItem: Item {
                            Image {
                                anchors.centerIn: parent
                                width: 18 * screenScaleFactor
                                height: 18 * screenScaleFactor
                                sourceSize.width: width
                                sourceSize.height: height
                                horizontalAlignment: Qt.AlignHCenter
                                verticalAlignment: Qt.AlignVCenter
                                fillMode: Image.PreserveAspectFit
                                source: {
                                    if (model_node.modifyed) {
                                        return model_node.modifyedIcon
                                    }

                                    if (highlighted) {
                                        return model_node.checkedIcon
                                    }

                                    return model_node.icon
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //全局拖放文件区域
    DropArea {
        id: idDropArea

        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls)
            {
                let fileUrls = drop.urls
                for(let fileUrl of fileUrls)
                {
                    let filepath = fileUrl.toString()
                    filepath = filepath.replace("file:///", "")
                    let fileName = filepath.split("/").pop(); // 提取文件名
                    let fileExtension = fileName.split(".").pop(); // 提取文件后缀
                    console.log("fileExtension =",fileExtension)
                    if(fileExtension.toLowerCase() === "cxprj" ||
                            fileExtension.toLowerCase() === "3mf")
                    {
                        project_kernel.openProject(filepath)   //Do not continue if there is a project file
                        return
                    }
                }
                kernel_io_manager.openWithUrls(drop.urls);
            }

        }
    }

    //全局程序缩放区域
    CusResizeBorder {
        id: resizeBorder

        visible: Qt.platform.os !== "windows"
        borderWidth: visible ? 4 * screenScaleFactor : 0
        enabled: visible
        anchors.fill: standaloneWindow
        control: frameLessView
    }

    // 主窗口 Dock 组件最大化区域
    DockContext {
        id: dock_context

        readonly property var followTarget: c3droot
        readonly property var followPosition: followTarget.mapToGlobal(0, 0)

        // 留 1 个单位的边界, 避免子窗口最大化时覆盖主窗口导致主窗口无法拖放大小
        tabBarX: frameLessView.x + followPosition.x + 1
        tabBarY: {
            let base_y = frameLessView.y + followPosition.y + 1
            if (Qt.platform.os === "windows" && frameLessView.isMax) {
                base_y += 8
            }
            return base_y
        }
        tabBarWidth: frameLessView.width - 2
        tabBarHeight: 30 * screenScaleFactor
        adsorbedItemX: tabBarX
        adsorbedItemY: tabBarY + tabBarHeight
        adsorbedItemWidth: tabBarWidth
        adsorbedItemHeight: followTarget.height - tabBarHeight - 2
    }
    Loader {
        active: Qt.platform.os == "osx"
        sourceComponent: macMenuBar
    }
    //菜单加载
    Loader {
        id: idMenuBar

        width: parent.width
        height: 32 * screenScaleFactor
        sourceComponent: {
                return winMenuBar;
        }
    }

    Component {
        id: winMenuBar

        CWinMenu {
            mainWindow: standaloneWindow
            cloudContext: cloud_context
            sliceBtnVisible: !idStatusBar.visible
            onSetPrepare: {
                topToolBar.switchPickMode();
                kernel_kernel.setKernelPhase(0);
                editorContent.focus = true;
            }
            onSetPreview: {
                if (kernel_kernel.currentPhase !== 1) {
                    idSliceBtn.startSlice();
                    dock_context.closeAllItem();
                }
            }
            onSetDevice: {
                topToolBar.switchPickMode();
                //                idMachineManager.item.close();
                kernel_kernel.setKernelPhase(2);
                dock_context.closeAllItem();
            }
        }

    }

    Component {
        id: macMenuBar

        CMacMenu {

        }

    }

    // BasicRClickMenu {
    //     id: multiModelsMenu
    //     menuItems: kernel_rclick_menu_data.multiModelsMenuList
    //     // filamentObj: kernel_rclick_menu_data.filamentModel
    // }
    NoModelMenu {
        id: noModelMenu
    }

    SingleModelMenu {
        id: singleModelMenu
    }

    MultiModelsMenu {
        id: multiModelsMenu
    }

    Connections {
        target: kernel_kernelui
        onSigShowRightMenu: {
            kernel_rclick_menu_data.updateState();
            if (kernel_rclick_menu_data.menuType === 0) {
                noModelMenu.popup();
            } else if (!kernel_model_selector.wipeTowerSelected) {
                if (kernel_rclick_menu_data.menuType === 1)
                    singleModelMenu.popup();
                else if (kernel_rclick_menu_data.menuType === 2)
                    multiModelsMenu.popup();
            }
        }
    }
    // Loader {

    Loader {
        id: __LANload

        property bool wifiSwitch: false
        property string curGcodeFileName: ""
        property string curGcodeFilePath: ""

        z: 1000
        anchors.fill: parent
        anchors.topMargin: idMenuBar.height
        //sourceComponent: __printerComponet
        visible: kernel_kernel.currentPhase == 2
        onVisibleChanged: {
            if (visible) {
                __LANload.item.setRealEntryPanel(wifiSwitch);
                if (wifiSwitch) {
                    __LANload.item.setCurGcodeFileName(curGcodeFileName);
                    __LANload.item.setCurGcodeFilePath(curGcodeFilePath);
                }
            } else {
                wifiSwitch = false;
                printerListModel.setCurrentDevice("");
            }
        }
    }

    AllMenuDialog {
        id: idAllMenuDialog

        objectName: "idAllMenuDialog"
        onSigShowLoginPanel: idMenuBar.item.showLoginPanel()
        onDialogShowed: function(name) {
            if (name == "slice_height_setting_dialog")
                zSlider.resetValue();

        }
    }

   //模型库弹窗
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
        authorInfoDialog: model_author_info
        visible: false
        onRequestLogin: {
            kernel_ui.requestQmlDialog("idWarningLoginDlg");
        }
        onRequestToShowDownloadTip: {
            download_manage_button.showTip();
            cloud_context.downloadManageDialog.show();
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

    //用户信息弹窗
    ModelLibraryAuthorInfo {
        id: model_author_info

        visible: false
        context: standaloneWindow.dockContext
        onRequestToShowDownloadTip: {
            download_manage_button.showTip();
            cloud_context.downloadManageDialog.show();
        }
    }

    //上传GCode弹窗
    UploadGcodeDlg {
        id: idUploadGcodeDialog

        objectName: "idUploadGcodeDialog"
        onSigCancelBtnClicked: {
            visible = false;
        }
        cloudContext :cloud_context
        onSigConfirmBtnClicked: {
            if (uploadDlgType) {
                visible = false;
                __LANload.wifiSwitch = true;
                __LANload.curGcodeFileName = gcodeFileName + ".gcode";
                __LANload.curGcodeFilePath = gcodeFilePath;
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

    //上传模型弹窗
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

            function onUploadModelGroupFailed() {
                idUploadModelDialog.uploadModelError();
            }

        }

    }

    //创想云相关的页面
    CloudContext {
        id: cloud_context

        modelLibarayDialog: idModelLibraryInfoDialog // temp
        sliceFileUploadDialog: idUploadGcodeDialog // temp
        modelGroupUploadDialog: idUploadModelDialog // temp
    }

    Component {
        id: __printerComponet

        LanPrinterPanel {
        }

    }

    Component {
        id: __printerMangerComponet

        MachineManagerPanel_New {
        }

    }

    Connections {
        target: kernel_kernelui
        //Triggered when switching language,Handle menu bar switching separately
        onSigChangeMenuLanguage: {
        }
        onSigCloseAction: {
            saveParemDialog();
            //standaloneWindow.close();
        }
        onSigOpenglOld: {
            dlgTimer.message = qsTr("The OpenGL version is too low and may not func properly. Please upgrade the graphics card driver!");
            dlgTimer.start();
        }
        onSigUseOpenGLES: {
            dlgTimer.message = qsTr("Creality Print only partially supports OpenGL ES.");
            dlgTimer.start();
        }
    }

    Connections {
        target: frameLessView
        onClosing: {
            var a = kernel_kernel.checkUnsavedChanges();
            if(!a)
            {
                kernel_project_ui.clearSecen();
                //normal exit，delete tmpProject
                kernel_project_ui.deleteTempProjectDirectory()
                close.accepted = true;
            }else{
                close.accepted = false;
            }

        }
    }
    Connections {

         target: kernel_parameter_manager
        onMachinesCountChanged: {
                if (kernel_parameter_manager.machinesCount === 0) {
                    idAllMenuDialog.requestDialog("idAddPrinterDlgNew");
                } else {
                }
            }
        }


}
