import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"
import "../components"
Rectangle{

    id: root
    color: "transparent"
    width: 100
    height: parent.height

    property string uploadGcodeList:"qrc:/UI/photo/uploadGcodeList.svg"

    ColumnLayout{
        CusImglButton {
            Layout.preferredWidth:  80 * screenScaleFactor
            Layout.preferredHeight: 36 * screenScaleFactor
            opacity: enabled ? 1 : 0.5
            btnRadius:height/2
            imgWidth: 24 * screenScaleFactor
            imgHeight: 24 * screenScaleFactor
            borderWidth: 1

            state: "imgOnly"
            cusTooltip.position: BasicTooltip.Position.RIGHT

            enabledIconSource:uploadGcodeList
            pressedIconSource: uploadGcodeList
            highLightIconSource: uploadGcodeList

            defaultBtnBgColor: Constants.leftBtnBgColor_normal
            hoveredBtnBgColor: Constants.leftBtnBgColor_normal
            selectedBtnBgColor:Constants.leftBtnBgColor_normal

            btnTextColor: Constants.leftTextColor
            btnTextColor_d : Constants.leftTextColor_d
        }
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 10 * screenScaleFactor
        }

        Rectangle{
            property var contentHeight: root.height - 200* screenScaleFactor
            width:idContent.height > contentHeight ? 90 * screenScaleFactor : 80 * screenScaleFactor
            height: idContent.height > contentHeight?contentHeight:idContent.height
            color: Constants.leftBarBgColor
            ScrollView{
                anchors.fill: parent
                clip: true
                ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                ColumnLayout{
                    spacing: 2
                    id:idContent
                    Repeater{
                        model: ["PLA", "ABS", "PETG","TPU", "PA-CF","PET-CF","PLA", "ABS", "PA-CF","PET-CF","PLA", "ABS"]
                        delegate: Rectangle{
                            width: 80* screenScaleFactor
                            height: 80* screenScaleFactor
                            radius: 5* screenScaleFactor
                            border.width: 1
                            border.color: isHover? Constants.leftBtnBgColor_normal : Constants.right_panel_border_default_color
                            color: "transparent"
                            property bool isHover: false
                            MouseArea{
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onEntered: parent.isHover = true
                                onExited: parent.isHover = false
                            }

                        }


                    }

                }


            }
        }


    }


}
