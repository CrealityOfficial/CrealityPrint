import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import ".."
import "../qml"

ListView {
    readonly property int totalCount: kernel_modelspace.modelNNum
    property real leftRecWidth: 160 * screenScaleFactor

    id: root

    clip: true
    spacing: 2 * screenScaleFactor
    model: kernel_plates_list_data

    ScrollBar.vertical: ScrollBar {
        policy: contentHeight > height ?  ScrollBar.AlwaysOn :ScrollBar.AlwaysOff
    }

    delegate: ItemDelegate {
        id: itemDel

        width: root.width
        padding: 0

        background: Rectangle {
            color: "transparent"
        }

        contentItem: ColumnLayout {
            spacing: 0

            Rectangle{
                property bool isHovered: false
                property bool isExpend: model.plateInfo.isExpend
                property bool isChecked: kernel_plates_list_data.currentPlateIndex === model.index

                id: delRec

                color: /*isChecked ? (Constants.currentTheme ? "#B7E5FF" : "#739AB0") :*/ "transparent"

                Layout.preferredWidth: parent.width - 2
                Layout.preferredHeight: 20 * screenScaleFactor

                StackLayout{
                    id: textStack
                    width: root.leftRecWidth - 25 * screenScaleFactor
                    height: parent.height
                    anchors.left: parent.left
                    anchors.leftMargin: 25 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    CusText{
                        hAlign: 0
                        fontText: {
                            if(model.plateIndex === -1){
                                qsTranslate("ModelListPop", "Outside Plate")
                            }else{
                                qsTranslate("ModelListPop", "Plate")  + " " + (model.plateIndex + 1) +
                                        (model.plateObj.name ? ("(" +  model.plateObj.name + ")") : "")
                            }
                        }
                        fontElide: Text.ElideRight
                        fontWidth: leftRecWidth - 25 * screenScaleFactor
                        fontPointSize: Constants.labelFontPointSize_9
                        fontColor: Constants.themeTextColor
                        MouseArea{
                            anchors.fill: parent
                            onDoubleClicked: {
                                plateEdit.focus = true
                                textStack.currentIndex = 1
                            }
                        }
                    }

                    BasicDialogTextField {
                        id: plateEdit
                        color: Constants.themeTextColor
                        font.pointSize: Constants.labelFontPointSize_9
                        text: model.plateObj.name
                        validator: RegExpValidator{}
                        onEditingFinished: {
                            console.log("onEditingFinished")
                            textStack.currentIndex = 0
                            model.plateObj.name = text
                        }
                        onActiveFocusChanged: {
                            console.log("index = ", model.index, "activeFocus = ", activeFocus)
                            if(!activeFocus){
                                textStack.currentIndex = 0
                            }
                        }
                    }
                }

                MouseArea{
                    anchors.fill: parent
                    Item{
                        anchors.left: parent.left
                        anchors.leftMargin: 8 *screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10*screenScaleFactor
                        height: 10*screenScaleFactor
                        Image {
                            property string upImgSource: delRec.isHovered ? Constants.upBtnImgSource_d:Constants.upBtnImgSource
                            property string downImgSource: delRec.isHovered ?  Constants.downBtnImgSource_d : Constants.downBtnImgSource
                            id: idImg
                            sourceSize: Qt.size(7 * screenScaleFactor, 4 * screenScaleFactor)
                            anchors.centerIn: parent
                            source: model.plateInfo.isExpend ? downImgSource : upImgSource
                            MouseArea{
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    model.plateInfo.isExpend = !model.plateInfo.isExpend
                                }
                                onEntered: {
                                    delRec.isHovered = true
                                }
                                onExited: {
                                    delRec.isHovered = false
                                }
                            }
                        }
                    }



                    onClicked: {
                        kernel_plates_list_data.currentPlateIndex = model.index
                    }
                }
            }

            CusListView {
                id: itemList
                leftRecWidth: root.leftRecWidth
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOff
                }

                visible: delRec.isExpend
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                model: plateModels
            }
        }
    }
}
