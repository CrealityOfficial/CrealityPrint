import QtQuick 2.0
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

Menu {
    id:idMenu
    property var myItemObj
    property var separator
    property int maxImplicitWidth: 200
    property alias mymodel: idMenuItems.model
    property alias mysecondmodel: idMenuSecondItems.model
    Column
    {
        Repeater
        {
            id : idMenuItems

            delegate:BasicMenuItemStyle
            {
                id : idMenuItem
                text: actionNameRole
                icon.source: actionIcon
                height: 32* screenScaleFactor
                width: maxImplicitWidth  + 10* screenScaleFactor
                separatorVisible:  false//index === 0 ? false : (actionSeparator ? actionSeparator : false)
                checkable: false
                itemChecked: actionChecked
                Image{
                    anchors.left: parent.left
                    anchors.leftMargin: 10* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20*screenScaleFactor
                    height: 20*screenScaleFactor
                    source: actionIcon
                    Component.onCompleted: {
                        console.log("____________image = ", actionSource)
                    }
                }

                onTriggered:
                {
                    idClickVisible.onTriggered()
                    actionItem.execute();
                }
            }
        }

        Repeater
        {
            id : idMenuSecondItems

            delegate:BasicMenuItemStyle
            {
                text: actionNameRole
                icon.source: actionIcon
                height: 32* screenScaleFactor
                width: maxImplicitWidth  + 10* screenScaleFactor
                separatorVisible: index === 0  //? true : (actionSeparator ? actionSeparator : false)
                checkable: false
                itemChecked: actionChecked
                separatorHeight : 2
                Image{
                    anchors.left: parent.left
                    anchors.leftMargin: 10* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22*screenScaleFactor
                    height: 22*screenScaleFactor
                    source: actionIcon
                    Component.onCompleted: {
                        console.log("____________image = ", actionSource)
                    }
                }

                onTriggered:
                {
                    idClickVisible.onTriggered()
                    actionItem.execute();
                }
            }
        }
    }

    BasicMenuItemStyle
    {
        id: idClickVisible
        //text: "test"
        visible: false
        height: 1
    }

    background: Rectangle {
        implicitWidth: maxImplicitWidth + 10* screenScaleFactor
        Rectangle {
            id: mainLayout
            anchors.fill: parent
            anchors.margins: 5
            color: Constants.menuStyleBgColor
            opacity: 1
        }

        DropShadow {
            anchors.fill: mainLayout
            horizontalOffset: 3
            verticalOffset: 3
            //radius: 8
            samples:9// 17
            source: mainLayout
            color: Constants.dropShadowColor
        }
    }

    onOpened: {
        var max = getMaxWidth()
        //console.log("MaxWidth ====== ", max)
        maxImplicitWidth = max
        //idMenu.width = max +  20
    }

    function getMaxWidth()
    {
        var max = 200;
        for(var cIndex = 0; cIndex < idMenuItems.count; ++cIndex)
        {
            var obj = idMenuItems.itemAt(cIndex);
            max = Math.max(obj.implicitWidth,max);
        }
        return max;
    }
}
