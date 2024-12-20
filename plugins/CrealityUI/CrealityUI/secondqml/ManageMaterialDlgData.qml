import QtQuick 2.10
import QtQml.Models 2.13

Item{
    property alias attributesItems: attributesItems //属性
    property alias tempItems: tempItems //温度
    property alias flowItems: flowItems //流量
    property alias coolingItems: coolingItems //冷却
    property alias retractionItems: retractionItems //回抽
    property alias gCodeItems: gCodeItems //G-Code

    property var materialParams
    property int currentExtruder
    property var dataProView
    property var dataRetView

    id: rootData

    ListModel{
        id: attributesItems
        ListElement{unitChar: "gfcm"; labelText: qsTr("Density"); fieldKey:"material_density"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm"; labelText: qsTr("Diameter"); fieldKey:"material_diameter"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "$"; labelText: qsTr("Filament Cost"); fieldKey:"filament_cost"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "g"; labelText: qsTr("Filament weight"); fieldKey:"filament_weight"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "m"; labelText: qsTr("Filament length"); fieldKey:"filament_length"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "$/m"; labelText: qsTr("Cost per Meter"); fieldKey:"cost_per_meter"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Description"); fieldKey:"material_description"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Soluble meterial"); fieldKey:"material_description"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Support meterial"); fieldKey:"material_description"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Enable pressure advance"); fieldKey:"material_description"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Pressure advance"); fieldKey:"material_description"; fieldValue:""; isDefault:true}
        //console.log("key = ", itemKey)
        //console.log("ParamsList = ", materialParams.parameterList())
        //return materialParams.parameterList()
    }

    function appendAI(){
        attributesItems.append({"unitChar": "gfcm", "labelText": qsTr("Density"), "fieldKey":"material_density", "fieldValue":"", "isDefault":true})
    }

    ListModel{
        id:tempItems
        ListElement{unitChar: "℃"; labelText: qsTr("Printing Temperature"); fieldKey:"material_print_temperature"; fieldValue:0; isDefault:true}
        ListElement{unitChar: "℃"; labelText: qsTr("Printing Temperature Initial Layer"); fieldKey:"material_print_temperature_layer_0"; fieldValue:0; isDefault:true}
        ListElement{unitChar: "℃"; labelText: qsTr("Initial Printing Temperature"); fieldKey:"material_initial_print_temperature"; fieldValue:0; isDefault:true}
        ListElement{unitChar: "℃"; labelText: qsTr("Final Printing Temperature"); fieldKey:"material_final_print_temperature"; fieldValue:0; isDefault:true}
        ListElement{unitChar: "℃"; labelText: qsTr("Build Plate Temperature"); fieldKey:"material_bed_temperature"; fieldValue:0; isDefault:true}
        ListElement{unitChar: "layer"; labelText: qsTr("Build Plate Temperature Initial Layer"); fieldKey:"material_bed_temperature_layer_0"; fieldValue:0; isDefault:true}
    }

    ListModel{
        id:flowItems
        ListElement{unitChar: "%"; labelText: qsTr("Flow"); fieldKey:"material_flow"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "%"; labelText: qsTr("Wall Flow"); fieldKey:"wall_material_flow"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Outer Wall Flow"); fieldKey:"wall_0_material_flow"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Inner Wall Flow"); fieldKey:"wall_x_material_flow"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Top/Bottom Flow"); fieldKey:"skin_material_flow"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Infill Flow"); fieldKey:"infill_material_flow"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Prime Tower Flow"); fieldKey:"prime_tower_flow"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Initial Layer Flow"); fieldKey:"material_flow_layer_0"; fieldValue:""; isDefault:true}
    }

    ListModel{
        id:coolingItems
        ListElement{unitChar: ""; labelText: qsTr("Enable Print Cooling"); fieldKey:"cool_fan_enabled"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "%"; labelText: qsTr("Fan Speed"); fieldKey:"cool_fan_speed"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Regular Fan Speed"); fieldKey:"cool_fan_speed_min"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Maximum Fan Speed"); fieldKey:"cool_fan_speed_max"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "s"; labelText: qsTr("Regular Maximum Fan Speed Threshold"); fieldKey:"cool_min_layer_time_fan_speed_max"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; labelText: qsTr("Initial Fan Speed"); fieldKey:"cool_fan_speed_0"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "mm"; labelText: qsTr("Regular Fan Speed At Height"); fieldKey:"cool_fan_full_at_height"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Regular Fan Speed At Layer"); fieldKey:"cool_fan_full_layer"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "s"; labelText: qsTr("Minimum Layer Time"); fieldKey:"cool_min_layer_time"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm/s"; labelText: qsTr("Min-Speed"); fieldKey:"cool_min_speed"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Lift Head"); fieldKey:"cool_lift_head"; fieldValue:""; isDefault:true}
    }

    ListModel{
        id:retractionItems
        //        ListElement{unitChar: ""; isVisible: true; isEnabled: true; labelText: qsTr("Enable Retraction"); fieldKey:"retraction_enable"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: ""; isVisible: true; isEnabled: false; labelText: qsTr("Wipe Retraction Enable"); fieldKey:"wipe_retraction_enable"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "mm/s"; isVisible: false; isEnabled: true; labelText: qsTr("Wipe length"); fieldKey:"wipe_length"; fieldValue:""; isDefault:true}
        //        ListElement{unitChar: "%"; isVisible: false; isEnabled: true; labelText: qsTr("Before Wipe Retraction Amount Percent"); fieldKey:"before_wipe_retraction_amount_percent"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; isVisible: true; isEnabled: true; labelText: qsTr("Retraction Distance"); fieldKey:"retraction_amount"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm/s"; isVisible: true; isEnabled: true; labelText: qsTr("Retraction Speed"); fieldKey:"retraction_speed"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm/s"; isVisible: true; isEnabled: true; labelText: qsTr("Retraction Prime Speed"); fieldKey:"retraction_prime_speed"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm³"; isVisible: true; isEnabled: true; labelText: qsTr("Retraction Extra Prime Amount"); fieldKey:"retraction_extra_prime_amount"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; isVisible: true; isEnabled: true; labelText: qsTr("Travel Avoid Distance"); fieldKey:"travel_avoid_distance"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm"; isVisible: true; isEnabled: true; labelText: qsTr("Retract at Layer Change"); fieldKey:"retract_at_layer_change"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm"; isVisible: true; isEnabled: true; labelText: qsTr("Retraction Minimum Travel"); fieldKey:"retraction_min_travel"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; isVisible: true; isEnabled: true; labelText: qsTr("Retract at Layer Change"); fieldKey:"retract_at_layer_change"; fieldValue:""; isDefault:true}

        ListElement{unitChar: ""; isVisible: true; isEnabled: true; labelText: qsTr("Maximum Retraction Count"); fieldKey:"retraction_count_max"; fieldValue:""; isDefault:true}

        ListElement{unitChar: "mm"; isVisible: true; isEnabled: true; labelText: qsTr("Minimum Extrusion Distance Window"); fieldKey:"retraction_extrusion_window"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "%"; isVisible: true; isEnabled: true; labelText: qsTr("Combing Mode"); fieldKey:"retraction_combing"; fieldValue:""; isDefault:true}
        ListElement{unitChar: "mm"; isVisible: true; isEnabled: true; labelText: qsTr("Max Comb Distance With No Retract"); fieldKey:"retraction_combing_max_distance"; fieldValue:""; isDefault:true}
    }

    ListModel{
        id:gCodeItems
        ListElement{unitChar: ""; labelText: qsTr("Filament start G-code"); fieldKey:"start_gCode"; fieldValue:""; isDefault:true}
        ListElement{unitChar: ""; labelText: qsTr("Filament end G-code"); fieldKey:"end_gCode"; fieldValue:""; isDefault:true}
    }

    //循环创建参数列表对应的item
    function creatItems(){

    }

    //刷新回抽的数据
    function refreshRetractionItems(){
        retractionItems.set(1, {"isEnabled": !retractionItems.get(1).isEnabled})

        retractionItems.set(2, {"isVisible": retractionItems.get(1).isEnabled})
        retractionItems.set(3, {"isVisible": retractionItems.get(1).isEnabled})

        retractionItems.set(7, {"isVisible": retractionItems.get(0).isEnabled})
        retractionItems.set(9, {"isVisible": retractionItems.get(0).isEnabled})
        retractionItems.set(10, {"isVisible": retractionItems.get(0).isEnabled})
    }

    function getIsDefaultValue(dataObj, curIndex){
        //        return materialListModel.checkExtruderKey(currentExtruder, dataObj.get(curIndex).fieldKey)
        return true
    }

    function getItemKey(dataObj, itemIndex){
        return dataObj.get(itemIndex).fieldKey
    }

    function getItemValue(dataObj, itemIndex){
        console.log("*value = ", materialParams.value(getItemKey(dataObj, itemIndex)))
        console.log("*typeOfValue = ", typeof(materialParams.value(getItemKey(dataObj, itemIndex))))

        return materialParams.value(getItemKey(dataObj, itemIndex))
    }

    function settingData(){
        attributesItems.set(0, {"fieldValue": getItemValue(attributesItems, 0),"isDefault": getIsDefaultValue(attributesItems, 0)})
        attributesItems.set(1, {"fieldValue": getItemValue(attributesItems, 1),"isDefault": getIsDefaultValue(attributesItems, 1)})
        attributesItems.set(2, {"fieldValue": getItemValue(attributesItems, 2),"isDefault": getIsDefaultValue(attributesItems, 2)})
        attributesItems.set(3, {"fieldValue": getItemValue(attributesItems, 3),"isDefault": getIsDefaultValue(attributesItems, 3)})
        attributesItems.set(4, {"fieldValue": getItemValue(attributesItems, 4),"isDefault": getIsDefaultValue(attributesItems, 4)})
        attributesItems.set(5, {"fieldValue": getItemValue(attributesItems, 5),"isDefault": getIsDefaultValue(attributesItems, 5)})
        attributesItems.set(6, {"fieldValue": getItemValue(attributesItems, 6),"isDefault": getIsDefaultValue(attributesItems, 6)})

        //        dataProView.model = attributesItems

        tempItems.set(0, {"fieldValue": getItemValue(tempItems, 0),"isDefault": getIsDefaultValue(tempItems, 0)})
        tempItems.set(1, {"fieldValue": getItemValue(tempItems, 1),"isDefault": getIsDefaultValue(tempItems, 1)})
        tempItems.set(2, {"fieldValue": getItemValue(tempItems, 2),"isDefault": getIsDefaultValue(tempItems, 2)})
        tempItems.set(3, {"fieldValue": getItemValue(tempItems, 3),"isDefault": getIsDefaultValue(tempItems, 3)})
        tempItems.set(4, {"fieldValue": getItemValue(tempItems, 4),"isDefault": getIsDefaultValue(tempItems, 4)})
        tempItems.set(5, {"fieldValue": getItemValue(tempItems, 5),"isDefault": getIsDefaultValue(tempItems, 5)})

        flowItems.set(0, {"fieldValue": getItemValue(flowItems, 0),"isDefault": getIsDefaultValue(flowItems, 0)})
        flowItems.set(1, {"fieldValue": getItemValue(flowItems, 1),"isDefault": getIsDefaultValue(flowItems, 1)})
        flowItems.set(2, {"fieldValue": getItemValue(flowItems, 2),"isDefault": getIsDefaultValue(flowItems, 2)})
        flowItems.set(3, {"fieldValue": getItemValue(flowItems, 3),"isDefault": getIsDefaultValue(flowItems, 3)})
        flowItems.set(4, {"fieldValue": getItemValue(flowItems, 4),"isDefault": getIsDefaultValue(flowItems, 4)})
        flowItems.set(5, {"fieldValue": getItemValue(flowItems, 5),"isDefault": getIsDefaultValue(flowItems, 5)})
        flowItems.set(6, {"fieldValue": getItemValue(flowItems, 6),"isDefault": getIsDefaultValue(flowItems, 6)})
        flowItems.set(7, {"fieldValue": getItemValue(flowItems, 7),"isDefault": getIsDefaultValue(flowItems, 7)})

        coolingItems.set(0, {"fieldValue": getItemValue(coolingItems, 0),"isDefault": getIsDefaultValue(coolingItems, 0)})
        coolingItems.set(1, {"fieldValue": getItemValue(coolingItems, 1),"isDefault": getIsDefaultValue(coolingItems, 1)})
        coolingItems.set(2, {"fieldValue": getItemValue(coolingItems, 2),"isDefault": getIsDefaultValue(coolingItems, 2)})
        coolingItems.set(3, {"fieldValue": getItemValue(coolingItems, 3),"isDefault": getIsDefaultValue(coolingItems, 3)})
        coolingItems.set(4, {"fieldValue": getItemValue(coolingItems, 4),"isDefault": getIsDefaultValue(coolingItems, 4)})
        coolingItems.set(5, {"fieldValue": getItemValue(coolingItems, 5),"isDefault": getIsDefaultValue(coolingItems, 5)})
        coolingItems.set(6, {"fieldValue": getItemValue(coolingItems, 6),"isDefault": getIsDefaultValue(coolingItems, 6)})
        coolingItems.set(7, {"fieldValue": getItemValue(coolingItems, 7),"isDefault": getIsDefaultValue(coolingItems, 7)})
        coolingItems.set(8, {"fieldValue": getItemValue(coolingItems, 8),"isDefault": getIsDefaultValue(coolingItems, 8)})
        coolingItems.set(9, {"fieldValue": getItemValue(coolingItems, 9),"isDefault": getIsDefaultValue(coolingItems, 9)})

        retractionItems.set(0, {"fieldValue": getItemValue(retractionItems, 0),"isDefault": getIsDefaultValue(retractionItems, 0)})
        retractionItems.set(1, {"fieldValue": getItemValue(retractionItems, 1),"isDefault": getIsDefaultValue(retractionItems, 1)})

        gCodeItems.set(0, {"fieldValue": getItemValue(gCodeItems, 0),"isDefault": getIsDefaultValue(gCodeItems, 0)})
        gCodeItems.set(1, {"fieldValue": getItemValue(gCodeItems, 1),"isDefault": getIsDefaultValue(gCodeItems, 1)})
    }
}

