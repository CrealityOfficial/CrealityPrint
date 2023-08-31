import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"
import "qrc:/qml/CrealityUIComponent/CusText"
Item {
    Column{
        anchors.fill: parent
        spacing: 10
        CusText{
            text: qsTr("跑马灯")
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
                CusText{
                    text: qsTr("文本长度超过150向左滚的跑马灯文字文本长度超过150向左滚的跑马灯文字")
                    needRuning: true
                    fontWidth: 150
                }
                CusText{
                    text: qsTr("文本长度超过150宽度的不滚动的超长文字文本长度超过150向左滚的跑马灯文字")
                    isDefault: false
                }
            }
        }

        CusText{
            text: qsTr("省略号")
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
                CusText{
                    text: qsTr("文本长度超过150中间出现省略号，鼠标放上去出现tip")
                    elide: Text.ElideMiddle
                }
                CusText{
                    text: qsTr("文本长度超过150右边出现省略号，鼠标放上去出现tip")
                    elide: Text.ElideRight
                }
            }
        }
    }
}
