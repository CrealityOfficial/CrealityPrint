import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item {
	property var com
    width: rotatePanel.width
    height: rotatePanel.height
	
	function execute()
	{
		//console.log("Rotate Panel execute")
	}
	
    RotatePanel {
        id: rotatePanel
        mrotate: com

    }

}
