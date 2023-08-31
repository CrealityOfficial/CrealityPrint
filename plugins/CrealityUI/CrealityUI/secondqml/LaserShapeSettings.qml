import QtQuick 2.0
import QtQuick.Controls 2.5
import "../qml"
Item {
	property var oldX
    property var oldY
    property var oldWidth
    property var oldHeight
    property var oldRotation

	signal objectChanged(var obj, var oldX, var oldY, var oldWidth, var oldHeight, var oldRotation, var newX, var newY, var newWidth, var newHeight, var newRotation)
	function initImagePos()
	{
		//console.log("origin x:",control.origin.x,"--origin y:",control.origin.y)
		//console.log("before initImagePos--- x:",selShape.x,"-y:",selShape.y);
		selShape.x = control.origin.x+selShape.x //control.origin.x
		selShape.y = control.origin.y-selShape.y//control.origin.y
		//console.log("after initImagePos--- x:",selShape.x,"-y:",selShape.y);
	}
	
	function updateImagePos(oldPosX,oldPosY)
	{	
		selShape.x = parseInt(((selShape.x-oldPosX)/3).toFixed(2))*3+control.origin.x
		selShape.y = control.origin.y - parseInt(((oldPosY-selShape.y)/3).toFixed(2))*3
	}

    Item {
       id:idMoveXY
        width: 200 * screenScaleFactor
        height: 60* screenScaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 18* screenScaleFactor
		
		Column
		{
			spacing: 2
			Rectangle{
				width: parent.width
				height: 5
				color: "transparent"
			}
			Row
			{
				spacing: 5
				Label {
					y:17
					width: 90* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
					text: qsTr("Move")
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignLeft
				}
				Label {
					width: 15* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: "X"
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignRight
				}
				BasicDialogTextField {
					id:idMoveX
					width: 120* screenScaleFactor
					height: 28* screenScaleFactor
					horizontalAlignment: Text.AlignLeft
					text: {
						if(selShape && control)
						{//console.log("read move x")
							objectChanged(selShape, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)		
							return ((selShape.x-control.origin.x)/3 + selShape.width/3/2).toFixed(2)
						}
						else
							return 0
					}
					unitChar:"mm"
					onEditingFinished:{
						if(selShape)
						{
							oldX = selShape.x
							selShape.x = ((Number(idMoveX.text)-selShape.width/3/2).toFixed(2))*3+control.origin.x
							//console.log("edit move x")							
							if(oldX != selShape.x){
								objectChanged(selShape, oldX, selShape.y, selShape.width, selShape.height, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
							}
						}
					}
				}
			}
			
			Row
			{
				spacing: 5
				Label {
					width: 90* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: ""
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignLeft
				}
				Label {
					width: 15* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: "Y"
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignRight
				}
				BasicDialogTextField {
					id:idMoveY
					width: 120* screenScaleFactor
					height: 28* screenScaleFactor
					text: {
						if(selShape && control)
						{	
							return ((control.origin.y-selShape.y)/3 - selShape.height/3/2).toFixed(2)
						}
						else 
							return 0
					}
					unitChar:"mm"
					onEditingFinished:{
						if(selShape)
						{
							oldY = selShape.y
							selShape.y = control.origin.y - ((Number(idMoveY.text) + selShape.height/3/2).toFixed(2))*3
							if(oldY != selShape.y){
								objectChanged(selShape, selShape.x, oldY, selShape.width, selShape.height, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
							}
						}
					}
				}
			}

			Rectangle{
				width: parent.width
				height: 6
				color: "transparent"
			}
			
			Row
			{
				spacing: 5
				Label {
					y:17
					width: 90* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
					text: qsTr("Size")
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignLeft
				}
				Label {
					width: 15* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: "X"
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignRight
				}
				BasicDialogTextField {
					id:idScaleX
					width: 120* screenScaleFactor
					height: 28* screenScaleFactor
					text: 
					{
						if(selShape)
						{	
							return (selShape.width/3).toFixed(2)
						}
						else 
							return 0
					}
					unitChar:"mm"
					onEditingFinished:{
						if(selShape)
						{
							oldWidth = selShape.width;
							selShape.width = (Number(idScaleX.text).toFixed(2))*3
							var factor =  selShape.width/oldWidth

							if(uniformScalingCheckbox.checked)
							{
								oldHeight = selShape.height
								selShape.height = oldHeight*factor//(Number(idScaleY.text).toFixed(2))*3	

								if(oldHeight != selShape.height || oldWidth != selShape.width)
								{
									objectChanged(selShape, selShape.x, selShape.y, oldWidth, oldHeight, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
								}
							}
							else
							{
								if(oldWidth != selShape.width)
								{
									objectChanged(selShape, selShape.x, selShape.y, oldWidth, selShape.height, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
								}
							}
						}
					}
				}
			}
			
			Row
			{
				spacing: 5
				Label {
					width: 90* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: ""
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignLeft
				}
				Label {
					width: 15* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: "Y"
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignRight
				}
				BasicDialogTextField {
					id:idScaleY
					width: 120* screenScaleFactor
					height: 28* screenScaleFactor
					text: 
					{
						if(selShape)
						{	
							console.log("selShape.height text: "+ selShape.height + " -> " + (selShape.height/3).toFixed(2))
							return (selShape.height/3).toFixed(2)
						}
						else 
							return 0
					}
					unitChar:"mm"
					onEditingFinished:{
						if(selShape)
						{
							oldHeight = selShape.height
							selShape.height = (Number(idScaleY.text).toFixed(2))*3	
							var factor =  selShape.height/oldHeight

							if(uniformScalingCheckbox.checked)
							{
								oldWidth = selShape.width;
								selShape.width = oldWidth*factor//(Number(idScaleX.text).toFixed(2))*3

								if(oldHeight != selShape.height || oldWidth != selShape.width)
								{
									objectChanged(selShape, selShape.x, selShape.y, oldWidth, oldHeight, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
								}
							}
							else
							{
								if(oldHeight != selShape.height)
								{
									objectChanged(selShape, selShape.x, selShape.y, selShape.width, oldHeight, selShape.rotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
								}
							}
						}
					}
				}
			}

			Row
			{
				Rectangle {
					color:"transparent"
					width: 115* screenScaleFactor
					height: 20* screenScaleFactor
				}
				StyleCheckBox {
                    id: uniformScalingCheckbox          
                    checked: Constants.bIsLaserSizeLoced
					height: 20* screenScaleFactor
					width: 120* screenScaleFactor
                    text: qsTr("Uniform")
                    onCheckedChanged:
                    {
						Constants.bIsLaserSizeLoced = checked
                        //msale.uniformCheck = checked
                    }
                }
			}

			Rectangle{
				width: parent.width
				height: 6
				color: "transparent"
			}
			
			Row
			{
				spacing: 5
				Label {
					width: 90* screenScaleFactor
					height: 28* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
					text: qsTr("Rotate")
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignLeft
				}
				Label {
					width: 15* screenScaleFactor
					font.pointSize:Constants.labelFontPointSize_9
                    text: ""
					font.family: Constants.labelFontFamily
					color: Constants.textColor
					verticalAlignment: Qt.AlignVCenter
					horizontalAlignment: Qt.AlignRight
				}
				BasicDialogTextField {
					id: idRotate
					width: 120* screenScaleFactor
					height: 28* screenScaleFactor
					unitChar:"(°)"
					text: 
					{
						if(selShape)
						{	
							return selShape.rotation.toFixed(2)
						}
						else 
							return 0
					}
					onEditingFinished:{
					if(selShape)
						{
							oldRotation = selShape.rotation.toFixed(2)
							selShape.rotation = Number(idRotate.text).toFixed(2)							
							if(oldRotation != selShape.rotation)
							{
								objectChanged(selShape, selShape.x, selShape.y, selShape.width, selShape.height, oldRotation, selShape.x, selShape.y, selShape.width, selShape.height, selShape.rotation)
							}
						}
					}
				}
			}
		}			
    }

}
