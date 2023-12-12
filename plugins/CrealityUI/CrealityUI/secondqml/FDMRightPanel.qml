import QtQml 2.13

import QtQuick 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5

import "../qml"
import "../secondqml"
import "../gcodepreview"

Rectangle
{
    id: idFDMRight

    property var moneyType : "money"
    property int marginSpacing: 10 * screenScaleFactor
    property color backReportPanelColor: Constants.itemBackgroundColor//"#061F3B"

    property string printTime: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.printing_time() : ""
    property double materialMoney: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_money() : ""
    property double materialLength: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_length() : ""
    property double materialWeight: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_weight() : ""

	function update() {
	}

	function changedType() {
	}

	function showPlatform(visibility) {
		idPrintPlatForm.checked = visibility
	}

	function showNozzle(visibility){
		idNozzle.checked = visibility
	}

	function tranlateMoneyType(type) {
		if (type === "RMB") { return "￥" }
		return type
	}

	function setHighlightRow(step) {
		idGCodeTextEdit.select(400, 500)
	}

	function revertStructureColor() {
		structure_panel.resetChecked()
	}

	function setLayerGCodesIndex(index){
		if(index < 0)
			return

        idStepCodeview.lineIndex = index
	}

	MouseArea {
		id: wheelFilter
		anchors.fill: parent
		propagateComposedEvents: false
		onWheel: {}
	}
	
	Column {
		anchors.fill: parent
		anchors.margins: idFDMRight.marginSpacing
		spacing: idFDMRight.marginSpacing

		Label {
            text: qsTr("G-code Preview")
			color: Constants.right_panel_text_default_color
			font.pointSize: Constants.labelFontPointSize_12
			font.family: Constants.panelFontFamily
			font.bold: true
		}

		Grid {
			columns: 2
			spacing: idFDMRight.marginSpacing

			Repeater {
				model: kernel_global_const.customized ? [
                    qsTr("Printing Time")   + ": " + printTime,
					qsTr("Material Weight") + ": " + materialWeight + " g",
                    qsTr("Material Cost")   + ": " + materialMoney //+ " " + moneyType
				] : [
                    qsTr("Printing Time")   + ": " + printTime,
					qsTr("Material Weight") + ": " + materialWeight + " g",
                    qsTr("Material Length") + ": " + materialLength.toFixed(2) + " m",
                    qsTr("Material Cost")   + ": " + materialMoney //+ " " + moneyType
				]

				delegate: Label {
					verticalAlignment: Qt.AlignVCenter
					text: modelData
					color: Constants.right_panel_text_default_color
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
			anchors.rightMargin:  idFDMRight.marginSpacing

			text: qsTr("Show in Preview")
			color: Constants.right_panel_text_default_color
			font.pointSize: Constants.labelFontPointSize_12
			font.family: Constants.panelFontFamily
			font.bold: true
		}

		Grid {
			columns: 2
			columnSpacing: 30 * screenScaleFactor

			StyleCheckBox {
				id: idPrintPlatForm
				text: qsTr("Printing Platform")
				checked : true
				height: 20 * screenScaleFactor

				indicatorImage: "qrc:/UI/images/check2.png"
				indicatorColor: checked ? "#1E9BE2" : "transparent"
				indicatorBorderColor: checked || hovered ? "#1E9BE2"
														: Constants.right_panel_border_default_color

				onCheckedChanged: {
					kernel_slice_model.setPrinterVisible(checked)
				}
			}

			StyleCheckBox {
				id: idNozzle
				height: 20 * screenScaleFactor
				text: qsTr("Nozzle")
				checked : true

				indicatorImage: "qrc:/UI/images/check2.png"
				indicatorColor: checked ? "#1E9BE2" : "transparent"
				indicatorBorderColor: checked || hovered ? "#1E9BE2"
														: Constants.right_panel_border_default_color

				onCheckedChanged: {
					kernel_slice_model.setIndicatorVisible(checked)
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
				height : 30 * screenScaleFactor
				textColor: checked || hovered ? Constants.right_panel_slice_text_default_color
                        : Constants.right_panel_gcode_text_color
				buttonColor : checked || hovered ? Constants.right_panel_slice_button_default_color
												: Constants.right_panel_button_default_color
				buttonBorder.width: checked || hovered ? 0 : 1
				buttonBorder.color: Constants.right_panel_border_default_color
				font.pointSize: Constants.labelFontPointSize_12
				font.family: Constants.panelFontFamily
				font.bold: true
			}

			BasicTabButton {
				id: idStructureTab
				text: qsTr("G-Code")
				width: 100 * screenScaleFactor
				height : 30 * screenScaleFactor
				textColor: checked || hovered ? Constants.right_panel_slice_text_default_color
                        : Constants.right_panel_gcode_text_color
				buttonColor : checked || hovered ? Constants.right_panel_slice_button_default_color
												: Constants.right_panel_button_default_color
				buttonBorder.width: checked || hovered ? 0 : 1
				buttonBorder.color: Constants.right_panel_border_default_color
				font.pointSize: Constants.labelFontPointSize_12
				font.family: Constants.panelFontFamily
				font.bold: true
			}
		}

		StackLayout {
			id: swipeView

			width: parent.width
			height: 310 * screenScaleFactor
			currentIndex: idTabBar.currentIndex

			CustomTabView {
				id: preview_tab_view

				readonly property real itemMargins: 10 * screenScaleFactor

				anchors.fill: parent

        titleStyle: CustomTabView.TitleStyle.COMBO_BOX
				titleBarRightMargin: width / 2
				defaultPanel: structure_view
				onCurrentPanelChanged: {
					let type = 0
					switch (this.currentPanel) {
						case speed_view      : type = kernel_slice_model.speedModel.visualType      ; break
						case structure_view  : type = kernel_slice_model.structureModel.visualType  ; break
						case extruder_view   : type = kernel_slice_model.extruderModel.visualType   ; break
						case layer_hight_view: type = kernel_slice_model.layerHightModel.visualType ; break
						case line_width_view : type = kernel_slice_model.lineWidthModel.visualType  ; break
						case flow_view       : type = kernel_slice_model.flowModel.visualType       ; break
						case layer_time_view : type = kernel_slice_model.layerTimeModel.visualType  ; break
						case fan_speed_view  : type = kernel_slice_model.fanSpeedModel.visualType   ; break
						case temperature_view: type = kernel_slice_model.temperatureModel.visualType; break
						default: break
					}
					kernel_slice_model.setGCodeVisualType(type)
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
						id : structure_panel
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
						anchors.margins: preview_tab_view.itemMargins

						Connections {
							target: kernel_parameter_manager
							onCurMachineIndexChanged:{
								extruder_panel.nozzlenum = kernel_parameter_manager.curExtruderCount()
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
			}

			CusTextEditListView {
				id :idStepCodeview
				width: parent.width
				height: parent.height
				backgroundRadius:5
				backgroundColor: "transparent"
				backgroundBorder.color: Constants.right_panel_border_default_color
				model: kernel_slice_model.layerGCodes

				onDoubleClickItem:{
					var stepIndex = kernel_slice_model.findStepFromViewIndex(viewIndex)
					if(stepIndex < 0)
						return

					idFDMFooter.currentStep = stepIndex
				}
			}
		}
	}
}
