import QtQuick 2.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
Menu {
    id:idMenu
    property alias mymodel: idMenuItems.model
    property var myItemObj
    property var separator
    property int  maxImplicitWidth: 200
//    property int index : 0
    property var taskMap: {0:0}
    function mapColor()
    {

        taskMap[0] = "red"
        taskMap[1] = "green"
        taskMap[2] = "blue"
    }
    Column
    {
        Repeater
        {
            id : idMenuItems

            BasicRClickMenuItemStyle
            {
                id : idMenuItem
                text: actionNameRole
                height: 32//25
                width: maxImplicitWidth + 20
                separatorVisible: actionSeparator
                enabled: actionItem.enabled
                checkable: actionItem.checkable
                checked: actionItem.checked
                indicatorColor: actionItem.nozzleColor
                onTriggered:
                {
                    actionItem.checked = checked
//                    visible = false
                    Qt.callLater(actionItem.execute)
                    idClickVisible.onTriggered()
                }
                Component.onCompleted:
                {
                    maxImplicitWidth = implicitContentWidth > maxImplicitWidth ? implicitContentWidth :maxImplicitWidth;
                }
            }
        }
    }


    background: Rectangle {
        implicitWidth:maxImplicitWidth + 20
        color: "white"/*Constants.itemBackgroundColor*///"#061F3B"
        border.width: 1
        border.color: "transparent "
        Rectangle
        {
            id: mainLayout2
            anchors.fill: parent
            anchors.margins: 5
            color: Constants.themeColor
            opacity: 1
        }
	
        DropShadow {
            anchors.fill: mainLayout2
            horizontalOffset: 5
            verticalOffset: 5
            radius: 8
            samples: 17
            source: mainLayout
            color: Constants.dropShadowColor // "#333333"
        }
    }
    Component.onCompleted:
    {
        mapColor()
    }
}
