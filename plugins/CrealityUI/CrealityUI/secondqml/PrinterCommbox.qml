import QtQuick 2.0
import QtQuick.Controls 2.12
//import CrealityUI 1.0
//import "qrc:/CrealityUI"
import ".."
import "../qml"
import QtQuick.Layouts 1.13
ComboBox {
    id: control

    property color textColor: Constants.textColor
    property color backgroundTheme: Constants.dialogItemRectBgColor
    property color backgroundColor: control.down
                                    ? backgroundTheme
                                    : control.hovered
                                      ? Qt.lighter(backgroundTheme)
                                      : backgroundTheme
    property color itemHighlightColor: "#74C9FF"
    property color itemNormalColor: Constants.printerCommboxPopBgColor//Constants.cmbPopupColor//"#E3EBEE"
    property  color  itemBorderColor: "#707070"
    property int radius: 2
    property int showCount: 6
    //
    property int cmbHeight: 28
    property int popHeight: 25
    property bool isHaveBtn: false
    width : 220
    height: cmbHeight
    property var mysource : "qrc:/UI/photo/down.png"
    signal sigAddPrinter()
    signal sigManagerPrinter()
    signal currentMachineChanged(string txt);

    visible: width > 10 ? true:false
    onCurrentTextChanged:
    {
        //        console.log("currentText =" + currentText)
        //        console.log("currentIndex =" + currentIndex)
        currentMachineChanged(currentText)
    }

    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    //popup
    delegate: ItemDelegate
    {
        x: 1
        width: control.width - 2
        height : popHeight
        contentItem: Rectangle
        {
            anchors.fill: parent
            Text {
                id:myText
                x:20
                height: popHeight
                text: control.textRole
                      ? (Array.isArray(control.model)
                         ? modelData[control.textRole]
                         : model[control.textRole])
                      : modelData
                color: "#333333"
                font: control.font
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            color: (control.highlightedIndex === index)
                   ? control.itemHighlightColor
                   : control.itemNormalColor
        }
        hoverEnabled: control.hoverEnabled

    }
    indicator:
        Rectangle
    {
        anchors.right: control.right
        anchors.rightMargin: 1
        width: control.height - 4
        height: control.height - 4
        border.width: 0//idpop.visible ? 1 : (control.hovered ? 1 : 0)
        border.color: Constants.printerCommboxBgColor//"#181818"
        anchors.verticalCenter: control.verticalCenter
        color: idpop.visible ? Constants.printerCommboxIndicatorPopShowBgColor : (control.hovered ? Constants.printerCommboxIndicatorPopShowBgColor : Constants.printerCommboxIndicatorBgColor)
        Image {
            width: 7//10/*14*/
            height: 4//8/*10*/
            anchors.centerIn: parent
            source: mysource //"qrc:/qt-project.org/imports/QtQuick/Controls.2/images/double-arrow.png"
            opacity: enabled ? 1 : 0.3

            visible: control.width > 20 ? true : false
        }
    }
    contentItem: Text {
        anchors.left: control.left
        anchors.leftMargin: 10
        anchors.right: control.indicator.left
        anchors.rightMargin: control.indicator.width + control.spacing
        text: control.displayText
        font: control.font
        
        color: textColor//"#333333" control.pressed ? "#17a81a" : textColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        Component.onCompleted:{
            font.pointSize = Constants.labelFontPointSize_10
        }
    }

    //框中
    background: Rectangle {
        border.color: idpop.visible ? Constants.printerCommboxIndicatorPopShowBgColor : (control.hovered ?Constants.textRectBgHoveredColor : Constants.dialogItemRectBgBorderColor)
        border.width: 1//control.visualFocus ? 2 : 1
        color: Constants.printerCommboxBgColor//"#181818"
    }
    //弹出
    popup: Popup {
        id:idpop
        y: control.height
        width: control.width
        implicitHeight: control.delegateModel.count<showCount
                        ?contentItem.implicitHeight
                        :isHaveBtn ? showCount*popHeight + 35 : showCount*popHeight + 35
        padding: 1
        contentItem:
            Rectangle
        {
            id: testssss
            x: 1
            width: control.width - 2
            //anchors.fill: parent
            Column {
                height: idpop.height
                spacing: 0
                ListView {
                    id: listview
                    clip: true
                    width: control.width - 2
                    implicitHeight: contentHeight > showCount*popHeight ? showCount*popHeight : contentHeight
                    model: control.popup.visible ? control.delegateModel : null
                    currentIndex: control.highlightedIndex
                    snapMode: ListView.SnapToItem
                    ScrollBar.vertical: ScrollBar {
                        id :box_bar
                        implicitWidth: 10
                        visible: (control.delegateModel.count > showCount)
                        contentItem: Rectangle{
                            implicitWidth:10
                            radius: width/2
                            color: box_bar.pressed
                                   ? Qt.rgba(0.6,0.6,0.6)
                                   : Qt.rgba(0.6,0.6,0.6,0.5)
                        }
                    }
                }
                Rectangle {
                    id: name
                    width: parent.width
                    height: 26
                    color: Constants.printerCommboxPopBgColor
                    Row{
                        y: 1
                        //spacing: 2
                        Rectangle{
                            width: 1
                            height: 24
                            color: "transparent"
                        }
                        BasicButton{
                            id: idAdd
                            width: 80
                            height: 24
                            btnRadius: 3
                            text: qsTr("Add")
                            btnBorderW: 0
                            btnTextColor: hovered ? "#FFFFFF": "#333333"
                            defaultBtnBgColor: Constants.printerCommboxBtnDefaultColor//"#B7B7B7"
                            hoveredBtnBgColor: "#1E9BE2"
                            onSigButtonClicked:
                            {
                                sigAddPrinter()
                            }
                        }
                        Rectangle{
                            width: 2
                            height: 24
                            color: "transparent"
                        }
                        BasicButton{
                            id: idManager
                            width: 80
                            height: 24
                            btnRadius: 3
                            text: qsTr("Manage")
                            btnBorderW: 0
                            btnTextColor: hovered ? "#FFFFFF": "#333333"
                            defaultBtnBgColor: Constants.printerCommboxBtnDefaultColor
                            hoveredBtnBgColor: "#1E9BE2"
                            onSigButtonClicked:
                            {
                                sigManagerPrinter()
                            }
                        }
                    }
                    
                }
            }

        }
        background: Rectangle {
            width: control.width
            height: listview.height + 28//showCount*popHeight + 30
            color: Constants.backgroundColor
            border.width: 1
            border.color: Constants.printerCommboxIndicatorPopShowBgColor
        }
    }
}

