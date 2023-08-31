import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
Rectangle
{
	id:idControl
	width: 220
	height: 280
	y: -20
	property var com
	color: Constants.itemBackgroundColor
	function execute()
	{
		console.log("execute")
		idPlatform.bottonSelected=true;
		com.startPreventWarpe()
	}
	Component.onCompleted:
	{
		//com.onSizeChanged.disconnect(execute)
		//com.onSizeChanged.connect(execute)
	}
	radius: 6

	Column
	{
		spacing: 10
		
		Rectangle
		{
			height: 30
			width: idControl.width
			color: Constants.dialogTitleColor
			radius: 5
			Rectangle
			{
				color: parent.color
				y:10
				height: 20
				width: parent.width
			}
			StyledLabel {
				color: "#FFFFFF"
				text: qsTr("PreventWarpe")
				x:5
				anchors.verticalCenter: parent.verticalCenter
			}
		}

		Item
		{
			width: 220
			height: 180
			x: 15
			Column
			{
				spacing: 10
				
				Row{
					id:idSizeRow
		
					StyledLabel {
						id:idSizeLabel
						color: "#FFFFFF"
						text: qsTr("Size:")
						height: 30
						width: 75
						verticalAlignment: Qt.AlignVCenter
					}
					StyledSpinBox
					{
						id: idSize
						height : 30
						width: idControl.width - idSizeLabel.width - 25
						decimals:0
						realStepSize:1.0
						realFrom:2
						realTo:9999
						realValue: com.size
						onValueEdited:
						{
							com.size = idSize.realValue
						}
					}
				}


		
				//Row{
				//	id:idDistanceRow
				//
				//	StyledLabel {
				//		id:idDistanceLabel
				//		color: "#FFFFFF"
				//		text: qsTr("x/y Distance:")
				//		height: 30
				//		width: 75
				//		verticalAlignment: Qt.AlignVCenter
				//	}
				//	StyledSpinBox
				//	{
				//		id: idDistance
				//		height : 30
				//		width: idControl.width - idHeightLabel.width - 25
				//		realStepSize:0.1
				//		realFrom:0
				//		realTo:9999
				//		realValue: com.distance
				//		onValueEdited:
				//		{
				//			com.distance = idDistance.realValue
				//		}
				//	}
				//}

				Row{
					id:idLayerRow
		
					StyledLabel {
						id:idLayerLabel
						color: "#FFFFFF"
						text: qsTr("Layer:")
						height: 30
						width: 75
						verticalAlignment: Qt.AlignVCenter
					}
					StyledSpinBox
					{
						id: idLayer
						height : 30
						width: idControl.width - idLayerLabel.width - 25
						decimals:0
						realStepSize:1.0
						realFrom:1
						realTo:3
						realValue: com.layer
						onValueEdited:
						{
							com.layer = idLayer.realValue
						}
					}
				}
			}
		}

		BasicButton {
			id : idPlatform
			width: 190
			height: 32
			text : bottonSelected ? qsTr("Cancel PreventWarpe") : qsTr("Start PreventWarpe")
			property var bottonSelected: true
			anchors.horizontalCenter: parent.horizontalCenter

			onSigButtonClicked:
			{
				if(!bottonSelected)
					com.startPreventWarpe()
				else
				{
					com.endPreventWarpe()
				}
				bottonSelected =!bottonSelected
			}
		}
	}

}
