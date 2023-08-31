import QtQuick 2.0
import CrealityUI 1.0
import QtQuick.Scene3D 2.0
import "qrc:/CrealityUI"
Item {
    property var com

    function execute()
    {
        //console.log("res/NoSelect.qml start:=========================")
    }
    SelectModelDlg {
        control:com
    }
}
