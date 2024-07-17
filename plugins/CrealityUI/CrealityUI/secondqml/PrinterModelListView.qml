import QtQuick 2.0
import QtQuick.Controls 2.12
//import QtQuick.Controls 2.0 as QQC2
import ".."
import "../qml"
Rectangle
{
    height: 200
    width: 300
    //color: "transparent"
    property color defaultBgColor: Constants.treeItemColor // "#424242"//"transparent"//"#ffffff"
    property color hoveredBgColor: Constants.treeItemColor_pressed //"#E4E4E4"//"#618BFF"
    property color selectBgColor: Constants.treeItemColor_pressed
    property alias myModel: listView.model
    property var imgSource : "qrc:/UI/images/highquality.png"
    color: Constants.treeBackgroundColor//"white"
    property int currentItem: listView.currentIndex //Currently selected item
    property bool bottonSelected:false

    property int myContentSpacing: 2
    property int myContentHeight: 30
	
	signal sigDoubleClicked()
    signal sigModelChanged()
//    property var mapTranlate :{0:0}

//    function translateInit()
//    {
//        mapTranlate["high"] = qsTr("High Quality")
//        mapTranlate["middle"] = qsTr("Quality")
//        mapTranlate["low"] = qsTr("Normal")
//    }

    function findTranslate(key)
    {

        var tranlateValue = ""
        if(key === "high")tranlateValue = qsTr("High Quality")
        else if(key === "middle") tranlateValue= qsTr("Quality")
        else if(key === "low")tranlateValue = qsTr("Normal")
        else if(key === "fast")tranlateValue = qsTr("Fast")
        else
        {
            tranlateValue = key
        }

        return tranlateValue
    }

    function findTranslateLayer(key)
    {

        var tranlateValue = ""
        if(key === "high")tranlateValue = "0.10mm"
        else if(key === "middle") tranlateValue= "0.15mm"
        else if(key === "low")tranlateValue = "0.20mm"
        else
        {
            tranlateValue = key
        }

        return tranlateValue
    }
//    function setImage(index)
//    {
//        var source;
//        if(index ===0)source = "qrc:/UI/images/highquality.png";
//        else if(index === 1) source = "qrc:/UI/images/midquality.png"
//        else if(index === 2) source = "qrc:/UI/images/lowquality.png"
//        else
//        {
//            source = "qrc:/UI/images/lowquality.png"
//        }
//        return source;
//    }

    BasicScrollView
    {
        width: parent.width
        height: parent.height
        ListView {
            id: listView
            //property int  itemCount: 0
            anchors.fill: parent
            clip: true
            focus:true
            snapMode: ListView.SnapToItem //Scroll by row
            spacing: myContentSpacing

            Rectangle
            {
                anchors.fill: parent
                border.width: 1
                border.color: Constants.rectBorderColor
                color: "transparent"
            }
            delegate:
            Button
            {
                id : delegateBtn
                x:1
                y:2
                width: listView.width -2
                height: myContentHeight

                property int itemIndex: index
                Item
                {
                    id : spaceRect
                    height: delegateBtn.height
                    width : 5
                    anchors.left: delegateBtn.left
                   // color: "transparent"
                }
                // BasicTooltip {
                //     id: idTooptip
                //     visible: delegateBtn.hovered && (delegateBtn.itemIndex === currentItem)
                //     text: "本模式的主要参数配置为层高（0.15mm），走线宽度（0.40mm），壁走线次数（3），填充密度（10%），平台附着类型（简单底座），打印温度（200℃），打印平台温度（60℃），打印速度（50mm/s）"
                // }
                
                ToolTip {
                    id: tipCtrl
                    visible: delegateBtn.hovered && (delegateBtn.itemIndex === currentItem)
                    //timeout: 2000
                    delay: 100
                    width: 400
                    implicitHeight: idTextArea.contentHeight + 40
                    
                    background: Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                    }
                    
                    contentItem: TextArea{
                        id: idTextArea
                        wrapMode: TextEdit.WordWrap
                        color: Constants.textColor
                        font.family: Constants.labelFontFamily
                        font.weight: Constants.labelFontWeight
                        font.pointSize: Constants.labelFontPointSize_12
                        readOnly: true
                        background: Rectangle
                        {
                            anchors.fill : parent
                            color: Constants.tooltipBgColor
                            border.width:1
                        }
                    }
                }
                Rectangle
                {
                    id : btn
                    width : 5//24
                    height : myContentHeight
                    anchors.left: spaceRect.right
//                    Image {
//                        id: childImg
//                        y:4
//                        width: btn.width
//                        height: btn.height - 8
//                        source: setImage(index)
//                    }

                    color: (delegateBtn.itemIndex === currentItem) ?  selectBgColor: delegateBtn.hovered ?hoveredBgColor:defaultBgColor

//                    visible: (imgSource.length > 0) ? true :false
                }
                
                StyledLabel{
                    id: idMaterial
                    width: 50
                    height: 24
                    font.pointSize: Constants.labelFontPointSize_10
                    anchors.left:   btn.right
                    anchors.leftMargin: 0
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignRight
                    text: modelDataMaterial
                }

                Text {
                    id: chlidtextId
                    width: 50
                    // y: 5//8/*5*/
                    height : 24
                    anchors.left: idMaterial.right
                    anchors.leftMargin: 20
                    //  anchors.horizontalCenter: delegateBtn.horizontalCenter
                    text:findTranslate(modelDataName)
                    font.pointSize: Constants.labelFontPointSize_10
                    font.family: Constants.labelFontFamily
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignRight
                    color: Constants.textColor

                    
                }

                StyledLabel{
                    width: 50
                    height: 24
                    font.pointSize: Constants.labelFontPointSize_10
                    anchors.left:  chlidtextId.right
                    anchors.leftMargin: 20
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignRight
                    text: Number(modelDataLayer).toFixed(2) + "mm"//Number(myModel.getProfileLayerHeight(modelDataName)).toFixed(2) + "mm"//findTranslateLayer(modelDataName)
                }

                


                background :Rectangle
                {
                    anchors.fill: delegateBtn
                    /*radius : 4*/
                    opacity: enabled ? 1 : 0.3
                    color: (delegateBtn.itemIndex === currentItem)? selectBgColor :delegateBtn.hovered ? hoveredBgColor:defaultBgColor
                }
                Component.onCompleted:
                {
                    //delegateBtn.itemIndex=listView.itemCount;
                    //listView.itemCount+=1;
                }
                onClicked:
                {
                    currentItem = delegateBtn.itemIndex;
//                    myModel.setSelectIndex(currentItem)
                    sigModelChanged()
                    //console.log(currentItem)
                    //console.log("findTranslate(modelDataName) =" + findTranslate(modelDataName))
                }
				onDoubleClicked:
				{
					//console.log("double click~~~~~~~~~~~~")
					sigDoubleClicked()
				}
            }
        }
    }

}

