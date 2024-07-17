import QtQuick 2.0
import QtQuick.Controls 2.0
Rectangle{
    id:control

    property int currentItem: 0 //Currently selected item

    property int spacing: 0    //Distance between items

    property int indent: 3      //Indent the subitem. Note that there is also the icon distance
    property string onSrc: "qrc:/UI/images/on.png"
    property string offSrc: "qrc:/UI/images/off.png"
    property string checkedSrc: "qrc:/UI/images/check.png"
    property string uncheckSrc: "qrc:/UI/images/uncheck.png"

    property var checkedArray: [] //Currently checked items
    property bool autoExpand: true


    color: "white"
    border.color: "darkCyan"
    property alias myModel: list_view.model
    property color treeItemBgHoverColor : "#E4E4E4"//"#618BFF"
    property color treeItemBgSelectColor : "#E4E4E4"
    property color treeItemBgColor : "#ffffff"
    BasicScrollView
    {
        id: idScollView
        anchors.fill: parent
        ListView{
            id: list_view
            anchors.fill: parent
            anchors.margins: 0
            //Give each item a unique index by + 1
            //It can be highlighted with the current item of root
            property int itemCount: 0
            spacing: 2
            delegate: list_delegate
            clip: true
        }
    }
    Component{
        id:list_delegate
        Row{
            id:list_itemgroup
            spacing: 0
            Canvas{
                id:list_canvas
                width: item_titleicon.width+4
                height: list_itemcol.height
                antialiasing: false
                //The connection line of the last item has no tail
                property bool isLastItem: (index==parent.ListView.view.count-1)
                onPaint: {
                    var ctx = getContext("2d")
                    var i=0
                    //ctx.setLineDash([4,2]); //can't draw a dotted line
                    // setup the stroke
                    ctx.strokeStyle = Qt.rgba(201/255,202/255,202/255,1)
                    ctx.lineWidth=1
                    // create a path
                    ctx.beginPath()
                    //Using short line segment to achieve the effect of dashed line, judge li-3 is to prevent the width (4) from exceeding the judgment length
                    //-5 is in order not to contaminate the icon
                    //Here I'm a dotted line with a length of 4 and an interval of 2, which adds up to 6 cycles
                    ctx.moveTo(width/2,0)
                    for(i=0;i<list_itemrow.height/2-5-3;i+=6){
                        ctx.lineTo(width/2,i+4);
                        ctx.moveTo(width/2,i+6);
                    }

                    ctx.moveTo(width/2+5,list_itemrow.height/2)
                    for(i=width/2+5;i<width-3;i+=6){
                        ctx.lineTo(i+4,list_itemrow.height/2);
                        ctx.moveTo(i+6,list_itemrow.height/2);
                    }

                    if(!isLastItem){
                        ctx.moveTo(width/2,list_itemrow.height/2+5)
                        for(i=list_itemrow.height/2+5;i<height-3;i+=6){
                            ctx.lineTo(width/2,i+4);
                            ctx.moveTo(width/2,i+6);
                        }
                        //ctx.lineTo(10,height)
                    }
                    // stroke path
                    ctx.stroke()
                }

                //Item icon box -- can be racangle or image
                Image {
                    id:item_titleicon
                    width: 12
                    height: 12
                    visible: false
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: list_canvas.width/2-width/2
                    anchors.topMargin: list_itemrow.height/2-width/2

                    //Load different pictures / colors depending on whether there are children / expand or not
                    source: item_repeater.count?item_sub.visible?offSrc:onSrc:offSrc

                    MouseArea{
                        anchors.fill: parent
                        onClicked: {
                            if(item_repeater.count)
                                item_sub.visible=!item_sub.visible;
                        }
                    }
                }

                //Item check box
                Image {
                    id:item_optionicon
                    visible: false
                    width: 12
                    height: 12
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: list_canvas.width/2-width/2
                    anchors.topMargin: list_itemrow.height/2-width/2
                    property bool checked: false
                    source: checked?checkedSrc:uncheckSrc
                    MouseArea{
                        anchors.fill: parent
                        onClicked: {
                            item_optionicon.checked=!item_optionicon.checked;
                            if(item_optionicon.checked){
                                check(list_itemrow.itemIndex);
                            }else{
                                uncheck(list_itemrow.itemIndex);
                            }
                            var str="checked ";
                            for(var i in checkedArray)
                                str+=String(checkedArray[i])+" ";
                            console.log(str)
                        }
                    }
                }

            }

            //Item content: a listview containing a line of item and subitems
            Column{
                id:list_itemcol

                //one Item content
                Row {
                    id:list_itemrow
                    width: control.width
                    height: item_text.contentHeight+control.spacing
                    spacing: 5

                    property int itemIndex;

                    //Rectangle will cause hovered all change color,so  use Button
                    Button{
                        id: delegateBtn
                        height: item_text.contentHeight+control.spacing
                        width: parent.width
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id:item_text
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: name   //modelData.text
                            font.pointSize: Constants.labelFontPointSize_12
                            font.family: "Microsoft YaHei UI"
                            color: "black"//Qt.rgba(101/255,1,1,1)
                        }

                        MouseArea{
                            anchors.fill: parent
                            onClicked: {
                                currentItem=list_itemrow.itemIndex;
                                console.log("selected",list_itemrow.itemIndex)
                            }

                        }
                        background:Rectangle
                         {
                             anchors.fill: delegateBtn
                             opacity: enabled ? 1 : 0.3
                             color: (currentItem===list_itemrow.itemIndex)  ? treeItemBgSelectColor
                                                                            :delegateBtn.hovered ? treeItemBgHoverColor
                                                                                                 : treeItemBgColor
                         }
                    }


                    Component.onCompleted: {
                        list_itemrow.itemIndex=list_view.itemCount;
                        list_view.itemCount+=1;
                        if(level === 1  || level === 0)
                        {
                            item_titleicon.visible=true
                        }
                        else if(level === 2)
                        {
                            item_optionicon.visible=true;
                        }
                        currentItem = -1

//                        if(modelData.istitle)
//                            item_titleicon.visible=true;
//                        else if(modelData.isoption)
//                            item_optionicon.visible=true;
                    }
                }

                //child item
                Column{
                    id:item_sub
                    visible: control.autoExpand
                    //Upper left distance = small icon width + X offset
                    x:indent
                    Item {
                        width: 10
                        height: item_repeater.contentHeight
                        //need to add an item to open it. If you can't get count with repeater
                        ListView{
                            id:item_repeater
                            anchors.fill: parent
                            delegate: list_delegate
                            model:subnodes
                        }
                    }
                }

            }
        }
        //end list_itemgroup
    }
    //end list_delegate

    //check push arry
    function check(index){
        checkedArray.push(index);
    }

    //cancel check remove arry
    function uncheck(index){
        var i = checkedArray.length;

        while (i--) {
            if (checkedArray[i] === index) {
                checkedArray.splice(i,1);
                break;
            }
        }
    }
}
