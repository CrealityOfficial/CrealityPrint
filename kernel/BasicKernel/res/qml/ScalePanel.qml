import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item {
	property var com
    width: scalePanel.width
    height: scalePanel.height
	
	function execute()
	{
		//console.log("Rotate Panel execute")
	}
    ScalePanel {
        id: scalePanel
        msale : com
    }
}
