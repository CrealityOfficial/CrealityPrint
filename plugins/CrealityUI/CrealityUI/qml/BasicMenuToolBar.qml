import QtQuick 2.0
import QtQuick.Controls 2.12
import "qrc:/CrealityUI"
import CrealityUI 1.0

Rectangle {
    id : idMenuBar
    // property color backgroundColor: "white"
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: 24 /*Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)*/
    signal barItemTriggered()
    Row
    {
        spacing: 10
        height: parent.height
        BasicMenuBarButton{
            id : __fileBarButton
            anchors.verticalCenter: parent.verticalCenter
            text:qsTr("File") + "(F)"
            height: 38
            icon.source : "qrc:/UI/photo/menufileIcon.png"
            icon.color: "#9B9BA0"
            //        icon.source : text.length > 0 ? "qrc:/UI/photo/menuFileIcon.png"
            onClicked:
            {
                __FDMFileMenu.popup(this.x,this.y + this.height)

            }
        }

        BasicMenuBarButton{
            id : __otherBarButton
            anchors.verticalCenter: parent.verticalCenter
            text:qsTr("")
            height: 38
            icon.source : "qrc:/UI/photo/menuOtherIcon.png"
            icon.color: "#9B9BA0"
            //        icon.source : text.length > 0 ? "qrc:/UI/photo/menuFileIcon.png"
            onClicked:
            {
                __FDMOtherMenu.popup(this.x,this.y + this.height)
            }
        }
    }



    BasicMenu {
        id: __FDMFileMenu
        title: qsTr("File(&F)")
//        anchors.left: previewBtn.left
//        anchors.top : previewBtn.bottom
        BasicMenuItem{
            id : __openfile
            actionName: qsTr("Open File")
            enabled : true
            actionShortcut : "Ctrl+O"
            separatorVisible : false
            onTriggered:
            {
                excuteOpt("Open File")
            }
        }
        BasicMenuItem{
            id : __openProject
            actionName: qsTr("Open Project")
            enabled : true
            separatorVisible: true
            onTriggered:
            {
                excuteOpt("Open Project")
            }
        }
        BasicSubMenu{
            id : __fileHistory
            separator : false
            title : cxTr("Recent Files")
            mymodel : actionCommands.getOpt("Recently files").subMenuActionmodel
        }
        BasicSubMenu{
            id : __projectHistory
            title : qsTr("Recently Opened Project")
            mymodel : actionCommands.getOpt("Recently Opened Project").subMenuActionmodel
        }
        BasicMenuItem{
            id : __saveSTL
            separatorVisible: true
            actionName: qsTr("Save STL")
            enabled : HalotContext.obj("ModelSelector").nocxbinSelectionNum > 0
            onTriggered:
            {
                excuteOpt("Save STL")
            }
        }
        BasicMenuItem{
            id : __saveProject
            separatorVisible: true
            actionName: qsTr("Save As Project")
            actionCmdItem:HalotContext.obj("Command.saveProject");
            enabled : HalotContext.obj("Command.saveProject");
            actionShortcut : HalotContext.obj("Command.saveProject").shortcut
            onTriggered:
            {
                excuteOpt("Save As Project")
            }
        }
        BasicMenuItem{
            id : __closeProject
            separatorVisible: true
            actionName: qsTr("Close")
            enabled : true
            actionShortcut : "Ctrl+Q"
            onTriggered:
            {
                excuteOpt("Close")
            }
        }
        // ...
    }
    BasicMenu
    {
       id : __FDMOtherMenu
//       title: qsTr("Other(&O)")
       BasicMenu {
           id: editMenu
           title: qsTr("Edit")
           BasicMenuItem{
               actionName: qsTr("Undo")
               enabled : true
               actionShortcut : "Ctrl+Z"
               onTriggered: {
                   excuteOpt("Undo")
               }
           }
           BasicMenuItem{
               separatorVisible: true
               actionName: qsTr("Redo")
               enabled : true
               actionShortcut : "Ctrl+Shift+Z"
               onTriggered: {
                   excuteOpt("Redo")
               }
           }
       }

       BasicMenu {
           id: viewMenu
           title: qsTr("View")
           // ...
           BasicMenuItem{
               separatorVisible: true
               actionName: qsTr("ShowModelLine")
               enabled : true
               onTriggered: {
                   excuteOpt("ShowModelLine")
               }
           }
           BasicMenuItem{
               actionName: qsTr("ShowModelFace")
               enabled : true
               onTriggered: {
                   excuteOpt("ShowModelFace")
               }
           }

           BasicMenuItem{
               actionName: qsTr("ShowModelFaceLine")
               enabled : true
               onTriggered: {
                   excuteOpt("ShowModelFaceLine")
               }
           }
           BasicMenuItem{
               actionName: qsTr("Mirrror X")
               enabled : true
               onTriggered: {
                   excuteOpt("Mirrror X")
               }
           }
           BasicMenuItem{
               actionName: qsTr("Mirrror Y")
               enabled : true
               onTriggered: {
                   excuteOpt("Mirrror Y")
               }
           }
           BasicMenuItem{
               actionName: qsTr("Mirrror Z")
               enabled : true
               onTriggered: {
                   excuteOpt("Mirrror Z")
               }
           }
           BasicMenuItem{
               actionName: qsTr("Mirrror ReSet")
               enabled : true
               onTriggered: {
                   excuteOpt("Mirrror ReSet")
               }
           }
           BasicSubMenu{
               separator : false
               title : qsTr("View Show")
               mymodel : actionCommands.getOpt("View Show").subMenuActionmodel
           }
           BasicMenuItem{
               actionName: qsTr("Reset All Model")
               enabled : true
               onTriggered: {
                   excuteOpt("Reset All Model")
               }
           }
           BasicMenuItem{
               actionName: qsTr("Merge Model Locations")
               enabled : true
               onTriggered: {
                   excuteOpt("Unit As One")
               }
           }
       }

       BasicMenu {
           id: toolMenu
           title: qsTr("Tool")
           // ...
           BasicSubMenu{
               separator : false
               title : qsTr("Language")
               mymodel : actionCommands.getOpt("Language").subMenuActionmodel
           }

           BasicMenuItem{
               actionName: qsTr("Model Repair")
               enabled : true
               onTriggered: {
                   excuteOpt("Model Repair")
               }
           }

           BasicMenuItem{
               actionName: qsTr("Manage Printer")
               enabled : true
               onTriggered: {
                   excuteOpt("Manage Printer")
               }
           }

           BasicSubMenu{
               separator : false
               title : qsTr("Theme color change")
               mymodel : actionCommands.getOpt("Theme color change").subMenuActionmodel
           }

           BasicMenuItem{
               actionName: qsTr("Log View")
               enabled : true
               onTriggered: {
                   excuteOpt("Log View")
               }
           }
       }

       BasicMenu {
           id: modelLibraryMenu
           title: qsTr("Model Library")
           // ...
           BasicMenuItem{
               actionName: qsTr("Model Library")
               enabled : true
               onTriggered: {
                   excuteOpt("Model Library")
               }
           }
       }

       BasicMenu {
           id: printerCrlMenu
           title: qsTr("PrinterControl")
           // ...
           BasicMenuItem{
               actionName: qsTr("Creality Cloud Printing")
               enabled : true
               //onTriggered:
           }
           BasicMenuItem{
               actionName: qsTr("USB Printing")
               enabled : true
               onTriggered: {
                   excuteOpt("USB Printing")
               }
           }
       }

       BasicMenu {
           id: helpMenu
           title: qsTr("Help")
           // ...
           BasicMenuItem{
               actionName: qsTr("About Us")
               enabled : true
               onTriggered: {
                   excuteOpt("About Us")
               }
           }

           BasicMenuItem{
               actionName: qsTr("Update")
               enabled : true
               onTriggered: {
                   excuteOpt("Update")
               }
           }

           BasicMenuItem{
               actionName: qsTr("Use Course")
               enabled : true
               onTriggered: {
                   excuteOpt("Use Course")
               }

           }

           BasicMenuItem{
               actionName: qsTr("User Feedback")
               enabled : true
               onTriggered: {
                   excuteOpt("User Feedback")
               }
           }
       }
    }


    BasicMenu
    {
       id : __PreOtherMenu
//       title: qsTr("Other(&O)")
       BasicMenu {
           title: qsTr("Tool")
           // ...
           BasicSubMenu{
               separator : false
               title : qsTr("Language")
               mymodel : actionCommands.getOpt("Language").subMenuActionmodel
           }


           BasicSubMenu{
               separator : false
               title : qsTr("Theme color change")
               mymodel : actionCommands.getOpt("Theme color change").subMenuActionmodel
           }

           BasicMenuItem{
               actionName: qsTr("Log View")
               enabled : true
               onTriggered: {
                   excuteOpt("Log View")
               }
           }
       }
       BasicMenu {
           title: qsTr("Help")
           // ...
           BasicMenuItem{
               actionName: qsTr("About Us")
               enabled : true
               onTriggered: {
                   excuteOpt("About Us")
               }
           }

           BasicMenuItem{
               actionName: qsTr("Update")
               enabled : true
               onTriggered: {
                   excuteOpt("Update")
               }
           }

           BasicMenuItem{
               actionName: qsTr("Use Course")
               enabled : true
               onTriggered: {
                   excuteOpt("Use Course")
               }

           }
           BasicMenuItem{
               actionName: qsTr("User Feedback")
               enabled : true
               onTriggered: {
                   excuteOpt("User Feedback")
               }
           }
       }
    }

    function showFDMMenuBar()
    {

        __fileBarButton.visible = true
        __otherBarButton.visible = true
    }

    function onlyShowOtherMenuBar()
    {
        __fileBarButton.visible = false
        __otherBarButton.visible = true
    }

    function excuteOpt(optName)
    {
        actionCommands.getOpt(optName).execute()
    }
}
