import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

import ".."
import "../qml"

BasicDialog
{
    id: editProfiledlg

    width: 700 * screenScaleFactor
    height: 557 * screenScaleFactor
    title: qsTr("Edit")
    //titleHeight : 30
    property int spacing: 5

	property var currentMachine : null
	property var currentProfile : null

	property int addOrEdit: 1 //0: add, 1: edit

	property var simpleProfileLabelMap: {0:0}
	property var simpleProfileItemMap: {0:0}

	property var profileButtonMap: {0:0}
	property var profileLabelMap: {0:0}
	property var profileItemMap: {0:0}
	property var classTypeFilter: "Quality"

	function showEditProfile(_addOrEdit, machine){
		addOrEdit = _addOrEdit
		currentMachine = machine
		currentProfile = machine.currentProfileObject

		idProfileName.text = findTranslate(currentProfile.name())
		idEditFilterProfileName.text = findTranslate(currentProfile.name())

		if(simpleProfileLabelMap[0] === 0){
			simpleProfileLabelMap = {}
			simpleProfileItemMap = {}
		}

		visible = true
	}

	function hideEditProfile(){
		currentProfile.cancel()
		currentMachine = null
		currentProfile = null

		editProfiledlg.close()
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

    function updateLanguage(){
        idEditProfileByFilter.updateLanguage()
    }

    Item//Rectangle
    {
        id: rectangle
        x:30* screenScaleFactor
        y :25 + titleHeight
        width: parent.width-40* screenScaleFactor
        height: parent.height-titleHeight-50* screenScaleFactor
        Grid{
            width: parent.width
            height: parent.height
            rows: 6
            spacing: 20
            Row {
                width: parent.width
                height: 25* screenScaleFactor
                y: 5* screenScaleFactor
                spacing: 10
                StyledLabel {
                    id: element
                    text: qsTr("Profile:")
                    width: 163* screenScaleFactor
                    height: 25* screenScaleFactor
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignRight
                }
                StyledLabel {//by TCJ
                    id: idProfileName
                    width: 160* screenScaleFactor
                    height: 25* screenScaleFactor
                    verticalAlignment: Qt.AlignVCenter
                }

                StyledLabel {
                    id: elementMaterial
                    text: qsTr("Material:")
					visible : false
                    width: 160* screenScaleFactor
                    height: 25* screenScaleFactor
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignRight
                }

                StyledLabel{
                    height: 25* screenScaleFactor
                    width: 140* screenScaleFactor
					visible: false
                    verticalAlignment: Qt.AlignVCenter
                }
            }

            Item {
                width: editProfiledlg.width - 13* screenScaleFactor
                height : 2

                Rectangle {
                    x: -23* screenScaleFactor
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

            BasicGroupBox {
                id: basicGroupBox

                width: parent.width
                height: 340* screenScaleFactor
                BasicScrollView{
                    width: parent.width
                    height: 340* screenScaleFactor
                    hpolicy: ScrollBar.AlwaysOff
                    vpolicy: ScrollBar.AsNeeded
                    clip : true
                    Grid{
                        anchors.fill:parent
                        columns: 2
                        columnSpacing: 40
                        Grid{
                            id: idGrid1
                            columns: 2
                            rows: 10
                            rowSpacing: 7
                            columnSpacing: 10
                        }
                        Grid{
                            id: idGrid2
                            columns: 2
                            rows: 10
                            rowSpacing: 7
                            columnSpacing:10
                        }
                    }
                }
            }

            Item {
                width: editProfiledlg.width - 13* screenScaleFactor
                height : 2* screenScaleFactor

                Rectangle {
                    // anchors.left: idCol.left
                    // anchors.leftMargin: -10
                    x: -23* screenScaleFactor
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

            Item {
                width: parent.width
                height:  30* screenScaleFactor
                Grid {
                    columns: 4
                    spacing: 10
                    width: 430* screenScaleFactor
                    height: 30* screenScaleFactor
                    anchors.centerIn:parent
                    BasicButton {
                        id: basicComButton1
                        width: 98* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Save")
                        btnRadius:3
                        btnBorderW:0
                        defaultBtnBgColor: Constants.profileBtnColor
                        hoveredBtnBgColor: Constants.profileBtnHoverColor
                        onSigButtonClicked:
                        {
                            currentProfile.save()
							hideEditProfile()
                        }
                    }

                    BasicButton {
                        id: basicComButton
                        width: 98* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Reset")
                        btnRadius:3
                        btnBorderW:0
                        defaultBtnBgColor: Constants.profileBtnColor
                        hoveredBtnBgColor: Constants.profileBtnHoverColor
                        onSigButtonClicked:
                        {
                            currentProfile.reset()
                        }
                    }

                    BasicButton {
                        id: basicComButton3
                        width: 98* screenScaleFactor
                        height: 28* screenScaleFactor
                        text: qsTr("Advance Setting")
                        btnRadius:3
                        btnBorderW:0
                        defaultBtnBgColor: Constants.profileBtnColor
                        hoveredBtnBgColor: Constants.profileBtnHoverColor
                        onSigButtonClicked:{
                            idEditProfileFilter.showEditProfileFilter()
                        }
                    }

                    BasicButton {
                        id : comCancel
                        width: 98* screenScaleFactor
                        height: 28* screenScaleFactor

                        text: qsTr("Cancel")
                        btnRadius:3
                        btnBorderW:0
                        defaultBtnBgColor: Constants.profileBtnColor
                        hoveredBtnBgColor: Constants.profileBtnHoverColor

                        onSigButtonClicked:{
							if (addOrEdit == 0)
								currentMachine.removeProfile(currentProfile.name());
                            hideEditProfile()
                        }
                        Connections{//关闭和取消进行相同的操作
                            target: editProfiledlg
                            onDialogClosed:{
								if (addOrEdit == 0)
									currentMachine.removeProfile(currentProfile.name());
                                hideEditProfile()
                            }
                        }
                    }
                }

            }

        }
    }

    BasicDialog {
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
                font.pixelSize: Constants.labelFontPixelSize
                color: Constants.textColor
            }
        }
    }

    Component {
        id: someComponent
        ListModel {
        }
    }

	BasicDialog {
		id: idEditProfileFilter

		width: 700 * screenScaleFactor
		height: 670 * screenScaleFactor
		title: qsTr("Edit")
		property int spacing: 5

		function createProfileButtons(){
			if(profileButtonMap[0] === 0){
				console.log("idEditProfileFilter createProfileButtons profileButtonMap.")
				profileButtonMap = {}
			}
		}

		function createProfileButton(key){
			console.log("createProfileButton : " + key)
			var componentButton = Qt.createComponent("../qml/BasicButton.qml")
			if (componentButton.status === Component.Ready) {
				var obj = componentButton.createObject(profileTypeBtnList, {"width": 120*screenScaleFactor,
																			"height" : 30*screenScaleFactor,
																			"btnRadius" : 3,
																			"keyStr" : key,
																			"pointSize":Constants.labelFontPointSize,
																			"btnBorderW":0,
																			"defaultBtnBgColor": Constants.dialogContentBgColor,
																			"hoveredBtnBgColor": Constants.typeBtnHoveredColor,
																			"text": qsTr(key)}
																			)
				obj.sigButtonClickedWithKey.connect(onProfileTypeChanged)
				profileButtonMap[key] = obj
			}
		}

		function createProfileValues(profileType){
			if(!profileLabelMap[profileType]){

				profileLabelMap[profileType] = {}
				profileItemMap[profileType] = {}
			}
		}

		function showEditProfileFilter(){
			createProfileButtons()
			onProfileTypeChanged(classTypeFilter)

			filterProfileTypeList()
			this.visible = true
		}

		function hideEditProfileFilter(){
			this.close()
		}

		function onProfileTypeChanged(type){
			for(var key in profileButtonMap)
			{
				if(key == type){
					profileButtonMap[key].defaultBtnBgColor = "#1E9BE2"//"#1EB6E2"
					classTypeFilter = key

					createProfileValues(classTypeFilter)
					filterProfileValueList()
				}
				else
				{
					profileButtonMap[key].defaultBtnBgColor = Constants.dialogContentBgColor
				}
			}
			idParamScrollView.setVScrollBarPosition(0)
		}

		function filterProfileTypeList(){
			var strArray = []
			for(var key in profileButtonMap){
				var nameFilter = idSearch.text

				var lable = key.toLowerCase()
				var paramlevel = (key == "Special Machine Type" || key == "Mesh Fixes") ? 3 : 1
				if(nameFilter == "")
				{
					if((idConfiguration.currentIndex + 1) >= paramlevel)
					{
						strArray.push(key)
					}
				}
				else
				{
					if(lable.search(nameFilter.toLowerCase()) != -1 && (idConfiguration.currentIndex + 1) >= paramlevel)
					{
						strArray.push(key)
					}
				}
			}

			for(var key in profileButtonMap){
				profileButtonMap[key].visible = false
			}

			for(var key in strArray)
			{
				profileButtonMap[strArray[key]].visible = true
			}

		}

		function filterProfileValueList(){
			var roscnt = 1
			var nameFilter = idSearch.text
			for(var classType in profileLabelMap)
			{
			    if(classType == 0)
			    {
			        continue
			    }

				var labelContainer = profileLabelMap[classType]
				var itemContainer = profileItemMap[classType]
				for(var key in labelContainer)
				{
				}
			}
			profileValuelList.rows = roscnt
		}

		Item {
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
							id: idFilterProfile
                            text: qsTr("Profile:")
							width: idFilterProfile.contentWidth//70
							height: 30*screenScaleFactor
							verticalAlignment: Qt.AlignVCenter
							horizontalAlignment: Qt.AlignLeft
						}
						StyledLabel {
							id: idEditFilterProfileName
							width: idFilterProfile.contentWidth//150
							height: 30*screenScaleFactor
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
							width : 200*screenScaleFactor
							font.pointSize:Constants.labelFontPointSize
							headImageSrc:hovered ? Constants.searchBtnImg_d : Constants.searchBtnImg
							tailImageSrc: hovered && idSearch.text != "" ? Constants.clearBtnImg : ""
							hoveredTailImageSrc:Constants.clearBtnImg_d
							radius : 14
							text: ""
							onEditingFinished: {
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
							onSigButtonClicked:{
								idEditProfileFilter.filterProfileValueList()
								idEditProfileFilter.filterProfileTypeList()
								//showProfileType()
							}
						}

						Rectangle {
							color: "transparent"
							width: 175*screenScaleFactor - idFilterProfile.contentWidth - element.contentWidth
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
								onClicked:{
									idConfiguration.currentIndex = 1
									idEditProfileFilter.filterProfileValueList()
									idEditProfileFilter.filterProfileTypeList()
								}
							}
							BasicRadioButton {
								id: idTextOutside
								text: qsTr("Expert")
								ButtonGroup.group: idConfigurationGroup
								onClicked:{
									idConfiguration.currentIndex = 2
									idEditProfileFilter.filterProfileValueList()
									idEditProfileFilter.filterProfileTypeList()
								}
							}
						}
					}
				}
				Item {
					width: idEditProfileFilter.width - 13
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
					BasicScrollView {
						width: 125*screenScaleFactor
						height: 450*screenScaleFactor
						hpolicy: ScrollBar.AlwaysOff
						vpolicy: ScrollBar.AsNeeded
						clip : true
						Column
						{
							id: profileTypeBtnList
							width:125
						}
					}

					Item {
						width:2
						height: 450*screenScaleFactor
						BasicSeparator{
							anchors.fill: parent
						}
					}
					BasicScrollView	{
						id: idParamScrollView
						width: 500*screenScaleFactor
						height: 450*screenScaleFactor
						hpolicy: ScrollBar.AlwaysOff
						vpolicy: ScrollBar.AsNeeded
						clip : true
						Grid{
							id: profileValuelList
							columns: 2
							rows: 6
							rowSpacing: 5
							columnSpacing: 10
						}
					}
				}
				Item {
					width: idEditProfileFilter.width - 13*screenScaleFactor
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
						width: 125 * screenScaleFactor
						height: 28 * screenScaleFactor
						btnRadius:3
						btnBorderW:0
						defaultBtnBgColor: Constants.profileBtnColor
						hoveredBtnBgColor: Constants.profileBtnHoverColor
						text: qsTr("Reset")
						onSigButtonClicked: {
							idEditProfileFilter.hideEditProfileFilter()
						}
					}

					BasicButton {
						width: 125*screenScaleFactor
						height: 28*screenScaleFactor
						btnRadius:3
						btnBorderW:0
						defaultBtnBgColor: Constants.profileBtnColor
						hoveredBtnBgColor: Constants.profileBtnHoverColor
						text: qsTr("Save")
						onSigButtonClicked:{
							idEditProfileFilter.hideEditProfileFilter()
						}
					}

					BasicButton {
						width: 125*screenScaleFactor
						height: 28*screenScaleFactor
						btnRadius:3
						btnBorderW:0
						defaultBtnBgColor: Constants.profileBtnColor
						hoveredBtnBgColor: Constants.profileBtnHoverColor
						text: qsTr("Cancel")
						onSigButtonClicked: {
							idEditProfileFilter.hideEditProfileFilter()
						}
					}
				}
			}
		}
	}

}
