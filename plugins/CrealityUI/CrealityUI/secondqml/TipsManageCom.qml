import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQml 2.13
import "../qml"


BasicScrollView{
    property var funcs:[]
    readonly property real maxHeight: 280*screenScaleFactor
    id: tipsItem
    width: 560*screenScaleFactor
    height: implicitContentHeight < maxHeight ? implicitContentHeight : maxHeight
    hpolicyVisible: false
    vpolicyVisible: contentHeight > height
    clip: true

    background: Item{

    }

    //添加项目
    function addMsgTip(content1, content2, func){
        scrollViewModel.append({"content1" : qsTr(content1), "content2" :qsTr(content2), "func" : func})
        funcs[scrollViewModel.count - 1] = func

        tipsItem.height = implicitContentHeight < maxHeight ? implicitContentHeight : maxHeight
    }

    //删除项目
    function delMsgTip(itemIndex){
        funcs.splice(itemIndex, 1)
        scrollViewModel.remove(itemIndex)

        if(scrollViewModel.count == 0){
            tipsItem.visible = false
            return;
        }

        tipsItem.height = implicitContentHeight < maxHeight ? implicitContentHeight : maxHeight
    }

    ListModel{
        id: scrollViewModel
        ListElement{content1: qsTr("The model has not been supported and may fail to print. Do you want to continue adding supports before slicing?");  content2: qsTr("Add Support"); func: ""}
    }

    Column{
        id: col
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10*screenScaleFactor
        Repeater{
            id: rep
            model:scrollViewModel
            delegate: tipCom
        }
    }

    Component{
        id: tipCom
        Control{
            id: tipsItem
            width: 430 * screenScaleFactor
            height: tipsContent.height + tipsContent1.height + 10
            background: Rectangle{
                color:"#000000"
                radius: 5 * screenScaleFactor
                opacity: 0.2
            }

            contentItem: Item{
                CusText{
                    id: tipsContent
                    anchors.top: parent.top
                    anchors.topMargin: 5
                    anchors.left: parent.left
                    anchors.leftMargin: 5
                    aliasText.textFormat: Text.RichText
                    fontWidth: tipsItem.width - 50* screenScaleFactor
                    fontWrapMode: Text.WordWrap
                    fontText: model.content1
                    fontColor: Constants.manager_printer_button_text_color
                }

                CusText{
                    id: tipsContent1
                    hAlign: 2
                    anchors.top: tipsContent.bottom
                    anchors.topMargin: 5
                    anchors.right: tipsContent.right
                    fontWidth: 200* screenScaleFactor
                    fontWrapMode: Text.WordWrap
//                    aliasText.textFormat: Text.RichText
                    fontText: `<a href='https://www.example.com'}><font color="#00a3ff" size="1-7">${content2}</font></a>`
                    fontColor: Constants.manager_printer_button_text_color
                    aliasText.onLinkActivated: funcs[index]()
                }


                CusButton{
                    id:closeBtn
                    anchors.right: parent.right
                    anchors.rightMargin: 10* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    width: 30 * screenScaleFactor
                    height: 30 * screenScaleFactor
                    allRadius: true
                    radius: width/2
                    normalColor:  "transparent"
                    hoveredColor:  Constants.topToolBarColor
                    pressedColor: Constants.topToolBarColor
                    borderWidth: 0

                    Image{
                        anchors.centerIn: parent
                        sourceSize.width: 10* screenScaleFactor
                        sourceSize.height: 10* screenScaleFactor
                        source: "qrc:/UI/photo/lanPrinterSource/closeBtn_light.svg"
                    }
                    onClicked: {
                        delMsgTip(model.index)
                    }
                }
            }
        }
    }
    function func1(){
        console.log("=========== func1")
    }

    function func2(){
        console.log("=========== func2")
    }

    function func3(){
        console.log("=========== func3")
    }
}
