import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
//import CrealityUI 1.0
//import "qrc:/CrealityUI"
import ".."
import "../qml"

BasicDialog
{
    id: idEditProfileDialog

    width: 700 * screenScaleFactor
    height: 670 * screenScaleFactor
    title: qsTr("Edit")
    //titleHeight : 30
    property int spacing: 5

    property var labelMap: {0:0}
    property var textBoxMap: {0:0}
    property var buttonMap: {0:0}

    property var labelMapCnt: 1
    property var buttonMapCnt: 1

    property var translateMap: {0:0}
    property var classTypeFilter: "Quality"
    property var nameFilter: ""
    property var printModeFilter: ""
    property string profileName:qsTr("new")

    signal valchangedAd(string key,string value)
    signal resetedAd()
    signal saveProfileAd()


    property var isReseting : false
	
    function findTranslate(key)
    {
        var quality = ""
        var tempArray = key.split("_")
        quality = tempArray[tempArray.length-1]
        var tranlateValue = ""
        if(quality === "high")tranlateValue = qsTr("High Quality")
        else if(quality === "middle") tranlateValue= qsTr("Quality1")
        else if(quality === "low")tranlateValue = qsTr("Normal")
        else if(quality === "best")tranlateValue = qsTr("Best quality")
        else if(quality === "fast")tranlateValue = qsTr("Fast")
        else
        {
            tranlateValue = key
        }
        return tranlateValue
    }


    function editProfiledchanged(key,value)
    {
        if(SettingJson.checkSettingValue(key,value))
        {
            if ("support_enable" == key && "false" == value)
            {
               //console.log("support_enable == false")
               valchangedAd("support_tree_enable","false")
               SettingJson.setSettingValue("support_tree_enable","false")
            }
            valchangedAd(key,value)
            SettingJson.setSettingValue(key,value)
            if(!isReseting)
            {
                updateComponent()
            }
            return true
        }
        else
        {
            return false
        }
    }

    Component {
        id: someComponent
        ListModel {
        }
    }

    function getCreateFinished()
    {
        if(Object.keys(buttonMap).length-1 == SettingJson.classListLength && Object.keys(labelMap).length-1 == SettingJson.paramListLength)
        {
            return true
        }
        else
        {
            return false
        }
    }

    function initClassTypeList()
    {
        var keyList = SettingJson.getClassTypeList()
        for(var i in keyList)
        {
            var key = keyList[i]
            async_createButton(key)
        }

    }

    function async_createButton(key)
    {
        var finishCreation = function()
        {
            if (componentButton.status === Component.Ready)
            {

                var obj1 = componentButton.incubateObject(profileTypeBtnList, {"width": 120*screenScaleFactor,
                                                              "height" : 30*screenScaleFactor,
                                                              "btnRadius" : 3,
                                                              "keyStr" : key,
                                                              "pointSize":Constants.labelFontPointSize_9,
                                                              "btnBorderW":0,
                                                              "defaultBtnBgColor": Constants.dialogContentBgColor,
                                                              "hoveredBtnBgColor": Constants.typeBtnHoveredColor,
                                                              "text": qsTr(key)})
                if (obj1.status !== Component.Ready)
                {
                    obj1.onStatusChanged = function(status)
                    {
                        if (status === Component.Ready)
                        {
                            obj1.object.sigButtonClickedWithKey.connect(onClassTypeButtonClicked)
                            buttonMap[key] = obj1.object
                        }
                    }
                }
                else
                {
                    obj1.object.sigButtonClickedWithKey.connect(onClassTypeButtonClicked)
                    buttonMap[key] = obj1.object
                }
            }
        }

        var componentButton = Qt.createComponent("../qml/BasicButton.qml", Component.Asynchronous);
        if (componentButton.status === Component.Ready)
        {
            finishCreation();
        }
        else
        {
            componentButton.statusChanged.connect(finishCreation);
        }
    }

    function async_createStyledLabel(key)
    {
        var finishCreation = function()
        {
            if (componentLabel.status === Component.Ready)
            {
                var obj1 = componentLabel.incubateObject(profileValuelList, {"width": 260*screenScaleFactor,
                                                             "height" : 30*screenScaleFactor,
                                                             "text": qsTr(labelString),
                                                             "strToolTip": qsTr(labelToolTip),
                                                             "verticalAlignment": Qt.AlignVCenter,
                                                             "horizontalAlignment" : Qt.AlignRight})
                if (obj1.status !== Component.Ready)
                {
                    obj1.onStatusChanged = function(status)
                    {
                        if (status === Component.Ready)
                        {
                            labelMap[key] = obj1.object
                        }
                    }
                }
                else
                {
                    labelMap[key] = obj1.object
                }
            }
        }

        var componentLabel = Qt.createComponent("../qml/StyledLabel.qml", Component.Asynchronous)
        var comType = SettingJson.getSettingType(key)
        var labelString = SettingJson.getLabel(key)
        var labelToolTip = SettingJson.getDescription(key)
        if (componentLabel.status === Component.Ready)
        {
            finishCreation();
        }
        else
        {
            componentLabel.statusChanged.connect(finishCreation);
        }
    }

    function async_createCheckBox(key)
    {
        var finishCreation = function()
        {
            if (componentCheckbox.status === Component.Ready)
            {

                var obj1 = componentCheckbox.incubateObject(profileValuelList, {"width": 129*screenScaleFactor,
                                                                "height" : 30*screenScaleFactor,
                                                                "checked": checkBoxValue,
                                                                "strToolTip": qsTr(labelToolTip),
                                                                "keyStr": key,
                                                                "id": key})
                if (obj1.status !== Component.Ready)
                {
                    obj1.onStatusChanged = function(status)
                    {
                        if (status === Component.Ready)
                        {
                            obj1.object.styleCheckedChanged.connect(textFieldEditFinish)
                            textBoxMap[key] = obj1.object
                        }
                    }
                }
                else
                {
                    obj1.object.styleCheckedChanged.connect(textFieldEditFinish)
                    textBoxMap[key] = obj1.object
                }
            }
        }

        var componentCheckbox = Qt.createComponent("../qml/StyleCheckBox.qml", Component.Asynchronous)
        var checkBoxValue = SettingJson.initSettingValue(key)
        var labelToolTip = SettingJson.getDescription(key)
        if (componentCheckbox.status === Component.Ready)
        {
            finishCreation();
        }
        else
        {
            componentCheckbox.statusChanged.connect(finishCreation);
        }
    }

    function async_createCombobox(key)
    {
        var finishCreation = function()
        {
            if (componentCombobox.status === Component.Ready)
            {
                var hasImg
                if(key === "infill_pattern")
                    hasImg = true
                else
                    hasImg = false
                var obj1 = componentCombobox.incubateObject(profileValuelList, {"width": 180*screenScaleFactor,
                                                                "height" : 30*screenScaleFactor,
                                                                "currentIndex": currentIdx,
                                                                "model": newModel,
                                                                "strToolTip": qsTr(labelToolTip),
                                                                "keyStr": key,
                                                                "id": key,
                                                                "textRole": "modelData",
                                                                "hasImg": hasImg,
                                                            "showCount": 10})
                if (obj1.status !== Component.Ready)
                {
                    obj1.onStatusChanged = function(status)
                    {
                        if (status === Component.Ready)
                        {
                            obj1.object.comboBoxIndexChanged.connect(textFieldEditFinish)
                            textBoxMap[key] = obj1.object
                        }
                    }
                }
                else
                {
                    obj1.object.comboBoxIndexChanged.connect(textFieldEditFinish)
                    textBoxMap[key] = obj1.object
                }
            }
        }

        var componentCombobox = Qt.createComponent("../qml/BasicCombobox.qml", Component.Asynchronous)
        var modelMap = SettingJson.getSettingOptions(key)
        var newModel = someComponent.createObject(parent)
        var labelToolTip = SettingJson.getDescription(key)
        newModel.clear()
        for(var modelKey in modelMap){
            var imgPath = ""
            imgPath = infillByName(modelMap[modelKey])
            newModel.append({"key":modelKey, "modelData":qsTr(modelMap[modelKey]), "shapeImg":imgPath})
        }

        var currentType = SettingJson.initSettingValue(key)
        var currentIdx = SettingJson.findIndexByType(modelMap,currentType)
        if (componentCombobox.status === Component.Ready)
        {
            finishCreation();
        }
        else
        {
            componentCombobox.statusChanged.connect(finishCreation);
        }
    }

    function async_createTextField(key)
    {
        var finishCreation = function()
        {
            if (componentTextField.status === Component.Ready)
            {
                var obj1 = componentTextField.incubateObject(profileValuelList, {"width": 180*screenScaleFactor,
                                                                 "height": 30*screenScaleFactor,
                                                                 "onlyPositiveNum": false,
                                                                 "text": textFieldValue,
                                                                 "errorCode": errorcode,
                                                                 "unitChar": textUnit,
                                                                 "strToolTip": qsTr(labelToolTip),
                                                                 "id": key,
                                                                 "keyStr": key})

                if (obj1.status !== Component.Ready)
                {
                    obj1.onStatusChanged = function(status)
                    {
                        if (status === Component.Ready)
                        {
                            obj1.object.textFieldChanged.connect(textFieldTextChanged)
                            obj1.object.editingFieldFinished.connect(textFieldEditFinish)

                            if(comType == "[int]")
                            {
                                obj1.object.limitEnable = false
                            }

                            textBoxMap[key] = obj1.object
                        }
                    }
                }
                else
                {
                    obj1.object.textFieldChanged.connect(textFieldTextChanged)
                    obj1.object.editingFieldFinished.connect(textFieldEditFinish)
                    textBoxMap[key] = obj1.object
                }
            }
        }

        var componentTextField = Qt.createComponent("../secondqml/BasicDialogTextField.qml")
        var textFieldValue = SettingJson.initSettingValue(key)
        var textUnit = SettingJson.getUnitStr(key)
        var labelToolTip = SettingJson.getDescription(key)
        var comType = SettingJson.getSettingType(key)
        var errorcode = 0

        if (componentTextField.status === Component.Ready)
        {
            finishCreation();
        }
        else
        {
            componentTextField.statusChanged.connect(finishCreation);
        }
    }


    //function showInit()
    function initProfileValueList()
    {
        console.log("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~initProfileValueList() beg~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
        var keyList = SettingJson.getAllSettingKeyList()
        console.log("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~initProfileValueList() keyList.length:" + keyList.length )
        for(var i in keyList)
        {
            var key = keyList[i]
            var comType = SettingJson.getSettingType(key)

            if(comType == "bool")
            {
                async_createCheckBox(key)
                async_createStyledLabel(key)
            }
            else if(comType == "enum" || comType == "optional_extruder" || comType == "extruder")
            {
                async_createCombobox(key)
                async_createStyledLabel(key)
            }
            else{
                async_createTextField(key)
                async_createStyledLabel(key)
            }
        }
    }

    function filterprofileTypeList()
    {
        var strArray = []
        for(var key in labelMap)
        {
            if(key == 0)
            {
                continue
            }

            var classtype = SettingJson.getClassTypeBykey(key)
            var lable = labelMap[key].text.toLowerCase()
            var paramlevel = Number(SettingJson.getParamLevelBykey(key))
            if(nameFilter == "")
            {
                if((idConfiguration.currentIndex + 1) >= paramlevel && textBoxMap[key].enabled)
                {
                    strArray.push(classtype)
                }
            }
            else
            {
                if(lable.search(nameFilter.toLowerCase()) != -1 && (idConfiguration.currentIndex + 1) >= paramlevel && textBoxMap[key].enabled)
                {
                    strArray.push(classtype)
                }
            }

        }

        for(var key in buttonMap)
        {
            if(key == 0)
            {
                continue
            }
            buttonMap[key].visible = false
        }

        for(var key in strArray)
        {
            buttonMap[strArray[key]].visible = true
        }

    }

    function filterProfileValueList()
    {
        var roscnt = 1
        for(var key in labelMap)
        {
            if(key == 0)
            {
                continue
            }

            var classtype = SettingJson.getClassTypeBykey(key)
            var paramlevel = Number(SettingJson.getParamLevelBykey(key))
            if(classTypeFilter == classtype && (idConfiguration.currentIndex + 1) >= paramlevel)
            {
                if(nameFilter == "")
                {
                    labelMap[key].visible = true && textBoxMap[key].enabled
                    textBoxMap[key].visible = true && textBoxMap[key].enabled
                    if(textBoxMap[key].enabled)
                    {
                        roscnt++
                    }
                }
                else
                {
                    var lable = labelMap[key].text.toLowerCase()
                    if(lable.search(nameFilter.toLowerCase()) != -1)
                    {
                        //包含关键字
                        labelMap[key].visible = true && textBoxMap[key].enabled
                        textBoxMap[key].visible = true && textBoxMap[key].enabled
                        if(textBoxMap[key].enabled)
                        {
                            roscnt++
                        }
                    }
                    else
                    {
                        labelMap[key].visible = false && textBoxMap[key].enabled
                        textBoxMap[key].visible = false && textBoxMap[key].enabled
                    }
                }

            }
            else
            {
                labelMap[key].visible = false && textBoxMap[key].enabled
                textBoxMap[key].visible = false && textBoxMap[key].enabled
            }
        }
        profileValuelList.rows = roscnt
    }

    function showProfileType()
    {
        for(var key in buttonMap)
        {
            if(key == 0)
            {
                continue
            }

            if(buttonMap[key].visible == true)
            {
                buttonMap[key].sigButtonClickedWithKey(key)
                break
            }
        }
    }

    function updateLanguage()
    {
        console.log("updateLanguage ~~~~~~~")
        isReseting = true
        for(var key in labelMap)
        {
            if(key != 0)
            {
                var labelString = SettingJson.getLabel(key)
                var labelToolTip = SettingJson.getDescription(key)
                labelMap[key].text = qsTr(labelString)
                labelMap[key].strToolTip = qsTr(labelToolTip)
                textBoxMap[key].strToolTip = qsTr(labelToolTip)

                var comType = SettingJson.getSettingType(key)
                if(comType == "enum" || comType == "optional_extruder"|| comType == "extruder")
                {
                    var modelMap = SettingJson.getSettingOptions(key)

                    var newModel = someComponent.createObject(parent);
                    newModel.clear()
                    for(var modelKey in modelMap){
                        var imgPath = ""
                        imgPath = infillByName(modelMap[modelKey])
                        newModel.append({"key":modelKey, "modelData":qsTr(modelMap[modelKey]), "shapeImg":imgPath})
                    }

                    var currentType = SettingJson.getSettingValue(key)


                    textBoxMap[key].model = newModel

                    textBoxMap[key].currentIndex = SettingJson.findIndexByType(modelMap,currentType)

                }
            }
        }


        for(var key in buttonMap)
        {
            if(key != 0)
            {
                buttonMap[key].text = qsTr(key)
            }
        }
        isReseting = false
    }

    function checkErrorState()
    {
        for(var key in textBoxMap)
        {
            if(key != 0)
            {
                var comType = SettingJson.getSettingType(key)
                var errorState = textBoxMap[key].errorCode
                if(comType != "bool" && comType != "enum" && comType != "optional_extruder" && comType != "extruder" && errorState == 1)
                {
                    return false
                }
            }
        }
        return true
    }

    function deleteComponent()
    {
        for(var key in labelMap)
        {
            if(key != 0)
            {
                labelMap[key].destroy()
                delete labelMap[key]
            }
        }

        for(var key in textBoxMap)
        {
            if(key != 0)
            {
                textBoxMap[key].destroy()
                delete textBoxMap[key]
            }
        }

        for(var key in buttonMap)
        {
            if(key != 0)
            {
                buttonMap[key].destroy()
                delete buttonMap[key]
            }
        }

    }

    function updateComponent()
    {
        for(var key in textBoxMap)
        {
            if(key == 0)
            {
                continue
            }

            var comType = SettingJson.getSettingType(key)

            if(comType == "bool")
            {
                textBoxMap[key].checked = SettingJson.getSettingValue(key)
            }
            else if(comType == "enum" || comType == "optional_extruder" || comType == "extruder")
            {
                var modelMap = SettingJson.getSettingOptions(key)
                var currentType = SettingJson.getSettingValue(key)
                textBoxMap[key].currentIndex = SettingJson.findIndexByType(modelMap,currentType)
            }
            else
            {
                var errorState = textBoxMap[key].errorCode
                if(errorState == 1)
                {
                    continue
                }
                else
                {
                    textBoxMap[key].text = SettingJson.getSettingValue(key)
                }

            }
            textBoxMap[key].enabled = SettingJson.getSettingEnable(key)
        }

        filterProfileValueList()
        filterprofileTypeList()
    }

    function initComponent()
    {
        var cnt = 0
        for(var key in textBoxMap)
        {
            if(key == 0)
            {
                continue
            }

            var comType = SettingJson.getSettingType(key)

            if(comType == "bool")
            {
                textBoxMap[key].checked = SettingJson.initSettingValue(key)
            }
            else if(comType == "enum" || comType == "optional_extruder" || comType == "extruder")
            {
                var modelMap = SettingJson.getSettingOptions(key)
                var currentType = SettingJson.initSettingValue(key)
                textBoxMap[key].currentIndex = SettingJson.findIndexByType(modelMap,currentType)
            }
            else
            {
                textBoxMap[key].text = SettingJson.initSettingValue(key)
            }
            textBoxMap[key].enabled = SettingJson.getSettingEnable(key)
        }
    }

    function resetComponent()
    {

        isReseting = true
        initComponent()

        for(var key in textBoxMap)
        {
            if(key == 0)
            {
                continue
            }

            var comType = SettingJson.getSettingType(key)
            if(comType != "bool" && comType != "enum" && comType != "optional_extruder" && comType != "extruder")
            {
                textBoxMap[key].defaultBackgroundColor = Constants.dialogItemRectBgColor
                textBoxMap[key].errorCode = 0
            }
        }

        isReseting = false
    }

    function onClassTypeButtonClicked(keystr)
    {
        for(var key in buttonMap)
        {
            if(key == 0)
            {
                continue
            }

            if(key == keystr)
            {
                buttonMap[key].defaultBtnBgColor = "#1E9BE2"//"#1EB6E2"
                classTypeFilter = key
                filterProfileValueList()
            }
            else
            {
                buttonMap[key].defaultBtnBgColor = Constants.dialogContentBgColor
            }
        }
        idParamScrollView.setVScrollBarPosition(0)
    }

    function textFieldTextChanged(key, value)
    {
        valchangedAd(key,value)
    }

    function textFieldEditFinish(key, value)
    {
        var comType = SettingJson.getSettingType(key)
        if(comType == "bool")
        {
            editProfiledchanged(key,value)
        }
        else if(comType == "enum" || comType == "optional_extruder" || comType == "extruder")
        {
            var selectKey = textBoxMap[key].model.get(value).key
            editProfiledchanged(key,selectKey)
        }
        else
        {
            if(editProfiledchanged(key,value))
            {
                textBoxMap[key].defaultBackgroundColor = Constants.dialogItemRectBgColor
                textBoxMap[key].errorCode = 0
            }
            else
            {
                textBoxMap[key].defaultBackgroundColor = "red"
                textBoxMap[key].errorCode = 1
            }
        }

    }

    Item {
        id: rectangle
        x:30
        y :25 + titleHeight
        width: parent.width-40*screenScaleFactor
        height: parent.height-titleHeight-50*screenScaleFactor
        Column {
            spacing: 20
            Item {
                implicitWidth: innerRow.width
                implicitHeight: innerRow.height
                clip: true
                Row
                {
                    id: innerRow
                    spacing:10
                    StyledLabel {
                        id: element
                        text: qsTr("Profile:")
                        width: element.contentWidth//70
                        height: 30*screenScaleFactor
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignLeft
                    }
                    StyledLabel {
                        id: textEdit
                        width: textEdit.contentWidth//150
                        height: 30*screenScaleFactor
                        text: findTranslate(profileName)
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignLeft
                    }

                    Item {
                        width:2*screenScaleFactor
                        height: 30*screenScaleFactor
                        BasicSeparator
                        {
                            anchors.fill: parent
                        }
                    }

                    BasicLoginTextEdit
                    {
                        id: idSearch
                        placeholderText: qsTr("Search")
                        height : 30*screenScaleFactor
                        width : 200*screenScaleFactor//320-textEdit.contentWidth//350
                        font.pointSize:Constants.labelFontPointSize_9
                        headImageSrc:hovered ? Constants.searchBtnImg_d : Constants.searchBtnImg
                        tailImageSrc: hovered && idSearch.text != "" ? Constants.clearBtnImg : ""
                        hoveredTailImageSrc:Constants.clearBtnImg_d
                        radius : 14
                        text: ""
                        onEditingFinished:
                        {
                            console.log("idSearch onEditingFinished : " + idSearch.text)
                            nameFilter = idSearch.text
                            filterProfileValueList()
                            filterprofileTypeList()
                            showProfileType()
                        }
                        onTailBtnClicked:
                        {
                            idSearch.text = ""
                            idSearchButton.sigButtonClicked()
                        }
                    }

                    BasicButton {
                        id: idSearchButton
                        width: 60*screenScaleFactor
                        height: 30*screenScaleFactor
                        btnRadius:13
                        btnBorderW:0
                        enabled: idSearch.text != "" ? true : false
                        defaultBtnBgColor: enabled ?  Constants.searchBtnNormalColor : Constants.searchBtnDisableColor
                        hoveredBtnBgColor: Constants.searchBtnHoveredColor
                        text: qsTr("Search")
                        onSigButtonClicked:
                        {
                            console.log("idSearchButton clicked : " + idSearch.text)
                            nameFilter = idSearch.text
                            filterProfileValueList()
                            filterprofileTypeList()
                            showProfileType()
                        }
                    }

                    Rectangle {
                        color: "transparent"
                        width: 175*screenScaleFactor - textEdit.contentWidth - element.contentWidth
                        height: 30*screenScaleFactor
                    }

                    Row{
                        id:idConfiguration
                        height: 30*screenScaleFactor
                        x:-4
                        y:1
                        width:160*screenScaleFactor
                        spacing:5
                        property var currentIndex:1
                        ButtonGroup { id: idConfigurationGroup }
                        BasicRadioButton {
                            id: idTextInside
                            text: qsTr("Advanced")
                            checked: true
                            ButtonGroup.group: idConfigurationGroup
                            onClicked:
                            {
                                idConfiguration.currentIndex = 1
                                filterProfileValueList()
                                filterprofileTypeList()
                            }
                        }
                        BasicRadioButton {
                            id: idTextOutside
                            text: qsTr("Expert")
                            ButtonGroup.group: idConfigurationGroup
                            onClicked:
                            {
                                idConfiguration.currentIndex = 2
                                filterProfileValueList()
                                filterprofileTypeList()
                            }
                        }
                    }
                }
            }
            Item {
                width: idEditProfileDialog.width - 13
                height : 2

                Rectangle {
                    // anchors.left: idCol.left
                    // anchors.leftMargin: -10
                    x: -23*screenScaleFactor
                    width:parent.width > parent.height ?  parent.width : 2
                    height: parent.width > parent.height ?  2 : parent.height
                    color: Constants.splitLineColor
                    Rectangle {
                        width: parent.width > parent.height ? parent.width : 1
                        height: parent.width > parent.height ? 1 : parent.height
                        color: Constants.splitLineColorDark
                    }
                }
            }
            Row {
                spacing: 20
                width: parent.width
                BasicScrollView
                {
                    width: 125*screenScaleFactor
                    height: 450*screenScaleFactor
                    clip : true
                    Column
                    {
                        id: profileTypeBtnList
                        width:125
                    }
                }

                Item {
                    id: name
                    width:2
                    height: 450*screenScaleFactor
                    BasicSeparator
                    {
                        anchors.fill: parent
                    }
                }
                BasicScrollView
                {
                    id: idParamScrollView
                    width: 500*screenScaleFactor
                    height: 450*screenScaleFactor
                    clip : true
                    Grid
                    {
                        id: profileValuelList
                        columns: 2
                        rows: 6
                        rowSpacing: 5
                        columnSpacing: 10
                    }
                }
            }
            Item {
                width: idEditProfileDialog.width - 13*screenScaleFactor
                height : 2

                Rectangle {
                    // anchors.left: idCol.left
                    // anchors.leftMargin: -10
                    x: -23*screenScaleFactor
                    width:parent.width > parent.height ?  parent.width : 2
                    height: parent.width > parent.height ?  2 : parent.height
                    color: Constants.splitLineColor
                    Rectangle {
                        width: parent.width > parent.height ? parent.width : 1
                        height: parent.width > parent.height ? 1 : parent.height
                        color: Constants.splitLineColorDark
                    }
                }
            }
            Row {
                id: idBtnrow
                spacing: 10
                width: 395*screenScaleFactor
                height: 30*screenScaleFactor
                x: 120
                BasicButton {
                    id: basicComButton
                    width: 125*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius:3
                    btnBorderW:1*screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    text: qsTr("Reset")
                    onSigButtonClicked:
                    {
                        resetedAd()
                        resetComponent()
                    }
                }

                BasicButton {
                    id: basicComButton1
                    width: 125*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius:3
                    btnBorderW:1*screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    text: qsTr("Save")
                    onSigButtonClicked:
                    {
                        if(checkErrorState())
                        {
                            saveProfileAd()
                            idEditProfileDialog.close()
                        }
                        else
                        {
                            idMessageDlg.show()
                        }
                    }
                }

                BasicButton {
                    id : comCancel
                    width: 125*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius:3
                    btnBorderW:1*screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    text: qsTr("Cancel")
                    onSigButtonClicked: {
                        idEditProfileDialog.close()
                    }
                }
            }
        }
    }

    BasicDialog {
        id: idMessageDlg
        width: 400*screenScaleFactor
        height: 200*screenScaleFactor
        titleHeight : 30*screenScaleFactor
        title: qsTr("Message")

        Rectangle{
            anchors.centerIn: parent
            width: parent.width/2
            height: parent.height/2
            color: "transparent"
            Text {
                anchors.centerIn: parent
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                text: qsTr("Invalid parameter!!")
                font.pointSize: Constants.labelFontPointSize_6
                color:Constants.textColor
            }
        }
    }

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

        return imgPath;
    }
}

