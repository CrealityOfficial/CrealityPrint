import QtQuick 2.10

Item {
	id : contex
	property var settings : null
	property var currentProfile
	function infillByName(shapeName){
		var imgPath;
		if(shapeName === "Concentric")//同心圆
			imgPath = "qrc:/UI/photo/Infill/Concentric.png"
		if(shapeName === "Onewall")//单壁
			imgPath = "qrc:/UI/photo/Infill/Onewall.png"
		if(shapeName === "Grid")//网格
			imgPath = "qrc:/UI/photo/Infill/Grid.png"
		if(shapeName === "Triangles")//三角形
			imgPath = "qrc:/UI/photo/Infill/Triangles.png"
		if(shapeName === "Tri-Hexagon")//内六角
			imgPath = "qrc:/UI/photo/Infill/Tri-Hexagon.png"
		if(shapeName === "Cubic")//立方体
			imgPath = "qrc:/UI/photo/Infill/Cubic.png"
		if(shapeName === "Cubic Subdivision")//立方体分区
			imgPath = "qrc:/UI/photo/Infill/Cubic Subdivision.png"
		if(shapeName === "Octet")//八角形
			imgPath = "qrc:/UI/photo/Infill/Octet.png"
		if(shapeName === "Quarter Cubic")//四面体
			imgPath = "qrc:/UI/photo/Infill/Quarter Cubic.png"
		if(shapeName === "Cross")//交叉
			imgPath = "qrc:/UI/photo/Infill/Cross.png"
		if(shapeName === "Cross 3D")//交叉3D
			imgPath = "qrc:/UI/photo/Infill/Cross 3D.png"
		if(shapeName === "Gyroid")//螺旋24面体
			imgPath = "qrc:/UI/photo/Infill/Gyroid.png"
		if(shapeName === "Lines")//直线
			imgPath = "qrc:/UI/photo/Infill/Lines.png"
		if(shapeName === "Zig Zag")//锯齿状
			imgPath = "qrc:/UI/photo/Infill/Zig Zag.png"
		if(shapeName === "Honeycomb")//蜂窝状
			imgPath = "qrc:/UI/photo/Infill/Honeycomb.png"
        if(shapeName === "Lightning")//闪电
            imgPath = "qrc:/UI/photo/Infill/Lightning.png"
        if(shapeName === "Aligned Rectilinear")//直线排列
            imgPath = "qrc:/UI/photo/Infill/Aligned Rectilinear.svg"
	
		return imgPath;
	}
	
	function createStyledLabel(setting, itemParent, w, h){
		var componentLabel = Qt.createComponent("../qml/StyledLabel.qml")
		if (componentLabel.status === Component.Ready) {
			var obj = componentLabel.createObject(itemParent, {"width": w * screenScaleFactor,
														"height" : h * screenScaleFactor,
														"text": qsTr(setting.label()),
														"strToolTip": qsTr(setting.description()),
														"verticalAlignment": Qt.AlignVCenter,
														"horizontalAlignment" : Qt.AlignRight})
			return obj
		}
		return null
	}
	
	function createCheckBox(setting, itemParent, w, h){
		var componentCheckbox = Qt.createComponent("../qml/StyleCheckBox.qml")
		if (componentCheckbox.status === Component.Ready)
		{
			var key = setting.key()
			var obj = componentCheckbox.createObject(itemParent, {"width": w * screenScaleFactor,
															"height" : h * screenScaleFactor,
															"checked": setting.enabled(),
															"strToolTip": qsTr(setting.description()),
															"keyStr": key,
															"id": key})

			obj.styleCheckChanged.connect(textFieldEditFinish)
			return obj
		}
		return null
	}
	
	Component {
		id: someComponent
		ListModel {
		}
	}
				
	function createEnumListModel(key){
		console.log("createEnumListModel++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++:"+key);
		var modelMap = settings.settingObject(key).enums()
		var newModel = someComponent.createObject()
		newModel.clear()
		for(var i =0;i<modelMap.length;i++)
		{
			var modelKey = modelMap[i]
			var imgPath = ""
			var kv = modelKey.split('=');
			if(kv.length==2)
			{
				imgPath = infillByName(kv[1])
				newModel.append({"key":kv[0], "modelData":qsTr(kv[1]), "shapeImg":imgPath})
				console.log("createEnumListModel:"+kv[0]+":"+kv[1])
			}
		}
		
		return newModel
	}
	
	function createCombobox(setting, itemParent, w, h){
		var modelMap = setting.enums()
		var newModel = someComponent.createObject(this)
		newModel.clear()
		for(var modelKey in modelMap){
			var imgPath = ""
			imgPath = infillByName(modelMap[modelKey])
            newModel.append({"key":modelKey, "modelData":qsTranslate("ParameterContext", modelMap[modelKey]), "shapeImg":imgPath})
		}
		
		var key = setting.key()
		var currentIdx = setting.enumIndex()
		
		var componentCombobox = Qt.createComponent("../qml/BasicCombobox.qml")
		if (componentCombobox.status === Component.Ready)
		{	
			var obj = componentCombobox.createObject(itemParent, {"width": w * screenScaleFactor,
																"height" : h * screenScaleFactor,
																"currentIndex": currentIdx,
																"model": newModel,
																"strToolTip": qsTr(setting.description()),
																"keyStr": key,
																"textRole": "modelData",
																"hasImg": false,
																"showCount": 10}
																)

			obj.styleComboBoxIndexChanged.connect(textFieldEditFinish)
			return obj
		}
		return null
	}
	
	function createTextField(setting, itemParent, w, h){
		var componentTextField = Qt.createComponent("../secondqml/BasicDialogTextField.qml")
		if (componentTextField.status === Component.Ready)
		{
			var key = setting.key()
			var obj = componentTextField.createObject(itemParent, {"width": w * screenScaleFactor,
																"height": h * screenScaleFactor,
																"onlyPositiveNum": false,
																"text": setting.str(),
																"errorCode": 0,
																"unitChar": setting.unit(),
																"strToolTip": qsTr(setting.description()),
																"id": key,
																"keyStr": key}
																)

			obj.styleTextFieldFinished.connect(textFieldTextChanged)
			obj.styleTextFieldChanged.connect(textFieldEditFinish)
			return obj
		}
		return null
	}
	
	function createLabelItemFromList(keyList, parentItem, labelContainer, itemContainer){
		for(var i in keyList)
		{
			var key = keyList[i]
			var setting = settings.settingObject(key)
			var type = setting.type()
			if(type == "bool")
			{
				labelContainer[key] = createStyledLabel(setting, parentItem, 160, 30)
				itemContainer[key] = createCheckBox(setting, parentItem, 120, 30)
			}
			else if(type == "enum" || type == "optional_extruder" || type == "extruder")
			{
				labelContainer[key] = createStyledLabel(setting, parentItem, 160, 30)
				itemContainer[key] = createCombobox(setting, parentItem, 120, 30)
			}
			else{
				labelContainer[key] = createStyledLabel(setting, parentItem, 160, 30)
				itemContainer[key] = createTextField(setting, parentItem, 120, 30)
			}
		}
	}
	function resetValue(key)
	{
		currentProfile.resetValue(key);
	}
	function textFieldEditFinish(key, item){
        console.log("textFieldEditFinish++++++++++++++++++++++++++++++++=",key);
		var setting = settings.settingObject(key)
		var type = setting.type()
		if(type == "bool"){
			var value = item.checked.toString()
			setValue(key, value, setting)
		}
		else if(type == "enum" || type == "optional_extruder" || type == "extruder"){
			var value = item.model.get(item.currentIndex).key
            console.log("value =",value)
			setValue(key, value, setting)
		}
		else{
			if(setValue(key, item.text, setting))
			{
                item.defaultBackgroundColor = "transparent"  // Constants.dialogItemRectBgColor
			}
			else
			{
				item.defaultBackgroundColor = "red"
			}
		}
	}
	
	function textFieldTextChanged(key, item){
	}
	
	function checkValue(key, value, setting){
		if(value == ""){
			return false
		}
		
		var type = setting.type()	
		if(type == "float" || type == "int")
		{
			var min_str = setting.minStr()
			var max_str = setting.maxStr()
			console.log("checkValue min: " + key + " " + min_str)
			console.log("checkValue max: " + key + " " + max_str)
			
			var min = eval(min_str)
			console.log("checkValue min value: " + min )
			var max = eval(max_str)
		
			if(min != "" && Number(value) < Number(min).toFixed(3)){
				return false
			}
			else if(max != "" && Number(value) > Number(max).toFixed(3)){
				return false
			}
			else{
				return true
			}
		}else{
			return true
		}
	}
	function setOverideValue(key,value)
	{
		if(value == ""){
			return false
		}
		currentProfile.setOverideValue(key,value);
	}
	function realSetValue(key, value, setting){
		if(value == ""){
			return false
		}
		
		//var type = setting.type()	
		//if(type == "float"){
		//	value = Number(value).toFixed(2)
		//}
		//else if(type == "int"){
		//	value = Number(value).toFixed()
		//}
		//else if(type == "bool"){
		//	value = 
		//}
		
		console.log("realSetValue key :" + key + " " + value)
		currentProfile.setValue(key,value);
		//currentProfile.reCalculateSetting(key)
	}
	
	function setValue(key, value, setting){
        if(checkValue(key, value, setting)){
            realSetValue(key,value, setting)
            return true
        }else{
            return false
        }
	}
	
	function updateItems(itemList){
		for(var key in itemList){
			var item = itemList[key]
			var setting = settings.settingObject(key)
			updateItem(item, setting)
		}
	}
	
	function updateItem(item, setting){
		var type = setting.type()
		if(type == "bool"){
			item.checked = setting.enabled()
		}
		else if(type == "enum" || type == "optional_extruder" || type == "extruder"){
			item.currentIndex = setting.enumIndex()
		}
		else{
			item.text = setting.str()
			if(checkValue(setting.key(), item.text, setting))
			{
                item.defaultBackgroundColor = "transparent" // Constants.dialogItemRectBgColor
			}
			else
			{
				item.defaultBackgroundColor = "red"
			}
		}
	}
	
	function value(key){
		//var setting = settings.settingObject(key)
		return currentProfile.value(key);//setting.str()
	}

	function updateSettingKey(context, key){

	}
}
