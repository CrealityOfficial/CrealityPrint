import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Rectangle
{
    color: Constants.itemBackgroundColor
    width: 190
    height: 100
	property var com
	
	function execute()
	{
		//console.log("split.qml start:=========================");
        console.log(com)
	}
    PartPanel {
        control:com
    }

}
