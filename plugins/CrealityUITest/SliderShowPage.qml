import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"
import "qrc:/qml/CrealityUIComponent/CusText"
import "qrc:/qml/CrealityUIComponent/CusSlider"
Item {
    Column{
        anchors.fill: parent
        spacing: 10
        CusText{
            text: qsTr("滑条")
        }

        Rectangle{
            width: parent.width
            height: 50
            color:"lightBlue"

            ButtonGroup{
                id: btnGroup
                buttons: btns.children
            }

            Row{
                id: btns
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                spacing: 20
                CusSlider{

                }
            }
        }

        CusText{
            text: qsTr("带文字")
        }

        Rectangle{
            width: parent.width
            height: 70
            color:"lightBlue"

            ButtonGroup{
                id: btnGroup1
                buttons: btns1.children
            }

            Row{
                id: btns1
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                spacing: 20
                CusSlider{
                    labelLeft: "label"
                }
            }
        }
        CusText{
            text: qsTr("进度条")
        }

        Rectangle{
            width: parent.width
            height: 70
            color:"lightBlue"

            ButtonGroup{
                id: btnGroup2
                buttons: btns2.children
            }

            Row{
                id: btns2
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 20
                spacing: 20
                CusSlider{
                    labelLeft: "label"
                    isProgress: true
                    showRightLabel: true
                    initValue: 0.3
                }
            }
        }
    }
}
