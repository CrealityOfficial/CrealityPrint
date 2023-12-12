import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQml 2.13

import ".."
import "../qml"
import "../components"

Item{
    property alias proFieldCom: textEditCom
    property alias checkBoxCom: checkBoxCom
    property alias textAreaCom: textAreaCom
    property alias comboboxCom: comboboxCom
    property alias leftBtnCom: leftBtn
    property alias itemRow: itemRow
    property alias recoverItemRowCom: recoverItemRow
    property var curGroupKey
    property var parameterContext
    id: gCompent

    Component{
        id: recoverItemRow
        RowLayout{
            width: parent.width
            visible: {
                if (model.key === "machine_start_gcode" ||
                        model.key === "machine_end_gcode"){

                    return false;
                }else
                    return true;
            }
            CusCheckBox{
                id: cbcOverride
                text: qsTr(model.label) + ":"
                checked: model.isOverride
                onClicked:{
                    if(checked)
                    {
                        parameterContext.setOverideValue(model.key,model.str)
                    }else{
                        parameterContext.resetValue(model.key)
                    }
                }
            }

            Item{
                Layout.fillWidth: true
            }

            CusImglButton{
                id: proBtn
                enabled: cbcOverride.checked
                Layout.preferredWidth: 20 * screenScaleFactor
                Layout.preferredHeight: 20 * screenScaleFactor
                visible:model.userModify
                borderWidth: 0
                enabledIconSource: "qrc:/UI/photo/recoverBtn.svg"
                highLightIconSource: "qrc:/UI/photo/recoverBtn_hover.svg"
                pressedIconSource: "qrc:/UI/photo/recoverBtn_hover.svg"
                onClicked: {
                    parameterContext.resetValue(key)
                    if (kernel_parameter_manager.checkVisSceneSensitiveParam(key)) {
                        kernel_parameter_manager.syncCurParamToVisScene()
                    }
                }
            }

            Loader{
                property var rowItemModel: model
                property var textEditRegExp: /.*/
                property bool isEnable:cbcOverride.checked
                enabled: cbcOverride.checked
                sourceComponent: {
                    if(model.key === "material_description"){
                        return textAreaCom
                    }
                    else if(model.type === "bool"){
                        return checkBoxCom
                    }
                    else if(model.type === "enum" || model.type === "optional_extruder" || model.type === "extruder"){
                        return comboboxCom
                    }
                    else if(model.type === "int"){
                        textEditRegExp = /[\-\[\],0-9\x22]+/
                        return textEditCom
                    }
                    else if(model.type === "float"){
                        textEditRegExp = /(^-?[1-9]\d*\.\d+$|^-?0\.\d+$|^-?[1-9]\d*$|^0$)/
                        return textEditCom
                    }
                    else if(model.type === "str" ){
                        textEditRegExp = /.*/
                        return textEditCom
                    }
                    else{
                        return textEditCom
                    }
                }
            }
        }
    }

    Component{
        id: itemRow
        RowLayout{
            visible: {
                if (model.level == -2 || model.key === "machine_start_gcode" ||
                        model.key === "machine_end_gcode" ||model.key === "machine_extruder_start_code"||model.key === "machine_extruder_end_code"|| !model.enabled){

                    return false;
                }else
                    return true;
            }

            StyledLabel{
                activeFocusOnTab: false
//                Layout.preferredWidth: 200* screenScaleFactor
                Layout.preferredHeight: 30* screenScaleFactor
                text: ((model.level===2 || model.level===4)?"    ":"") + qsTr(model.label) + ":"
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
                strToolTip: qsTr(model.tips)
            }

            Item{
                Layout.fillWidth: true
            }

            CusImglButton{
                id: proBtn
                Layout.preferredWidth: 20 * screenScaleFactor
                Layout.preferredHeight: 20 * screenScaleFactor
                visible:model.userModify
                borderWidth: 0
                Layout.alignment: Qt.AlignVCenter
                enabledIconSource: "qrc:/UI/photo/recoverBtn.svg"
                highLightIconSource: "qrc:/UI/photo/recoverBtn_hover.svg"
                pressedIconSource: "qrc:/UI/photo/recoverBtn_hover.svg"
                onClicked: {
                    parameterContext.resetValue(key)
                    if (kernel_parameter_manager.checkVisSceneSensitiveParam(key)) {
                        kernel_parameter_manager.syncCurParamToVisScene()
                    }
                }
            }

            Loader{
                property var rowItemModel: model
                property var textEditRegExp: /.*/
                id: rowLoader
                sourceComponent: {
                    if(model.key === "material_description"){
                        return textAreaCom
                    }
                    else if(model.type === "bool"){
                        return checkBoxCom
                    }
                    else if(model.type === "enum" || model.type === "optional_extruder" || model.type === "extruder"){
                        return comboboxCom
                    }
                    else if(model.type === "int"){
                        textEditRegExp = /[\-\[\],0-9\x22]+/
                        return textEditCom
                    }
                    else if(model.type === "float"){
                        textEditRegExp = /(^-?[1-9]\d*\.\d+$|^-?0\.\d+$|^-?[1-9]\d*$|^0$)/
                        return textEditCom
                    }
                    else if(model.type === "str" ){
                        textEditRegExp = /.*/
                        return textEditCom
                    }
                    else{
                        //                        textEditRegExp = ""
                        return textEditCom
                    }
                }
            }
        }
    }

    Component{
        id: textEditCom
        BasicDialogTextField{
            id: proField
            width: 260* screenScaleFactor
//            height: if(rowItemModel.key === "acceleration_limit_mess" ||
//                            rowItemModel.key === "speed_limit_to_height"){
//                        return 48* screenScaleFactor
//                    }
//                    else{
//                        return 28* screenScaleFactor
//                    }
            radius: 5* screenScaleFactor
            wrapMode: TextInput.WordWrap
            visible: {
                if(rowItemModel)
                {
                    return rowItemModel.level>=0
                }else{
                    return false;
                }
            }
//            rightInset: 230
            objectName: !rowItemModel ? "" : rowItemModel.key
            text: !rowItemModel ? "" : rowItemModel.str
            unitChar: !rowItemModel ? "" : qsTr(rowItemModel.unit)
            validator: RegExpValidator { regExp:  textEditRegExp?textEditRegExp:/.*/}
            onEditingFinished: {
                if(rowItemModel.type === "float"){
                    let intText = Number(text)
                    if(rowItemModel.key === "material_advance_length"){
                        text = intText.toFixed(3)
                    }
                    else{
                        text = intText.toFixed(2)
                    }
                    parameterContext.textFieldEditFinish(rowItemModel.key, this)
                    text = Qt.binding(function(){ return !rowItemModel ? "" : rowItemModel.str } )
                }else{
                    parameterContext.textFieldEditFinish(rowItemModel.key, this)
                }
            }

            onTextChanged:
            {
            }
        }
    }

    Component{
        id: checkBoxCom
        CusCheckBox{
            width: 260* screenScaleFactor
            height: 28* screenScaleFactor
            //            strToolTip: qsTr(rowItemModel.tips)
            checked : !rowItemModel ? false : rowItemModel.checked
            Component.onCompleted:{
            }

            onClicked:{
                parameterContext.textFieldEditFinish(rowItemModel ? rowItemModel ? rowItemModel.key : "" : "", this)
            }
        }
    }

    Component{
        id: textAreaCom
        BasicDialogTextArea {
            id:end_gcode_editor
            width: 500*screenScaleFactor
            height: rowItemModel? ((rowItemModel.enabled && rowItemModel)?80*screenScaleFactor:0):0
            wrapMode: TextEdit.WordWrap
            enabled: idAddPrinterDlg.curGroupKey === "override" ? isEnable==true : true
            text: rowItemModel ? rowItemModel.str : ""
            color: Constants.manager_printer_gcode_text_color
            onEditingFinished: {
                parameterContext.textFieldEditFinish(rowItemModel ? rowItemModel.key : "", this)
            }
        }
    }

    Component{
        id: comboboxCom
        CXComboBox{
            width: 260* screenScaleFactor
            height:  (!rowItemModel ? false : rowItemModel.enabled)?28* screenScaleFactor:0
            enabled: false
            hasImg: {
                if(rowItemModel) return rowItemModel.key === "infill_pattern" ||
                                 rowItemModel.key === "top_bottom_pattern" ||
                                 rowItemModel.key === "top_bottom_pattern_0" ||
                                 rowItemModel.key === "ironing_pattern" ||
                                 rowItemModel.key === "support_pattern" ||
                                 rowItemModel.key === "support_interface_pattern" ||
                                 rowItemModel.key === "support_roof_pattern" ||
                                 rowItemModel.key === "support_bottom_pattern" ||
                                 rowItemModel.key === "roofing_pattern"

            }
            Component.onCompleted:{
                if (!model)
                {
                    model = parameterContext.createEnumListModel(rowItemModel ? rowItemModel.key : "")
                }
                enabled = true;
                if(rowItemModel)
                {
                    currentIndex = rowItemModel.currentIndex
                    currentIndex = Qt.binding(function() { return rowItemModel.currentIndex})
                }
            }

            onStyleComboBoxIndexChanged:{
                if (!enabled)
                {
                    return false;
                }
                parameterContext.textFieldEditFinish(rowItemModel ? rowItemModel.key : "", this)
            }
        }
    }


    Component{
        id: leftBtn
        CusImglButton{
            property string imgPath: "qrc:/UI/photo/AdvanceParam/"
            width: 200*screenScaleFactor
            height : 28*screenScaleFactor
            btnRadius : 3
            state: "imgLeft"
            checkable: true
            textAlign: 0
            leftMargin: 10 *screenScaleFactor
            font.pointSize:Constants.labelFontPointSize_10
            bottonSelected: model.key === currentCategry
            defaultBtnBgColor: "transparent"
            hoverBorderColor: "transparent"
            selectBorderColor: "transparent"
            hoveredBtnBgColor: "#1E9BE2"
            ButtonGroup.group: leftBtnsGroup
            allowTooltip: false
            text: qsTr(model.label)
            enabledIconSource:imgPath + model.label+".svg"
            highLightIconSource:imgPath + model.label+".svg"
            pressedIconSource:imgPath + model.label+"_checked.svg"
            checked: {
                //console.log(currentCategry+"---click--"+model.key)
                return model.key === currentCategry
            }
            onClicked: {
                console.log("---click--"+model.key)
                changeCategory(model.key)
                idCategoryRepeater.currentIndex = model.index
            }
        }
    }
}



