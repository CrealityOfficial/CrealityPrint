import QtQuick 2.5
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
import "../qml"

Menu {
    id: firstMenu

    property var actionEnabled: false
    signal rClickDeleteSelect()
	signal sigSetTop()
	signal sigSetBottom()

    Action { 
        text: qsTr("Delete Object")  
        enabled: actionEnabled
        onTriggered: {
            rClickDeleteSelect()
        }
    }
	
	Action { 
        text: qsTr("Set the top")  
        enabled: actionEnabled
        onTriggered: {
            sigSetTop()
        }
    }
	
	Action { 
        text: qsTr("Set the bottom")  
        enabled: actionEnabled
        onTriggered: {
            sigSetBottom()
        }
    }

    delegate: BasicRClickMenuItemStyle{
        textPadding: 10
    }
    background: Rectangle {
        implicitWidth:150
        color: "white"
        border.width: 1
        border.color: "transparent "
        Rectangle
		{
			id: mainLayout			
			anchors.fill: parent			
			anchors.margins: 5
			color: Constants.themeColor
			opacity: 1
		}
	
		DropShadow {
			anchors.fill: mainLayout
			horizontalOffset: 5
			verticalOffset: 5
			radius: 8
			samples: 17
			source: mainLayout
            color:  Constants.dropShadowColor
		}
    }
}
