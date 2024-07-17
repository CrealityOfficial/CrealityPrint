import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"
import QtQuick.Layouts 1.1
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
        CusText{
            text: qsTr("test")
        }

        Rectangle{
            width: parent.width
            height: 150
            color:"lightBlue"
            anchors.margins: 20
            Row{
                height: parent.height
                anchors.left: parent.left
                Rectangle{
                    id: author_image
                    width: 80
                    height: 80
                    radius: height/2
                    anchors.verticalCenter: parent.verticalCenter
                    Image {
                        anchors.fill: parent
                        source: "qrc:/res/img/warning_icon.svg"
                    }

                }
                Column{
                    anchors.left: author_image.right
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 5
                    Text {
                        text: qsTr("text1")
                    }
                    Text {
                        text: qsTr("text2")
                    }
                }

            }
            Row{
                height: parent.height
                anchors.right: parent.right
                anchors.rightMargin: 10
                ListModel{
                    id:info_count
                    ListElement{ count:58;label:"关注" }
                    ListElement{ count:220;label:"粉丝" }
                    ListElement{ count:130000;label:"获赞" }
                    ListElement{ count:1.5;label:"收藏";noline:true }
                }

                Repeater{
                    model: info_count
                    delegate: Row{
                        height: parent.height
                        Column{
                            id: follow_num
                            anchors.verticalCenter: parent.verticalCenter
                            width: 100
                            spacing: 8
                            Text {
                                text: count
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                            Text {
                                text: label
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                        Rectangle{
                            id:line
                            width: 1
                            height: 20
                            color: "#55555B"
                            visible: !noline
                            anchors.verticalCenter: parent.verticalCenter

                        }

                    }

                }

            }


        }

    }
}
