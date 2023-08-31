import QtQuick 2.13
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0 as QQC2
import QtQuick.Controls 2.5 as QQC25

import Qt.labs.platform 1.0
import Qt.labs.settings 1.1
import QtQml 2.13

import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"
import "./mainwindowCom"
import "./CusMenu"


QQC2.ApplicationWindow {
    id: standaloneWindow
    flags: Qt.Widget
    title: qsTr("Creative3D")
    color: "blue"
    minimumHeight: ( 720* screenScaleFactor) > Screen.desktopAvailableHeight? Screen.desktopAvailableHeight:(720 * screenScaleFactor)//400
    minimumWidth: ( 1280 * screenScaleFactor) > Screen.desktopAvailableWidth ? Screen.desktopAvailableWidth:(1280 * screenScaleFactor)

    property int screenScaleFactor: 1
    property int tmpWidth: 0
    property int defultRightPanelHeight: 630 * screenScaleFactor
    property var imageSource: (Constants.currentTheme === 0) ? "qrc:/UI/photo/bgImg_black.png" : "qrc:/UI/photo/bgImg.png"
    property bool isWindow: Qt.platform.os === "windows" ? true : false

//    property alias leftToolBar: leftToolBar
//    property alias statusBarItem: idStatusBar
//    property alias g_rightPanel: swipeView
    //    property alias dockContext: dock_context
//    property alias zSlider: layerSliderRange
    property alias resizeItem: resize_item

//    property alias menuDialog: idAllMenuDialog

    signal sigInitAdvanceParam()



    function startSlice() {
        var ret = kernel_modelspace.haveModelsOutPlatform()
        if(ret)
        {
            idOutPlatform.visible = true
        }
        else {
            doSlice()
        }
    }

    function doSlice(){
        kernel_kernel.setKernelPhase(1)
        kernel_slice_flow.slice()
    }

    Loader {
        id: __LANload
        property bool wifiSwitch: false
        property string curGcodeFileName: ""

        z: 1000
        anchors.fill: parent
        anchors.topMargin: idMenuBar.height + topToolBar.height
        //        visible: kernel_kernel.currentPhase == 2

        onVisibleChanged: {
            if(visible)
            {
                __LANload.item.setRealEntryPanel(wifiSwitch)
                if(wifiSwitch) __LANload.item.setCurGcodeFileName(curGcodeFileName)
            }
            else {
                wifiSwitch = false
            }
        }
    }

    DropArea{
        id: idDropArea
        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls){
                kernel_io_manager.openWithUrls(drop.urls)
            }
        }
    }

    function doMinimized() {
        if(!isWindow)
        {
            flags = Qt.Window | Qt.WindowFullscreenButtonHint | Qt.CustomizeWindowHint | Qt.WindowMinimizeButtonHint
        }
        else
        {
            flags = Qt.Window | Qt.FramelessWindowHint | Qt.WindowMinimizeButtonHint
        }
        visibility = Window.Minimized
    }

    function showReleaseNote(){
        idReleaseNote.exec()
    }

    function changeThemeColor(themeType){
        Constants.currentTheme = themeType
    }

    //lisugui 2020-10-29
    function beforeCloseWindow(){
        kernel_kernelui.beforeCloseWindow()
    }

    function showFdmPmDlg(filePath){
        idFdmPmDlg.visible = true
        idFdmPmDlg.filePath = filePath
    }

    function showWizardDlg() {
        sigInitAdvanceParam()
        __LANload.sourceComponent = __printerComponet
    }

    onClosing:{
        var a = kernel_kernel.checkUnsavedChanges();
        close.accepted = a;
    }

    Component.onCompleted:{
        showMaximized()
    }

    background:Rectangle{
        anchors.fill:parent
        color: Constants.themeColor_secondary
    }

    ResizeItem {
        id: resize_item

        enableSize: 8
        anchors.fill: parent
        focus: true
        cursorEnable : standaloneWindow.visibility !== Window.Maximized && standaloneWindow.visibility !== Window.FullScreen

        CusMenuBar{
            id: idMenuBar
            width: parent.width
            //myLoad:content
            height: 32 * screenScaleFactor
            mainWindow: standaloneWindow

            onCloseWindow:
            {
                beforeCloseWindow()
                standaloneWindow.close()
            }
            onChangeServer:
            {
                idLoginBtn.setCurrentServer(serverIndex)
            }
        }

        Rectangle{
            id: depRec
            width: parent.width
            height: 0
            anchors.top: idMenuBar.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            color: Constants.themeColor_secondary
        }

        //        ZSliderRange {
        //            id: layerSliderRange

        //            property int currentPhase: kernel_kernel.currentPhase
        //            property string labelTextBegin: (currentPhase == 0 ? "Z" : qsTr("Layer")) + ": "
        //            property int labelDecimal: currentPhase == 0 ? 1 : 0

        //            function resetValue() {
        //                first.value = Qt.binding(function() { return from })
        //                second.value = Qt.binding(function() {
        //                    return kernel_modelspace.modelNNum <= 0 ? from : to
        //                })
        //            }

        //            z: 3
        //            width: 18 * screenScaleFactor
        //            height: 360 * screenScaleFactor
        //            anchors.right: parent.right
        //            anchors.rightMargin: Constants.rightPanelWidth + 65 * screenScaleFactor
        //            anchors.verticalCenter: idFDMPreview.verticalCenter
        //            anchors.verticalCenterOffset: 35 * screenScaleFactor
        //            orientation: Qt.Vertical

        //            visible: currentPhase != 2 && kernel_parameter_manager.functionType == 0
        //            enabled: currentPhase == 0 ? kernel_modelspace.modelNNum > 0 : true

        //            from: currentPhase == 0 ? 0 : 1
        //            to: currentPhase == 0 ? plugin_info_sliderinfo.maxLayer + 0.01 : kernel_slice_model.layers
        //            stepSize: currentPhase == 0 ? 0.1 : 1
        //            first.value: from
        //            second.value: kernel_modelspace.modelNNum <= 0 ? from : to

        //            animation: idFDMFooter.startAnimation
        //            firstControl: currentPhase == 0
        //            firstLabelText: labelTextBegin + first.value.toFixed(labelDecimal)
        //            secondLabelText: labelTextBegin + second.value.toFixed(labelDecimal)

        //            first.onValueChanged: {
        //                if (currentPhase == 0) {
        //                    plugin_info_sliderinfo.setBottomCurrentLayer(first.value)
        //                }
        //            }

        //            second.onValueChanged: {
        //                if (currentPhase == 0) {
        //                    plugin_info_sliderinfo.setTopCurrentLayer(second.value)
        //                } else {
        //                    if(!idFDMFooter.startAnimation)
        //                        kernel_slice_model.setAnimationState(0)
        //                    if(idFDMFooter.onlyLayer)
        //                    {
        //                        kernel_slice_model.setCurrentLayer(second.value, idFDMFooter.randomstep)
        //                        kernel_slice_model.showOnlyLayer(second.value)
        //                    }else{
        //                        kernel_slice_model.setCurrentLayer(second.value, idFDMFooter.randomstep)
        //                    }

        //                }
        //            }

        //        }

        //        FDMPreviewPanel {
        //            id: idFDMPreview
        //            objectName: "FDMPreviewPanel"
        //            z: 3
        //            anchors.top: topToolBar.bottom
        //            anchors.right: parent.right
        //            anchors.topMargin: 10 * screenScaleFactor
        //            anchors.rightMargin: 10 * screenScaleFactor
        ////            fdmMainObj: editorContent
        //        }

        //        FDMFooterPanel {
        //            id: idFDMFooter

        //            z: 3
        //            anchors.bottom: parent.bottom
        //            anchors.left: leftToolBar.right
        //            anchors.right: idFDMPreview.left
        //            anchors.bottomMargin: 10 * screenScaleFactor
        //            anchors.leftMargin: 10 * screenScaleFactor
        //            anchors.rightMargin: 10 * screenScaleFactor

        //            visible: idFDMPreview.visible
        //            previewSlider: layerSliderRange

        //            stepMax: kernel_slice_model.steps
        //            currentStep: kernel_slice_model.steps

        //            Connections {
        //                target: kernel_slice_model
        //                onStepsChanged: {
        //                    idFDMFooter.stepMax = kernel_slice_model.steps
        //                    idFDMFooter.currentStep = kernel_slice_model.steps
        //                }
        //            }
        //            onVisibleChanged:
        //            {
        //                if(visible)
        //                {
        //                    kernel_slice_model.setCurrentLayerFocused(false)
        //                }
        //            }
        //            onStepSliderChange:
        //            {
        //                kernel_slice_model.setCurrentStep(value)
        //                idFDMPreview.aFDMRightPanel.setLayerGCodesIndex(
        //                            kernel_slice_model.findViewIndexFromStep(value))
        //            }
        //        }

        //        FDMPreviewButtonBox {
        //            id: idFDMButtonBox

        //            z: 3
        //            anchors.right: parent.right
        //            anchors.bottom: parent.bottom
        //            anchors.rightMargin: 10 * screenScaleFactor
        //            anchors.bottomMargin: 10 * screenScaleFactor

        //            onUsbButtonClicked:
        //            {
        //                if(kernel_slice_flow.sliceAttain)
        //                {
        //                    plugin_usb_printer.startUSBPrint(kernel_slice_flow.sliceAttain.sourceFileName(), kernel_slice_flow.sliceAttain.printTime())
        //                }
        //            }
        //            onWifiButtonClicked:
        //            {
        //                idUploadGcodeDialog.showUploadDialog(1)
        //            }
        //            onUploadButtonClicked:
        //            {
        //                if(cxkernel_cxcloud.accountService.logined)
        //                {
        //                    idUploadGcodeDialog.showUploadDialog(0)
        //                }
        //                else {
        //                    kernel_ui.requestQmlDialog("idWarningLoginDlg")
        //                }
        //            }
        //            onExportButtonClicked:
        //            {
        //                kernel_slice_flow.saveGCodeLocal()
        //            }
        //        }

        //        ReleaseNote {
        //            id:idReleaseNote
        //            onRequestNoNewVersionTip: {
        //                idUpdateDlg.show()
        //            }
        //        }

        Timer {
            id: dlgTimer
            interval: 1000; running: false; repeat: false
            onTriggered: {
                idConfirmMessageDlg.visible = true
                idConfirmMessageDlg.msgText = qsTr("The OpenGL version is too low to start Creality Print, please upgrade the graphics card driver!")
            }
        }

        QQC25.Control{
            id: topToolBar
            width: parent.width
            height: 60  * screenScaleFactor
            anchors.top: depRec.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            background:Rectangle{
                opacity: 1.0
                gradient: Gradient {
                    GradientStop {  position: 0.0;    color: Constants.themeColor_third  }
                    GradientStop {  position: 1.0;    color: "transparent"  }
                }
            }

            Item{
                anchors.fill: parent
                CusText{
                    text: "Test"
                    fontPointSize: 18
                    fontBold: true
                    fontColor: Constants.currentTheme === 0 ? "#ababab":"#7d7d82"
                    //                fontWeight: Font.Bold
                    fontFamily : Constants.labelFontFamilyBold
                    fontWidth: 300*  screenScaleFactor
                    //                    opacity: 0.5
                    hAlign: 0
                    anchors.left: parent.left
                    anchors.leftMargin: 5 *  screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                }

                Row{
                    anchors.centerIn: parent
                    spacing: 10 * screenScaleFactor
                    CusButton{
                        anchors.verticalCenter: parent.verticalCenter
                        id:prepairBtn
                        width: 100 * screenScaleFactor
                        height: 32 * screenScaleFactor
                        btnBg.radius: 5 * screenScaleFactor
                        checkable: true
                        checked: true
                        normalColor: "transparent"
                        hoveredColor: Constants.btnHoverColor
                        pressedColor: Constants.btnNormalColor

                        bText.fontPointSize: Constants.labelFontPointSize_12
                        bText.fontFamily: Constants.labelFontFamilyBold
                        bText.fontBold: true
                        bText.text:qsTr("Prepare")
                        onClicked: {
                            kernel_kernel.setKernelPhase(0)
                            editorContent.focus = true
                        }
                    }

                    CusButton{
                        anchors.verticalCenter: parent.verticalCenter
                        id: previewBtn
                        width: 100 * screenScaleFactor
                        height: 32 * screenScaleFactor
                        btnBg.radius: 5 * screenScaleFactor
                        checkable: true
                        normalColor: "transparent"
                        hoveredColor: Constants.btnNormalColor
                        pressedColor: Constants.btnHoverColor

                        bText.fontPointSize: Constants.labelFontPointSize_12
                        bText.fontFamily: Constants.labelFontFamilyBold
                        bText.fontBold: true
                        bText.text:qsTr("Preview")

                        onClicked: {
                            startSlice()
                            dock_context.closeAllItem()
                        }
                    }

                    CusButton{
                        anchors.verticalCenter: parent.verticalCenter
                        width: 100 * screenScaleFactor
                        height: 32 * screenScaleFactor
                        btnBg.radius: 5 * screenScaleFactor
                        enabled: __LANload.status == Loader.Ready
                        checkable: true
                        normalColor: "transparent"
                        hoveredColor: Constants.btnHoverColor
                        pressedColor: Constants.btnCheckedColor

                        bText.fontPointSize: Constants.labelFontPointSize_12
                        bText.fontFamily: Constants.labelFontFamilyBold
                        bText.fontBold: true
                        bText.text:qsTr("Device")
                        onClicked: {
                            kernel_kernel.setKernelPhase(2)
                            dock_context.closeAllItem()
                        }
                    }
                }

                RowLayout {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: spacing
                    spacing: 10 * screenScaleFactor

                    QQC25.Button {
                        id:undoBtn

                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 40 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

//                        BasicTooltip {
//                            id: undoTooptip
//                            visible: undoBtn.hovered && String(text).length
//                            position: BasicTooltip.Position.BOTTOM
//                            text: qsTr("Undo")
//                        }

                        background: Rectangle {
                            color: parent.hovered ? (Constants.currentTheme ? "#FFE0E1E5" : "#80000000") : "transparent"
                            radius: parent.width / 2

                            Image {
                                anchors.centerIn: parent
                                width: 16 * screenScaleFactor
                                height: 15 * screenScaleFactor
                                source: (undoBtn.hovered && !Constants.currentTheme) ? "qrc:/res/img/undo_hover.svg" : "qrc:/res/img/undo_normal.svg"
                            }
                        }

                        onClicked: {
                            //添加模型是否有添加支撑的判断，给出提示
                            var hasSupport = kernel_model_list_data.hasSupport()

                            console.log("****hasSupport = ", hasSupport)
                            if(hasSupport){
                                idClosePro.visible = true
                            }else{
                                kernel_undo_proxy.undo()
                            }
                        }
                    }

                    QQC25.Button {
                        id: redoBtn
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredWidth: 40 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

//                        BasicTooltip {
//                            id: redoTooptip
//                            visible: redoBtn.hovered && String(text).length
//                            position: BasicTooltip.Position.BOTTOM
//                            text: qsTr("Redo")
//                        }

                        background: Rectangle {
                            color: parent.hovered ? (Constants.currentTheme ? "#FFE0E1E5" : "#80000000") : "transparent"
                            radius: parent.width / 2

                            Image {
                                anchors.centerIn: parent
                                width: 16 * screenScaleFactor
                                height: 15 * screenScaleFactor
                                source: (redoBtn.hovered && !Constants.currentTheme) ? "qrc:/res/img/redo_hover.svg" : "qrc:/res/img/redo_normal.svg"
                            }
                        }

                        onClicked: kernel_undo_proxy.redo()
                    }

//                    DownloadManageButton {
//                        id: download_manage_button

//                        visible: kernel_global_const.cxcloudEnabled

//                        Layout.alignment: Qt.AlignRight
//                        Layout.preferredWidth: 40 * screenScaleFactor
//                        Layout.preferredHeight: 40 * screenScaleFactor

//                        downloadTaskCount: download_manage_dialog.downloadTaskCount
//                        finishedTaskCount: download_manage_dialog.finishedTaskCount

//                        BasicTooltip {
//                            id: downLoadTooptip
//                            visible: download_manage_button.hovered && String(text).length
//                            position: BasicTooltip.Position.BOTTOM
//                            text: qsTr("Download Manager")
//                        }

//                        onClicked: {
//                            download_manage_dialog.visible = true
//                        }
//                    }

//                    LoginBtn {
//                        id: idLoginBtn
//                        objectName: "loginBtn"
//                        visible: kernel_global_const.cxcloudEnabled

//                        BasicTooltip {
//                            id: loginTooptip
//                            visible: idLoginBtn.hovered && String(text).length
//                            position: BasicTooltip.Position.BOTTOM
//                            text: cxkernel_cxcloud.accountService.logined
//                                  ? qsTr("Personal Center") : qsTr("Login")
//                        }

//                        Layout.alignment: Qt.AlignRight
//                        Layout.preferredWidth: 40 * screenScaleFactor
//                        Layout.preferredHeight: 40 * screenScaleFactor
//                    }
                }
            }
        }

        StackLayout {
            id: btnStack
            width: 254 * screenScaleFactor
            height: 58 * screenScaleFactor

            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10 * screenScaleFactor
            anchors.bottomMargin: 10 * screenScaleFactor

            currentIndex: 0

            Repeater {
                model: ListModel {
                    id: sliceBtnsModel
                    ListElement{ textContent: qsTr("Slice"); isEnabled: false }
                    ListElement{ textContent: qsTr("Generate GCode"); isEnabled: false }
                    ListElement{ textContent: qsTr("Generate GCode"); isEnabled: false }
                }
                delegate: Item {
                    implicitWidth: 254 * screenScaleFactor
                    implicitHeight: 58 * screenScaleFactor

                    Rectangle{
                        anchors.fill: parent
                        color: Constants.currentTheme === 0 ? "#000000" : "#ffffff"
                        opacity: Constants.currentTheme === 0 ? 0.1 : 1
                        radius: 5
                    }

                    CusButton {
                        id: idSliceBtn
                        width: 234 * screenScaleFactor
                        height: 36 * screenScaleFactor
                        anchors.centerIn: parent

                        normalColor: enabled ? Constants.textNormalColor : Constants.textDisableColor
                        hoveredColor: Constants.btnHoverColor
                        pressedColor: Constants.btnCheckedColor

                        btnBg.radius: 5
                        enabled: isEnabled

                        bText.fontColor : enabled ? Constants.textNormalColor : Constants.textDisableColor
                        bText.fontPointSize: Constants.labelFontPointSize_12
                        bText.fontFamily: Constants.panelFontFamily
                        bText.text:qsTr("Device")

                        onClicked: {
                            switch(model.index)
                            {
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
                    }
                }
            }
        }

        //        CusImglButton {
        //            id: idModelRecommendButton
        //            width: 50 * screenScaleFactor
        //            height: 50 * screenScaleFactor
        //            imgWidth: 24 * screenScaleFactor
        //            imgHeight: 24 * screenScaleFactor
        //            visible: kernel_global_const.cxcloudEnabled && kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType == 0

        //            anchors.top: topToolBar.bottom
        //            anchors.topMargin: 10 * screenScaleFactor
        //            anchors.horizontalCenter: leftToolBar.horizontalCenter

        //            text: qsTr("Models")
        //            cusTooltip.position: BasicTooltip.Position.RIGHT
        //            shadowEnabled: Constants.currentTheme ? true : false
        //            bottonSelected: idModelRecommendDialog.visible

        //            borderColor: Constants.leftbar_btn_border_color
        //            hoverBorderColor: Constants.leftbar_btn_border_color
        //            selectBorderColor: Constants.leftbar_btn_border_color
        //            enabledIconSource: Constants.leftbar_rocommand_btn_icon_default
        //            pressedIconSource: Constants.leftbar_rocommand_btn_icon_pressed
        //            highLightIconSource: Constants.leftbar_rocommand_btn_icon_hovered

        //            onClicked: {
        //                if(!kernel_global_const.cxcloudEnabled) return
        //                if(!idModelRecommendDialog.visible && !idModelRecommendDialog.visibleFlag)
        //                {
        //                    if(idModelLibraryInfoDialog.visible)
        //                        idModelLibraryInfoDialog.refreshData()
        //                    else
        //                        actionCommands.getOpt("Models").execute()
        //                }
        //                else {
        //                    idModelRecommendDialog.hide()
        //                }
        //            }
        //        }

        //        CusImglButton {
        //            id: openFileBtn
        //            width: 50 * screenScaleFactor
        //            height: 50 * screenScaleFactor
        //            imgWidth: 24 * screenScaleFactor
        //            imgHeight: 24 * screenScaleFactor
        //            visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType == 0

        //            anchors.top: idModelRecommendButton.visible ? idModelRecommendButton.bottom : topToolBar.bottom
        //            anchors.topMargin: idModelRecommendButton.visible ? 8 * screenScaleFactor : 10 * screenScaleFactor
        //            anchors.horizontalCenter: leftToolBar.horizontalCenter

        //            text: qsTr("Open File")
        //            cusTooltip.position: BasicTooltip.Position.RIGHT
        //            shadowEnabled: Constants.currentTheme ? true : false

        //            borderColor: Constants.leftbar_btn_border_color
        //            hoverBorderColor: Constants.leftbar_btn_border_color
        //            selectBorderColor: Constants.leftbar_btn_border_color
        //            enabledIconSource: Constants.leftbar_open_btn_icon_default
        //            pressedIconSource: Constants.leftbar_open_btn_icon_pressed
        //            highLightIconSource: Constants.leftbar_open_btn_icon_hovered

        //            onClicked: {
        //                kernel_kernel.openFile()
        //            }
        //        }


        //        LeftToolBar {
        //            id: leftToolBar
        //            objectName: "leftToolObj"
        //            property bool enabledState: true

        //            topLeftButton: kernel_global_const.cxcloudEnabled ? idModelRecommendButton : openFileBtn

        //            enabled: kernel_model_selector.selectedNum > 0
        //            visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType === 0

        //            anchors.top: openFileBtn.bottom
        //            anchors.topMargin: 8 * screenScaleFactor
        //            anchors.left: parent.left
        //            anchors.leftMargin: 10* screenScaleFactor
        //        }

        //        ModelListCombo {
        //            id:mlcBtn
        //            anchors.left: parent.left
        //            anchors.leftMargin: 10 * screenScaleFactor
        //            anchors.bottom: parent.bottom
        //            anchors.bottomMargin: 10 * screenScaleFactor

        //            width: 50 * screenScaleFactor
        //            height: 50 * screenScaleFactor
        //            visible: kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType == 0
        //        }
    }

    Item{
        id:idRightClickMenu
        objectName: "idRightClickMenu"
        property var rightMenu
        function requstShow()
        {
            if (rightMenu)
            {
                rightMenu.destroy()
            }
            var component = Qt.createComponent("qrc:/CrealityUI/qml/BasicRClickMenu.qml")
            if (component.status === Component.Ready)
            {
                rightMenu = component.createObject(standaloneWindow, {
                                                       menuItems: kernel_rclick_menu_data.getCommandsList(),
                                                       nozzleObj: kernel_rclick_menu_data.getNozzleModel(),
                                                       uploadObj: kernel_rclick_menu_data.getUploadModel(),
                                                   });
                rightMenu.popup()
            }
        }
    }

    function showManagePrinterDialog() {
        idManagerPrinterWizard.visible = true
    }

    function showManageMaterialDialog() {
        //        idManagerPrinterWizard.visible = true
        idSlicePanel.showEditMaterialDlg()
    }
}
