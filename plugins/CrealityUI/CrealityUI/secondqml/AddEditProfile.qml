import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
//import CrealityUI 1.0
//import "qrc:/CrealityUI"
import "../qml"
import "../components"
DockItem
{
    id: addEditProfiledlg

    width: 600*screenScaleFactor
    height: 270*screenScaleFactor
    titleHeight : 30*screenScaleFactor
    title: qsTr("New")
	property var currentMachine
    signal sigAddProfile(string newProfileName, var templateObject)

	function showAddProfile(machine){
		currentMachine = machine
		if(!currentMachine)
			return;
		
		checkProfileName()
		
		idProfileName.text = currentMachine.generateNewProfileName()
        idProfileList.model = currentMachine.profileNames()
		idProfileList.currentIndex = currentMachine.curProfileIndex
		idPrintList.model = kernel_parameter_manager.machinesNames()
		idPrintList.currentIndex = kernel_parameter_manager.curMachineIndex
					
		visible = true
	}
	
	function hideAddProfile(){
		close()
	}
	
	function tryAddProfile(){
		hideAddProfile()
		sigAddProfile(idProfileName.text, currentMachine.profileObject(idProfileList.currentIndex).profileFileName())
	}
	
	function checkProfileName()
	{
        var res = currentMachine.checkProfileName(idProfileName.text)
		errorMsg.fontText = qsTr("Name repeater, please rename!")
        if(res){
            basicComButton.enabled = false
            errorMsg.visible = true
        }else{
			if(!idProfileName.text.match("^(?! +$).*$") || idProfileName.text.length ===0){
                errorMsg.fontText = qsTr("The input can't be empty!!")
				basicComButton.enabled = false
				errorMsg.visible = true
            }else{
				basicComButton.enabled = true
				errorMsg.visible = false
			}
        }
	}

    Item //Rectangle
    {
        id: rectangle
        x:5*screenScaleFactor
        y :10*screenScaleFactor + titleHeight
        width: parent.width
        height: parent.height-titleHeight-20
        Grid
        {
            y:40*screenScaleFactor
            x:40*screenScaleFactor
            width: 350*screenScaleFactor
            height : 120*screenScaleFactor
            rowSpacing: 10
            rows: 4
            columns: 2

            StyledLabel {
                id: element
                width: 120*screenScaleFactor
                height : 30*screenScaleFactor
                visible:true
                text: qsTr("New Profile") + ":"
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }

            BasicDialogTextField {
                id: idProfileName
                width: 405*screenScaleFactor
                height: 30*screenScaleFactor
                visible:true
                cursorVisible: false
                validator: RegExpValidator { regExp: /^[^\\/:*?<>|]*$/ }

                onTextChanged: {
					checkProfileName()
                }
            }
            StyledLabel {
                id: element2
                text: qsTr("Printer") + ":"
                width: 120*screenScaleFactor
                height : 30*screenScaleFactor
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }

            CXComboBox {
                id: idPrintList
                width: 405*screenScaleFactor
                height: 30*screenScaleFactor
                enabled:false
                onCurrentIndexChanged: {
                }
            }

            StyledLabel {
                id: element4
                text: qsTr("Copy Profile From") + ":"
                width: 120*screenScaleFactor
                height : 30*screenScaleFactor
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }

            CXComboBox {
                id: idProfileList
                width: 405*screenScaleFactor
                height: 30*screenScaleFactor
                onCurrentIndexChanged: {
                }
            }
            CusText{
                id:errorMsg
                fontWidth: 120*screenScaleFactor
                visible: false
                fontColor: "red"
                Layout.columnSpan: 2
                fontText:qsTr("Name repeater, please rename!")
            }

            StyledLabel {
                width: 120*screenScaleFactor
                height : 30*screenScaleFactor
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }
        }

        Grid
        {
            y: 180*screenScaleFactor
            width : 210*screenScaleFactor
            height: 30*screenScaleFactor
            columns: 2
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            BasicDialogButton {
                id: basicComButton
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Next")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                    tryAddProfile()
                }
            }

            BasicDialogButton {
                id: basicComButton2
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Cancel")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                    hideAddProfile()
                }
            }
        }
    }
}
