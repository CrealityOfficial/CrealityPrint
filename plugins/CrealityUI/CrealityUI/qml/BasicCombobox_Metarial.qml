import QtQuick 2.0
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.13
ComboBox {
    id: control
    property bool addBtnVisible: true
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
    property real btnWidth: 120 * screenScaleFactor
    property real btnHeight: 24 * screenScaleFactor
    property int showCount: 7             //最多显示的item个数
    //
    property int cmbHeight: 28 * screenScaleFactor
    property int popHeight: 28 * screenScaleFactor
    property var mysource : "qrc:/UI/photo/downBtn.svg"
    property var mysource_d : "qrc:/UI/photo/downBtn_d.svg"
    property var popPadding: 20
    property var contentPadding: 10
    signal currentContentChanged(var ctext)
    signal addMaterialClick()
    signal sigManagerClick()

    delegate: ItemDelegate
    {
        width: control.width
        height : popHeight
        contentItem: Rectangle
        {
            anchors.fill: parent
            Text {
                id:myText
                x:5//popPadding
                height: popHeight
                text: modelData
                color: textColor
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            color: control.highlightedIndex === index ? control.itemHighlightColor
                                                      : control.itemNormalColor
        }
        hoverEnabled: control.hoverEnabled
    }
    indicator:
        Item
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

            visible: control.width > 20 ? true : false
        }
    }
    contentItem: Text {
        id:idContent
        anchors.left: control.left
        anchors.leftMargin: 10//contentPadding
        anchors.right: control.indicator.left
        anchors.rightMargin: control.indicator.width + control.spacing
        text: control.displayText
        font: control.font
        color: textColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        onTextChanged: {
            currentContentChanged(text)
        }
    }
    background: Rectangle {
        border.color: control.popup.visible? popupColor: itemBorderColor
        border.width:   1
        color: "transparent"
        radius: 5
    }
    //    //弹出
    popup: Popup {
        y: control.height
        width: control.width
        horizontalPadding: 1
        verticalPadding: 2
        implicitHeight:  __col.height + 10
        contentItem:Item{
            Column {
                id : __col
                width: parent.width
                spacing: 3
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
                        contentItem:
                            Rectangle
                        {
                            implicitWidth:10 * screenScaleFactor
                            //radius: width/2
                            color: box_bar.pressed
                                   ? Qt.rgba(0.6,0.6,0.6)
                                   : Qt.rgba(0.6,0.6,0.6,0.5)
                        }
                    }
                }

                Row{
                    spacing: 2
                    visible: addBtnVisible
                    anchors.horizontalCenter: parent.horizontalCenter
                    CusButton {
                        id:addBtn
                        width: btnWidth
                        height : btnHeight
                        radius: 5
                        txtContent: qsTr("Add")
                        enabled: true
                        onClicked:
                        {
                            control.popup.visible = false
                            addMaterialClick()
                        }
                    }

                    CusButton {
                        id:managerBtn
                        width: btnWidth
                        height : btnHeight
                        radius: 5
                        txtContent: qsTr("Manage")
                        enabled: true
                        onClicked:
                        {
                            control.popup.visible = false
                            sigManagerClick()
                        }
                    }
                }
            }
        }
        background: Rectangle {
            border.width: 1
            border.color: control.popup.visible? popupColor: itemBorderColor
            color: control.itemNormalColor
            radius: 5
        }
    }
}
