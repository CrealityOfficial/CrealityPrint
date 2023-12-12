import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Window 2.0

Window 
{
    id: idPythonConsole
    width: 600
    height: 500
	title: qsTr("Python")
	
    flags: Qt.Dialog
    modality: Qt.NonModal

    Rectangle
	{
        id: titleBar
		anchors.fill: parent
	}
}
