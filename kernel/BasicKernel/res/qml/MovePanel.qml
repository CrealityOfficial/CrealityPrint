import QtQuick 2.10
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item
{
    width: __movepanel.width
    height: __movepanel.height
	property var com
	function execute()
	{
		//com.positionChanged.connect(onMovePositionValueChanged)
		//console.log("Move Panel execute")
	}
    MovePanel {
        id : __movepanel
        mtranslate:com
    }
}
