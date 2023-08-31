import QtQuick 2.10
import Qt.labs.platform 1.1
import CrealityUI 1.0

Item
{
	property var com
	
	function execute()
	{
        console.log(com.historyPath())

        var historyPath = com.historyPath();
        cx3dExportDialog.currentFile = historyPath//historyPath.substr(historyPath.lastIndexOf('/'),-1);
        //cx3dExportDialog.folder = com.historyPath()
        cx3dExportDialog.nameFilters = com.nameFilters
        cx3dExportDialog.open()
	}
	
    FileDialog
    {
        id: cx3dExportDialog
        fileMode: FileDialog.SaveFile
        title: qsTr("Export 3MF")
        options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
        onAccepted:
        {
            com.save(files)
            GlobalCache.meshImportFolder = folder
        }
    }
}
