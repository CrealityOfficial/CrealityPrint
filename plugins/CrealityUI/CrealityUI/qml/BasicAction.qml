import QtQuick 2.12
import QtQuick.Controls 2.5

Action{
    property  var actionSource
    property  var  actionCmdItem;
    property var actionShortcut
    property var actionIconSource
    signal loadSouce(string title,var commd)

    shortcut: actionShortcut
    icon.name: ""
    icon.width: 20
    onTriggered: {
  //      console.log("action")
        actionCmdItem.execute();
        loadSouce(actionSource,actionCmdItem);
    }
} 
