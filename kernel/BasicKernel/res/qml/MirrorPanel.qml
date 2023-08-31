import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Rectangle {
    //color: Constants.itemBackgroundColor
    width: 140
    height: 200
    property  int buttonMargin: 5
	property var com
	
	function execute()
	{
	}
	
    MirrorPanel {
        control:com
    }

}
