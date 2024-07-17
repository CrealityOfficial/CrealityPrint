import QtQuick 2.12
import QtQuick.Controls 2.5
BasicMenuItemStyle
{
    id : idMenItem
    property alias actionName:idMenItem.text
    property var actionIconSource
    property  var actionSource
    property  var  actionCmdItem
    itemChecked:idMenItem.itemChecked
    separatorVisible: true
    signal loadSouce(string title,var commd)

    Shortcut {
        sequence: idMenItem.actionShortcut
        onActivated:
        {
            triggered()
        }
    }

    onTriggered: {
        if(actionCmdItem){
            Qt.callLater(actionCmdItem.execute)
        }
    }

}
