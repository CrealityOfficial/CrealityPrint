import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import "../qml"
ComboBox {
    id: control
    font.family: Constants.labelFontFamily
    height: cmbHeight
    opacity: enabled?1 : 0.7

    //像源码一样，定义一个全局的样式，然后取全局样式中对应的颜色
    //checked选中状态，down按下状态，hovered悬停状态
    property color textColor: Constants.right_panel_text_default_color //文字颜色

    property color backgroundTheme: Constants.right_panel_combobox_background_color
    property color backgroundColor: Constants.right_panel_combobox_background_color
    property var translater: source => qsTr(source)
    property color itemNormalColor: Constants.right_panel_item_default_color
    property color itemHighlightColor: Constants.themeGreenColor  // Constants.currentTheme ?"#d6d6dc": "#4E7992"   // Constants.right_panel_item_checked_color
    property color itemBorderColor: Constants.right_panel_border_default_color
    property color popupColor: Constants.themeGreenColor // "#1E9BE2"
    property int showCount: 7             //最多显示的item个数
    property string sectionProperty: ""
    property int cmbHeight: 28 * screenScaleFactor
    property int popHeight: 28 * screenScaleFactor
    property real textLeftMargin: 10 *screenScaleFactor
    property var mysource :  Constants.downBtnImgSource
    property var mysource_d : Constants.downBtnImgSource_d
    property var keyStr: ""
    property bool hasImg: false
    property var currentModelData
    property real widthMultiple: 1
    property real radius: 5 * screenScaleFactor
    property Component slotItem
    signal comboBoxIndexChanged(var key, var value)
    signal currentContentChanged(var ctext)
    signal styleComboBoxIndexChanged(var key, var item)
    hoverEnabled: true
    //property var modelText: "modelData"
    function findTranslate(key)/*by TCJ*/
    {

        var tranlateValue = ""
        if(key === "high")tranlateValue = qsTr("High Quality")
        else if(key === "middle") tranlateValue= qsTr("Quality")
        else if(key === "low")tranlateValue = qsTr("Normal")
        else if(key === "best")tranlateValue = qsTr("Best quality")
        else if(key === "fast")tranlateValue = qsTr("Fast")
        else
        {
            tranlateValue = key
        }
        return tranlateValue
    }
    displayText: translater(currentText)
    delegate: ItemDelegate
    {
        property bool currentItem: control.highlightedIndex === index
        enabled: !listview.ScrollBar.vertical.active && myText.text && myText.text != "/" && myText.text != "?"
        width: control.width * widthMultiple
        height : popHeight
        contentItem: Rectangle
        {
            anchors.fill: parent

            Image{
                id: iconImg
                visible: hasImg
                width: 20*screenScaleFactor
                height: 20*screenScaleFactor
                sourceSize.width: width
                sourceSize.height: height
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 5*screenScaleFactor
                source: model.shapeImg !== undefined ? model.shapeImg : ""
            }

            Rectangle{
                id: colorRec
                visible: model.defaultOrSync === "2"
                width: 26*screenScaleFactor
                height: 16*screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 5*screenScaleFactor
                border.width: 1
                border.color: "#ffffff"
                color: model.materialColor
                radius: 2*screenScaleFactor

                onColorChanged: {
                    colorIndexText.color = Constants.setTextColor(color)
                }


                Text{
                    id: colorIndexText
                    anchors.centerIn: parent
                    text: model.syncBoxIndex
                    font.family: Constants.textColor
                }
            }

            Text {
                id:myText
                anchors.left: iconImg.left
                anchors.leftMargin: hasImg || colorRec.visible ? iconImg.width + 10*screenScaleFactor : 0
                x:5* screenScaleFactor
                height: popHeight
                width: parent.width - 5*screenScaleFactor
                text: translater(control.textRole
                           ? (Array.isArray(control.model)
                              ? modelData[control.textRole]
                              : model[control.textRole])
                           : findTranslate(modelData))
                color: textColor
                font: control.font
                elide: Text.ElideMiddle
                verticalAlignment: Text.AlignVCenter
            }

            color: control.highlightedIndex === index ? control.itemHighlightColor
                                                      : control.itemNormalColor
        }
        hoverEnabled: control.hoverEnabled
        onCurrentItemChanged: {
            currentModelData = model
            if(currentItem && hasImg){
                imgPop.show("")
                imgPop.imgSource = shapeImg
            }
        }
        onClicked:{
        }

        Component.onCompleted:{
            if(model.index === currentIndex){
                currentModelData = model
            }
        }
    }
    indicator:Item
    {
        anchors.right: control.right
        anchors.rightMargin: 5
        implicitHeight: 16 * screenScaleFactor
        implicitWidth: 16 * screenScaleFactor
        anchors.verticalCenter: control.verticalCenter
        Image {
            width: sourceSize.width* screenScaleFactor
            height: sourceSize.height* screenScaleFactor
            anchors.centerIn: parent
            source:  control.down || control.hovered ? mysource_d :  mysource
            opacity: enabled ? 1 : 0.3
            fillMode: Image.Pad
            visible: control.width > 20
        }
    }
    contentItem: Text {
        id:idContent
        anchors.left: control.left
        anchors.leftMargin: textLeftMargin
        anchors.right: control.indicator.left
        text: findTranslate(control.displayText)
        font: control.font
        color: textColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        onTextChanged: currentContentChanged(text)

    }
    background: CusRoundedBg {
        borderColor: (control.popup.visible || control.hovered) ? popupColor: itemBorderColor
        borderWidth: 1
        //        border.color: (control.popup.visible || control.hovered) ? popupColor: itemBorderColor
        //        border.width:   1
        allRadius: true
        color: "transparent"
        radius: control.radius
    }
    popup: Popup {
        id: popCom
        y: control.height
        width: control.width * widthMultiple
        horizontalPadding: 1
        verticalPadding: 2* screenScaleFactor
        implicitHeight:  __col.height + 5* screenScaleFactor
        contentItem:Item{
            Column {
                id : __col
                width: parent.width
                spacing: 3* screenScaleFactor
                ListView {
                    id: listview
                    clip: true
                    width: parent.width
                    implicitHeight: contentHeight > showCount*popHeight ? showCount*popHeight : contentHeight
                    model: control.popup.visible ? control.delegateModel : null
                    currentIndex: control.highlightedIndex
                    snapMode: ListView.SnapToItem
                    section.property: sectionProperty
                    section.criteria: ViewSection.FullString
                    section.delegate: Item{
                        width: parent.width
                        height: control.popHeight
                        Text{
                            color: textColor
                            font: control.font
                            //                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            anchors.left: parent.left
                            anchors.leftMargin: 5 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                            text: {
                                if(section === "0"){
                                    "------" + qsTr("Default configuration") + "------"
                                }else if(section === "1"){
                                    "------" + qsTr("User configuration") + "------"
                                }else if(section === "2"){
                                    "------" + qsTr("CFS") + "------"
                                }
                            }

                        }
                    }
                    ScrollBar.vertical: ScrollBar {
                        id :box_bar
                        implicitWidth: 10 * screenScaleFactor
                        visible: control.delegateModel ? (control.delegateModel.count > showCount) : false
                        contentItem: Rectangle {
                            implicitWidth:10 * screenScaleFactor
                            color: box_bar.pressed
                                   ? Qt.rgba(0.6,0.6,0.6)
                                   : Qt.rgba(0.6,0.6,0.6,0.5)
                        }
                    }
                }
                Loader{
                    width: parent.width - 4* screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter
                    sourceComponent: slotItem
                }
            }
        }
        background: Rectangle {
            border.width: 1
            border.color: control.popup.visible? popupColor: itemBorderColor
            color: control.itemNormalColor
            radius: 5* screenScaleFactor
        }

        // hasImg 有图片的情况
        ToolTip{
            property string imgSource
            id:imgPop
            width: 200*screenScaleFactor
            height: 200*screenScaleFactor
            x: parent.x - width-5*screenScaleFactor
            y: parent.y
            z: 0
            visible: hasImg && popCom.visible
            padding: 0
            contentItem: Rectangle{
                radius: 5*screenScaleFactor
                Image{
                    anchors.centerIn: parent
                    sourceSize.width: 160*screenScaleFactor
                    sourceSize.height: 160*screenScaleFactor
                    source: imgPop.imgSource
                }
            }
        }
    }
    onCurrentIndexChanged:
    {
        //        comboBoxIndexChanged(keyStr, currentIndex)
        //        styleComboBoxIndexChanged(keyStr, this)
    }
    onActivated: {
        console.log("currentindex = ", currentIndex)
        comboBoxIndexChanged(keyStr, currentIndex)
        styleComboBoxIndexChanged(keyStr, this)
    }
}
