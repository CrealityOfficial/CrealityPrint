import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0

Rectangle 
{
	anchors.left : parent.right
	anchors.bottom : parent.top
    width: 100
    height: 23
    color: Constants.itemBackgroundColor
	property var com
	
	function execute()
	{
		console.log("slice preview execute")
		
		idPreview.visible = true
		idUnPreview.visible = false
	}
	
	Row {
		Button {
			id : idPreview
			width: 100
			height: 23
			text:  "<font color='#000000'>" + qsTr("Preview") + "</font>"
			font.pixelSize: 13
			onClicked: 
			{
				com.preview()
				
				idPreview.visible = false
				idUnPreview.visible = true
			}
		}	
		Button {
			id : idUnPreview
			width: 100
			height: 23
			text:  "<font color='#000000'>" + qsTr("UnPreview") + "</font>"
			font.pixelSize: 13
			onClicked: 
			{
				com.unpreview()
				idPreview.visible = true
				idUnPreview.visible = false
			}
		}
	}
}
