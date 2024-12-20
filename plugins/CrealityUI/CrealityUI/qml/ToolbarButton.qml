
import QtQuick 2.12
import QtQuick.Controls 2.12
Item {
    width: 68
    height: 40
    property var enabledIconSource
    property var pressedIconSource
	property var qmlSource
	property var itemSource
    ToolButton 
	{
		id: control
		anchors.fill: parent
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
		contentItem: Item
		{
			anchors.fill: parent
			Image 
			{
				anchors.centerIn: parent
				width: 24
				height: 24
				source: control.hovered? enabledIconSource: pressedIconSource
			}
		}
	
		background: Rectangle
		{
            anchors.fill: parent
            implicitWidth: 40
            implicitHeight: 40

            color: Qt.darker("#42bdd8", control.hovered ? 1.2 : 1.0)
            opacity: enabled ? 1 : 0.3
            visible: true
		}
		onClicked:
		{
			item.execute()
			content.source = source
			
			if(source.length > 0)
			{
				content.item.com = item
				content.item.execute()
			}
		}
    }
	
	Loader 
	{
		id:content
	}
	
    ToolbarSeparator
	{
        anchors.left: control.right
        anchors.verticalCenter: parent.verticalCenter
    }
	
    Component.onCompleted: 
	{
		//console.log(itemSource)
		//console.log(enabledIconSource)
    }
}
