//pragma Singleton
import QtQuick 2.10

QtObject {
	property var jsonMaps;
	property var globalSettingKeys;
	property var classListLength : 0;
	property var paramListLength : 0;

	//一般参数
	property var layer_height: 0
	property var line_width: 0
	property var wall_line_count: 0
	property var infill_sparse_density: 0
	property var material_print_temperature: 0
	property var material_bed_temperature: 0
	property var support_enable: 0
	property var prime_tower_enable: 0
	property var zig_zaggify_infill: 0
	property var support_angle: 0
	property var min_skin_width_for_expansion: 0
	property var min_infill_area: 0
	property var infill_before_walls: 0
	property var infill_wipe_dist: 0
	property var adhesion_type: 0
	property var support_infill_rate: 0
	property var small_feature_max_length: 0
	property var speed_print:0
	property var speed_infill:0
	property var speed_wall:0
	property var support_material_flow: 0
	property var infill_material_flow: 0
	property var wall_material_flow: 0
	property var small_feature_speed_factor:0
	property var minimum_support_area:0

	Component.onCompleted:{
		jsonMaps = actionCommands.getJsonMap()
		initSettingData()
	}

	function updateSettingData(){
		//一般
		layer_height = getSettingValue("layer_height")
		line_width = getSettingValue("line_width")
		wall_line_count = getSettingValue("wall_line_count")
		infill_sparse_density = getSettingValue("infill_sparse_density")
		material_print_temperature = getSettingValue("material_print_temperature")
		material_bed_temperature = getSettingValue("material_bed_temperature")
		support_enable = getSettingValue("support_enable")
		prime_tower_enable = getSettingValue("prime_tower_enable")
		zig_zaggify_infill = getSettingValue("zig_zaggify_infill")
		support_angle = getSettingValue("support_angle")
		min_skin_width_for_expansion = getSettingValue("min_skin_width_for_expansion")
		min_infill_area = getSettingValue("min_infill_area")
		infill_before_walls = getSettingValue("infill_before_walls")
		infill_wipe_dist = getSettingValue("infill_wipe_dist")
		adhesion_type = getSettingValue("adhesion_type")
		support_infill_rate = getSettingValue("support_infill_rate")
		small_feature_max_length = getSettingValue("small_feature_max_length")
		speed_print = getSettingValue("speed_print")
		speed_infill = getSettingValue("speed_infill")
		speed_wall = getSettingValue("speed_wall")
		support_material_flow = getSettingValue("support_material_flow")
		infill_material_flow = getSettingValue("infill_material_flow")
		wall_material_flow = getSettingValue("wall_material_flow")
		small_feature_speed_factor = getSettingValue("small_feature_speed_factor")
		minimum_support_area = getSettingValue("minimum_support_area")
	}
	
	function initSettingData()
	{
		//一般
		layer_height = initSettingValue("layer_height")
		line_width = initSettingValue("line_width")
		wall_line_count = initSettingValue("wall_line_count")
		infill_sparse_density = initSettingValue("infill_sparse_density")
		material_print_temperature = initSettingValue("material_print_temperature")
		material_bed_temperature = initSettingValue("material_bed_temperature")
		support_enable = initSettingValue("support_enable")
		prime_tower_enable = initSettingValue("prime_tower_enable")
		zig_zaggify_infill = initSettingValue("zig_zaggify_infill")
		support_angle = initSettingValue("support_angle")
		min_skin_width_for_expansion = initSettingValue("min_skin_width_for_expansion")
		min_infill_area = initSettingValue("min_infill_area")
		infill_before_walls = initSettingValue("infill_before_walls")
		infill_wipe_dist = initSettingValue("infill_wipe_dist")	
		adhesion_type = initSettingValue("adhesion_type")
		support_infill_rate = initSettingValue("support_infill_rate")
		small_feature_max_length = initSettingValue("small_feature_max_length")
		speed_print = initSettingValue("speed_print")
		speed_infill = initSettingValue("speed_infill")
		speed_wall = initSettingValue("speed_wall")
		support_material_flow = initSettingValue("support_material_flow")
		infill_material_flow = initSettingValue("infill_material_flow")
		wall_material_flow = initSettingValue("wall_material_flow")
		small_feature_speed_factor = initSettingValue("small_feature_speed_factor")
		minimum_support_area = initSettingValue("minimum_support_area")
	}
	
	function updateGlobalSettingValue()
	{
		console.log("----updateGlobalSettingValue beg")
		globalSettingKeys = actionCommands.getGlobalSettingKey()
		for(var key in globalSettingKeys)
		{
			initSettingValue(globalSettingKeys[key])
		}
		
		for(var key in globalSettingKeys)
		{
			getSettingValue(globalSettingKeys[key])
		}
		
		console.log("----updateGlobalSettingValue end")
	}
	
	function getSpecialKeys()
	{
		console.log("getSpecialKeys beg")
		var keyList = actionCommands.getSpecialSettingKey()
		for(var key in keyList)
		{
			initSettingValue(keyList[key])
		}
		return keyList
	}

	function getClassTypeList()
	{
		console.log("getClassType beg")
		var keyList = actionCommands.getClassTypeList()
		classListLength = keyList.length
		return keyList
	}

	function getAllSettingKeyList()
	{
		console.log("getAllSettingKeyList beg")
		var keyList = actionCommands.getAllSettingKey()
		paramListLength = keyList.length
		return keyList
	}
	
	function initSettingValue(key)
	{
		var rst
		var showvale
		var value

		var type = jsonMaps[key].getType();
		showvale = jsonMaps[key].getShowValue();
		if(type == "enum" || type == "[int]" || type == "str" || type == "polygons")
		{
			value = jsonMaps[key].getSourceValue();
			if(value.indexOf("eval") != -1 || value.indexOf("getEnumValue") != -1)
			{
				value = eval(value)
			}
		}
		else
		{
			value = eval(jsonMaps[key].getSourceValue());
		}
		
		if(type == "float")
		{
			showvale = Number(showvale).toFixed(2)
			value = Number(value).toFixed(2)
		}
		else if(type == "int")
		{
			showvale = Number(showvale).toFixed()
			value = Number(value).toFixed()
		}
		else if(type == "bool")
		{	
			if(showvale === "true" || showvale === true)
			{
				showvale = true
			}
			else
			{
				showvale = false
			}
			
			if(value === "true" || value === true)
			{
				value = true
			}
			else
			{
				value = false
			}
		}
		
		var oldShowValue = jsonMaps[key].getShowValue()
		if(showvale != value && oldShowValue.length != 0)
		{
			rst = showvale
			jsonMaps[key].setIsCustomSetting(true)
		}
		else
		{
			rst = value
			jsonMaps[key].setIsCustomSetting(false)
		}
		return rst;
	}
	
	function getSettingValue(key)
	{
		var rst
		var showvale
		var value
		
		var type = jsonMaps[key].getType();
		showvale = jsonMaps[key].getShowValue();
		if(type == "enum" || type == "[int]" || type == "str" || type == "polygons")
		{
			value = jsonMaps[key].getValue();

			if(value.indexOf("eval") != -1 || value.indexOf("getEnumValue") != -1)
			{
				value = eval(value)
			}
		}
		else
		{
			value = eval(jsonMaps[key].getValue());
		}

		if(type == "float")
		{
			showvale = Number(showvale).toFixed(2)
			value = Number(value).toFixed(2)
		}
		else if(type == "int")
		{
			showvale = Number(showvale).toFixed()
			value = Number(value).toFixed()
		}
		else if(type == "bool")
		{
			if(showvale === "true" || showvale === true)
			{
				showvale = true
			}
			else
			{
				showvale = false
			}
			
			if(value === "true" || value === true)
			{
				value = true
			}
			else
			{
				value = false
			}
		}
		
		if(jsonMaps[key].getIsCustomSetting())
		{
			rst = showvale
		}
		else
		{
			rst = value
		}

		jsonMaps[key].setValue(key,rst)
		return rst;
	}
	
	function getSettingOptions(key)
	{
		var type = jsonMaps[key].getType();
		var rst;
		if(type == "enum" || type == "optional_extruder" || type == "extruder")
		{
			rst = eval(jsonMaps[key].getOptions());
		}
		
		return rst;
	}
	
	function getSettingEnable(key)
	{	
		var rst = eval(jsonMaps[key].getEnableStatus());
		
		if(rst === "")
		{
			console.log("----getSettingEnable getSettingEnable is empty ")
			return true
		}
		return rst;
	}
	
	function checkSettingValue(key,value)
	{
		if(value == "")
		{
			return false;
		}
		
		var type = jsonMaps[key].getType();
		var min = eval(jsonMaps[key].getMin());
		var max = eval(jsonMaps[key].getMax());
		
		if(type == "float" || type == "int")
		{
			if(min != "" && Number(value) < Number(min).toFixed(3))
			{
				return false
			}
			else if(max != "" && Number(value) > Number(max).toFixed(3))
			{
				return false
			}
			else
			{
				return true
			}
		}
		else
		{
			return true
		}
	}
	
	function getMinValue(key)
	{
		var max = eval(jsonMaps[key].getMax());
		return max;
	}
	
	function getMaxValue(key)
	{
		var min = eval(jsonMaps[key].getMin());
		return min;
	}
	
	function setSettingValue(key,value)
	{
		var valuecale;
		var type = jsonMaps[key].getType();
		if(type == "float")
		{
			valuecale = eval(jsonMaps[key].getSourceValue());
			valuecale = Number(valuecale).toFixed(2)
			value = Number(value).toFixed(2)
		}
		else if(type == "int")
		{
			valuecale = eval(jsonMaps[key].getSourceValue());
			valuecale = Number(valuecale).toFixed()
			value = Number(value).toFixed()
		}
		else
		{
			valuecale = jsonMaps[key].getSourceValue()

			if(valuecale.indexOf("eval") != -1 || valuecale.indexOf("getEnumValue") != -1)
			{
				valuecale = eval(valuecale)
			}

			if(type == "bool")
			{
				if(valuecale === "true" || valuecale === true)
				{
					valuecale = true
				}
				else
				{
					valuecale = false
				}
				
				if(value === "true" || value === true)
				{
					value = true
				}
				else
				{
					value = false
				}
			}
		}
		
		if(key == "skin_angles")
		{
			console.log("----setSettingValue valuecale=" + valuecale)
			console.log("----setSettingValue value=" + value)
		}
		//console.log("----setSettingValue valuecale=" + valuecale)
		//console.log("----setSettingValue value=" + value)
		
		if(valuecale != value)
		{
			jsonMaps[key].setIsCustomSetting(true)
		}
		else
		{
			jsonMaps[key].setIsCustomSetting(false)
		}
		
		jsonMaps[key].setValue(key,value);
	}
	
	function getValueByInfillPattern()
	{
		var value_infill_pattern = jsonMaps["infill_pattern"].getShowValue();
		var temp = 0
		if(value_infill_pattern == 'grid' || value_infill_pattern == 'tetrahedral' || value_infill_pattern == 'quarter_cubic')
		{
			temp = 2
		}
		else if(value_infill_pattern == 'triangles' || value_infill_pattern == 'trihexagon' || value_infill_pattern == 'cubic' || value_infill_pattern == 'cubicsubdiv')
		{
			temp = 3
		}
		else
		{
			temp = 1
		}
		return temp;
	}
	
	function findIndexByType(modelMap, type)
	{
		var index = 0
		for(var key in modelMap){
            if(modelMap[key] == modelMap[type])
            {
                break
            }
			index++
        }
		return index
	}
	
	function getEnumValue(key)
	{
		var value = jsonMaps[key].getValue();

		if(value.indexOf("eval") != -1)
		{
			value = eval(value)
		}
		
		return value
	}
	
	function getSettingType(key)
	{
		return jsonMaps[key].getType()
	}
	
	function getLabel(key)
	{
		return jsonMaps[key].getlabel()
	}
	
	function getDescription(key)
	{
		return jsonMaps[key].getDescription()
	}
	
	function getUnitStr(key)
	{
		return jsonMaps[key].getUnit()
	}
	
	function deleteSpecialSetting()
	{
		actionCommands.deleteSpecialSetting()
	}

	function getClassTypeBykey(key)
	{
		return jsonMaps[key].getclasstypevalue()
	}

	function getParamLevelBykey(key)
	{
		return jsonMaps[key].getParamLevel()
	}
	  
}
