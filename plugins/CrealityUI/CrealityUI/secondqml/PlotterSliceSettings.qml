import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import "../qml"
Item {
    width: 230*screenScaleFactor
    property var settingsModel
    implicitHeight:200*screenScaleFactor
    anchors.leftMargin: 23*screenScaleFactor
    signal genPlotterGcode()
    Column{
        spacing:5*screenScaleFactor
        Item{
            width: 5*screenScaleFactor
            height: 5*screenScaleFactor
        }

        Row{
			spacing:25
			Label {
                width: 90*screenScaleFactor
				height: 28*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9
                font.family: Constants.labelFontFamily
                text:qsTr("Power Rate")
                color: Constants.textColor
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                width: 120*screenScaleFactor
                height: 28*screenScaleFactor
                horizontalAlignment: Text.AlignLeft
                unitChar:"%"
                text: settingsModel.plotterPowerRate
                onTextChanged: {
                    settingsModel.plotterPowerRate = parseInt(text);
                }
            }
			
		}
        Row{
			spacing:25
			Label {
                width: 90*screenScaleFactor
				height: 28*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9
                font.family: Constants.labelFontFamily
                text:qsTr("Speed Rate")
                color: Constants.textColor
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                width: 120*screenScaleFactor
                height: 28*screenScaleFactor
                horizontalAlignment: Text.AlignLeft
                unitChar:"%"
                text: settingsModel.plotterSpeedRate
                onTextChanged: {
                    settingsModel.plotterSpeedRate = parseInt(text);
                }
            }
			
		}
        Row{
			spacing:25
			Label {
                width: 90*screenScaleFactor
				height: 28*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9
                font.family: Constants.labelFontFamily               
                text:qsTr("Travel Speed")
                color: Constants.textColor
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                width: 120*screenScaleFactor
                height: 28*screenScaleFactor
                horizontalAlignment: Text.AlignLeft
                unitChar:"mm/min"
                text: settingsModel.plotterJogSpeed
                onTextChanged: {
                    settingsModel.plotterJogSpeed = parseInt(text);
                }
            }
			
		}
        Row{
			spacing:25
			Label {
                width: 90*screenScaleFactor
				height: 28*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9
                font.family: Constants.labelFontFamily                
                text:qsTr("Work Speed")
                color: Constants.textColor
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                width: 120*screenScaleFactor
                height: 28*screenScaleFactor
                horizontalAlignment: Text.AlignLeft
                unitChar:"mm/min"
                text: settingsModel.plotterWorkSpeed
                onTextChanged: {
                    settingsModel.plotterWorkSpeed = parseInt(text);
                }
            }
			
		}

        Row{
			spacing:25
			Label {
                width: 90*screenScaleFactor
				height: 28*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9
                font.family: Constants.labelFontFamily                
                text:qsTr("Work Depth")
                color: Constants.textColor
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                width: 120*screenScaleFactor
                height: 28*screenScaleFactor
                horizontalAlignment: Text.AlignLeft
                unitChar:"mm"
                text: settingsModel.plotterWorkDepth
                onTextChanged: {
                    settingsModel.plotterWorkDepth = parseInt(text);
                }
            }
			
		}

        Row{
			spacing:25
			Label {
                width: 90*screenScaleFactor
				height: 28*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9
                font.family: Constants.labelFontFamily                
                text:qsTr("Travel Height")
                color: Constants.textColor
				verticalAlignment: Qt.AlignVCenter
				horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                width: 120*screenScaleFactor
                height: 28*screenScaleFactor
                horizontalAlignment: Text.AlignLeft
                unitChar:"mm"
                text: settingsModel.plotterTravelHeight
                onTextChanged: {
                    settingsModel.plotterTravelHeight = parseInt(text);
                }
            }
			
		}

		
		Label {
            color: "#FFFFFF"
            font.family: Constants.labelFontFamily
            text:""
            font.pointSize:13
			width: 150*screenScaleFactor
			height:2
        }
    }

}
