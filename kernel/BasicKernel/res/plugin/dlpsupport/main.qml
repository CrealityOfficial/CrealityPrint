import QtQuick 2.0
import CrealityUI 1.0
import QtQuick.Controls 2.5
import QtQuick.Scene3D 2.0
import "qrc:/CrealityUI"
Rectangle 
{
//    width: 220
//    height: 480
    property var com
    property int intVal: 0

    function execute()
    {
        intVal=1
    }
    // DLPSupportPanel {
    //     id:dlpsupportpanel
    //     initValue:intVal
    //     control:com
    // }
}
