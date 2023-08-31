import QtQuick 2.10
import Qt.labs.platform 1.1
import CrealityUI 1.0

Item
{
	property var com
	
	function execute()
	{
		//console.log(com)
        cx3dExportDialog.folder = GlobalCache.meshImportFolder
        cx3dExportDialog.nameFilters = com.nameFilters
        cx3dExportDialog.open()
	}
	
    FileDialog
    {
        id: cx3dExportDialog
        fileMode: FileDialog.OpenFile
        title: qsTr("Open Projects")
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        onAccepted:
        {
            com.open(file)
            GlobalCache.meshImportFolder = folder
        }
    }
}
