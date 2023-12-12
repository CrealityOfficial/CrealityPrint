import QtQuick 2.5
import QtQuick.Controls 2.5
import Qt.labs.settings 1.0
import com.cxsw.demo 1.0
import QtQuick.Dialogs 1.2

ApplicationWindow 
{
    id: standaloneWindow
    title: qsTr("Demo3D")
    width: 1024
    height: 768
    visible: false
    color: "gray"
    minimumHeight: 400
    minimumWidth: 640
	property url importFolder: "file:///"

    Component.onCompleted: 
	{	
		scene.initialize()
        Qt.application.organization = "Creatity3D Company"
        Qt.application.domain = "szcxsw"
        Qt.application.name = "Demo3D"
        showMaximized()
		
		glItem.visible = true
		textEdit.visible = true
    }
	
    onClosing: 
	{
		scene.uninitialize()
		close.accepted = true
    }
	
	GLQuickItem
	{
		id:glItem
		width: parent.width
		anchors.top: parent.top
		anchors.bottom: textEdit.top
		visible: false
	}
	
	TextInput
	{
		id:textEdit
		width: parent.width
		height: 30
		anchors.bottom: parent.bottom
		anchors.left:parent.left
		font.pointSize: 15
		visible: false
		onAccepted:{
			//console.log(textEdit.text)
			scene.execute(textEdit.text)
			textEdit.clear()
		}
	}
	
	WholeDemo
	{
		id:scene
		glQuickItem:glItem
	}
	
	function open(filters, receiver)
	{
		importDialog.nameFilters = filters
		importDialog.folder = importFolder
		importDialog.receiver = receiver
		importDialog.open()
	}
	
    FileDialog
    {
        id: importDialog
        selectMultiple: false
        selectExisting: true
        title: qsTr("Import")
		property var receiver
        onAccepted:
        {
            receiver.load(fileUrl)
            importFolder = folder
        }
    }
}