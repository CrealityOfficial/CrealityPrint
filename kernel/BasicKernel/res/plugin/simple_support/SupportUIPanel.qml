import QtQuick 2.0
import QtQuick.Controls 2.0
import CrealityUI 1.0
import "qrc:/CrealityUI"
Item {
    anchors.fill: parent
    property var com

//    function execute()
//    {
//        //console.log("Rotate Panel execute")
//    }
    visible: false
    SupportTabPanel {
        id: idSupport
        visible : parent.visible
        anchors.top:parent.top
        anchors.right: parent.right
        anchors.topMargin: 7//8
        anchors.rightMargin: 10
        anchors.bottomMargin: 20
        control : com

        Rectangle
        {
            color: idSupport.color
            height: 3
            width: parent.width -2 
            anchors.top:parent.top
            anchors.left: parent.left
            anchors.topMargin: -1
            anchors.leftMargin: 1
        }

    }
    Component.onCompleted:
    {
        console.log("success!!!")
//        __dlg.show()
//        (Constants.supprotHandle()).connect(shoow)
        //parent.supprotExecute.connect(shoow)
    }
    function shoow()
    {
        console.log("shoow")
    }
}
