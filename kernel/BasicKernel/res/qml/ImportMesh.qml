import QtQuick 2.10
import QtQuick.Dialogs 1.2
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item
{
	property var com
	
	function execute()
	{
		//console.log(com)
		meshImportDialog.folder = GlobalCache.meshImportFolder
		meshImportDialog.nameFilters = com.nameFilters
        meshImportDialog.open()
	}
	
    FileDialog
    {
        id: meshImportDialog
        selectMultiple: true
        selectExisting: true
        title: qsTr("Import Mesh")
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        onAccepted:
        {
            com.load(fileUrls)
            GlobalCache.meshImportFolder = folder
        }
    }
}
