import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.0 as QQC2
import "../qml"
QQC2.ComboBox {
    id: control
    font.family: Constants.labelFontFamily
    width : 260 * screenScaleFactor
    height: cmbHeight
    opacity: enabled?1 : 0.7

    //像源码一样，定义一个全局的样式，然后取全局样式中对应的颜色
    //checked选中状态，down按下状态，hovered悬停状态
    property color textColor: Constants.right_panel_text_default_color //文字颜色

    property color backgroundTheme: Constants.right_panel_combobox_background_color
    property color backgroundColor: Constants.right_panel_combobox_background_color

    property color itemNormalColor: Constants.right_panel_item_default_color
    property color itemHighlightColor: Constants.currentTheme ?"#d6d6dc": "#4E7992"   // Constants.right_panel_item_checked_color
    property color itemBorderColor: Constants.right_panel_border_default_color
    property color popupColor: "#1E9BE2"
    property int showCount: 7             //最多显示的item个数
    property int cmbHeight: 28 * screenScaleFactor
    property int popHeight: 28 * screenScaleFactor
    property var mysource : "qrc:/UI/photo/downBtn.svg"
    property var mysource_d : "qrc:/UI/photo/downBtn_d.svg"
    property var keyStr: ""
    property bool hasImg: false
    property Component slotItem
    signal comboBoxIndexChanged(var key, var value)
    signal currentContentChanged(var ctext)
    signal styleComboBoxIndexChanged(var key, var item)
    delegate: ItemDelegate
    {
        property bool currentItem: control.highlightedIndex === index
        width: control.width
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
                source: shapeImg
            }

            Text {
                id:myText
                anchors.left: iconImg.left
                anchors.leftMargin: hasImg ? iconImg.width +5*screenScaleFactor : 0
                x:5* screenScaleFactor
                height: popHeight
                width: parent.width - 5*screenScaleFactor
                text: modelData
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
            if(currentItem && hasImg){
                imgPop.show("")
                imgPop.imgSource = shapeImg
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
        anchors.leftMargin: 10* screenScaleFactor
        anchors.right: control.indicator.left
        anchors.rightMargin: control.indicator.width + control.spacing
        text: control.displayText
        font: control.font
        color: textColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        onTextChanged: currentContentChanged(text)

    }
    background: Rectangle {
        border.color: control.popup.visible? popupColor: itemBorderColor
        border.width:   1
        color: "transparent"
        radius: 5* screenScaleFactor
    }
    popup: Popup {
        id: popCom
        y: control.height
        width: control.width
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
        comboBoxIndexChanged(keyStr, currentIndex)
        styleComboBoxIndexChanged(keyStr, this)
    }
}
