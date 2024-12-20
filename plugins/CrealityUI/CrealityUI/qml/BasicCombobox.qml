import QtQuick 2.0
import QtQuick.Controls 2.5
ComboBox {
    id: control
    //像源码一样，定义一个全局的样式，然后取全局样式中对应的颜色
    //checked选中状态，down按下状态，hovered悬停状态
    property color textColor: Constants.textColor          //文字颜色
    property color backgroundTheme: "transparent"  //Constants.dialogItemRectBgColor//"#FFFFFF"
    property color backgroundColor: control.down
                                    ? backgroundTheme
                                    : control.hovered
                                      ? Qt.lighter(backgroundTheme)
                                      : backgroundTheme
    property color itemHighlightColor:Constants.cmbPopupColor_pressed//"#42BDD8"
    property color itemNormalColor: Constants.cmbPopupColor//"#E3EBEE"
    //    property color indicatorColor: "#383C3E"     //勾选的指示器的颜色
    property  color  itemBorderColor: Constants.dialogItemRectBgBorderColor
    property  color  itemBorderColor_H: Constants.textRectBgHoveredColor  //"#1e9be2"
    property int cmbRadius: 5 * screenScaleFactor
    property int showCount: 15           //最多显示的item个数
    //
    property int cmbHeight: 28*screenScaleFactor
    property int popHeight: 25*screenScaleFactor
    property var mysource : Constants.downBtnImg
    property var popPadding: 20
    property var contentPadding: 10
    property bool popHoverEnable: true

    property var strToolTip: ""
    property var popRectWidth: control.width
    property var keyStr: ""
    property bool hasImg: false
    
    signal comboBoxIndexChanged(var key, var value)
    signal styleComboBoxIndexChanged(var key, var item)

    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    font.pointSize: Constants.labelFontPointSize_9

    ToolTip {
        id: tipCtrl
        visible: hovered&&strToolTip ? true : false
        //timeout: 2000
        delay: 100
        width: 400*screenScaleFactor
        implicitHeight: idTextArea.contentHeight + 40*screenScaleFactor

        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }

        contentItem: TextArea{
            id: idTextArea
            text: strToolTip
            wrapMode: TextEdit.WordWrap
            color: Constants.textColor
            font.family: Constants.panelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: control.font.pointSize
            readOnly: true
            background: Rectangle
            {
                anchors.fill : parent
                color: Constants.tooltipBgColor
                border.width:1
                //border.color:hovered ? "lightblue" : "#bdbebf"
            }
        }
    }

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

    width : 220
    height: cmbHeight
    opacity: enabled?1 : 0.7
    delegate: hasImg ? imgCom : normalCom

    indicator:Rectangle
    {
        y:1
        anchors.right: control.right
        anchors.rightMargin: 1
        width: control.height - 1
        height: control.height - 2
        radius: 0
        anchors.verticalCenter: control.verticalCenter
        color: control.pressed ? Constants.cmbIndicatorRectColor_pressed_basic : Constants.cmbIndicatorRectColor_basic
        Image {
            width: 7*screenScaleFactor
            height: 5*screenScaleFactor
            anchors.centerIn: parent
            source: mysource //"qrc:/qt-project.org/imports/QtQuick/Controls.2/images/double-arrow.png"
            opacity: enabled ? 1 : 0.3

            visible: control.width > 20 ? true : false
        }
    }
    contentItem: Text {
        id:idContent
        anchors.left: control.left
        anchors.leftMargin: 10*screenScaleFactor//contentPadding
        anchors.right: control.indicator.left
        anchors.rightMargin: control.indicator.width + control.spacing
        text: findTranslate(control.displayText)/**/
        font: control.font
        color: control.pressed ? "#17a81a" : textColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        border.color: control.pressed ? itemNormalColor : (control.hovered ? itemBorderColor_H : itemBorderColor)
        border.width: control.visualFocus ? 2 : 1
        color: control.backgroundColor
        radius: cmbRadius
    }
    clip: true
    //弹出
    popup: Popup {
        id: popCom
        width: popRectWidth//control.width

        implicitHeight: control.delegateModel ? (control.delegateModel.count<showCount
                                                 ?contentItem.implicitHeight
                                                 :showCount*popHeight) : popHeight
        
        contentItem:Rectangle{
            anchors.fill: parent
            MouseArea{
                id: popMouseArea
                anchors.fill: parent
                hoverEnabled: popHoverEnable
                ListView {
                    id: listview
                    clip: true
                    width: popRectWidth//control.width
                    implicitHeight: contentHeight > showCount*popHeight ? showCount*popHeight : contentHeight
                    model: control.popup.visible ? control.delegateModel : null
                    currentIndex: control.highlightedIndex
                    snapMode: ListView.SnapToItem
                    ScrollBar.vertical: ScrollBar {
                        id :box_bar
                        implicitWidth: 10*screenScaleFactor
                        visible: control.delegateModel ? (control.delegateModel.count > showCount) : false
                        contentItem:Rectangle
                        {
                            implicitWidth:10*screenScaleFactor
                            radius: width/2
                            color: box_bar.pressed
                                   ? Qt.rgba(0.6,0.6,0.6)
                                   : Qt.rgba(0.6,0.6,0.6,0.5)
                        }
                    }
                }
                onExited: {
                    popCom.close()
                }
                onEntered: {
                }
                onMouseYChanged: {
                    console.log("mouse.y = ", mouseY)
                }

            }
        }
        background: Rectangle {
            border.color: itemBorderColor
            radius: 2
        }

        ToolTip{
            property string imgSource
            id:imgPop
            width: 200*screenScaleFactor
            height: 200*screenScaleFactor
            x: parent.x - width
            y: parent.y
            z: 0
            visible: hasImg && popCom.visible
            padding: 0
            contentItem: Rectangle{
                radius: 5
                Image{
                    anchors.centerIn: parent
                    sourceSize.width: 160*screenScaleFactor
                    sourceSize.height: 160*screenScaleFactor
                    source: imgPop.imgSource
                }
            }
        }
    }

    Component{
        id: normalCom
        ItemDelegate
        {
            width: popRectWidth//control.width
            height : popHeight
            contentItem: Rectangle
            {
                anchors.fill: parent
                Text {
                    id:myText
                    x:10*screenScaleFactor//popPadding
                    height: popHeight
                    width: parent.width - 10*screenScaleFactor
                    text: qsTr(control.textRole
                               ? (Array.isArray(control.model)
                                  ? modelData[control.textRole]
                                  : model[control.textRole])
                               : findTranslate(modelData))/**/
                    color: "#333333"
                    font: control.font
                    elide: Text.ElideMiddle
                    verticalAlignment: Text.AlignVCenter
                }
                color: (control.highlightedIndex === index)
                       ? control.itemHighlightColor
                       : control.itemNormalColor
            }
            hoverEnabled: control.hoverEnabled
        }
    }

    Component{
        id: imgCom
        ItemDelegate
        {
            property bool isCurrentItem: control.highlightedIndex === index
            width: popRectWidth//control.width
            height : popHeight
            padding: 0
            contentItem: Rectangle
            {
                Image{
                    id: iconImg
                    width: 20*screenScaleFactor
                    height: 20*screenScaleFactor
                    sourceSize.width: width
                    sourceSize.height: height
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 3
                    source: shapeImg
                }

                Text {
                    id:myText
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: iconImg.right
                    anchors.leftMargin: 3
                    width: parent.width - 10*screenScaleFactor
                    text: control.textRole
                          ? (Array.isArray(control.model)
                             ? modelData[control.textRole]
                             : model[control.textRole])
                          : findTranslate(modelData)/**/
                    color: "#333333"
                    font: control.font
                    elide: Text.ElideMiddle
                    verticalAlignment: Text.AlignVCenter
                }
                color: (control.highlightedIndex === index)
                       ? control.itemHighlightColor
                       : control.itemNormalColor
            }
            hoverEnabled: control.hoverEnabled
            onIsCurrentItemChanged: {
                if(isCurrentItem){
                    imgPop.show("")
                    imgPop.imgSource = shapeImg
                }
            }
        }
    }

    onCurrentIndexChanged:
    {
        comboBoxIndexChanged(keyStr, currentIndex)
        styleComboBoxIndexChanged(keyStr, this)
    }
}

