import QtQuick 2.10
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.0
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4 as Control14
import CrealityUI 1.0
import QtQml 2.13
import Qt.labs.platform 1.1
import "../components"
import "../qml"
import ".."

BasicDialogV4
{
    id: idAddPrinterDlg
    width: 1040 * screenScaleFactor
    height: 591 * screenScaleFactor
    titleHeight : 30 * screenScaleFactor
    maxBtnVis: false
    title: qsTr("Manage Material")

    property var currentMachine : null
    property var currentMaterial : null

    property bool isCurMaterialUser: false
    property string lastMaterialName
    property string currentMaterialName
    property string btnState : "addState"
    property int extruderNum : 0
    property int extruderIndex : 0
    property int currentMaterialIndex: 0
    property var materialParams
    property var itemsModel : currentMaterial? currentMaterial.materialParameterModel(extruderIndex) : null
    property string curGroupKey: "properties"

    ManageMaterialDlgData{
        id: itemsData
        currentExtruder: extruderNum
    }

    ColorPanel{
        id:colorPanel
        visible: false
        x: 600
        y: 83
        onCurrentColorChanged: {
        }
    }

    function changeItemsModelByKey(groupKey){
        if(!currentMaterial)
            return
        console.log("currentMaterial.materialParameterModel(extruderIndex) ==========", currentMaterial.materialParameterModel(extruderIndex))
        if(groupKey === "properties"){
            currentMaterial.materialParameterModel(extruderIndex).setMaterialCategory("property")
        }  if(groupKey === "temperature"){
            currentMaterial.materialParameterModel(extruderIndex).setMaterialCategory("temperature")
        }  if(groupKey === "flow"){
            currentMaterial.materialParameterModel(extruderIndex).setMaterialCategory("flow")
        }  if(groupKey === "cooling"){
            currentMaterial.materialParameterModel(extruderIndex).setMaterialCategory("cool")
        }  if(groupKey === "override"){
            currentMaterial.materialParameterModel(extruderIndex).setMaterialCategory("override")
        }if(groupKey === "gcode"){
            currentMaterial.materialParameterModel(extruderIndex).setMaterialCategory("gcode")
        }
    }

    function resetFunc() {
        currentMaterial.reset()
        warningDlg.visible = false
    }

    function deleteFunc() {
        if(currentMachine)
            currentMachine.deleteUserMaterial(addMaterialDlg.materialName, idAddPrinterDlg.extruderIndex)
        warningDlg.visible = false
    }

    function generateDefaultName(format, existingNames) {
        let counter = 1;
        let defaultName = format + counter;

        let existingSet = new Set;
        for (let name of existingNames) {
            let index = name.lastIndexOf("_")
            if (index !== -1)
                name = name.substring(0, index)
            existingSet.add(name)
        }

        while (existingSet.has(defaultName)) {
            counter++;
            defaultName = format + counter;
        }

        return defaultName;
    }

    UploadMessageDlg{
        id: warningDlg
    }

    BasicDialogV4{
        property string materialName: "Ender-3"
        property string defaultMaterialName: ""
        id: addMaterialDlg
        width: 600*screenScaleFactor
        height: 206*screenScaleFactor
        title: qsTr("Custom Material")
        maxBtnVis: false
        bdContentItem:Item{
            GridLayout{
                anchors.top: parent.top
                anchors.topMargin: 20*screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                columns: 2

                StyledLabel{
                    text : qsTr("Material Name") + ": "
                    Layout.preferredWidth: 120*screenScaleFactor
                }

                BasicDialogTextField{
                    id: materialNameField
                    Layout.preferredWidth: 405*screenScaleFactor
                    Layout.preferredHeight : 28*screenScaleFactor
                    onlyPositiveNum: false
                    radius: 5
                    text : addMaterialDlg.defaultMaterialName
                    font.pointSize:Constants.labelFontPointSize_10
                    validator: RegExpValidator { regExp: null}
                    onEditingFinished: {
                    }
                }

                StyledLabel{
                    text : qsTr("Material Diameter") + ": "
                    Layout.preferredWidth: 120*screenScaleFactor
                }

                BasicDialogTextField{
                    Layout.preferredWidth: 405*screenScaleFactor
                    Layout.preferredHeight : 28*screenScaleFactor
                    onlyPositiveNum: false
                    radius: 5
                    text : addMaterialDlg.materialName == "" ? "1.75" : materialsComb.currentText.split('_')[1]
                    font.pointSize:Constants.labelFontPointSize_10
                    validator: RegExpValidator { regExp:  /(^-?[1-9]\d*\.\d+$|^-?0\.\d+$|^-?[1-9]\d*$|^0$)/}
                    onEditingFinished: {
                    }
                }

                StyledLabel{
                    text : qsTr("Chose Model") + ": "
                    Layout.preferredWidth: 120*screenScaleFactor
                }

                BasicCombobox{
                    id: materialsComb
                    showCount: 3
                    Layout.preferredWidth: 405*screenScaleFactor
                    Layout.preferredHeight : 28*screenScaleFactor
                    model: currentMachine ? currentMachine.materialsName : []
                    currentIndex: currentMaterialIndex
                    Component.onCompleted:{
                    }

                    onStyleComboBoxIndexChanged:{
                    }
                }
            }

            Row{
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20*screenScaleFactor
                spacing: 20*screenScaleFactor
                BasicDialogButton {
                    text: qsTr("Create")
                    width: 120*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius: height / 2
                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color

                    onSigButtonClicked: {
                        let modelName = currentMachine.materialsNameList()[materialsComb.currentIndex]
                        currentMachine.addUserMaterial(modelName, materialNameField.text, idAddPrinterDlg.extruderIndex)
                        addMaterialDlg.close()
                        cloader.item.leftListView.currentIndex = idAddPrinterDlg.cloader.item.leftListView.count - 1
                    }
                }

                BasicDialogButton {
                    text: qsTr("Cancel")
                    width: 120*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius: height / 2
                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color

                    onSigButtonClicked: {
                        addMaterialDlg.close()
                    }
                }
            }
        }
    }

    BasicDialogV4{
        property string materialType: "PLA"
        property string materialFileName: ""
        id: importMaterialFileDlg
        width: 600*screenScaleFactor
        height: 206*screenScaleFactor
        title: qsTr("Import Material")
        maxBtnVis: false
        bdContentItem:Item{
            GridLayout{
                anchors.top: parent.top
                anchors.topMargin: 20*screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                columns: 2
                StyledLabel{
                    text : qsTr("Material Name") + ": "
                    Layout.preferredWidth: 120*screenScaleFactor
                }

                BasicDialogTextField{
                    id: mnTextField
                    Layout.preferredWidth: 405*screenScaleFactor
                    Layout.preferredHeight : 28*screenScaleFactor
                    onlyPositiveNum: false
                    radius: 5
                    text : importMaterialFileDlg.materialFileName
                    font.pointSize:Constants.labelFontPointSize_10
                    validator: RegExpValidator { regExp:  /[\[\],0-9\x22]+/}
                    onEditingFinished: {
                    }
                }

                StyledLabel{
                    text : qsTr("Material Type") + ": "
                    Layout.preferredWidth: 120*screenScaleFactor
                }

                BasicDialogTextField{
                    Layout.preferredWidth: 405*screenScaleFactor
                    Layout.preferredHeight : 28*screenScaleFactor
                    onlyPositiveNum: false
                    radius: 5
                    enabled: false
                    text : importMaterialFileDlg.materialType
                    font.pointSize:Constants.labelFontPointSize_10
                    validator: RegExpValidator { regExp:  /[\[\],0-9\x22]+/}
                    onEditingFinished: {
                    }
                }
            }

            Row{
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20*screenScaleFactor
                spacing: 20*screenScaleFactor
                BasicDialogButton {
                    text: qsTr("OK")
                    width: 120*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius: height / 2
                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color

                    onSigButtonClicked: {
                        currentMachine.importMaterial(inputDialog.currentFile, mnTextField.text)
                        importMaterialFileDlg.close()
                        cloader.item.leftListView.currentIndex = idAddPrinterDlg.cloader.item.leftListView.count - 1
                    }
                }

                BasicDialogButton {
                    text: qsTr("Cancel")
                    width: 120*screenScaleFactor
                    height: 28*screenScaleFactor
                    btnRadius: height / 2
                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color

                    onSigButtonClicked: {
                        importMaterialFileDlg.close()
                    }
                }
            }
        }
    }

    bdContentItem: Rectangle {
        property alias leftListView: idLeftListView

        color: Constants.themeColor
        RowLayout{
            anchors.fill: parent
            anchors.margins: 20* screenScaleFactor
            ListView{
                id: idLeftListView
                spacing: 20* screenScaleFactor
                Layout.preferredWidth: 140* screenScaleFactor
                Layout.fillHeight: true
                Layout.topMargin: 0
                currentIndex: 0
                ScrollBar.vertical: ScrollBar {
                }
                model: currentMachine ? currentMachine.materialsName : []
                clip: true
                delegate:RadioButton{
                    id: cbItem
                    width: idLeftListView.width - 10*screenScaleFactor
                    checked: model.index === idLeftListView.currentIndex
                    indicator: Rectangle {
                        implicitWidth: 10* screenScaleFactor
                        implicitHeight: 10* screenScaleFactor
                        x: cbItem.leftPadding
                        y: parent.height / 2 - height / 2
                        radius: 5* screenScaleFactor
                        color:cbItem.checked ? "#00a3ff" : "transparent"
                        border.color: "#cbcbcc"
                        border.width: cbItem.checked ? 0 : 1* screenScaleFactor
                        Rectangle {
                            width: 4* screenScaleFactor
                            height: 4* screenScaleFactor
                            anchors.centerIn: parent
                            visible: cbItem.checked
                            color: "#ffffff"
                            radius: 2* screenScaleFactor
                            anchors.margins: 4* screenScaleFactor
                        }
                    }

                    contentItem:Label {
                        text: modelData
                        font.family: Constants.labelFontFamily
                        font.weight: Constants.labelFontWeight
                        font.pointSize: Constants.labelFontPointSize_9
                        leftPadding: cbItem.indicator.width + cbItem.spacing
                        color: cbItem.checked || cbItem.hovered
                               ? Constants.manager_printer_switch_checked_color
                               : Constants.manager_printer_switch_default_color
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignLeft
                        elide: Text.ElideRight
                        wrapMode: Text.NoWrap
                    }

                    onCheckedChanged: {

                    }

                    onClicked: {
                        idLeftListView.currentIndex = model.index
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: modelData

                }
                onCurrentIndexChanged: {
                    console.log("+++currentIndex = ", currentIndex)
                    currentMaterialIndex = currentIndex
                    let curMaterialName = currentMachine.materialsNameList()[currentIndex]

                    setCurrentMaterial(curMaterialName)
                    addMaterialDlg.materialName = curMaterialName
                    idAddPrinterDlg.lastMaterialName = currentMaterial.name()
                    currentMaterialName = curMaterialName
                    duplicateLabel.visible = idAddPrinterDlg.isCurMaterialUser && duplicateLabel.visible
                }
            }

            Rectangle{
                color: Constants.dialogItemRectBgBorderColor
                anchors.top: parent.top
                Layout.preferredWidth: 1* screenScaleFactor
                Layout.preferredHeight: columnItem.height - 60* screenScaleFactor
            }

            Item{
                Layout.fillWidth: true
            }

            Column{
                id: columnItem
                Layout.preferredWidth: 830* screenScaleFactor
                Layout.fillHeight: true
                Layout.topMargin: 0
                spacing: 22* screenScaleFactor
                Grid{
                    rows: 2
                    columns: 4
                    rowSpacing: 2* screenScaleFactor
                    columnSpacing: 10* screenScaleFactor
                    verticalItemAlignment: Grid.AlignVCenter
                    Layout.leftMargin: 0
                    StyledLabel{
                        height : 30* screenScaleFactor
                        text: qsTr("Material Name") + ":"
                        font.pointSize:Constants.labelFontPointSize_9
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    Row{
                        BasicDialogTextField{
                            id: materialNameEdit
                            readOnly: !idAddPrinterDlg.isCurMaterialUser
                            width: 160* screenScaleFactor
                            height : 28* screenScaleFactor
                            radius: 5* screenScaleFactor
                            objectName: "material_name"
                            validator: null
                            text: currentMachine ? currentMachine.materialsName[currentMaterialIndex] : ""
                            onTextChanged: {

                            }

                            onEditingFinished: {
                                let isDuplicate = currentMachine.checkMaterialName(currentMaterial.uniqueName(), text)
                                if (isDuplicate) {
                                    duplicateLabel.visible = idAddPrinterDlg.isCurMaterialUser
                                    return
                                }

                                let tempIndex = currentMaterialIndex
                                duplicateLabel.visible = false
                                currentMachine.modifyMaterialName(lastMaterialName, materialNameEdit.text)
                                idAddPrinterDlg.lastMaterialName = text
                                cloader.item.leftListView.currentIndex = tempIndex
                                idLeftListView.currentIndex = tempIndex
                            }
                        }

                        StyledLabel{
                            id: duplicateLabel
                            visible: false//idAddPrinterDlg.isCurMaterialUser
                            color:"red"
                            height : 30* screenScaleFactor
                            text: qsTr("Duplicate name")
                            font.pointSize:Constants.labelFontPointSize_9
                            verticalAlignment: Qt.AlignVCenter
                            horizontalAlignment: Qt.AlignRight
                        }
                    }

                    StyledLabel{
                        height : 30* screenScaleFactor
                        text: qsTr("Brand") + ":"
                        font.pointSize:Constants.labelFontPointSize_9
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    BasicDialogTextField{
                        readOnly: true
                        width: 160* screenScaleFactor
                        height : 28* screenScaleFactor
                        radius: 5* screenScaleFactor
                        objectName: "brand"
                        text: currentMaterial? currentMaterial.brand() : "materialBrand"
                        //                        strToolTip: qsTr(SettingJson.getDescription(objectName))
                        onEditingFinished: {
                        }
                        onTextChanged:
                        {
                        }
                    }

                    StyledLabel{
                        height : 30* screenScaleFactor
                        text: qsTr("Material Type") + ":"
                        font.pointSize:Constants.labelFontPointSize_9
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }

                    Row{
                        BasicDialogTextField{
                            id : idMaterialType
                            readOnly: true
                            width: 160* screenScaleFactor
                            height : 28* screenScaleFactor
                            radius: 5* screenScaleFactor
                            text: currentMaterial? currentMaterial.type() : "materialType"
                            objectName: "material_type"
                            onEditingFinished: {
                                materialParams.mg.materialType = text
                            }
                            onTextChanged:
                            {
                            }
                        }

                        StyledLabel{
                            width : 100* screenScaleFactor
                            height : 30* screenScaleFactor
                            text: ""
                            font.pointSize:Constants.labelFontPointSize_9
                            verticalAlignment: Qt.AlignVCenter
                            horizontalAlignment: Qt.AlignRight
                        }
                    }

                    StyledLabel{
                        height : 30* screenScaleFactor
                        text: qsTr("Color") + ":"
                        font.pointSize:Constants.labelFontPointSize_9
                        verticalAlignment: Qt.AlignVCenter
                        horizontalAlignment: Qt.AlignRight
                    }
                    Rectangle{
                        id: colorBtn
                        objectName: "color"
                        width: 20
                        height: 20
                        radius: 5
                        color:colorPanel.currentColor
                        MouseArea{
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                colorPanel.visible = true
                            }
                        }
                    }
                }
                CustomTabView{
                    id: cusTableView
                    width: 830* screenScaleFactor
                    height: 373* screenScaleFactor
                    Layout.leftMargin: 0
                    onCurrentPanelChanged: {
                        idAddPrinterDlg.curGroupKey = currentPanel.objectName
                        changeItemsModelByKey(currentPanel.objectName)
                    }

                    Repeater{
                        id: repter
                        model:ListModel{
                            ListElement{labelText: qsTr("Properties"); itemKey: "properties"}
                            ListElement{labelText: qsTr("Temperature"); itemKey: "temperature"}
                            ListElement{labelText: qsTr("Flow"); itemKey: "flow"}
                            ListElement{labelText: qsTr("Cooling"); itemKey: "cooling"}
                            ListElement{labelText: qsTr("Override"); itemKey: "override"}
                            ListElement{labelText: qsTr("G-code"); itemKey: "gcode"}
                        }
                        delegate:tabViewItem
                    }
                }

                Row{
                    id: btnsRow
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10* screenScaleFactor
                    state: idAddPrinterDlg.btnState
                    states: [
                        State {
                            name: "addState"
                            PropertyChanges {
                                target: resetBtn
                                visible: false
                            }
                            PropertyChanges {
                                target: deleteBtn
                                visible: false
                            }
                        },
                        State {
                            name: "manageState"
                            PropertyChanges {
                                target: resetBtn
                                visible: true
                            }
                            PropertyChanges {
                                target: deleteBtn
                                visible: true
                            }
                        }
                    ]
                    BasicButton {
                        id: newBtn
                        width: 120* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Create")
                        btnRadius:14* screenScaleFactor
                        btnBorderW:1* screenScaleFactor
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        onSigButtonClicked:{
                            if (!currentMachine)
                                return

                            /* 获取模板名称 */
                            let templateName = currentMachine.materialsName[currentMaterialIndex]
                            let index = templateName.lastIndexOf("_")
                            if (index !== -1)
                                templateName = templateName.substring(0, index)

                            let defaultName
                            let copyPrefix = "@copy"
                            let prefixIndex = templateName.indexOf(copyPrefix)
                            if (prefixIndex !== -1) {
                                let format = templateName.substring(0, prefixIndex + 5)
                                defaultName = generateDefaultName(format, currentMachine.materialsNameInMachine())
                            } else {
                                let format = templateName + copyPrefix
                                defaultName = generateDefaultName(format, currentMachine.materialsNameInMachine())
                            }

                            addMaterialDlg.defaultMaterialName = defaultName
                            addMaterialDlg.visible = true
                        }
                    }
                    BasicButton {
                        id: resetBtn
                        width: 120* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Reset")
                        btnRadius:14* screenScaleFactor
                        btnBorderW:1* screenScaleFactor
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        onSigButtonClicked:
                        {
                            warningDlg.msgText = qsTr("Do you want to reset the selected filament parameters?")
                            warningDlg.visible = true
                            warningDlg.sigOkButtonClicked.disconnect(resetFunc)
                            warningDlg.sigOkButtonClicked.disconnect(deleteFunc)
                            warningDlg.sigOkButtonClicked.connect(resetFunc)

                        }
                    }
                    BasicButton {
                        id: deleteBtn
                        width: 120* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Delete")
                        btnRadius:14* screenScaleFactor
                        btnBorderW:1* screenScaleFactor
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        enabled : idAddPrinterDlg.isCurMaterialUser
                        onSigButtonClicked:{
                            warningDlg.msgText = qsTr("Do you want to delete the selected filament?")
                            warningDlg.visible = true
                            warningDlg.sigOkButtonClicked.disconnect(resetFunc)
                            warningDlg.sigOkButtonClicked.disconnect(deleteFunc)
                            warningDlg.sigOkButtonClicked.connect(deleteFunc)
                        }
                    }

                    BasicButton {
                        id: importBtn
                        width: 120* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Import")
                        btnRadius:14* screenScaleFactor
                        btnBorderW:1* screenScaleFactor
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        onSigButtonClicked:{
                            inputDialog.open()
                        }
                    }
                    BasicButton {
                        id: exportBtn
                        width: 120* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Export")
                        btnRadius:14* screenScaleFactor
                        btnBorderW:1* screenScaleFactor
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        onSigButtonClicked:{
                            saveDialog.open()
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: inputDialog
        title: qsTr("Import")
        fileMode: FileDialog.OpenFile
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        nameFilters: ["Profiles (*.cxfilament)", "All files (*)"]
        onAccepted: {
            importMaterialFileDlg.materialType =  currentMachine.getMaterialType(inputDialog.currentFile)
            importMaterialFileDlg.materialFileName = currentMachine.getFileNameByPath(inputDialog.currentFile)
            importMaterialFileDlg.visible = true
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Export")
        fileMode: FileDialog.SaveFile
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        nameFilters: ["Profiles (*.cxfilament)", "All files (*)"]
        defaultSuffix : "cxfilament"
        onAccepted: {
            currentMaterial.exportMaterialFromQml(saveDialog.currentFile)
        }
    }

    ParameterComponent{
        id:gCom
    }

    Component{
        id: emptyItem
        Item {}
    }

    Component{
        id: tabViewItem
        CustomTabViewItem{
            id: modelItem
            title: model.labelText
            objectName: model.itemKey
            BasicScrollView{
                id: bsv
                anchors.fill: parent
                padding: 10* screenScaleFactor
                hpolicyVisible: false
                vpolicyVisible: contentHeight > height
                clip: true
                ColumnLayout{
                    width: bsv.contentItem.width
                    Repeater{
                        model: idAddPrinterDlg.itemsModel
                        delegate: {
                            if (modelItem.objectName !== idAddPrinterDlg.curGroupKey)
                                return emptyItem
                            if(idAddPrinterDlg.curGroupKey === "override")
                                return gCom.recoverItemRowCom;
                            else
                                return gCom.itemRow
                        }
                    }

                    Item{
                        Layout.fillHeight: true
                    }
                }
            }
            Component.onCompleted: {
                if(model.index === 0){
                    parent.defaultPanel = this
                }
            }
        }
    }

    //获取左侧列表
    function getleftBtnList(){
        return currentMachine.materialsList()
    }

    //
    function startShowMaterialDialog(_state, _extruderIndex, materialName, machineObject){
        btnState = _state
        extruderIndex = _extruderIndex
        currentMachine = machineObject
        //        cloader.item.leftListView.model = currentMachine.materialNames()
        cloader.item.leftListView.currentIndex = currentMachine.extruderMaterialIndex(_extruderIndex)
        var material = currentMachine.materialObject(materialName)
        itemsModel.sestExtruderIndex(extruderIndex)
        changeItemsModelByKey(curGroupKey)

        visible = true

        //        let curName = currentMachine.materialsName[cloader.item.leftListView.currentIndex]

        console.log("+++curName = ", materialName)
        setCurrentMaterial(materialName)
    }

    function hideMaterialDialog(){
        currentMachine = null
        currentMaterial = null
        idAddPrinterDlg.close()
    }

    function setCurrentMaterial(materialName){
        if(!currentMachine)
            return

        var material = currentMachine.materialObject(materialName)
        if(!material || (material == currentMaterial)){
            return
        }
        currentMaterial = material
        console.log("==============materialName = ", materialName)
        idAddPrinterDlg.isCurMaterialUser = currentMaterial.isUserDef()
        console.log("==============isUser = ", idAddPrinterDlg.isCurMaterialUser)
        itemsData.materialParams = currentMaterial.settingsObject()
        idAddPrinterDlg.materialParams = currentMaterial.settingsObject()
        itemsModel.sestExtruderIndex(extruderIndex)
        changeItemsModelByKey(curGroupKey)
    }
}
