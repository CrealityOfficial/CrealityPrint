import QtQuick 2.0
import QtQuick.Controls 2.0
import "../qml"
Item {
    id: root

    property alias lineIndex: textView.currentIndex
    //    property variant model
    property alias model: textView.model

    property alias backgroundColor: idBackground.color
    property alias backgroundBorder: idBackground.border
    property alias backgroundRadius: idBackground.radius
    
    property bool clickEnabled: true

    width: parent.width
    height: parent.height

    signal doubleClickItem(var viewIndex)
    signal preparePressed(var viewIndex)
    signal prepareReleased(var viewIndex)

    Rectangle{
        id: idBackground
        width: parent.width
        height: parent.height
        color: Constants.tabButtonSelectColor
        border.width: 1
        border.color: Constants.dialogItemRectBgBorderColor
        BasicScrollView {
            anchors.top: parent.top
            anchors.topMargin: 10 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            width: parent.width - 10 * screenScaleFactor
            height: parent.height - 20 * screenScaleFactor
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
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
            property bool pressed: false

            Rectangle {
                id: bgColor
                anchors.fill: parent
                radius: 5
                color: (idtext.text && wrapper.ListView.isCurrentItem) || wrapper.pressed ? Constants.themeGreenColor: "transparent"
            }
            Text {
                id : idtext
                x:5 * screenScaleFactor
                y:1 * screenScaleFactor
                property var myIndex
                text: modelData
                myIndex : index
                height: parent.height
                wrapMode: Text.WrapAnywhere
                font.family: Constants.labelFontFamily
                font.pointSize: Constants.labelFontPointSize_9
                color: wrapper.ListView.isCurrentItem? "red": "#ffffff"
            }
            MouseArea {
                id: mousearea
                anchors.fill: parent
                onDoubleClicked: {
                    if (!clickEnabled)
                        return

                    textView.currentIndex = index;
                    doubleClickItem(index)
                }
                onPressed: {
                    preparePressed(index)
                    if (!clickEnabled)
                        return

                    wrapper.pressed = true
                }
                onReleased: {
                    prepareReleased(index)
                    if (!clickEnabled)
                        return

                    wrapper.pressed = false
                }
            }
        }
    }

    Component {
        id: highlightComponent
        Rectangle {
            color: "transparent"
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
