import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import ".."
import "../qml"

Item{

    function updateValue1()
    {
        idprime_tower_line_width1.text = paraSettingUI.extruder1("prime_tower_line_width")
        idmaterial_print_temperature1.text = paraSettingUI.extruder1("material_print_temperature")
        idmaterial_print_temperature_layer_01.text = paraSettingUI.extruder1("material_print_temperature_layer_0")
        idmaterial_initial_print_temperature1.text = paraSettingUI.extruder1("material_initial_print_temperature")
        idmaterial_final_print_temperature1.text = paraSettingUI.extruder1("material_final_print_temperature")
        idmaterial_bed_temperature1.text = paraSettingUI.extruder1("material_bed_temperature")
        idmaterial_bed_temperature_layer_01.text = paraSettingUI.extruder1("material_bed_temperature_layer_0")
        idmaterial_standby_temperature1.text = paraSettingUI.extruder1("material_standby_temperature")
        idprime_tower_flow1.text = paraSettingUI.extruder1("prime_tower_flow")
        idswitch_extruder_retraction_amount1.text = paraSettingUI.extruder1("switch_extruder_retraction_amount")
        idswitch_extruder_retraction_speeds1.text = paraSettingUI.extruder1("switch_extruder_retraction_speeds")
        idswitch_extruder_prime_speed1.text = paraSettingUI.extruder1("switch_extruder_prime_speed")
        idprime_tower_min_volume1.text = paraSettingUI.extruder1("prime_tower_min_volume")
    }
	
    RowLayout{
        anchors.fill: parent
        anchors.margins: 20*screenScaleFactor
        ListView{
            id:btnsList
            Layout.preferredWidth: 200
            Layout.preferredHeight: 430*screenScaleFactor
            model:[qsTr("Quality"),qsTr("Matrerial"),qsTr("Dual Extrusion")]
            delegate:  CusImglButton{
                width: 200*screenScaleFactor
                height : 28*screenScaleFactor
                btnRadius : 3
                state: "wordsOnly"
                checkable: true
                checked: btnsList.currentIndex == model.index
                textAlign: 1
                text: modelData
                ButtonGroup.group: leftBtnsGroup
                leftMargin: 10 *screenScaleFactor
                defaultBtnBgColor: Constants.dialogContentBgColor
                hoveredBtnBgColor: Constants.typeBtnHoveredColor
                onClicked: {
                    btnsList.currentIndex = model.index
                }
            }
        }

        Item{
            Layout.fillWidth: true
        }

        StackLayout{
            Layout.preferredWidth: 500*screenScaleFactor
            Layout.preferredHeight: 430*screenScaleFactor
            currentIndex: btnsList.currentIndex
            Column{
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Prime Tower Line Width") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("prime_tower_line_width"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idprime_tower_line_width1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "prime_tower_line_width"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "mm"
                    }
                }
            }
            Column{
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Printing Temperature") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_print_temperature1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_print_temperature"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Printing Temperature Initial Layer") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_print_temperature_layer_01
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_print_temperature_layer_0"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Initial Printing Temperature") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_initial_print_temperature1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_initial_print_temperature"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Final Printing Temperature") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_final_print_temperature1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_final_print_temperature"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Build Plate Temperature") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_bed_temperature1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_bed_temperature"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Build Plate Temperature Initial Layer") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_bed_temperature_layer_01
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_bed_temperature_layer_0"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Standby Temperature") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idmaterial_standby_temperature1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "material_standby_temperature"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "℃"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Prime Tower Flow") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idprime_tower_flow1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "prime_tower_flow"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "%"
                    }
                }
            }
            Column{
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Nozzle Switch Retraction Distance") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idswitch_extruder_retraction_amount1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "switch_extruder_retraction_amount"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "mm"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Nozzle Switch Retraction Speed") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idswitch_extruder_retraction_speeds1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "switch_extruder_retraction_speeds"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "mm/s"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Nozzle Switch Prime Speed") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idswitch_extruder_prime_speed1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "switch_extruder_prime_speed"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "mm/s"
                    }
                }
                Row{
                    spacing: 20* screenScaleFactor
                    StyledLabel {
                        width : 160* screenScaleFactor
                        height : 30* screenScaleFactor
                        text: qsTr("Prime Tower Minimum Volume") + ":"
                        fontNormalColor: Constants.right_panel_item_text_default_color
                        fontDisableColor: Constants.right_panel_item_text_default_color
                        font.pointSize:Constants.labelFontPointSize
                        strToolTip: qsTr(SettingJson.getDescription("material_print_temperature"))
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField {
                        id:idprime_tower_min_volume1
                        width: 260* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5
                        objectName: "prime_tower_min_volume"
                        text: paraSettingUI.extruder1(objectName)
                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                            paraSettingUI.setExtruder1(objectName,text)
                        }
                        onTextChanged:
                        {

                        }
                        unitChar: "mm³"
                    }
                }
            }
        }
    }
}


