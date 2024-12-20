import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0 as QQC2
import CrealityUI 1.0
import "qrc:/CrealityUI"
import ".."
import "../qml"

Button{
    id: idDeviceItemBtn
    width: 417 - 2
    height: 30

    property var isGcodeItem: false

    property var itemName: "CR-6 SE"
    property var seriesId: ""
    property int allCount: 8
    property var sliceType: "---"
    property int deviceCount: 1

    property var gcodeId: ""
    property var gcodeLink: ""

    property alias itemChecked: idCheckBox.checked

    signal sigRedefineFileSliceType(var id, var name)

    MouseArea
    {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: isGcodeItem ? 97 : 0
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        hoverEnabled:true
        onClicked:
        {
            if(isGcodeItem === true && sliceType === "---" )
            {
                idGcodeItemUnknownBtnClk.msgText = qsTr("The slice type is unknown and cannot be checked. Redefine slice type?")
                idGcodeItemUnknownBtnClk.visible = true
                return
            }
            idCheckBox.checked = !idCheckBox.checked
        }
    }

    contentItem: Item{
        width: parent.width
        height:parent.height
        Row{
            width: parent.width
            height: parent.height
            Rectangle{
                width: 18
                height: parent.height
                color: "transparent"
            }
            StyleCheckBox{
                id: idCheckBox
                width: 26
                height: parent.height
                checked: false
                isGroupPrintUsed: true
            }
            StyledLabel{
                width: 130
                height: parent.height
                font.pointSize: Contants.labelFontPointSize_12
                color: "#333333"
                text: itemName
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment:Text.AlignHCenter
            }
            StyledLabel{
                width: 130
                height: parent.height
                font.pointSize: Contants.labelFontPointSize_12
                color: "#333333"
                text: isGcodeItem ? sliceType : allCount
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment:Text.AlignHCenter
            }
            Rectangle{
                width: 97
                height: parent.height
                color: "transparent"
                Row{
                    anchors{
                        horizontalCenter: parent.horizontalCenter
                    }
                    width: (isGcodeItem & idCheckBox.checked) ? (13 + 13 + idCountLabel.width + 20) : idCountLabel.width
                    height: parent.height
                    spacing: 10
                    CusSkinButton_Image{
                        anchors{
                            verticalCenter: parent.verticalCenter
                        }
                        width: 13
                        height: 13
                        visible: isGcodeItem & idCheckBox.checked
                        btnImgNormal: "qrc:/UI/photo/group_print_task_create_img_count_reduce.png"
                        btnImgHovered: "qrc:/UI/photo/group_print_task_create_img_count_reduce.png"
                        btnImgPressed: "qrc:/UI/photo/group_print_task_create_img_count_reduce.png"
                        onClicked:
                        {
                            if(deviceCount == 1)
                            {
                                return
                            }
                            else{
                                deviceCount--
                            }
                        }
                    }
                    StyledLabel{
                        id: idCountLabel
                        width: idCountLabel.contentWidth
                        height: parent.height
                        font.pointSize: Contants.labelFontPointSize_12
                        color: "#333333"
                        text: deviceCount
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment:Text.AlignHCenter
                    }
                    CusSkinButton_Image{
                        anchors{
                            verticalCenter: parent.verticalCenter
                        }
                        width: 13
                        height: 13
                        visible: isGcodeItem & idCheckBox.checked
                        btnImgNormal: "qrc:/UI/photo/group_print_task_create_img_count_add.png"
                        btnImgHovered: "qrc:/UI/photo/group_print_task_create_img_count_add.png"
                        btnImgPressed: "qrc:/UI/photo/group_print_task_create_img_count_add.png"
                        onClicked:
                        {
                            deviceCount++
                        }
                    }
                }
            }
        }
    }

    background: Rectangle{
        width: parent.width
        height: parent.height
        color: idDeviceItemBtn.hovered ? "#C9EBFF" : (idCheckBox.checked ? "#ECECEC" : "#FFFFFF")
    }

    UploadMessageDlg{
        id:idGcodeItemUnknownBtnClk
        visible: false
        onSigOkButtonClicked:
        {
            idGcodeItemUnknownBtnClk.close()
            sigRedefineFileSliceType(gcodeId, itemName)
        }
    }
}
