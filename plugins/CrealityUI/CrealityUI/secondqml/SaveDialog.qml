import QtQuick 2.5
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15


import "./"
import "../qml"
import "../components"

DockItem {
    property alias cText: proField.text
    property alias clabel: contentLabel

    id: root
    width: 600 * screenScaleFactor
    height: 206 * screenScaleFactor
    signal okBtnClicked()
    signal cancelBtnClicked()


    Item{
        anchors.fill: parent

        Column{
            spacing: 20 * screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 60 * screenScaleFactor

            StyledLabel {
                id: contentLabel
                font.weight: Font.Normal
                font.pointSize: Constants.labelFontPointSize_10
                color: Constants.right_panel_text_default_color
            }

            BasicDialogTextField {
                id: proField
                width: 518 * screenScaleFactor
                height: 28 * screenScaleFactor
                radius: 5 * screenScaleFactor
                wrapMode: TextInput.WordWrap
                validator: RegExpValidator{}
                text: ""
                onAccepted: {
                }
            }
        }

        Row{
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20 * screenScaleFactor
            spacing: 10 * screenScaleFactor

            BasicButton {
                width: 80 * screenScaleFactor
                height: 28 * screenScaleFactor

                text: qsTranslate("ExpertParamPanel", "OK")
                btnRadius: 14 * screenScaleFactor
                btnBorderW: 1 * screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: function() {
                    okBtnClicked()
                }
            }

            BasicButton {
                width: 80 * screenScaleFactor
                height: 28 * screenScaleFactor

                text: qsTranslate("ExpertParamPanel", "Cancel")
                btnRadius: 14 * screenScaleFactor
                btnBorderW: 1 * screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: function() {
                    cancelBtnClicked()
                }
            }
        }
    }
}
