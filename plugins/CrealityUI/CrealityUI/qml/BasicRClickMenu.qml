import QtQuick 2.13
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.13

import "./"
import "../secondqml"

BasicMenuStyle
{
    id: root
    property int maxImplicitWidth: 200
    property var menuItems
    property var recentFilesObj
    property var testModelsObj
    property var filamentObj
    property var nozzleObj
    property var uploadObj
    property bool recentFilesMenuCreated: false
    property bool testModelsMenuCreated: false
    property bool filamentMenuCreated

    function active() {
        initSubMenu()
        popup()
    }

    function initSubMenu() {
        if (recentFilesObj && !recentFilesMenuCreated) {
            recentFilesMenuCreated = true
            let obj = recentFilesCom.createObject(root, {});
            addMenu(obj)
        }
 
        if (testModelsObj && !testModelsMenuCreated) {
            testModelsMenuCreated = true
            let obj = testModelsCom.createObject(root, {});
            addMenu(obj)
        }

        if (filamentObj && !filamentMenuCreated) {
            filamentMenuCreated = true
            let obj = filamentModelCom.createObject(root, {});
            addMenu(obj)
        }
    }

    Repeater {
        model: menuItems
        delegate: BasicMenuItem {
            actionCmdItem: modelData
            actionName: modelData.name
            actionShortcut: modelData.shortcut
            enabled: modelData.enabled
            separatorVisible: modelData.bSeparator
            actionSource: modelData.source

            Rectangle {
                height: 1
                width: parent.width
                color: "#BBBBBB"
                visible: false
                Component.onCompleted: {
                    console.log(modelData.icon)
                    if (modelData.location == "top") {
                        visible = true;
                        anchors.top = parent.top
                    } else if (modelData.location == "bottom") {
                        visible = true;
                        anchors.bottom = parent.bottom
                    }
                }
            }
            
            Image {
                id: img
                anchors.left: parent.left
                anchors.leftMargin: 12 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                width: 12 * screenScaleFactor
                height: 12 * screenScaleFactor
                source: modelData.source
            }

        }

    }

    background: Rectangle {
        implicitWidth:maxImplicitWidth + 20
        color: Constants.itemBackgroundColor//"#061F3B"
        border.width: 1
        border.color: "black"
        radius: 5

        Rectangle
        {
            id: mainLayout
            anchors.fill: parent
            anchors.margins: 5
            color: Constants.themeColor
            opacity: 1
        }

        DropShadow {
            anchors.fill: mainLayout
            horizontalOffset: 5
            verticalOffset: 5
            radius: 8
            samples: 17
            source: mainLayout
            color: Constants.dropShadowColor // "#333333"
        }
    }

    Component.onCompleted:{
        // if (nozzleObj) {
        //     var noObj = nozzleMenuCom.createObject(root, {});
        //     addMenu(noObj)
        // }

        // if (uploadObj) {
        //     var uploadObj = upload_sub_menuCom.createObject(root, {});
        //     addMenu(uploadObj)
        // }

        // maxImplicitWidth = Math.max(implicitContentWidth, maxImplicitWidth)
        // if (!kernel_global_const.cxcloudEnabled) {
        //     removeMenu(upload_sub_menu)
        // }
    }

    Component{
        id: recentFilesCom

        BasicRClickSubMenu {
            id: recentFilesMenu
            myItemObj: recentFilesObj
            title: recentFilesObj.name
            mymodel: recentFilesObj.subMenuActionmodel
            enabled: recentFilesObj.enabled
            separator: recentFilesObj.Separator
        }
    }

    Component{
        id: testModelsCom


        BasicRClickSubMenu {
            id: testModelsMenu
            myItemObj: testModelsObj
            title: testModelsObj.name
            mymodel: testModelsObj.subMenuActionmodel
            enabled: testModelsObj.enabled
            separator: testModelsObj.Separator
        }
    }

    Component{
        id: filamentModelCom

        BasicRClickSubMenu {
            id: filamentModelMenu
            myItemObj: filamentModelObj
            title: filamentModelObj.name
            mymodel: filamentModelObj.subMenuActionmodel
            enabled: filamentModelObj.enabled
            separator: filamentModelObj.Separator
        }
    }

    Component{
        id: nozzleMenuCom
        BasicRClickSubMenu {
            id: nozzleMenu
            myItemObj: nozzleObj
            title: nozzleObj.name
            mymodel: nozzleObj.subMenuActionmodel
            enabled: nozzleObj.enabled
            separator: nozzleObj.Separator
        }
    }

    Component{
        id: upload_sub_menuCom
        BasicRClickSubMenu {
            id: upload_sub_menu
            myItemObj: uploadObj
            title: uploadObj.name
            mymodel: uploadObj.subMenuActionmodel
            enabled: uploadObj.enabled
            separator: uploadObj.Separator
        }
    }
}
