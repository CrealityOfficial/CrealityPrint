import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import "../components"
import "../qml"


DockItem {
    id: simpDialog
    ColumnLayout{
        spacing: 5 * screenScaleFactor
        anchors.fill: parent
        anchors.margins: 10
        anchors.topMargin: titleHeight + 10
        RowLayout{
            spacing: 10
            Layout.fillWidth: true
            CusText{
                hAlign: 0
                fontColor: Constants.textColor
                fontWidth: 80 * screenScaleFactor
                fontText: "Mesh Name" + ":"
            }

            CusText{
                hAlign: 0
                fontColor: Constants.textColor
                fontWidth: 60 * screenScaleFactor
                fontText: "text.stl"
            }
        }

        RowLayout{
            spacing: 10
            Layout.fillWidth: true
            CusText{
                hAlign: 0
                fontColor: Constants.textColor
                fontWidth: 80 * screenScaleFactor
                fontText: "Triangle" + ":"
            }

            CusText{
                hAlign: 0
                fontColor: Constants.textColor
                fontWidth: 60 * screenScaleFactor
                fontText: "2000"
            }
        }

        Rectangle{
            Layout.fillWidth: true
            Layout.preferredHeight: 1 * screenScaleFactor
            color: Constants.right_panel_border_default_color
        }

        RowLayout{
            spacing: 10
            Layout.fillWidth: true
            CustomRadioButton{
                id: detailEnable
                Layout.preferredWidth: 20 * screenScaleFactor
                Layout.preferredHeight: 20 * screenScaleFactor
                ButtonGroup.group: btnGroup
            }

            CusText{
                hAlign: 0
                fontColor: Constants.textColor
                enabled: detailEnable.checked
                fontWidth: 130 * screenScaleFactor
                fontText: "Level Of Detail"
            }

            CXComboBox{
                id: detailLevel
                enabled: detailEnable.checked
                Layout.fillWidth: true
                Layout.preferredHeight: 20 * screenScaleFactor
                model:["1", "2"]
                currentIndex: 0
            }
        }

        ButtonGroup{
            id: btnGroup
        }

        RowLayout{
            spacing: 10
            Layout.fillWidth: true
            CustomRadioButton{
                id: simplificationRateEnable
                Layout.preferredWidth: 20 * screenScaleFactor
                Layout.preferredHeight: 20 * screenScaleFactor
                ButtonGroup.group: btnGroup
            }

            CusText{
                hAlign: 0
                fontColor: Constants.textColor
                enabled: simplificationRateEnable.checked
                fontWidth: 130 * screenScaleFactor
                fontText: "Simplification rate"
            }

            StyledSlider{
                id: triangleNum
                realFrom: 0
                realTo: 100
                popTipVisible: false
                enabled: simplificationRateEnable.checked
                Layout.fillWidth: true
            }

            BasicDialogTextField{
                id: tNumField
                enabled: simplificationRateEnable.checked
                Layout.preferredWidth: 45 * screenScaleFactor
                Layout.preferredHeight: 20 * screenScaleFactor
                text: triangleNum.value
                radius: 5
                baseValidator: RegExpValidator { regExp: /^(0|[1-9][0-9]{0,2})$/ }
                unitChar: "%"

                onEditingFinished: {
                    triangleNum.value = Number(tNumField.text)
                }
            }
        }

        CusCheckBox{
            Layout.preferredWidth: 200 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor
            text: qsTr("Show Wireframe")
            textColor: Constants.right_panel_item_text_default_color
            font.pointSize: Constants.labelFontPointSize_10
        }

        RowLayout{
            Layout.fillWidth: true
            Item{
                Layout.fillWidth: true
            }

            BasicButton{
                id: confirmBtn
                Layout.preferredWidth: 50 * screenScaleFactor
                Layout.preferredHeight: 27 * screenScaleFactor
                text: qsTr("Confirm")
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {

                }
            }

            BasicButton{
                id: cancelBtn
                Layout.preferredWidth: 50 * screenScaleFactor
                Layout.preferredHeight: 27 * screenScaleFactor
                text: qsTr("Cancel")
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    simpDialog.close()
                }
            }
        }
    }

}
