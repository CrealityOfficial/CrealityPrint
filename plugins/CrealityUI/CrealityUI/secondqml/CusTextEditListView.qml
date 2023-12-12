import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import "../qml"
Item {
    id: root

    property alias lineIndex: textView.currentIndex
    //    property variant model
    property alias model: textView.model

    property alias backgroundColor: idBackground.color
    property alias backgroundBorder: idBackground.border
    property alias backgroundRadius: idBackground.radius

    width: parent.width
    height: parent.height
    signal doubleClickItem(var viewIndex)
    Rectangle{
        id: idBackground
        width: parent.width
        height: parent.height
        color: Constants.tabButtonSelectColor
        border.width: 1
        border.color: Constants.dialogItemRectBgBorderColor
        ScrollView {
            anchors.top: parent.top
            anchors.topMargin: 10 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            width: parent.width - 10 * screenScaleFactor
            height: parent.height - 20 * screenScaleFactor
            ListView {
                id: textView
                width: parent.width// - 20
                delegate: delegateItem
                highlight: highlightComponent
                focus: true
                clip: true
            }
        }
    }

    Component {
        id: delegateItem
        Item {
            id :wrapper
            width: idtext.contentWidth + 10 * screenScaleFactor
            height: 20 * screenScaleFactor
            Rectangle {
                id: bgColor
                anchors.fill: parent
                color: "transparent"
            }
            Text {
                id : idtext
                x:5 * screenScaleFactor
                y:1 * screenScaleFactor
                property var myIndex
                text: modelData
                myIndex : index
                //                width: parent.width
                height: parent.height
                wrapMode: Text.WrapAnywhere
                font.family: Constants.labelFontFamily
                font.pointSize: Constants.labelFontPointSize_9
                color: wrapper.ListView.isCurrentItem? "red": Constants.textColor
            }
            MouseArea {
                id: mousearea
                anchors.fill: parent
                onDoubleClicked: {
                    textView.currentIndex = index;
                    doubleClickItem(index)
                }
                onPressed: {
                    bgColor.color = Constants.right_panel_item_checked_color
                }
                onReleased: {
                    bgColor.color = "transparent"
                }
            }
        }
    }

    Component {
        id: highlightComponent
        Rectangle {
            color: "lightblue"
            radius: 5
            y: textView.currentItem.y
            Behavior on y {
                SpringAnimation {
                    spring: 3
                    damping: 0.2
                }
            }
        }
    }

}
