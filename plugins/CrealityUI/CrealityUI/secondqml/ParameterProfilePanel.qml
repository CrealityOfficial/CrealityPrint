import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import QtQml 2.13
import ".."
import "../components"
import "../qml"

DockItem
{
    id: editProfiledlg

    width: 1040 * screenScaleFactor
    height: 635 * screenScaleFactor
    title: qsTr("Edit")
    titleHeight: 30* screenScaleFactor
    property int spacing: 5
    property int addOrEdit: 0 //0: add, 1: edit

    property var currentProfile
    property var currentMachine
    property var currentMaterial
    property var currentExtruder
    property string curProfileName
    property bool isSingleExtruder : false

    property string currentCategry: "resolution"
    signal removeProfile(var profileObject)

    function clearSearchText(){
        if(idSearch.text == "")
            return;
        idSearch.text = ""
        idCategoryRepeater.model = currentProfile.profileCategoryModel(isSingleExtruder)
        var profileModel = currentProfile.profileParameterModel(isSingleExtruder)
        profileModel.search(currentCategry,idSearch.text,idAdvance.checked,isSingleExtruder,idSearch.text);
        changeCategory(idCategoryRepeater.model.getFirtCategory())
    }

    function startEditProfile(machine, _addOrEdit, extruderCount) {

        console.log("++++++++++++++startEditProfile")
        if(currentMachine !== machine)
        {
            currentCategry = "resolution"
        }
        currentMachine = machine
        currentProfile = currentMachine.profileObject(currentMachine.curProfileIndex)
        addOrEdit = _addOrEdit
        currentExtruder = currentMachine.extruderObject(0);
        currentMaterial = currentMachine.materialObject(currentExtruder.materialName())

        isSingleExtruder = extruderCount === 1
        idProfileName.text = findTranslate(currentProfile.name())
        curProfileName = idProfileName.text
        idCategoryRepeater.model = null
        idCategoryRepeater.model = currentProfile.profileCategoryModel(isSingleExtruder)

        var profileModel = currentProfile.profileParameterModel(isSingleExtruder)
        idParameterRepeater.model = profileModel
        changeCategory(currentCategry)

        visible = true
    }

    function endEditProfile() {
//        currentProfile = null
        close()
    }

    function changeCategory(category){
        currentCategry = category
        updateFilter()
    }

    function updateFilter(){
        idParameterRepeater.model.setFilter(currentCategry, idSearch.text, idAdvance.checked)
    }

    function findTranslate(key){
        var quality = ""
        var tempArray = key.split("_")
        quality = tempArray[tempArray.length-1]
        var tranlateValue = ""
        if(quality === "high")tranlateValue = qsTr("High Quality")
        else if(quality === "middle") tranlateValue= qsTr("Quality")
        else if(quality === "low")tranlateValue = qsTr("Normal")
        else if(quality === "best")tranlateValue = qsTr("Best quality")
        else if(quality === "fast")tranlateValue = qsTr("Fast")
        else
        {
            tranlateValue = key
        }
        return tranlateValue
    }

    function searchEditChanged(){
        var materialSettings = currentMaterial.materialParameterModel(0).search(currentCategry,idSearch.text,idAdvance.checked);
        var extruderSettings = currentExtruder.extruderParameterModel("",idAdvance.checked).search(currentCategry,idSearch.text,idAdvance.checked);
        var profileModel = currentProfile.profileParameterModel(isSingleExtruder)
        profileModel.search(currentCategry,idSearch.text,idAdvance.checked);
        if(isSingleExtruder)
        {
            profileModel.addSearchSettings(materialSettings)
            profileModel.addSearchSettings(extruderSettings)
        }
        if (idSearch.text.trim().length >0)
        {
            var lmodel = profileModel.searchCategoryModel(idAdvance.checked,isSingleExtruder,idSearch.text)
            if(lmodel)
            {
                idCategoryRepeater.model = lmodel;
            }else{
                idCategoryRepeater.model = currentProfile.profileCategoryModel(isSingleExtruder)
            }
        }else{
            idCategoryRepeater.model = currentProfile.profileCategoryModel(isSingleExtruder)
        }
        changeCategory(idCategoryRepeater.model.getFirtCategory())
    }

    onVisibleChanged: {
        clearSearchText()
    }

    ParameterComponent{
        id:gCom
    }

    Item{
        anchors.fill: parent
        anchors.top: editProfiledlg.top
        anchors.topMargin: editProfiledlg.titleHeight

        Rectangle{
            id: wrapperRec
            //            border.width: 1* screenScaleFactor
            //            border.color: Constants.leftBtnBgColor_hovered
            radius: 5* screenScaleFactor
            anchors.top: parent.top
            anchors.topMargin: 20*screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 10* screenScaleFactor
            anchors.right: parent.right
            anchors.rightMargin: 10* screenScaleFactor
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 69* screenScaleFactor
            color:"transparent"
            Item
            {
                anchors.fill: parent
                anchors.margins: 20
                property int spacing: 5

                ButtonGroup { id: leftBtnsGroup }

                RowLayout
                {
                    id:topRow
                    width: parent.width
                    anchors.top: parent.top
                    anchors.topMargin: 7*screenScaleFactor
                    spacing:10
                    StyledLabel {
                        id: element
                        text: qsTr("Profile:")
                        Layout.preferredHeight: 25* screenScaleFactor
                        color:  "#cbcbcc"
                    }

                    StyledLabel {
                        id: idProfileName
                        Layout.preferredHeight: 25*screenScaleFactor
                        height: 25* screenScaleFactor
                        color: "#cbcbcc"
                    }

                    Item{
                        Layout.preferredWidth: 100*screenScaleFactor
                    }

                    StyledLabel {
                        Layout.preferredHeight: 25*screenScaleFactor
                        height: 25* screenScaleFactor
                        color: "#cbcbcc"
                        text: qsTr("Advanced")
                    }

                    CusSwitchBtn{
                        id : idAdvance
                        Layout.preferredWidth: 50*screenScaleFactor
                        Layout.preferredHeight: 28*screenScaleFactor
                        btnHeight: 22*screenScaleFactor
                        Layout.bottomMargin: 8*screenScaleFactor
                        onCheckedChanged: {
                            updateFilter()
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    BasicLoginTextEdit
                    {
                        id: idSearch
                        placeholderText: qsTr("Search")
                        Layout.preferredWidth : 260*screenScaleFactor
                        Layout.preferredHeight : 28*screenScaleFactor
                        font.pointSize:Constants.labelFontPointSize_9
                        headImageSrc:hovered ? Constants.sourchBtnImg_d : Constants.sourchBtnImg
                        tailImageSrc: hovered && idSearch.text != "" ? Constants.clearBtnImg : ""
                        hoveredTailImageSrc:Constants.clearBtnImg_d
                        radius : 14
                        text: ""
                        onEditingFinished:{
                            searchEditChanged();
                        }
                        onTailBtnClicked:
                        {
                            console.log("++++++++++onTailBtnClicked")
                            clearSearchText()
                        }
                    }

                    BasicButton {
                        id: idSearchButton
                        Layout.preferredWidth:  66*screenScaleFactor
                        Layout.preferredHeight: 28*screenScaleFactor
                        btnRadius:13
                        btnBorderW:0
                        enabled: idSearch.text != "" ? true : false
                        defaultBtnBgColor: enabled ?  Constants.searchBtnNormalColor : Constants.searchBtnDisableColor
                        hoveredBtnBgColor: Constants.searchBtnHoveredColor
                        text: qsTr("Search")
                        onSigButtonClicked:{
                            var materialSettings = currentMaterial.materialParameterModel(0).search(currentCategry,idSearch.text,idAdvance.checked);
                            var extruderSettings = currentExtruder.extruderParameterModel("",idAdvance.checked).search(currentCategry,idSearch.text,idAdvance.checked);
                            var profileModel = currentProfile.profileParameterModel(isSingleExtruder)
                            profileModel.search(currentCategry,idSearch.text,idAdvance.checked);
                            if(isSingleExtruder)
                            {
                                profileModel.addSearchSettings(materialSettings)
                                profileModel.addSearchSettings(extruderSettings)
                            }
                            var lmodel = profileModel.searchCategoryModel(idAdvance.checked,isSingleExtruder,idSearch.text)
                            if(lmodel)
                            {
                                idCategoryRepeater.model = lmodel;
                            }else{
                                idCategoryRepeater.model = currentProfile.profileCategoryModel(isSingleExtruder)
                            }
                            changeCategory(idCategoryRepeater.model.getFirtCategory())
                        }
                    }
                }

                Item {
                    id:depItem
                    width: editProfiledlg.width - 40*screenScaleFactor
                    height : 2
                    anchors.top: topRow.bottom
                    anchors.topMargin: 10*screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    Rectangle {
                        anchors.centerIn: parent
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
                RowLayout
                {
                    anchors.top: depItem.bottom
                    anchors.topMargin: 20* screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 20
                    width: parent.width

                    ListView{
                        id: idCategoryRepeater
                        Layout.preferredWidth: 200*screenScaleFactor
                        Layout.preferredHeight: 430*screenScaleFactor
                        spacing: 3*screenScaleFactor
                        clip : true
                        delegate: gCom.leftBtnCom
                    }

                    //                    Item{
                    //                        Layout.fillWidth: true
                    //                    }

                    BasicScrollView
                    {
                        id: idParamScrollView
                        Layout.fillWidth: true
                        hpolicyVisible: false
                        vpolicyVisible: contentHeight > height
                        Layout.preferredHeight: 430*screenScaleFactor
                        padding: 10* screenScaleFactor
                        clip : true

                        background: Rectangle{
                            color: Constants.right_panel_item_default_color
                            radius: 5 * screenScaleFactor
                            border.width: 1*screenScaleFactor
                            border.color: Constants.rectBorderColor
                        }

                        ColumnLayout{
                            width: idParamScrollView.contentItem.width
                            Repeater{
                                id: idParameterRepeater
                                delegate: gCom.itemRow
                            }

                            Item{
                                Layout.fillHeight: true
                            }
                        }
                    }
                }
            }
        }

        Item {
            width: parent.width
            height:  30* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20 * screenScaleFactor
            Row
            {
                spacing: 10
                anchors.centerIn:parent
                BasicButton {
                    id: basicComButton1
                    width: 98* screenScaleFactor
                    height: 28* screenScaleFactor
                    text: qsTr("Save")
                    btnRadius:14* screenScaleFactor
                    btnBorderW:1* screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        currentProfile.save()
                        endEditProfile()
                        kernel_parameter_manager.syncCurParamToVisScene()
                    }
                }

                BasicButton {
                    id: basicComButton
                    width: 98* screenScaleFactor
                    height: 28* screenScaleFactor
                    text: qsTr("Reset")
                    btnRadius: 14* screenScaleFactor
                    btnBorderW:1* screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        warningDlg.msgText = qsTr("Do you want to reset the selected slice configuration parameters?")
                        warningDlg.visible = true
                    }
                }

                BasicButton
                {
                    id : comCancel
                    width: 98* screenScaleFactor
                    height: 28* screenScaleFactor

                    text: qsTr("Cancel")
                    btnRadius:14* screenScaleFactor
                    btnBorderW:1* screenScaleFactor
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered

                    onSigButtonClicked:
                    {
                        if(!addOrEdit)
                            removeProfile(currentProfile)

                        endEditProfile()
                    }
                    Connections{//关闭和取消进行相同的操作
                        target: editProfiledlg
                        onDialogClosed:{
                            if(!addOrEdit)
                                removeProfile(currentProfile)

                            endEditProfile()
                        }
                    }
                }
            }
        }
    }

    function deleteFunc() {
        currentProfile.reset()
        currentMaterial.reset()
        currentExtruder.reset(currentExtruder.materialName(),idAdvance.checked)
        updateFilter()
    }

    UploadMessageDlg{
        id: warningDlg
        onSigOkButtonClicked: {
            console.log("______________")
            deleteFunc()
            warningDlg.visible = false
        }
    }

    BasicDialog
    {
        id: idMessageDlg
        width: 400* screenScaleFactor
        height: 200* screenScaleFactor
        titleHeight : 30
        title: qsTr("Message")

        Rectangle{
            anchors.centerIn: parent
            width: parent.width/2
            height: parent.height/2
            color: "transparent"
            Text {
                id: name
                anchors.centerIn: parent
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                text: qsTr("Invalid parameter!!")
                font.pointSize: Constants.labelFontPointSize_6
                color: Constants.textColor
            }
        }
    }
}
