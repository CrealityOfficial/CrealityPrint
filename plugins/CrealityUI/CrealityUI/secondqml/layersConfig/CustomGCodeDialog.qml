import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "./"
import "../"
import "../../qml"
import "../../components"

DockItem{
    id: idRoot
    title: qsTr("Custom G-code")
    width: 500 * screenScaleFactor
    height: 240 * screenScaleFactor
    visible: false

    property var gcode: ""

    function init(_gcode) {
        gcode = _gcode
        gcodeEdit.text = _gcode
    }

    signal ok()
    signal cancel()

    function clear() {
        gcodeEdit.text = ""
    }

    Column{
        y:50* screenScaleFactor
        width: parent.width - 40* screenScaleFactor
        height: parent.height-20* screenScaleFactor
        spacing:10* screenScaleFactor
        anchors.horizontalCenter: parent.horizontalCenter
        //   anchors.verticalCenter: parent.verticalCenter
        Text {
            y:60* screenScaleFactor
            id:tipText
            text: qsTr("Enter Custom G-code used on current layer") + ":"
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_10
            font.family: Constants.labelFontFamilyBold
        }

        BasicDialogTextArea{
            id:gcodeEdit
            width: parent.width
            height: 109*screenScaleFactor
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            text: ""

        }

        Grid
        {
            width : basicComButton.width + basicComButton2.width + spacing
            height: 30*screenScaleFactor
            columns: 2
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            Image {
                id: name
                source: "file"
                Rectangle{

                }
            }
            BasicDialogButton {
                id: basicComButton
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("OK")
                btnRadius:15*screenScaleFactor
                enabled: gcodeEdit.text
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                    idRoot.gcode = gcodeEdit.text
                    idRoot.visible = false
                    ok()
                }
            }

            BasicDialogButton {
                id: basicComButton2
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Cancel")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    idRoot.visible = false
                    idRoot.cancel()
                }

            }
        }

    }


}
