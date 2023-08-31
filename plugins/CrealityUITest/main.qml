import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQml 2.3
import QtQuick.Layouts 1.13
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"


ApplicationWindow {
    id: standaloneWindow
    visible: true
    width: 1024//Constants.width
    height: 768//Constants.height
    title: qsTr("Creality Slicer UI Test Project!")

    Control{
        anchors.fill: parent

        contentItem: Item{
            Rectangle{
                id: titleRec
                width: parent.width
                height: 30
                color:"grey"
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle{
                id: leftToolBar
                width: 100
                height: parent.height - titleRec.height
                anchors.top: titleRec.bottom
                anchors.topMargin: 10
                anchors.left: titleRec.left
                anchors.leftMargin: 10
                color:"lightBlue"

                CusListView{
                    id: leftListView
                    width: parent.width - 4
                    height: 300
                    anchors.top: parent.top
                    anchors.topMargin: 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 5

                    model: ["Button", "Text", "Slider", "ListView"]
                    delegate: CusButton{
                        checked: leftListView.currentIndex === model.index
                        width: parent.width
                        height: 30
                        checkable: true
                        bText.text: modelData
                        onClicked:{
                            leftListView.currentIndex = model.index
                        }
                    }
                }
            }

            Control{
                width: 890
                height: 710
                anchors.top: leftToolBar.top
                anchors.left: leftToolBar.right
                anchors.leftMargin: 4
                padding: 5
                background: Rectangle{
                    radius: 5
                    border.width: 1
                    border.color: "grey"
                }

                contentItem: Item{
                    StackLayout{
                        id: stackLayout
                        anchors.fill: parent
                        currentIndex: leftListView.currentIndex
                        ButtonShowPage{

                        }
                        TextShowPage{}
                        SliderShowPage{}

                    }
                }
            }
        }
    }

    Component.onCompleted:
    {
    }
}
