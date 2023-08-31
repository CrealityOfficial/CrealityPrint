import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"
//import "qrc:/qml/CrealityUIComponent/CusText"
//import "qrc:/qml/CrealityUIComponent/CusButton"
Item {
    Column{
        anchors.fill: parent
        spacing: 10
        CusText{
            text: qsTr("不同的圆角设置")
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
                CusButton{
                    width: 100
                    height: 30
                    btnBg.radius: 10
                    checkable: true
                    bText.text: qsTr("正常圆角")
                }

                CusButton{
                    width: 100
                    height: 30
                    btnBg.radius: 10
                    btnBg.allRadius: true
                    checkable: true
                    bText.text: qsTr("正常圆角")
                }

                CusButton{
                    width: 100
                    height: 30
                    btnBg.radius: 10
                    btnBg.leftTop: true
                    checkable: true
                    bText.text: qsTr("左上角")
                }

                CusButton{
                    width: 100
                    height: 30
                    btnBg.radius: 10
                    btnBg.rightTop: true
                    checkable: true
                    bText.text: qsTr("右上角")
                }

                CusButton{
                    width: 100
                    height: 30
                    btnBg.radius: 10
                    btnBg.leftBottom: true
                    checkable: true
                    bText.text: qsTr("左下角")
                }

                CusButton{
                    width: 100
                    height: 30
                    btnBg.radius: 10
                    btnBg.rightBottom: true
                    checkable: true
                    bText.text: qsTr("右下角")
                }
            }
        }

        CusText{
            text: qsTr("图片的不同位置")
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
                CusButton{
                    width: 50
                    height: 50
                    checkable: true
                    bText.text: qsTr("Btn")
                    state: "imageOnly"
                    bImg.source: "qrc:/res/img/warning_icon.svg"
                }
                CusButton{
                    width: 50
                    height: 50
                    checkable: true
                    bText.text: qsTr("Btn")
                    state: "textOnly"
                    bImg.source: "qrc:/res/img/warning_icon.svg"
                }
                CusButton{
                    width: 80
                    height: 50
                    checkable: true
                    bText.text: qsTr("Btn")
                    state: "imgLeft"
                    bImg.source: "qrc:/res/img/warning_icon.svg"
                }
                CusButton{
                    width: 50
                    height: 50
                    checkable: true
                    bText.text: qsTr("Btn")
                    state: "imgTop"
                    bImg.source: "qrc:/res/img/warning_icon.svg"
                }
                CusButton{
                    width: 80
                    height: 50
                    checkable: true
                    bText.text: qsTr("Btn")
                    state: "imgRight"
                    bImg.source: "qrc:/res/img/warning_icon.svg"
                }
                CusButton{
                    width: 50
                    height: 50
                    checkable: true
                    bText.text: qsTr("Btn")
                    state: "imgBottom"
                    bImg.source: "qrc:/res/img/warning_icon.svg"
                }
                CXButton{
                    width: 150
                    height: 50
                  //  borderRadius: 5
                }
            }
        }
    }
}
