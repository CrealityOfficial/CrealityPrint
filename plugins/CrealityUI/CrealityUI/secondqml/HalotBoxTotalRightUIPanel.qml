import Qt.labs.platform 1.1
import QtGraphicalEffects 1.12
import QtQml 2.13
import QtQuick 2.13
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import "../qml"
import "../components"
Rectangle {
    id: rootRec
    property var currentMachine
    property string curProfileName: ""
    property bool curProfileIsDefault: true
    property int curMachineExtruderCount: 1
    property var extruder0
    property var extruder1
    property string profileTipBgColor: "#6E6E73"
    property string extruder0Material: "PLA"
    property string extruder1Material: "PLA"
    function updateProfileLanguage(){
        editProfileWizard.updateLanguage()
    }

    function checkParam() {
        paramCheck.cloader.item.dataList.model = null
        paramCheck.cloader.item.dataList.model = currentMachine.generateParamCheck()
        console.log("****data count = ", paramCheck.cloader.item.dataList.count)
        if(paramCheck.cloader.item.dataList.count > 0)
            paramCheck.visible = true
    }

    Connections{
        target: kernel_parameter_manager

        onCurMachineIndexChanged: {
            console.log("HalotBoxTotalRightUIPanel.qml onCurMachineIndexChanged.")

            currentMachine = null
            var machine = kernel_parameter_manager.currentMachineObject()
            currentMachine = machine

            //profile
            idProfileListView.model = currentMachine.profilesModel()
            idProfileListView.currentIndex = Qt.binding(function() { return currentMachine.curProfileIndex; } )
            //extruder
            curMachineExtruderCount = currentMachine.extruderCount()
            //material
            if(curMachineExtruderCount > 0)
            {
                extruder0 = currentMachine.extruderObject(0)
            }

            if(curMachineExtruderCount > 1)
            {
                extruder1 = currentMachine.extruderObject(1)
            }
        }
    }

    function showAddMaterial(extruderIndex, materialName,extruderItem){
        addMaterialDlg.eItem = extruderItem
        addMaterialDlg.startShowMaterialDialog("addState", extruderIndex, materialName, currentMachine)
        console.log("---------------------------------", currentMachine.generateParamCheck())
    }

    function showEditMaterial(extruderIndex, materialName){
        manageMaterialDlg.startShowMaterialDialog("manageState", extruderIndex, materialName, currentMachine)
    }

    function showEditMaterialDlg(){
        manageMaterialDlg.startShowMaterialDialog("manageState", 0, extruder1Material, currentMachine)
    }

    function showEditProfile(_addOrEdit){
        //临时版本
        //idEditProfile.showEditProfile(_addOrEdit, currentMachine)
        //正式版本
        //cusEditProfile.visible = true
        idParameterProfile.startEditProfile(currentMachine, _addOrEdit, currentMachine.extruderCount())
    }

    width: 300*screenScaleFactor
    color: "transparent"
    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }

    BasicDialogV4{
        id: paramCheck
        title: qsTr("Tips")
        maxBtnVis: false
        width: 600*screenScaleFactor
        height: 380*screenScaleFactor
        bdContentItem: Rectangle{
            property alias dataList: dataList
            color: Constants.themeColor_primary
            StyledLabel{
                id: topLabel
                color: Constants.manager_printer_label_color
                anchors.left: parent.left
                anchors.leftMargin: 41 * screenScaleFactor
                anchors.top: parent.top
                anchors.topMargin: 33 * screenScaleFactor
                text: qsTr("You have redefine some directives.") + "\n" +
                      qsTr("Do you want to keep these changed settings after switching materials?") + "\n" +
                      qsTr("Or you can discard it for changes and use the default parameter configuration.")
            }

            Rectangle{
                width: 517 * screenScaleFactor
                implicitHeight: 180 * screenScaleFactor
                clip: true
                border.width: 1* screenScaleFactor
                border.color: Constants.dialogItemRectBgBorderColor
                radius: 5 * screenScaleFactor
                color: Constants.themeColor_primary
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: topLabel.bottom
                anchors.topMargin: 10
                //                anchors.bottom: parent.bottom
                //                anchors.bottomMargin: 70 * screenScaleFactor
                Row{
                    id: topTitle
                    spacing: 0
                    Rectangle{
                        width: 172*screenScaleFactor
                        height: 28*screenScaleFactor
                        color:Constants.themeColor_primary
                        border.width: 1* screenScaleFactor
                        border.color: Constants.dialogItemRectBgBorderColor
                        StyledLabel{
                            color: Constants.topBtnTextColor_normal
                            anchors.centerIn: parent
                            text: qsTr("Parameter setting items")
                        }
                    }
                    Rectangle{
                        width: 172*screenScaleFactor
                        height: 28*screenScaleFactor
                        color:Constants.themeColor_primary
                        border.width: 1* screenScaleFactor
                        border.color: Constants.dialogItemRectBgBorderColor
                        StyledLabel{
                            color: Constants.topBtnTextColor_normal
                            anchors.centerIn: parent
                            text: qsTr("Default parameter value")
                        }
                    }
                    Rectangle{
                        width: 172*screenScaleFactor
                        height: 28*screenScaleFactor
                        color:Constants.themeColor_primary
                        border.width: 1* screenScaleFactor
                        border.color: Constants.dialogItemRectBgBorderColor
                        StyledLabel{
                            color: Constants.topBtnTextColor_normal
                            anchors.centerIn: parent
                            text: qsTr("Current parameter value")
                        }
                    }
                }

                ListView{
                    id: dataList
                    clip: true
                    width: topTitle.width
                    height: 152 * screenScaleFactor
                    anchors.top: topTitle.bottom
                    anchors.horizontalCenter: topTitle.horizontalCenter
                    delegate: Row{
                        spacing: 0
                        Rectangle{
                            width: 172*screenScaleFactor
                            height: 28*screenScaleFactor
                            color:"transparent"
                            border.width: 1* screenScaleFactor
                            border.color: Constants.dialogItemRectBgBorderColor

                            StyledLabel{
                                color: Constants.topBtnTextColor_normal
                                anchors.centerIn: parent
                                text: qsTranslate("ParameterGeneralCom", modelData.getKeyStr())
                            }
                        }
                        Rectangle{
                            width: 172*screenScaleFactor
                            height: 28*screenScaleFactor
                            color:"transparent"
                            border.width: 1* screenScaleFactor
                            border.color: Constants.dialogItemRectBgBorderColor

                            StyledLabel{
                                color: Constants.topBtnTextColor_normal
                                anchors.centerIn: parent
                                text: modelData.getDefaultValue()
                            }
                        }
                        Rectangle{
                            width: 172*screenScaleFactor
                            height: 28*screenScaleFactor
                            color:"transparent"
                            border.width: 1* screenScaleFactor
                            border.color: Constants.dialogItemRectBgBorderColor

                            StyledLabel{
                                color: Constants.topBtnTextColor_normal
                                anchors.centerIn: parent
                                text: modelData.getCurrentValue()
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { }
                }
            }

            Row{
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20 * screenScaleFactor
                spacing: 10 * screenScaleFactor
                BasicDialogButton {
                    id: keepBtn
                    text: qsTr("Keep Changes")
                    width: 120* screenScaleFactor
                    height: 28* screenScaleFactor

                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color

                    onSigButtonClicked: {
                        paramCheck.visible = false
                    }
                }

                BasicDialogButton {
                    id: discardBtn
                    text: qsTr("Discard changes")
                    width: 120* screenScaleFactor
                    height: 28* screenScaleFactor

                    btnBorderW: 1 * screenScaleFactor
                    btnTextColor: Constants.manager_printer_button_text_color
                    borderColor: Constants.manager_printer_button_border_color
                    defaultBtnBgColor: Constants.manager_printer_button_default_color
                    hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                    selectedBtnBgColor: Constants.manager_printer_button_checked_color

                    onSigButtonClicked: {
                        currentMachine.resetProfileModify();
                        paramCheck.visible = false
                    }
                }
            }
        }
        onVisibleChanged: {

        }
    }
    AddMaterialDlg{
        id:addMaterialDlg
        property var eItem: null
    }

    ManageMaterialDlg{
        id: manageMaterialDlg
    }

    ColumnLayout {
        spacing: 0 *screenScaleFactor
        anchors.fill: parent
        ColumnLayout{
            Layout.topMargin: 15*screenScaleFactor
            spacing: 8 *screenScaleFactor
            PanelComponent {
                id: modelListC
                imgUrl: "qrc:/UI/photo/materialPanel.svg"
                title: qsTr("Material")
                sourceSize.width: 11 * screenScaleFactor
                sourceSize.height: 11 * screenScaleFactor
                lineColor: "red"
            }

            //双喷的适配
            Item{
                Layout.preferredWidth: modelListC.width - 18* screenScaleFactor
                Layout.preferredHeight: extruderStack.currentIndex == 0 ? 28*screenScaleFactor : 60*screenScaleFactor
                Layout.alignment: Qt.AlignHCenter
                StackLayout{
                    id:extruderStack
                    anchors.fill: parent
                    currentIndex: rootRec.curMachineExtruderCount - 1

                    Row{
                        StyleCheckBox {
                            width: 110* screenScaleFactor
                            height : 30* screenScaleFactor
                            checked: true
                            text: qsTr("Extruder")
                            enabled: false//nozzle0_cb.checked
                            onCheckedChanged: {
                            }
                        }

                        CXComboBox {
                            id: idExtruder11
                            width: 150*screenScaleFactor
                            height: 28*screenScaleFactor
                            showCount: 20
                            currentIndex:extruder0.materialIndex
                            model: !currentMachine ? null : currentMachine.materialsName
                            clip: true
                            property var extruder : extruder0
                            onActivated:{
                                currentMachine.setExtruderMaterial(0, currentIndex)
                                checkParam()

                            }
                            onCurrentTextChanged: {
                                //                                currentMachine.setExtruderMaterial(0, currentText)
                                extruder0Material = currentText
                            }
                            slotItem:Row{
                                spacing: 2
                                anchors.left: parent.left
                                anchors.leftMargin: 2
                                CusButton {
                                    id:addBtn
                                    width: idExtruder11.width/2 - 4* screenScaleFactor
                                    height : 24 * screenScaleFactor
                                    radius: 5
                                    txtContent: qsTr("Add")
                                    enabled: true
                                    onClicked:
                                    {
                                        idExtruder11.popup.visible = false
                                        let materialName = currentMachine.materialsNameList()[idExtruder11.currentIndex]
                                        console.log("materialName",materialName)
                                        showAddMaterial(0, materialName,this)
                                    }
                                }

                                CusButton {
                                    id:managerBtn
                                    width: idExtruder11.width/2 - 4* screenScaleFactor
                                    height : 24 * screenScaleFactor
                                    radius: 5
                                    txtContent: qsTr("Manage")
                                    enabled: true
                                    onClicked:
                                    {
                                        idExtruder11.popup.visible = false
                                        let materialName = currentMachine.materialsNameList()[idExtruder11.currentIndex]
                                        showEditMaterial(0, materialName)
                                    }
                                }
                            }
                        }
                    }

                    //
                    Column{
                        spacing: 2
                        Row{
                            StyleCheckBox {
                                id:nozzle0_cb
                                width: 110* screenScaleFactor
                                height : 30* screenScaleFactor
                                checked: true
                                enabled: nozzle1_cb.checked
                                text: qsTr("Extruder 1")
                                onCheckedChanged: {
                                    currentMachine.setExtruderStatus(nozzle0_cb.checked,nozzle1_cb.checked)
                                }
                            }

                            CXComboBox {
                                id: idExtruder21
                                width: 150*screenScaleFactor
                                height: 28*screenScaleFactor
                                showCount: 20
                                currentIndex:extruder0.materialIndex
                                model: !currentMachine ? null : currentMachine.materialsName
                                clip: true
                                property var extruder : extruder0
                                onActivated:{
                                    currentMachine.setExtruderMaterial(0, currentIndex)
                                }
                                onCurrentTextChanged: {
                                    let materialName = currentMachine.materialsNameList()[currentIndex]
                                    extruder0Material = materialName
                                    paramCheck.cloader.item.dataList.model = null
                                    paramCheck.cloader.item.dataList.model = currentMachine.generateParamCheck()
                                    if(paramCheck.cloader.item.dataList.model.count > 0)
                                        paramCheck.visible = true
                                }
                                slotItem:Row{
                                    spacing: 2
                                    anchors.left: parent.left
                                    anchors.leftMargin: 2
                                    CusButton {
                                        width: idExtruder21.width/2 - 4* screenScaleFactor
                                        height : 24 * screenScaleFactor
                                        radius: 5
                                        txtContent: qsTr("Add")
                                        enabled: true
                                        onClicked:
                                        {
                                            idExtruder21.popup.visible = false
                                            let materialName = currentMachine.materialsNameList()[idExtruder21.currentIndex]
                                            showAddMaterial(0, materialName,this)
                                        }
                                    }

                                    CusButton {
                                        width: idExtruder21.width/2 - 4* screenScaleFactor
                                        height : 24 * screenScaleFactor
                                        radius: 5
                                        txtContent: qsTr("Manage")
                                        enabled: true
                                        onClicked:
                                        {
                                            idExtruder21.popup.visible = false
                                            let materialName = currentMachine.materialsNameList()[idExtruder21.currentIndex]
                                            showEditMaterial(0, materialName)
                                        }
                                    }
                                }
                            }
                        }
                        Row{
                            StyleCheckBox {
                                id:nozzle1_cb
                                width: 110* screenScaleFactor
                                height : 30* screenScaleFactor
                                checked: true
                                text: qsTr("Extruder 2")
                                enabled: nozzle0_cb.checked
                                onCheckedChanged: {
                                    currentMachine.setExtruderStatus(nozzle0_cb.checked,nozzle1_cb.checked)
                                }
                            }
                            CXComboBox {
                                id: idExtruder22
                                width: 150*screenScaleFactor
                                height: 28*screenScaleFactor
                                showCount: 20
                                currentIndex:extruder0.materialIndex
                                model: !currentMachine ? null : currentMachine.materialsName
                                clip: true
                                property var extruder : extruder0
                                onActivated:{
                                    currentMachine.setExtruderMaterial(1, currentIndex)
                                }
                                onCurrentTextChanged: {
                                    let materialName = currentMachine.materialsNameList()[currentIndex]
                                    extruder1Material = materialName
                                    paramCheck.cloader.item.dataList.model = null
                                    paramCheck.cloader.item.dataList.model = currentMachine.generateParamCheck()
                                    if(paramCheck.cloader.item.dataList.model.count > 0)
                                        paramCheck.visible = true

//                                    idProfileListView.model = undefined
//                                    idProfileListView.model = currentMachine.profilesModel()
                                }
                                slotItem:Row{
                                    spacing: 2
                                    anchors.left: parent.left
                                    anchors.leftMargin: 2
                                    CusButton {
                                        width: idExtruder22.width/2 - 4* screenScaleFactor
                                        height : 24 * screenScaleFactor
                                        radius: 5
                                        txtContent: qsTr("Add")
                                        enabled: true
                                        onClicked:
                                        {
                                            idExtruder22.popup.visible = false
                                            let materialName = currentMachine.materialsNameList()[idExtruder22.currentIndex]
                                            showAddMaterial(1, materialName,this)
                                        }
                                    }

                                    CusButton {
                                        width: idExtruder22.width/2 - 4* screenScaleFactor
                                        height : 24 * screenScaleFactor
                                        radius: 5
                                        txtContent: qsTr("Manage")
                                        enabled: true
                                        onClicked:
                                        {
                                            idExtruder22.popup.visible = false
                                            let materialName = currentMachine.materialsNameList()[idExtruder21.currentIndex]
                                            showEditMaterial(1, materialName)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        Item{
            Layout.preferredHeight: 10
        }
        PanelComponent {
            id: paramCfgC
            imgUrl: "qrc:/UI/photo/paramPanel.svg"
            title: qsTr("Parameter config")
            Layout.preferredHeight: 60* screenScaleFactor
            sourceSize.width: 11 * screenScaleFactor
            sourceSize.height: 12 * screenScaleFactor
            topLineEnabled: true
            CusImglButton {
                id: importBtn
                width: 28*screenScaleFactor
                height: 28*screenScaleFactor
                anchors.right: parent.right
                anchors.rightMargin: 53*screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                defaultBtnBgColor: Constants.right_panel_button_default_color
                hoveredBtnBgColor: Constants.right_panel_button_hovered_color
                selectedBtnBgColor: Constants.right_panel_button_checked_color
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                enabledIconSource: "qrc:/UI/photo/right_import.svg"
                highLightIconSource: "qrc:/UI/photo/right_import.svg"
                pressedIconSource: "qrc:/UI/photo/right_import.svg"
                cusTooltip.text: qsTr("Import")
                btnRadius: 5
                borderWidth: 0
                onClicked: {
                    inputDialog.open();
                }
            }
            CusImglButton {
                id: exportBtn
                width: 28*screenScaleFactor
                height: 28*screenScaleFactor
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                defaultBtnBgColor: Constants.right_panel_button_default_color
                hoveredBtnBgColor: Constants.right_panel_button_hovered_color
                selectedBtnBgColor: Constants.right_panel_button_checked_color
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                enabledIconSource: "qrc:/UI/photo/right_export.svg"
                highLightIconSource: "qrc:/UI/photo/right_export.svg"
                pressedIconSource: "qrc:/UI/photo/right_export.svg"
                cusTooltip.text: qsTr("Export")
                btnRadius: 5
                borderWidth: 0
                onClicked: {
                    saveDialog.open()
                }
            }
        }
        Rectangle {
            Layout.preferredWidth: paramCfgC.width - 20*screenScaleFactor
            Layout.fillHeight: true
            border.color: Constants.right_panel_menu_border_color
            border.width: 1
            color: Constants.right_panel_combobox_background_color
            Layout.alignment: Qt.AlignHCenter
            radius: 5
            clip: true
            ListView {
                id: idProfileListView
                spacing: 0
                focus: true
                anchors.margins: 2
                anchors.fill: parent
                currentIndex: currentMachine ? currentMachine.curProfileIndex : 0
                delegate: idProfileDelegate
                onCurrentIndexChanged: {
                    console.log("=================currentIndex = ", currentIndex)
                }
            }
        }
        Item{
            Layout.preferredHeight: 10
        }
        RowLayout {
            spacing: 6
            Layout.alignment: Qt.AlignHCenter
            CusImglButton {
                Layout.preferredWidth: 82*screenScaleFactor
                Layout.preferredHeight: 24*screenScaleFactor
                id: __addButton
                enabled: true
                shadowEnabled: false
                defaultBtnBgColor: Constants.right_panel_button_default_color
                hoveredBtnBgColor: Constants.right_panel_button_hovered_color
                selectedBtnBgColor: Constants.right_panel_button_checked_color
                borderColor:  Constants.right_panel_border_default_color
                enabledIconSource:"qrc:/UI/photo/rightDrawer/profile_addBtn.svg"
                pressedIconSource:"qrc:/UI/photo/rightDrawer/profile_addBtn_d.svg"
                state: "imgOnly"
                borderWidth: 1
                imgWidth : 14* screenScaleFactor
                imgHeight : 14* screenScaleFactor
                allowTooltip : true
                cusTooltip.text: qsTr("Add")
                onClicked: {
                    idAddProfile.showAddProfile(currentMachine)
                }
            }
            CusImglButton {
                Layout.preferredWidth: 82*screenScaleFactor
                Layout.preferredHeight: 24*screenScaleFactor
                id: __editButton
                enabled: true
                shadowEnabled: false
                defaultBtnBgColor: Constants.right_panel_button_default_color
                hoveredBtnBgColor: Constants.right_panel_button_hovered_color
                selectedBtnBgColor: Constants.right_panel_button_checked_color
                borderColor:  Constants.right_panel_border_default_color
                enabledIconSource:"qrc:/UI/photo/rightDrawer/profile_editBtn.svg"
                pressedIconSource:"qrc:/UI/photo/rightDrawer/profile_editBtn_d.svg"
                state: "imgOnly"
                borderWidth: 1
                imgWidth : 14* screenScaleFactor
                imgHeight : 14* screenScaleFactor
                allowTooltip : true
                cusTooltip.text: qsTr("Edit")
                onClicked: {
                    showEditProfile(1);
                }
            }
            CusImglButton {
                Layout.preferredWidth: 82*screenScaleFactor
                Layout.preferredHeight: 24*screenScaleFactor
                id: idDeleteButton
                enabled: !(currentMachine ? currentMachine.isCurrentProfileDefault : false)
                shadowEnabled: false
                defaultBtnBgColor: Constants.right_panel_button_default_color
                hoveredBtnBgColor: Constants.right_panel_button_hovered_color
                selectedBtnBgColor: Constants.right_panel_button_checked_color
                borderColor:  Constants.right_panel_border_default_color
                enabledIconSource:"qrc:/UI/photo/rightDrawer/profile_delBtn.svg"
                pressedIconSource:"qrc:/UI/photo/rightDrawer/profile_delBtn_d.svg"
                state: "imgOnly"
                borderWidth: 1
                imgWidth : 14* screenScaleFactor
                imgHeight : 14* screenScaleFactor
                allowTooltip : true
                cusTooltip.text: qsTr("Delete")
                onClicked: {
                    warningDlg.msgText = qsTr("Do you want to delete the selected slice configuration parameters?")
                    warningDlg.visible = true
                    //idProfileListView.currentIndex = currentMachine.curProfileIndex()
                }
            }
        }
        Item{
            Layout.preferredHeight: 5
        }
    }
    Loader {
        id: slicerPg
        onLoaded: slicerPg.item.show()
    }
    AddEditProfile {
        id: idAddProfile
        visible: false
        onSigAddProfile:{
            rootRec.currentMachine.addProfile(newProfileName, templateObject)
            //idProfileListView.currentIndex = currentMachine.curProfileIndex()
            showEditProfile(0)
        }
    }

    function deleteFunc() {
        currentMachine.removeProfile(currentMachine.currentProfileObject())
    }

    UploadMessageDlg{
        id: warningDlg
        onSigOkButtonClicked: {
            console.log("______________")
            deleteFunc()
            warningDlg.visible = false
        }
    }

    FileDialog {
        id: inputDialog
        title: qsTr("Import")
        fileMode: FileDialog.OpenFile
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        nameFilters: ["Profiles (*.cxprofile)", "All files (*)"]
        onAccepted: {
            currentMachine.importProfileFromQmlIni(inputDialog.currentFile);
            //idProfileListView.currentIndex = currentMachine.curProfileIndex()
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Export")
        fileMode: FileDialog.SaveFile
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        defaultSuffix : "cxprofile"
        nameFilters: ["Profiles (*.cxprofile)", "All files (*)"]
        onAccepted: {
            //            currentMachine.exportProfileFromQml(saveDialog.currentFile)
            currentMachine.exportProfileFromQmlIni(saveDialog.currentFile)
        }
    }

    ParameterProfilePanel{
        id : idParameterProfile
        visible : false

        onRemoveProfile:{
            currentMachine.removeProfile(profileObject)
            //idProfileListView.currentIndex = currentMachine.curProfileIndex()
        }

        onVisibleChanged: {
            if(!visible){
                currentMachine.profilesModel().notifyReset()
            }
        }
    }

    //    EditProfile {
    //        id: idEditProfile
    //        visible: false
    //    }

    Component {
        id: idProfileDelegate
        Rectangle {
            property bool isChecked: index === 0
            property bool checked: isChecked
            property bool isHovered: false
            property string normalColor: isHovered ? Constants.right_panel_item_hovered_color
                                                   : Constants.right_panel_item_default_color
            width: idProfileListView.width
            height: 24*screenScaleFactor
            color: ListView.isCurrentItem ? Constants.right_panel_item_checked_color : normalColor
            Image {
                id: img1
                property string defaultImage: getDefaultImg(profileName)
                property string checkedImage: getCheckImg(profileName)
                width: 12*screenScaleFactor
                height: 12*screenScaleFactor
                anchors.left: parent.left
                anchors.leftMargin: 10*screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: idProfileListView.currentIndex === model.index ? checkedImage : defaultImage
                function getCheckImg(profileName){
                    var checkedImage;
                    if(profileName === "High Quality"){
                        checkedImage = Constants.right_panel_quality_high_checked_image
                    }
                    if(profileName === "Quality"){
                        checkedImage = Constants.right_panel_quality_middle_checked_image
                    }
                    if(profileName === "Normal"){
                        checkedImage = Constants.right_panel_quality_low_checked_image
                    }
                    if(profileName === "Fast"){
                        checkedImage = Constants.right_panel_quality_verylow_checked_image
                    }
                    if(!profileIsDefault){
                        checkedImage = Constants.right_panel_quality_custom_checked_image
                    }
                    return checkedImage;
                }
                function getDefaultImg(profileName){
                    if(!profileIsDefault){
                        return Constants.right_panel_quality_custom_default_image
                    }
                    if(profileName === "High Quality"){
                        return Constants.right_panel_quality_high_default_image
                    }
                    if(profileName === "Quality"){
                        return Constants.right_panel_quality_middle_default_image
                    }
                    if(profileName === "Normal"){
                        return Constants.right_panel_quality_low_default_image
                    }
                    if(profileName === "Fast"){
                        return Constants.right_panel_quality_verylow_default_image
                    }else{
                        return ""
                    }
                }
            }
            CusText {
                id: profileText
                anchors.left: img1.right
                anchors.leftMargin: 7
                anchors.verticalCenter: parent.verticalCenter
                fontText: qsTr(profileName)
                fontColor: Constants.right_panel_text_default_color
                hAlign: 0
            }
            CusText {
                id: profileLayerText
                anchors.left: profileText.right
                anchors.leftMargin: 7
                anchors.verticalCenter: parent.verticalCenter
                fontText: profileLayerHeight.toFixed(2) + "mm"
                fontColor: Constants.right_panel_text_default_color
                hAlign: 0
            }
            BasicTooltip {
                visible: parent.isHovered
                textWrap: true
                textWidth: 360 * screenScaleFactor
                backgroundColor: profileTipBgColor
                position: BasicTooltip.Position.LEFT
                font.pointSize: Constants.labelFontPointSize_10
                timeout: -1
                text: qsTr("The main parameter configuration of this mode: Layer Height(") + profileLayerHeight.toFixed(2)
                      + qsTr("mm),Line Width(") + profileLineWidth.toFixed(2)
                      + qsTr("mm),Wall Line Count(") + profileWallLineCount.toFixed(2)
                      + qsTr("mm),Infill Density(") + profileInfillSparseDensity.toFixed(2)
                      + qsTr("%),Build Plate Adhesion Type(") + qsTr(profileAdhesionType)
                      + qsTr("),Printing Temperature(") + profileMaterialPrintTemperature.toFixed(2)
                      + qsTr("℃),Build Plate Temperature(") + profileMaterialBedTemperature.toFixed(2) + "℃), "
                      + qsTranslate("ParameterGeneralCom", "Inner Wall Speed") + "(" + profileSpeedInner.toFixed(2) + "mm/s), "
                      + qsTranslate("ParameterGeneralCom", "Outer Wall Speed") + "(" + profileSpeedOuter.toFixed(2) + "mm/s)"
            }
            MouseArea {
                id:ma
                anchors.fill: parent
                hoverEnabled: true
                propagateComposedEvents: true
                onEntered: {
                    parent.isHovered = true;
                }
                onExited: {
                    parent.isHovered = false;
                }
                onClicked: {


                    if(currentMachine){
                        currentMachine.setCurrentProfile(index)
                        //                        idDeleteButton.enabled = !currentMachine.isCurrentProfileDefault()
                        kernel_parameter_manager.syncCurParamToVisScene()
                        idProfileListView.currentIndex = index;
                        idProfileListView.currentIndex = Qt.binding(function(){ return currentMachine.curProfileIndex; });
                    }

                    mouse.accepted = false;
                }
                onDoubleClicked: {
                    showEditProfile(1)
                }
            }
        }
    }
}
