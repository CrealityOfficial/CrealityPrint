import QtQuick 2.12
Item{

     property alias mymodel: gridview.model
     property var selectItem
     anchors.fill: parent
 //    property bool visible: value

     Column{
        id:coloumn;
        spacing: 0;
        width: 150;
        height: parent.height;
        Rectangle {
            //anchors.fill: parent
            height: parent.height
            width : parent.width
            color: Constants.itemBackgroundColor
            ListView {
                id:gridview
                height: parent.height
                width : parent.width
                //anchors.fill: parent

                delegate: EnableButton{
                    width: 150
                    height: 60
                    text: name
                    enabledIconSource:enabledIcon
                    pressedIconSource:pressedIcon
                    selectedBgColor:Constants.selectionColor
                    hoveredBgColor:Constants.hoveredColor
                    defaultBgColor:Constants.itemBackgroundColor
                    onEnabledButtonClicked: {
                        console.log("onEnabledButtonClicked")
                        //加载某个xml
                        content.source = source

                        // mymodel.actions[name]()

                        //按的节点的位置
                        contentWrapper.anchors.topMargin=60*index

                        item.execute()
                        if(selectItem)
                        selectItem.bottonSelected = false
                        selectItem=this
                        selectItem.bottonSelected = true
                    }
                }

            }
        }
    }
    Rectangle {
    id:contentWrapper
    anchors.left: coloumn.right
    anchors.top:coloumn.top
    width:200
    //height: parent.height
    color: "#000000"
    Row{
        Triangle {
            anchors.top:parent.top
            anchors.topMargin: 20
            width: 20
            height: 30
        }
        Loader {
            property var modeldata
            //anchors.fill: parent
            id:content
        }
    }
    }
}

