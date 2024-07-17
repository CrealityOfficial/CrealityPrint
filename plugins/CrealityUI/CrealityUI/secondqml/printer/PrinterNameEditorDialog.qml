import QtQuick 2.10
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.12

import "../../qml"
import "../../secondqml"

BasicDialogV4
{
    id: root
    //color: "transparent"
    width: 420 * screenScaleFactor
    height: 202 * screenScaleFactor
    // flags: Qt.FramelessWindowHint | Qt.Dialog
    maxBtnVis: false
    title: qsTr("Edit Plate Name")
    titleIcon: "qrc:/scence3d/res/logo.png"
    property var printer: {}
    property string text: ""

    bdContentItem: Rectangle {
        anchors.fill: parent
        color: Constants.themeColor_primary
        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 40
            anchors.rightMargin: 40
            anchors.topMargin: 38
            anchors.bottomMargin: 80
            color: Qt.rgba(0, 0, 0, 0)

            StyledLabel {
                id: idPrinterNameTitle
                text: qsTr("Plate name") + ":"
                width: 80 * screenScaleFactor
                height: 28 * screenScaleFactor
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLCenter
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_9
            }

            BasicDialogTextField {
                id: idPrinterNameEdit
                anchors.left: idPrinterNameTitle.right
                anchors.leftMargin: 9
                anchors.right: parent.right
                height: 28 * screenScaleFactor
                baseValidator:RegExpValidator {}
                // placeholderText: qsTr("Please input text")
                font.pointSize: Constants.labelFontPointSize_9
                maximumLength: 256
                text: printer ? printer.name : ""

                onTextChanged: {
                    root.text = text
                }

                Connections {
                    target: root
   
                    onVisibleChanged: {
                        if (visible) {
                            idPrinterNameEdit.forceActiveFocus()
                        }
                    }
                }

            }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30 * screenScaleFactor
        width: 170 * screenScaleFactor
        height: 28 * screenScaleFactor
        color: Qt.rgba(0, 0, 0, 0)

        BasicButton {
            id: idOkBtn
            anchors.left: parent.left
            width: 80* screenScaleFactor
            height: 28* screenScaleFactor
            text : qsTr("OK")
            btnRadius:14
            btnBorderW:1
            pointSize: 9
            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.lpw_BtnColor
            hoveredBtnBgColor: Constants.lpw_BtnHoverColor
            visible: true

            onSigButtonClicked: {
                printer.name = root.text
                root.visible = false
            }
        }

        BasicButton {
            id: idCancelBtn
            anchors.left: idOkBtn.right
            anchors.leftMargin: 10
            width: 80* screenScaleFactor
            height: 28* screenScaleFactor
            text : qsTr("Cancel")
            btnRadius:14
            btnBorderW:1
            pointSize: 9
            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.lpw_BtnColor
            hoveredBtnBgColor: Constants.lpw_BtnHoverColor
            visible: true

            onSigButtonClicked: {
                root.visible = false
            }
        }
    }
}
