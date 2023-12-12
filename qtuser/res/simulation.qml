import QtQuick 2.5
import QtQuick.Controls 2.5
import Qt.labs.settings 1.0
import com.cxsw.demo 1.0
import QtQuick.Dialogs 1.2
import "qrc:/qtuserqml/res/"

ApplicationWindow 
{
    id: standaloneWindow
    title: qsTr("Simulation")
    width: 1024
    height: 768
    visible: false
    color: "gray"
    minimumHeight: 400
    minimumWidth: 640

	property var pythonConsole: null
	
    Component.onCompleted: 
	{		
        showMaximized()
		
		glItem.visible = true
    }
	
    onClosing: 
	{
		close.accepted = true
    }
	
	GLQuickItem
	{
		id:glItem
		width: parent.width
		height: parent.height
		visible: false
	}
	
	Kernel
	{
		id: cxxKernel
	}
	
	PythonConsole
	{
		id: idPythonConsole
		visible: false
	}
	
    CheckBox 
	{
        checked: idPythonConsole.visible
        text: qsTr("Python")
		anchors.top: parent.top
		anchors.right: parent.right
		onCheckStateChanged : 
		{
			idPythonConsole.visible = checkState == Qt.Checked
		}

    }
}