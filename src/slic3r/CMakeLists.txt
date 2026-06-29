cmake_minimum_required(VERSION 3.13)
project(libslic3r_gui)

include(PrecompiledHeader)

set(SLIC3R_GUI_SOURCES
    pchheader.cpp
    pchheader.hpp
    GUI/Printer/PrinterFileSystem.cpp
    GUI/Printer/PrinterFileSystem.h
    GUI/Widgets/AxisCtrlButton.cpp
    GUI/Widgets/AxisCtrlButton.hpp
    GUI/Widgets/Button.cpp
    GUI/Widgets/Button.hpp
    GUI/Widgets/CheckBox.cpp
    GUI/Widgets/CheckBox.hpp
    GUI/Widgets/RadioBox.hpp
    GUI/Widgets/RadioBox.cpp
    GUI/Widgets/ProgressDialog.hpp
    GUI/Widgets/ProgressDialog.cpp
    GUI/Widgets/RoundedRectangle.hpp
    GUI/Widgets/RoundedRectangle.cpp
    GUI/Widgets/ComboBox.cpp
    GUI/Widgets/ComboBox.hpp
    GUI/Widgets/SideButton.hpp
    GUI/Widgets/SideButton.cpp
    GUI/Widgets/SideMenuPopup.hpp
    GUI/Widgets/SideMenuPopup.cpp
    GUI/Widgets/DropDown.cpp
    GUI/Widgets/DropDown.hpp
    GUI/Widgets/PopupWindow.cpp
    GUI/Widgets/PopupWindow.hpp
    GUI/Widgets/Label.cpp
    GUI/Widgets/Label.hpp
    GUI/Widgets/Scrollbar.cpp
    GUI/Widgets/Scrollbar.hpp
    GUI/Widgets/ScrolledWindow.cpp
    GUI/Widgets/ScrolledWindow.hpp
    GUI/Widgets/StaticBox.cpp
    GUI/Widgets/StaticBox.hpp
    GUI/Widgets/ImageSwitchButton.cpp
    GUI/Widgets/ImageSwitchButton.hpp
    GUI/Widgets/SwitchButton.cpp
    GUI/Widgets/SwitchButton.hpp
    GUI/Widgets/SpinInput.cpp
    GUI/Widgets/SpinInput.hpp
    GUI/Widgets/StaticLine.cpp
    GUI/Widgets/StaticLine.hpp
    GUI/Widgets/StateColor.cpp
    GUI/Widgets/StateColor.hpp
    GUI/Widgets/StateHandler.cpp
    GUI/Widgets/StateHandler.hpp
    GUI/Widgets/TabCtrl.cpp
    GUI/Widgets/TabCtrl.hpp
    GUI/Widgets/TextInput.cpp
    GUI/Widgets/TextInput.hpp
    GUI/Widgets/TextInputCtrl.cpp
    GUI/Widgets/TextInputCtrl.hpp
    GUI/Widgets/TempInput.cpp
    GUI/Widgets/TempInput.hpp
    GUI/Widgets/AMSControl.cpp
    GUI/Widgets/AMSControl.hpp
    GUI/Widgets/FanControl.cpp
    GUI/Widgets/FanControl.hpp
    GUI/Widgets/Scrollbar.cpp
    GUI/Widgets/Scrollbar.hpp
    GUI/Widgets/ScrolledWindow.cpp
    GUI/Widgets/ScrolledWindow.hpp
    GUI/Widgets/StepCtrl.cpp
    GUI/Widgets/StepCtrl.hpp
    GUI/Widgets/ProgressBar.cpp
    GUI/Widgets/ProgressBar.hpp
    GUI/Widgets/SideTools.cpp
    GUI/Widgets/SideTools.hpp
    GUI/Widgets/WebView.cpp
    GUI/Widgets/WebView.hpp
    GUI/Widgets/ErrorMsgStaticText.cpp
    GUI/Widgets/ErrorMsgStaticText.hpp
    GUI/MixedFilamentDialog.cpp
    GUI/MixedFilamentDialog.hpp
    GUI/Widgets/TextDisplay.hpp
    GUI/Widgets/TextDisplay.cpp
    GUI/Widgets/HoverBorderIcon.hpp
    GUI/Widgets/HoverBorderIcon.cpp
    GUI/Widgets/HoverBorderBox.hpp
    GUI/Widgets/HoverBorderBox.cpp
    GUI/Widgets/NumberEntryDialog.hpp
    GUI/Widgets/NumberEntryDialog.cpp
    GUI/Widgets/ColorSlider.hpp
    GUI/Widgets/ColorSlider.cpp
    GUI/AboutDialog.cpp
    GUI/AboutDialog.hpp
    GUI/NetworkTestDialog.cpp
    GUI/NetworkTestDialog.hpp
    GUI/AuxiliaryDialog.cpp
    GUI/AuxiliaryDialog.hpp
    GUI/Auxiliary.cpp
    GUI/Auxiliary.hpp
    GUI/DailyTips.cpp
    GUI/DailyTips.hpp
    GUI/Project.cpp
    GUI/Project.hpp
    GUI/BackgroundSlicingProcess.cpp
    GUI/BackgroundSlicingProcess.hpp
    GUI/BitmapCache.cpp
    GUI/BitmapCache.hpp
    GUI/ImageGrid.cpp
    GUI/ImageGrid.h
    GUI/3DScene.cpp
    GUI/3DScene.hpp
    GUI/format.hpp
    GUI/GLShadersManager.hpp
    GUI/GLShadersManager.cpp
    GUI/GLShader.cpp
    GUI/GLShader.hpp
    GUI/GLCanvas3D.hpp
    GUI/GLCanvas3D.cpp
    GUI/GLEnums.hpp
    GUI/SceneRaycaster.hpp
    GUI/SceneRaycaster.cpp
    GUI/OpenGLManager.hpp
    GUI/OpenGLManager.cpp
    GUI/Selection.hpp
    GUI/Selection.cpp
    GUI/SlicingProgressNotification.cpp
    GUI/SlicingProgressNotification.hpp
    GUI/Gizmos/GLGizmosManager.cpp
    GUI/Gizmos/GLGizmosManager.hpp
    GUI/Gizmos/GLGizmosCommon.cpp
    GUI/Gizmos/GLGizmosCommon.hpp
    GUI/Gizmos/GLGizmoBase.cpp
    GUI/Gizmos/GLGizmoBase.hpp
    GUI/Gizmos/GLGizmoEmboss.cpp
    GUI/Gizmos/GLGizmoEmboss.hpp
    GUI/Gizmos/GLGizmoMove.cpp
    GUI/Gizmos/GLGizmoMove.hpp
    GUI/Gizmos/GLGizmoRotate.cpp
    GUI/Gizmos/GLGizmoRotate.hpp
    GUI/Gizmos/GLGizmoScale.cpp
    GUI/Gizmos/GLGizmoScale.hpp
    #GUI/Gizmos/GLGizmoSlaSupports.cpp
    #GUI/Gizmos/GLGizmoSlaSupports.hpp
    GUI/Gizmos/GLGizmoFdmSupports.cpp
    GUI/Gizmos/GLGizmoFdmSupports.hpp
    GUI/Gizmos/GLGizmoFlatten.cpp
    GUI/Gizmos/GLGizmoFlatten.hpp
    GUI/Gizmos/GLGizmoFuzzySkin.cpp
    GUI/Gizmos/GLGizmoFuzzySkin.hpp
    GUI/Gizmos/GLGizmoCut.cpp
    GUI/Gizmos/GLGizmoCut.hpp
    GUI/Gizmos/GLGizmoHollow.cpp
    GUI/Gizmos/GLGizmoHollow.hpp
    GUI/Gizmos/GLGizmoPainterBase.cpp
    GUI/Gizmos/GLGizmoPainterBase.hpp
    GUI/Gizmos/GLGizmoSimplify.cpp
    GUI/Gizmos/GLGizmoSimplify.hpp
    GUI/Gizmos/GLGizmoSVG.cpp
    GUI/Gizmos/GLGizmoSVG.hpp
    GUI/Gizmos/GLGizmoMmuSegmentation.cpp
    GUI/Gizmos/GLGizmoMmuSegmentation.hpp
    GUI/Gizmos/GLGizmoSimplePaint.cpp
    GUI/Gizmos/GLGizmoSimplePaint.hpp
    GUI/Gizmos/GLGizmoClone.hpp
    GUI/Gizmos/GLGizmoClone.cpp
    #GUI/Gizmos/GLGizmoFaceDetector.cpp
    #GUI/Gizmos/GLGizmoFaceDetector.hpp
	GUI/Gizmos/GLGizmoMeasure.cpp
    GUI/Gizmos/GLGizmoMeasure.hpp
    GUI/Gizmos/GLGizmoBrimEars.cpp
    GUI/Gizmos/GLGizmoBrimEars.hpp
    GUI/Gizmos/GLGizmoSeam.cpp
    GUI/Gizmos/GLGizmoSeam.hpp
    #GUI/Gizmos/GLGizmoText.cpp
    #GUI/Gizmos/GLGizmoText.hpp
    GUI/Gizmos/GLGizmoMeshBoolean.cpp
    GUI/Gizmos/GLGizmoMeshBoolean.hpp
    GUI/UIHelpers/MeshBooleanUI.cpp
    GUI/UIHelpers/MeshBooleanUI.hpp
    GUI/UIHelpers/TextEllipsis.cpp
    GUI/UIHelpers/TextEllipsis.hpp
    GUI/GLSelectionRectangle.cpp
    GUI/GLSelectionRectangle.hpp
    GUI/Gizmos/GizmoObjectManipulation.cpp
    GUI/Gizmos/GizmoObjectManipulation.hpp
    GUI/Gizmos/GLGizmoDrill.cpp
    GUI/Gizmos/GLGizmoDrill.hpp
    GUI/GLModel.hpp
    GUI/GLModel.cpp
    GUI/MeshHollow.hpp
    GUI/MeshHollow.cpp
    GUI/GLTexture.hpp
    GUI/GLTexture.cpp
    GUI/GLToolbar.hpp
    GUI/GLToolbar.cpp
    GUI/IMToolbar.hpp
    GUI/IMToolbar.cpp
    GUI/HybridIndexBuffer.hpp
    GUI/HybridIndexBuffer.cpp
    GUI/DiskVectorImp.hpp
    GUI/DiskVectorImp.cpp
    GUI/HybridVertexBuffer.hpp
    GUI/HybridVertexBuffer.cpp
    GUI/GCodeViewer.hpp
    GUI/GCodeViewer.cpp
    GUI/Preferences.cpp
    GUI/Preferences.hpp
    GUI/ConfigRelateDialog.cpp
    GUI/ConfigRelateDialog.hpp
    GUI/AMSSetting.cpp
    GUI/AMSSetting.hpp
    GUI/AMSMaterialsSetting.cpp
    GUI/AMSMaterialsSetting.hpp
    GUI/ExtrusionCalibration.cpp
    GUI/ExtrusionCalibration.hpp
    GUI/PresetHints.cpp
    GUI/PresetHints.hpp
    GUI/GUI.cpp
    GUI/GUI.hpp
    GUI/GUI_Init.cpp
    GUI/GUI_Init.hpp
    GUI/GUI_Preview.cpp
    GUI/GUI_Preview.hpp
    GUI/GUI_App_Parse.cpp
    GUI/GUI_App.cpp
    GUI/SupportMeshCreator.hpp
    GUI/SupportMeshCreator.cpp
    GUI/AppUpdater.cpp
    GUI/AppUpdater.hpp
    GUI/AppUpdateDialogs.cpp
    GUI/AppUpdateDialogs.hpp
    GUI/FileDownloader.hpp
    GUI/FileDownloader.cpp
    GUI/FileUploader.hpp
    GUI/FileUploader.cpp
    GUI/GUI_App.hpp
    GUI/GUI_Utils.cpp
    GUI/GUI_Utils.hpp
    GUI/GUI_Geometry.cpp
    GUI/GUI_Geometry.hpp
    GUI/I18N.cpp
    GUI/I18N.hpp
    GUI/IconManager.cpp
    GUI/IconManager.hpp
    GUI/MainFrame.cpp
    GUI/MainFrame.hpp
    GUI/BBLTopbar.cpp
    GUI/BBLTopbar.hpp
    GUI/Plater.cpp
    GUI/Plater.hpp
    GUI/PartPlate.cpp
    GUI/PartPlate.hpp
    GUI/UserNotification.cpp
    GUI/UserNotification.hpp
    GUI/PresetComboBoxes.hpp
    GUI/PresetComboBoxes.cpp
    GUI/BitmapComboBox.hpp
    GUI/BitmapComboBox.cpp
    GUI/EditGCodeDialog.hpp
    GUI/EditGCodeDialog.cpp
    GUI/SavePresetDialog.hpp
    GUI/SavePresetDialog.cpp
    GUI/GUI_Colors.hpp
    GUI/GUI_Colors.cpp
    GUI/PhysicalPrinterDialog.hpp
    GUI/PhysicalPrinterDialog.cpp
    GUI/PrintHostDialogs.cpp
    GUI/PrintHostDialogs.hpp
    GUI/GUI_Factories.cpp
    GUI/GUI_Factories.hpp
    GUI/GUI_ObjectList.cpp
    GUI/GUI_ObjectList.hpp
    GUI/GUI_ObjectLayers.cpp
    GUI/GUI_ObjectLayers.hpp
    GUI/GUI_AuxiliaryList.cpp
    GUI/GUI_AuxiliaryList.hpp
    GUI/GUI_ObjectSettings.cpp
    GUI/GUI_ObjectSettings.hpp
    GUI/GUI_ObjectTable.cpp
    GUI/GUI_ObjectTable.hpp
    GUI/GUI_ObjectTableSettings.cpp
    GUI/GUI_ObjectTableSettings.hpp
    GUI/MeshUtils.cpp
    GUI/MeshUtils.hpp
    GUI/TickCode.cpp
    GUI/TickCode.hpp
    GUI/Tab.cpp
    GUI/Tab.hpp
    GUI/ParamsDialog.cpp
    GUI/ParamsDialog.hpp
	GUI/UITour.cpp
    GUI/UITour.hpp
    GUI/ParamsPanel.cpp
    GUI/ParamsPanel.hpp
    GUI/PrintHostDialogs.cpp
    GUI/PrintHostDialogs.hpp
    GUI/AmsWidgets.cpp
    GUI/AmsWidgets.hpp
    GUI/MediaFilePanel.cpp
    GUI/MediaFilePanel.h
    GUI/MediaPlayCtrl.cpp
    GUI/MediaPlayCtrl.h
    GUI/MonitorBasePanel.cpp
    GUI/MonitorBasePanel.h
    GUI/UpgradePanel.cpp
    GUI/UpgradePanel.hpp
    GUI/HintNotification.hpp
    GUI/HintNotification.cpp
    GUI/HMSPanel.hpp
    GUI/HMSPanel.cpp
    GUI/MonitorPage.cpp
    GUI/MonitorPage.hpp
    GUI/StatusPanel.cpp
    GUI/StatusPanel.hpp
    GUI/HMS.hpp
    GUI/HMS.cpp
    GUI/SliceInfoPanel.cpp
    GUI/SliceInfoPanel.hpp
    GUI/CameraPopup.cpp
    GUI/CameraPopup.hpp
    GUI/Monitor.cpp
    GUI/Monitor.hpp
    GUI/WebViewDialog.cpp
    GUI/WebViewDialog.hpp
    GUI/LoginDialog.cpp
    GUI/LoginDialog.hpp
    GUI/PrinterWebView.cpp
    GUI/PrinterWebView.hpp
    GUI/print_manage/PrinterMgr.cpp
    GUI/print_manage/PrinterMgr.hpp
    GUI/print_manage/App/PrinterMgrView.cpp
    GUI/print_manage/App/PrinterMgrView.hpp
    GUI/print_manage/App/mqtt_client.h
    GUI/print_manage/App/mqtt_client.cpp
    GUI/print_manage/Device/LanPrinterInterface.hpp
    GUI/print_manage/Device/LanPrinterInterface.cpp
    GUI/print_manage/Device/Klipper4408Interface.hpp
    GUI/print_manage/Device/Klipper4408Interface.cpp
    GUI/print_manage/Device/KlipperCXInterface.hpp
    GUI/print_manage/Device/KlipperCXInterface.cpp
    GUI/print_manage/Device/KlipperInterface.hpp
    GUI/print_manage/Device/KlipperInterface.cpp
    GUI/print_manage/Device/OctoPrintInterface.hpp
    GUI/print_manage/Device/OctoPrintInterface.cpp
    GUI/print_manage/RemotePrinterManager.hpp
    GUI/print_manage/RemotePrinterManager.cpp
    GUI/print_manage/utils/mdns.h
    GUI/print_manage/utils/mdns.c
    GUI/print_manage/utils/cxmdns.h
    GUI/print_manage/utils/cxmdns.cpp
    GUI/print_manage/data/DataType.hpp
    GUI/print_manage/data/DataType.cpp
    GUI/print_manage/data/DataCenter.hpp
    GUI/print_manage/data/DataCenter.cpp
    GUI/print_manage/AppMgr.hpp
    GUI/print_manage/AppMgr.cpp
    GUI/print_manage/Routes/Routes.hpp
    GUI/print_manage/Routes/Routes.cpp
    GUI/print_manage/Routes/AppRoutes.hpp
    GUI/print_manage/Routes/AppRoutes.cpp
    GUI/print_manage/Routes/SysRoutes.hpp
    GUI/print_manage/Routes/SysRoutes.cpp
    GUI/print_manage/Routes/SenderRoutes.hpp
    GUI/print_manage/Routes/SenderRoutes.cpp
    GUI/print_manage/Routes/DeviceMgrRoutes.hpp
    GUI/print_manage/Routes/DeviceMgrRoutes.cpp
    GUI/print_manage/Routes/ModelLibraryRoutes.hpp
    GUI/print_manage/Routes/ModelLibraryRoutes.cpp
    GUI/print_manage/TypeDefine.hpp
    GUI/print_manage/AppUtils.hpp
    GUI/print_manage/AppUtils.cpp
    GUI/print_manage/Upload3mfToCloud.hpp
    GUI/print_manage/Upload3mfToCloud.cpp
    GUI/print_manage/UploadGcodeToCloud.hpp
    GUI/print_manage/UploadGcodeToCloud.cpp
    GUI/print_manage/Utils.hpp
    GUI/print_manage/Utils.cpp
    GUI/print_manage/MaterialMapPanel.hpp
    GUI/print_manage/MaterialMapPanel.cpp
    GUI/print_manage/App/SendToPrinter.hpp
    GUI/print_manage/App/SendToPrinter.cpp
    GUI/print_manage/AccountDeviceMgr.hpp
    GUI/print_manage/AccountDeviceMgr.cpp
    GUI/print_manage/PrinterBoxFilamentPanel.hpp
    GUI/print_manage/PrinterBoxFilamentPanel.cpp
    GUI/WebDownPluginDlg.hpp
    GUI/WebDownPluginDlg.cpp
    GUI/WebGuideDialog.hpp
    GUI/WebGuideDialog.cpp
    GUI/WebModelLibraryView.hpp
    GUI/WebModelLibraryView.cpp
    GUI/ModelDetailDialog.hpp
    GUI/ModelDetailDialog.cpp
    GUI/WebUserLoginDialog.cpp
    GUI/WebUserLoginDialog.hpp
    GUI/ConfigManipulation.cpp
    GUI/ConfigManipulation.hpp
    GUI/Field.cpp
    GUI/Field.hpp
    GUI/FileArchiveDialog.cpp
    GUI/FileArchiveDialog.hpp
    GUI/OptionsGroup.cpp
    GUI/OptionsGroup.hpp
    GUI/OG_CustomCtrl.cpp
    GUI/OG_CustomCtrl.hpp
    GUI/MarkdownTip.cpp
    GUI/MarkdownTip.hpp
    GUI/2DBed.cpp
    GUI/2DBed.hpp
    GUI/3DBed.cpp
    GUI/3DBed.hpp
    GUI/CrealityBedModelMapping.hpp
    GUI/CrealityBedModelMapping.cpp
    GUI/Camera.cpp
    GUI/Camera.hpp
    GUI/CameraUtils.cpp
    GUI/CameraUtils.hpp
    GUI/wxExtensions.cpp
    GUI/wxExtensions.hpp
    GUI/ObjColorDialog.cpp
    GUI/ObjColorDialog.hpp
    GUI/WipeTowerDialog.cpp
    GUI/RammingChart.cpp
    GUI/RammingChart.hpp
    GUI/RemovableDriveManager.cpp
    GUI/RemovableDriveManager.hpp
    GUI/SendSystemInfoDialog.cpp
    GUI/SendSystemInfoDialog.hpp
    GUI/SurfaceDrag.cpp
    GUI/SurfaceDrag.hpp
    GUI/TextLines.cpp
    GUI/TextLines.hpp
    GUI/TransmittanceDialog.hpp
    GUI/TransmittanceDialog.cpp
    GUI/PlateSettingsDialog.cpp
    GUI/PlateSettingsDialog.hpp
    GUI/ImGuiWrapper.hpp
    GUI/ImGuiWrapper.cpp
    GUI/DeviceManager.hpp
    GUI/DeviceManager.cpp
    GUI/UserManager.hpp
    GUI/UserManager.cpp
    GUI/HttpServer.hpp
    GUI/HttpServer.cpp
    Config/Snapshot.cpp
    Config/Snapshot.hpp
    Config/Version.cpp
    Config/Version.hpp
    Config/DispConfig.cpp
    Config/DispConfig.h
    Utils/ASCIIFolding.cpp
    Utils/ASCIIFolding.hpp
    GUI/ConfigWizard.cpp
    GUI/ConfigWizard.hpp
    GUI/ConnectPrinter.cpp
    GUI/ConnectPrinter.hpp
    GUI/ConfigWizard_private.hpp
    GUI/MsgDialog.cpp
    GUI/MsgDialog.hpp
    GUI/Downloader.hpp
    GUI/Downloader.cpp
    GUI/ModelDownloader.h
    GUI/ModelDownloader.cpp
    GUI/DownloadService.h
    GUI/DownloadService.cpp
    GUI/DownloaderFileGet.hpp
    GUI/DownloaderFileGet.cpp
    GUI/DownloadProgressDialog.hpp
    GUI/DownloadProgressDialog.cpp
    GUI/CloudDownloadProgressDialog.hpp
    GUI/CloudDownloadProgressDialog.cpp
    GUI/UpdateDialogs.cpp
    GUI/UpdateDialogs.hpp
    GUI/Jobs/Job.hpp
    GUI/Jobs/Worker.hpp
    GUI/Jobs/BoostThreadWorker.hpp
    GUI/Jobs/BoostThreadWorker.cpp
    GUI/Jobs/BusyCursorJob.hpp
    GUI/Jobs/PlaterWorker.hpp
    GUI/Jobs/UpgradeNetworkJob.hpp
    GUI/Jobs/UpgradeNetworkJob.cpp
    GUI/Jobs/ArrangeJob.hpp
    GUI/Jobs/ArrangeJob.cpp
    GUI/Jobs/CreateFontNameImageJob.cpp
    GUI/Jobs/CreateFontNameImageJob.hpp
    GUI/Jobs/CreateFontStyleImagesJob.cpp
    GUI/Jobs/CreateFontStyleImagesJob.hpp
    GUI/Jobs/EmbossJob.cpp
    GUI/Jobs/EmbossJob.hpp
    GUI/Jobs/OrientJob.hpp
    GUI/Jobs/OrientJob.cpp
    GUI/Jobs/RotoptimizeJob.hpp
    GUI/Jobs/RotoptimizeJob.cpp
    GUI/Jobs/FillBedJob.hpp
    GUI/Jobs/FillBedJob.cpp
    GUI/Jobs/SLAImportJob.hpp
    GUI/Jobs/SLAImportJob.cpp
    GUI/Jobs/ProgressIndicator.hpp
    GUI/Jobs/PrintJob.hpp
    GUI/Jobs/PrintJob.cpp
    GUI/Jobs/SendJob.hpp
    GUI/Jobs/SendJob.cpp
    GUI/Jobs/BindJob.hpp
    GUI/Jobs/BindJob.cpp
    GUI/Jobs/NotificationProgressIndicator.hpp
    GUI/Jobs/NotificationProgressIndicator.cpp
    GUI/Jobs/BooleanOperationJob.hpp
    GUI/Jobs/BooleanOperationJob.cpp
    GUI/Jobs/ThreadSafeQueue.hpp
    GUI/Jobs/SLAImportDialog.hpp
    GUI/PhysicalPrinterDialog.hpp
    GUI/PhysicalPrinterDialog.cpp
    GUI/ProgressStatusBar.hpp
    GUI/ProgressStatusBar.cpp
    GUI/BBLStatusBar.hpp
    GUI/BBLStatusBar.cpp
    GUI/BBLStatusBarSend.hpp
    GUI/BBLStatusBarSend.cpp
    GUI/BBLStatusBarBind.hpp
    GUI/BBLStatusBarBind.cpp
    GUI/Mouse3DController.cpp
    GUI/Mouse3DController.hpp
    GUI/IMSlider.cpp
    GUI/IMSlider.hpp
    GUI/Notebook.cpp
    GUI/Notebook.hpp
    GUI/TabButton.cpp
    GUI/TabButton.hpp
    GUI/Tabbook.cpp
    GUI/Tabbook.hpp
    GUI/ObjectDataViewModel.cpp
    GUI/ObjectDataViewModel.hpp
    GUI/AuxiliaryDataViewModel.cpp
    GUI/AuxiliaryDataViewModel.hpp
    GUI/InstanceCheck.cpp
    GUI/InstanceCheck.hpp
    GUI/Search.cpp
    GUI/Search.hpp
    GUI/NotificationManager.cpp
    GUI/NotificationManager.hpp
    GUI/UnsavedChangesDialog.cpp
    GUI/UnsavedChangesDialog.hpp
    GUI/ExtraRenderers.cpp
    GUI/ExtraRenderers.hpp
    GUI/ProjectDirtyStateManager.hpp
    GUI/ProjectDirtyStateManager.cpp
    GUI/DesktopIntegrationDialog.cpp
    GUI/DesktopIntegrationDialog.hpp
    GUI/DragCanvas.cpp
    GUI/DragCanvas.hpp
    GUI/PublishDialog.cpp
    GUI/PublishDialog.hpp
    GUI/RecenterDialog.cpp
    GUI/RecenterDialog.hpp
    GUI/PrivacyUpdateDialog.cpp
    GUI/PrivacyUpdateDialog.hpp
    GUI/EnableLiteModeDialog.cpp
    GUI/EnableLiteModeDialog.hpp
    GUI/BonjourDialog.cpp
    GUI/BonjourDialog.hpp
    GUI/BindDialog.cpp
    GUI/BindDialog.hpp
    GUI/ModelMall.hpp
    GUI/ModelMall.cpp
    GUI/SelectMachine.hpp
    GUI/SelectMachine.cpp
    GUI/SendToPrinter.hpp
    GUI/SendToPrinter.cpp
    GUI/AmsMappingPopup.hpp
    GUI/AmsMappingPopup.cpp
    GUI/ReleaseNote.hpp
    GUI/ReleaseNote.cpp
    GUI/SingleChoiceDialog.hpp
    GUI/SingleChoiceDialog.cpp
    GUI/CaliHistoryDialog.hpp
    GUI/CaliHistoryDialog.cpp
    GUI/CalibrationPanel.hpp
    GUI/CalibrationPanel.cpp
    GUI/CalibrationWizard.hpp
    GUI/CalibrationWizard.cpp
    GUI/CalibrationWizardPage.cpp
    GUI/CalibrationWizardPage.hpp
    GUI/CalibrationWizardStartPage.cpp
    GUI/CalibrationWizardStartPage.hpp
    GUI/CalibrationWizardPresetPage.cpp
    GUI/CalibrationWizardPresetPage.hpp
    GUI/CalibrationWizardCaliPage.cpp
    GUI/CalibrationWizardCaliPage.hpp
    GUI/CalibrationWizardSavePage.cpp
    GUI/CalibrationWizardSavePage.hpp
    GUI/Calibration.hpp
    GUI/Calibration.cpp
    GUI/PrintOptionsDialog.hpp
    GUI/PrintOptionsDialog.cpp
    GUI/BonjourDialog.hpp
    GUI/BonjourDialog.cpp
    GUI/BedShapeDialog.hpp
    GUI/BedShapeDialog.cpp
    GUI/CreatePresetsDialog.hpp
    GUI/CreatePresetsDialog.cpp
    Utils/json_diff.hpp
    Utils/json_diff.cpp
    GUI/KBShortcutsDialog.hpp
    GUI/KBShortcutsDialog.cpp
    GUI/MultiMachine.hpp
    GUI/MultiMachine.cpp
    GUI/MultiMachinePage.hpp
    GUI/MultiMachinePage.cpp
    GUI/MultiMachineManagerPage.cpp
    GUI/MultiMachineManagerPage.hpp
    GUI/MultiPrintJob.cpp
    GUI/MultiPrintJob.hpp
    GUI/MultiSendMachineModel.hpp
    GUI/MultiSendMachineModel.cpp
    GUI/MultiTaskManagerPage.hpp
    GUI/MultiTaskManagerPage.cpp
    GUI/MultiTaskModel.hpp
    GUI/MultiTaskModel.cpp
    GUI/SendMultiMachinePage.hpp
    GUI/SendMultiMachinePage.cpp
    GUI/TaskManager.cpp
    GUI/TaskManager.hpp
    GUI/RenderEnums.hpp
    GUI/GCodeRenderer/AdvancedRenderer.hpp
    GUI/GCodeRenderer/AdvancedRenderer.cpp
    GUI/GCodeRenderer/BaseRenderer.hpp
    GUI/GCodeRenderer/BaseRenderer.cpp
    GUI/GCodeRenderer/LegacyRenderer.hpp
    GUI/GCodeRenderer/LegacyRenderer.cpp
    Utils/Http.cpp
    Utils/Http.hpp
    Utils/FixModelByWin10.cpp
    Utils/FixModelByImatiSTL.cpp
    Utils/FixModelByWin10.hpp
    Utils/EmbossStyleManager.cpp
    Utils/EmbossStyleManager.hpp
    Utils/FontConfigHelp.cpp
    Utils/FontConfigHelp.hpp
    Utils/Bonjour.cpp
    Utils/Bonjour.hpp
    Utils/FileHelp.cpp
    Utils/FileHelp.hpp
    Utils/PresetUpdater.cpp
    Utils/PresetUpdater.hpp
    Utils/Process.cpp
    Utils/Process.hpp
    Utils/RaycastManager.cpp
    Utils/RaycastManager.hpp
    Utils/Profile.hpp
    Utils/UndoRedo.cpp
    Utils/UndoRedo.hpp
    Utils/HexFile.cpp
    Utils/HexFile.hpp
    Utils/TCPConsole.cpp
    Utils/TCPConsole.hpp
    Utils/minilzo_extension.hpp
    Utils/minilzo_extension.cpp
    Utils/ColorSpaceConvert.hpp
    Utils/ColorSpaceConvert.cpp
    Utils/OctoPrint.hpp
    Utils/OctoPrint.cpp
    Utils/PrintHost.hpp
    Utils/PrintHost.cpp
    Utils/NetworkAgent.cpp
    Utils/NetworkAgent.hpp
    Utils/OctoPrint.cpp
    Utils/OctoPrint.hpp
    Utils/PrintHost.cpp
    Utils/PrintHost.hpp
    Utils/Serial.cpp
    Utils/Serial.hpp
    Utils/SerialMessageType.hpp
    Utils/SerialMessage.hpp
    Utils/MKS.cpp
    Utils/MKS.hpp
    Utils/ESP3D.cpp
    Utils/ESP3D.hpp
    Utils/WxFontUtils.cpp
    Utils/WxFontUtils.hpp
    Utils/Duet.cpp
    Utils/Duet.hpp
    Utils/FlashAir.cpp
    Utils/FlashAir.hpp
    Utils/AstroBox.cpp
    Utils/AstroBox.hpp
    Utils/Repetier.cpp
    Utils/Repetier.hpp
    GUI/calib_dlg.hpp
    GUI/calib_dlg.cpp
    Utils/CalibUtils.cpp
    Utils/CalibUtils.hpp
    Utils/ProfileDescription.hpp
    GUI/PrinterCloudAuthDialog.cpp
    GUI/PrinterCloudAuthDialog.hpp
    Utils/Obico.cpp
    Utils/Obico.hpp
    Utils/Flashforge.cpp
    Utils/Flashforge.hpp
    Utils/ExportMetas.hpp
    Utils/ExportMetas.cpp
    GUI/OAuthDialog.cpp
    GUI/OAuthDialog.hpp
    GUI/Jobs/OAuthJob.cpp
    GUI/Jobs/OAuthJob.hpp
    Utils/SimplyPrint.cpp
    Utils/SimplyPrint.hpp
	Utils/FlushVolume.cpp
	Utils/FlushVolume.hpp
    GUI/FilamentPanel.h
    GUI/FilamentPanel.cpp
    GUI/SiderBar.h
    GUI/SiderBar.cpp
    GUI/GLToolbarCollapse.h
    GUI/GLToolbarCollapse.cpp
    GUI/GLToolbarProcess.h
    GUI/GLToolbarProcess.cpp
	GUI/UploadFile.hpp
	GUI/UploadFile.cpp
	GUI/LoadOldPresets.hpp
	GUI/LoadOldPresets.cpp
	GUI/LoadOldPresetsFromFolder.hpp
	GUI/LoadOldPresetsFromFolder.cpp
	GUI/SyncUserPresets.hpp
	GUI/SyncUserPresets.cpp
	GUI/CommunicateWithCXCloud.hpp
	GUI/CommunicateWithCXCloud.cpp
	GUI/UpdateParams.hpp
	GUI/UpdateParams.cpp
	GUI/LoginTip.hpp
	GUI/LoginTip.cpp
	GUI/PrinterPresetConfig.hpp
	GUI/PrinterPresetConfig.cpp
	GUI/Check3mfVendor.hpp
	GUI/Check3mfVendor.cpp
	GUI/AccelerationAndSpeedLimitDialog.hpp
	GUI/AccelerationAndSpeedLimitDialog.cpp
    Config/OptionConfig.h
    Config/OptionConfig.cpp
    GUI/AnalyticsDataUploadManager.hpp
    GUI/AnalyticsDataUploadManager.cpp
	GUI/AICloudServiceDialog.hpp
	GUI/AICloudServiceDialog.cpp
    Utils/ProfileFamilyLoader.hpp
    Utils/ProfileFamilyLoader.cpp
    Utils/TestHelper.cpp
    Utils/TestHelper.hpp
    GUI/StepMeshDialog.hpp
    GUI/StepMeshDialog.cpp
    GUI/CrProject.hpp
    GUI/CrProject.cpp
	GUI/PhysicalPrinter.hpp
    GUI/PhysicalPrinter.cpp
    GUI/SystemId/SystemId.hpp
    GUI/SystemId/SystemId.cpp
)

option(GUI_SIMPLE "Enable GUI Simple Mode" ON)
if(GUI_SIMPLE)
    list(APPEND SLIC3R_GUI_SOURCES
        GUI/simple/GLSimpleUtils.hpp
        GUI/simple/GLToolbarSimple.cpp
        GUI/simple/GLCanvas3DSimple.cpp
        GUI/simple/DeviceListSimple.cpp
        GUI/simple/DeviceListSimple.hpp
        GUI/simple/filamentSimplePage.cpp
        GUI/simple/SupportSimple.cpp
        GUI/simple/SupportSimple.hpp
        GUI/simple/filamentMapping/ImGuiFilamentPanel.hpp
        GUI/simple/filamentMapping/ImGuiFilamentPanel.cpp
        GUI/simple/filamentMapping/FilamentMappingService.hpp
        GUI/simple/filamentMapping/FilamentMappingService.cpp
        GUI/simple/filamentMapping/SceneFilamentSourceSnapshotManager.hpp
        GUI/simple/filamentMapping/SceneFilamentSourceSnapshotManager.cpp
        GUI/simple/filamentMapping/ResidentFilamentMappingLogic.hpp
        GUI/simple/filamentMapping/ResidentFilamentMappingLogic.cpp
        GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.hpp
        GUI/simple/filamentMapping/ResidentFilamentMappingAdapter.cpp
        GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.hpp
        GUI/simple/filamentMapping/ResidentFilamentMappingPopupPolicy.cpp
        GUI/simple/filamentMapping/ResidentFilamentMappingView.hpp
        GUI/simple/filamentMapping/ResidentFilamentMappingView.cpp
        GUI/simple/filamentMapping/ImGuiSpoolWidget.hpp
        GUI/simple/filamentMapping/ImGuiSpoolWidget.cpp
        GUI/simple/filamentMapping/ImGuiFilamentExtraPage.cpp
        GUI/simple/filamentMapping/ImGuiThumbnailPreview.hpp
        GUI/simple/filamentMapping/ImGuiThumbnailPreview.cpp
        GUI/simple/filamentMapping/ThumbnailDataRecolor.hpp
        GUI/simple/filamentMapping/ThumbnailDataRecolor.cpp
        GUI/simple/filamentMapping/match_color.hpp
        GUI/simple/filamentMapping/match_color.cpp
        GUI/simple/GLObjectManipulateToolbarSimple.cpp
        GUI/simple/PrintSettingsPanel.hpp
        GUI/simple/PrintSettingsPanel.cpp
        GUI/simple/sendWorkflow/EasyPrintSender.hpp
        GUI/simple/sendWorkflow/EasyPrintSender.cpp
        GUI/simple/UploadDialog.hpp
        GUI/simple/UploadDialog.cpp
        GUI/simple/GUI_ObjectListSimple.cpp
        GUI/simple/SimpleModelMgr.hpp
        GUI/simple/SimpleModelMgr.cpp
        GUI/simple/sendWorkflow/AISendWorkflowService.hpp
        GUI/simple/sendWorkflow/AISendWorkflowService.cpp
        GUI/simple/sendWorkflow/AIProcessPresetIntentResolver.hpp
        GUI/simple/sendWorkflow/AIProcessPresetIntentResolver.cpp
        GUI/simple/sendWorkflow/AIProcessSwitchService.hpp
        GUI/simple/sendWorkflow/AIProcessSwitchService.cpp
        GUI/simple/sendWorkflow/CxCloudPrintClient.hpp
        GUI/simple/sendWorkflow/CxCloudPrintClient.cpp
        GUI/simple/sendWorkflow/CxCloudPrintExecutor.hpp
        GUI/simple/sendWorkflow/CxCloudPrintExecutor.cpp
        GUI/simple/MCPChatPanel.hpp
        GUI/simple/MCPChatPanel.cpp
        GUI/simple/toolcalls/MCPToolCalls.cpp
        GUI/simple/toolcalls/MCPToolCallsRegistration.cpp
        GUI/simple/toolcalls/MCPToolCallsPrinter.cpp
        GUI/simple/toolcalls/MCPToolCallsObject.cpp
        GUI/simple/toolcalls/MCPToolCallsLayout.cpp
        GUI/simple/toolcalls/MCPToolCallsFilament.cpp
        GUI/simple/toolcalls/MCPToolCallsFilamentMapping.cpp
        GUI/simple/toolcalls/MCPToolCallsPrint.cpp
        GUI/simple/toolcalls/MCPToolCallsSceneContext.cpp
        GUI/simple/toolcalls/MCPToolCallsConfig.cpp
        GUI/simple/toolcalls/MCPToolCallsModelSource.cpp
        GUI/simple/toolcalls/MCPChatPanelSAgentNative.cpp
        GUI/simple/toolcalls/MCPToolCallsCommon.hpp
        GUI/simple/toolcalls/MCPToolCallsCommon.cpp
        GUI/simple/bridge/SlicerAction.hpp
        GUI/simple/bridge/CxAgentClientBridge.hpp
        GUI/simple/bridge/CxAgentClientBridge.cpp
        GUI/simple/bridge/SAgentMqttBridge.hpp
        GUI/simple/bridge/SAgentMqttBridge.cpp
        GUI/simple/bridge/SlicerBridge.hpp
        GUI/simple/bridge/SlicerBridge.cpp
        GUI/simple/bridge/SlicerBridgeDiagnostics.hpp
        GUI/simple/bridge/SlicerBridgeDiagnostics.cpp
        GUI/simple/bridge/MCPChatPanelBridgeActionExecutor.cpp
        GUI/simple/bridge/SlicerBridgeActionRegistry.cpp
        GUI/simple/bridge/SlicerBridgeState.cpp
        GUI/simple/bridge/SlicerBridgeActionsConfig.cpp
        GUI/simple/bridge/SlicerBridgeActionsObject.cpp
        GUI/simple/bridge/SlicerBridgeActionsProcess.cpp
        GUI/simple/bridge/SlicerBridgeActionFilament.cpp
        GUI/simple/bridge/SlicerBridgeActionsPrinter.cpp
        GUI/simple/gpu/GpuOrient.hpp
        GUI/simple/gpu/GpuOrient.cpp
        )
endif()

option(AUTO_CONVERT_3MF "Enable auto conversion of 3MF files" OFF)
if(AUTO_CONVERT_3MF)
    list(APPEND SLIC3R_GUI_SOURCES
        GUI/AutoConvert3mfMgr.hpp
        GUI/AutoConvert3mfMgr.cpp
        )
endif()

if (WIN32)
    list(APPEND SLIC3R_GUI_SOURCES
            GUI/dark_mode/dark_mode.hpp
            GUI/dark_mode/IatHook.hpp
            GUI/dark_mode/UAHMenuBar.hpp
            GUI/dark_mode.hpp
            GUI/dark_mode.cpp
        )
endif ()

if (APPLE)
    list(APPEND SLIC3R_GUI_SOURCES
            Utils/RetinaHelperImpl.mm
            Utils/MacDarkMode.mm
            Utils/MacDarkMode.hpp
            Utils/mouse_scheme.mm
            Utils/mouse_scheme.hpp
            GUI/RemovableDriveManagerMM.mm
            GUI/RemovableDriveManagerMM.h
            GUI/Mouse3DHandlerMac.mm
            GUI/InstanceCheckMac.mm
            GUI/InstanceCheckMac.h
            GUI/wxMediaCtrl2.mm
            GUI/wxMediaCtrl2.h
        )
    FIND_LIBRARY(DISKARBITRATION_LIBRARY DiskArbitration)
else ()
    list(APPEND SLIC3R_GUI_SOURCES
            GUI/wxMediaCtrl2.cpp
            GUI/wxMediaCtrl2.h
        )
endif ()

if (UNIX AND NOT APPLE)
    list(APPEND SLIC3R_GUI_SOURCES
        GUI/Printer/gstbambusrc.c
    )
endif ()

add_library(libslic3r_gui STATIC ${SLIC3R_GUI_SOURCES})
target_include_directories(libslic3r_gui PRIVATE Utils)

# AUE custom preview in GCodeViewer is development-oriented.
# The switch is declared at top-level CMake as SLIC3R_DEV_AUE_CUSTOM_PREVIEW.
if (SLIC3R_DEV_AUE_CUSTOM_PREVIEW)
    target_compile_definitions(libslic3r_gui PRIVATE ENABLE_AUE_CUSTOM_PREVIEW=1)
else ()
    target_compile_definitions(libslic3r_gui PRIVATE ENABLE_AUE_CUSTOM_PREVIEW=0)
endif ()

if (HAS_IMATISTL)
    if (IMATISTL_INCLUDE_DIR)
        target_include_directories(libslic3r_gui PRIVATE ${IMATISTL_INCLUDE_DIR} ${IMATISTL_INCLUDE_DIR}/Kernel ${IMATISTL_INCLUDE_DIR}/TMesh ${IMATISTL_INCLUDE_DIR}/ImatiSTL)
    endif()
    target_compile_definitions(libslic3r_gui PRIVATE USE_HYBRID_KERNEL USE_CGAL_LAZYNT)
endif()

if (WIN32)
    target_include_directories(libslic3r_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/WebView2/include)
endif()

if(GUI_SIMPLE)
    target_include_directories(libslic3r_gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/GUI)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SLIC3R_GUI_SOURCES})

encoding_check(libslic3r_gui)

target_link_libraries(libslic3r_gui libslic3r cereal::cereal imgui imguizmo minilzo GLEW::GLEW OpenGL::GL hidapi ${wxWidgets_LIBRARIES} glfw  libcurl OpenSSL::SSL OpenSSL::Crypto noise::noise)
if (HAS_IMATISTL)
    if (IMATISTL_TARGET)
        target_link_libraries(libslic3r_gui ${IMATISTL_TARGET})
    elseif (IMATISTL_LIBRARY)
        target_link_libraries(libslic3r_gui ${IMATISTL_LIBRARY} ${IMATISTL_DEP_LIBRARIES})
    endif()
endif()
if(WIN32)
    target_link_libraries(libslic3r_gui iphlpapi)
endif()


find_package(FFmpeg)
if(FFMPEG_VERSION)
        target_link_libraries(libslic3r_gui webrtc_video)
        add_definitions(-DENABLE_FFMPEG=1)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (FLATPAK)
        target_link_libraries(libslic3r_gui -lvdpau -lva -lva-drm -lva-x11)
    endif()
endif()

if (MSVC)
    target_link_libraries(libslic3r_gui Setupapi.lib)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    FIND_LIBRARY(WAYLAND_SERVER_LIBRARIES NAMES wayland-server)
    FIND_LIBRARY(WAYLAND_EGL_LIBRARIES    NAMES wayland-egl)
    FIND_LIBRARY(WAYLAND_CLIENT_LIBRARIES NAMES wayland-client)
    find_package(CURL REQUIRED)
    target_link_libraries(libslic3r_gui ${DBUS_LIBRARIES})
    target_link_libraries(libslic3r_gui
        OpenGL::EGL
        ${WAYLAND_SERVER_LIBRARIES}
        ${WAYLAND_EGL_LIBRARIES}
        ${WAYLAND_CLIENT_LIBRARIES}
        ${CURL_LIBRARIES}
    )
elseif (APPLE)
    target_link_libraries(libslic3r_gui ${DISKARBITRATION_LIBRARY})
endif()

if (SLIC3R_STATIC)
    # FIXME: This was previously exported by wx-config but the wxWidgets
    # cmake build forgets this and the build fails in debug mode (or on raspberry release)
    target_compile_definitions(libslic3r_gui PUBLIC -DwxDEBUG_LEVEL=0)
endif()

if (HAVE_SPNAV)
    target_link_libraries(libslic3r_gui spnav)
endif()

if (SLIC3R_STATIC AND NOT SLIC3R_STATIC_EXCLUDE_CURL AND UNIX AND NOT APPLE)
    target_compile_definitions(libslic3r_gui PRIVATE OPENSSL_CERT_OVERRIDE)
endif ()

if (SLIC3R_PCH AND NOT SLIC3R_SYNTAXONLY)
    add_precompiled_header(libslic3r_gui pchheader.hpp FORCEINCLUDE)
endif ()

# We need to implement some hacks for wxWidgets and touch the underlying GTK
# layer and sub-libraries. This forces us to use the include locations and
# link these libraries.
if (UNIX AND NOT APPLE)
    find_package(GTK${SLIC3R_GTK} REQUIRED)
    if (FLATPAK)
        # I don't know why this is needed, but for whatever reason slic3r isn't
        # linking to X11 and webkit2gtk. force it.
        find_package(X11 REQUIRED)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(webkit2gtk REQUIRED webkit2gtk-4.1)
        target_link_libraries (libslic3r_gui ${X11_LIBRARIES} ${webkit2gtk_LIBRARIES})
    endif()
    target_include_directories(libslic3r_gui PRIVATE ${GTK${SLIC3R_GTK}_INCLUDE_DIRS})
    target_link_libraries(libslic3r_gui ${GTK${SLIC3R_GTK}_LIBRARIES} fontconfig)


    # We add GStreamer for bambu:/// support.
    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
    pkg_check_modules(GST_BASE REQUIRED gstreamer-base-1.0)
    target_link_libraries(libslic3r_gui ${GSTREAMER_LIBRARIES} ${GST_BASE_LIBRARIES})
    target_include_directories(libslic3r_gui PRIVATE ${GSTREAMER_INCLUDE_DIRS} ${GST_BASE_INCLUDE_DIRS})
endif ()

# Add a definition so that we can tell we are compiling slic3r.
target_compile_definitions(libslic3r_gui PRIVATE SLIC3R_CURRENTLY_COMPILING_GUI_MODULE)
