import "../components"
import "../gcodepreview"
import "../qml"
import "../secondqml"
import QtQml 2.13
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

Rectangle {
    id: idFDMRight

    property var moneyType: "money"
    property int marginSpacing: 10 * screenScaleFactor
    property color backReportPanelColor: Constants.itemBackgroundColor //"#061F3B"
    property string printTime: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.printing_time() : ""
    property double materialMoney: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_money() : ""
    property double materialLength: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_length() : ""
    property double materialWeight: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_weight() : ""
    radius:5* screenScaleFactor
    function update() {
    }

    function changedType() {
    }

    function showPlatform(visibility) {
        idPrintPlatForm.checked = visibility;
    }

    function showNozzle(visibility) {
        idNozzle.checked = visibility;
    }

    function tranlateMoneyType(type) {
        if (type === "RMB")
            return "￥";

        return type;
    }

    function setHighlightRow(step) {
        idGCodeTextEdit.select(400, 500);
    }

    function revertStructureColor() {
        structure_panel.resetChecked();
    }

    function setLayerGCodesIndex(step) {
        if (step < 0)
            return ;
        
        //start from 1
        idStepCodeview.lineIndex = kernel_slice_model.findViewIndexFromStep(step + 1);
    }

    Connections {
        target: kernel_slice_flow
        onSliceAttainChanged: {
            if (!kernel_slice_flow.sliceAttain || kernel_slice_flow.isSlicing) {
                idCloseBtn.visible = false;
                idFdmRect.visible = false;
                foldBtn.visible = false;
            } else {
                idCloseBtn.visible = true;
                idFdmRect.visible = true;
                foldBtn.visible = false;
            }
        }
    }

    MouseArea {
        id: wheelFilter

        anchors.fill: parent
        propagateComposedEvents: false
        onWheel: {
        }
    }

    CusImglButton {
        id: foldBtn

        width: 30 * screenScaleFactor
        height: 30 * screenScaleFactor
        imgWidth: 7 * screenScaleFactor
        imgHeight: 5 * screenScaleFactor
        defaultBtnBgColor: Qt.rgba(0, 0, 0, 0.5)
        hoveredBtnBgColor: Qt.rgba(0, 0, 0, 0.5)
        selectedBtnBgColor: Qt.rgba(0, 0, 0, 0.5)
        opacity: 1
        borderWidth: 0
        text: qsTr("Expand")
        cusTooltip.position: BasicTooltip.Position.BOTTOM
        shadowEnabled: Constants.currentTheme
        enabledIconSource: "qrc:/UI/photo/unfold.svg"
        pressedIconSource: "qrc:/UI/photo/unfold.svg"
        highLightIconSource: "qrc:/UI/photo/unfold.svg"
        visible: false
        onClicked: {
            idFdmRect.visible = true;
            visible = false;
        }
    }

    CusRoundedBg {
        id: idFdmRect

        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.5)
        visible: false
        radius:5* screenScaleFactor
        borderWidth:0
        allRadius:true
        Column {
            id: idFdmColumn
            anchors.fill: parent
            anchors.margins: idFDMRight.marginSpacing
            spacing: idFDMRight.marginSpacing

            RowLayout {
                width: idFdmColumn.width
                height: 20 * screenScaleFactor

                Label {
                    text: qsTr("G-code Preview")
                    color: "#FFFFFF" //Constants.right_panel_text_default_color
                    font.pointSize: Constants.labelFontPointSize_12
                    font.family: Constants.panelFontFamily
                    font.bold: true
                }


                Item{
                    Layout.fillWidth: true
                }

                CusImglButton {
                    id: idCloseBtn
                    Layout.preferredWidth: 30 * screenScaleFactor
                    Layout.preferredHeight: 30 * screenScaleFactor
                    imgWidth: 7 * screenScaleFactor
                    imgHeight: 5 * screenScaleFactor

                    defaultBtnBgColor: "transparent"
                    hoveredBtnBgColor: "transparent"
                    selectedBtnBgColor: "transparent"
                    opacity: 1
                    borderWidth: 0
                    text: qsTr("Collapse")
                    cusTooltip.position: BasicTooltip.Position.BOTTOM
                    shadowEnabled: Constants.currentTheme
                    enabledIconSource: "qrc:/UI/photo/fold.svg"
                    pressedIconSource: "qrc:/UI/photo/fold.svg"
                    highLightIconSource: "qrc:/UI/photo/fold.svg"
                    onClicked: {
                        idFdmRect.visible = false;
                        foldBtn.visible = true;
                    }
                }

            }

            Label {
                text: cxTr("Current Machine") + ": " + kernel_parameter_manager.currentMachineObject.name()
                color: "#FFFFFF" //Constants.right_panel_text_default_color
                font.pointSize: Constants.labelFontPointSize_9
                font.family: Constants.panelFontFamily
            }

            Grid {
                columns: 2
                spacing: idFDMRight.marginSpacing

                Repeater {
                    //+ " " + moneyType
                    //+ " " + moneyType

                    model: kernel_global_const.customized ? [qsTr("Printing Time") + ": " + printTime,
                                                             qsTr("Material Weight") + ": " + materialWeight + " g", qsTr("Material Cost") + ": " + materialMoney] :
                                                            [qsTr("Printing Time") + ": " + printTime,
                                                             qsTr("Material Weight") + ": " + materialWeight + " g",
                                                             qsTr("Material Length") + ": " + materialLength.toFixed(2) + " m",
                                                             qsTr("Material Cost") + ": " + materialMoney]

                    delegate: Label {
                        verticalAlignment: Qt.AlignVCenter
                        text: modelData
                        color: "#FFFFFF" //Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        font.family: Constants.panelFontFamily
                    }

                }
            }

            Rectangle {
                x: -idFDMRight.marginSpacing
                width: parent.width + idFDMRight.marginSpacing * 2
                height: 1
                color: Constants.right_panel_menu_split_line_color
            }

            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                //	anchors.leftMargin: idFDMRight.marginSpacing
                anchors.rightMargin: idFDMRight.marginSpacing
                text: qsTr("Show in Preview")
                color: "#FFFFFF" // Constants.right_panel_text_default_color
                font.pointSize: Constants.labelFontPointSize_12
                font.family: Constants.panelFontFamily
                font.bold: true
            }

            Grid {
                columns: 2
                columnSpacing: 30 * screenScaleFactor

                StyleCheckBox {
                    id: idPrintPlatForm
                    width:100* screenScaleFactor
                    textColor: "#FFFFFF"
                    text: qsTr("Printing Platform")
                    checked: kernel_slice_flow.printerVisible
                    height: 20 * screenScaleFactor
                    indicatorImage: "qrc:/UI/images/check2.png"
                    onCheckedChanged: {
                        if (checked != kernel_slice_flow.printerVisible)
                            kernel_slice_flow.printerVisible = checked
                    }
                    Keys.onPressed: {
                        if (event.key === Qt.Key_Space) {
                            event.accepted = true;
                        }
                    }
                }

                StyleCheckBox {
                    id: idNozzle
                    width:100* screenScaleFactor
                    textColor: "#FFFFFF"
                    height: 20 * screenScaleFactor
                    text: qsTr("Nozzle")
                    checked: kernel_slice_model.indicatorVisible
                    indicatorImage: "qrc:/UI/images/check2.png"
                    onCheckedChanged: {
                        if (checked != kernel_slice_model.indicatorVisible)
                            kernel_slice_model.indicatorVisible = checked
                    }
                    Keys.onPressed: {
                        if (event.key === Qt.Key_Space) {
                            event.accepted = true;
                        }
                    }
                }

            }

            Rectangle {
                x: -idFDMRight.marginSpacing
                width: parent.width + idFDMRight.marginSpacing * 2
                height: 1
                color: Constants.right_panel_menu_split_line_color
            }

            BasicTabBar {
                id: idTabBar

                Layout.fillWidth: true
                Layout.margins: idFDMRight.marginSpacing
                height: 30 * screenScaleFactor
                spacing: idFDMRight.marginSpacing
                backgroundColor: "transparent"
                currentIndex: 0

                BasicTabButton {
                    id: idColor
                    text: qsTr("Color Show")
                    width: 100 * screenScaleFactor
                    height: 30 * screenScaleFactor
                    textColor: "#FFFFFF"
                    buttonColor: checked ? Constants.themeGreenColor : "transparent"
                    buttonBorder.width: checked?0:1
                    buttonBorder.color: hovered ? Constants.themeGreenColor : Constants.right_panel_border_default_color
                    font.pointSize: Constants.labelFontPointSize_12
                    font.family: Constants.panelFontFamily
                    font.bold: true
                }

                BasicTabButton {
                    id: idStructureTab

                    text: qsTr("G-Code")
                    width: 100 * screenScaleFactor
                    height: 30 * screenScaleFactor
                    textColor: "#FFFFFF"
                    buttonColor: checked ? Constants.themeGreenColor : "transparent"
                    buttonBorder.width: checked?0:1
                    buttonBorder.color: hovered ? Constants.themeGreenColor : Constants.right_panel_border_default_color
                    font.pointSize: Constants.labelFontPointSize_12
                    font.family: Constants.panelFontFamily
                    font.bold: true
                }

            }

            StackLayout {
                id: swipeView

                width: parent.width
                height: screenScaleFactor<=1? 395 * screenScaleFactor:245* screenScaleFactor
                currentIndex: idTabBar.currentIndex

                CustomTabView {
                    id: preview_tab_view

                    readonly property real itemMargins: 10 * screenScaleFactor

                    buttonTextCheckedColor: "#FFFFFF"
                    buttonColor: Qt.rgba(0, 0, 0, 0.5)
                    buttonHoveredColor: Constants.themeGreenColor
                    panelColor: "transparent"
                    borderColor:"#6E6E72"
                    anchors.fill: parent
                    titleStyle: CustomTabView.TitleStyle.COMBO_BOX
                    titleBarRightMargin: width / 2
                    defaultPanel: structure_view
                    onVisibleChanged:{
                        let type = kernel_printer_manager.selectedPrinter.hasModelInsideUseColors()
                        if(type){
                            defaultPanel = extruder_view
                        }else {
                            let extruderConfigs = kernel_printer_manager.selectedPrinter.settingsObject.specificExtruderHeights
                            if (extruderConfigs.length > 0) {
                                defaultPanel = extruder_view
                            } else {
                                defaultPanel = structure_view
                            }
                        }
                    }
                    onCurrentPanelChanged: {
                        let type = 0;
                        switch (this.currentPanel) {
                        case speed_view:
                            type = kernel_slice_model.speedModel.visualType;
                            break;
                        case structure_view:
                            type = kernel_slice_model.structureModel.visualType;
                            break;
                        case extruder_view:
                            type = kernel_slice_model.extruderModel.visualType;
                            break;
                        case layer_hight_view:
                            type = kernel_slice_model.layerHightModel.visualType;
                            break;
                        case line_width_view:
                            type = kernel_slice_model.lineWidthModel.visualType;
                            break;
                        case flow_view:
                            type = kernel_slice_model.flowModel.visualType;
                            break;
                        case layer_time_view:
                            type = kernel_slice_model.layerTimeModel.visualType;
                            break;
                        case fan_speed_view:
                            type = kernel_slice_model.fanSpeedModel.visualType;
                            break;
                        case temperature_view:
                            type = kernel_slice_model.temperatureModel.visualType;
                            break;
                        case acceleration_view:
                            type = kernel_slice_model.accModel.visualType;
                            break;
                        default:
                            break;
                        }
                        kernel_slice_model.setGCodeVisualType(type);
                    }

                    CustomTabViewItem {
                        id: speed_view

                        title: speed_panel.title

                        SpeedPanel {
                            id: speed_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: structure_view

                        title: structure_panel.title

                        StructurePanel {
                            id: structure_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: extruder_view

                        title: extruder_panel.title

                        ExtruderPanel {
                            id: extruder_panel

                            anchors.fill: parent
                            anchors.margins: 1 //preview_tab_view.borderRadius

                            Connections {
                                target: kernel_parameter_manager
                                onCurMachineIndexChanged: {
                                    extruder_panel.nozzlenum = kernel_parameter_manager.curExtruderCount();
                                }
                            }

                        }

                    }

                    CustomTabViewItem {
                        id: layer_hight_view

                        title: layer_hight_panel.title

                        LayerHightPanel {
                            id: layer_hight_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: line_width_view

                        title: line_width_panel.title

                        LineWidthPanel {
                            id: line_width_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: flow_view

                        title: flow_panel.title

                        FlowPanel {
                            id: flow_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: layer_time_view

                        title: layer_time_panel.title

                        LayerTimePanel {
                            id: layer_time_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: fan_speed_view

                        title: fan_speed_panel.title

                        FanSpeedPanel {
                            id: fan_speed_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: temperature_view

                        title: temperature_panel.title

                        TemperaturePanel {
                            id: temperature_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                    CustomTabViewItem {
                        id: acceleration_view

                        title: acceleration_panel.title

                        AccelerationPanel {
                            id: acceleration_panel

                            anchors.fill: parent
                            anchors.margins: preview_tab_view.itemMargins
                        }

                    }

                }

                CusTextEditListView {
                    id: idStepCodeview

                    width: parent.width
                    height: parent.height
                    backgroundRadius: 5
                    backgroundColor: "transparent"
                    backgroundBorder.color: Constants.right_panel_border_default_color
                    model: kernel_slice_model.layerGCodes
                    onDoubleClickItem: {
                        var stepIndex = kernel_slice_model.findStepFromViewIndex(viewIndex);
                        if (stepIndex < 0)
                            return ;

                        idFDMFooter.currentStep = stepIndex;
                    }

                    onPreparePressed: {
                        var stepIndex = kernel_slice_model.findStepFromViewIndex(viewIndex);
                        if (stepIndex < 0)
                            clickEnabled = false
                    }

                    onPrepareReleased: {
                        clickEnabled = true
                    }
                }

            }

        }

    }

}
