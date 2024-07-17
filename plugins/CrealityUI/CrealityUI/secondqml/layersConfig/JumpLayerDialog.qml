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
    title: qsTr("Jump To Layer")
    width: 400 * screenScaleFactor
    height: 170 * screenScaleFactor
    visible: false

    property var from: 1
    property var to: 1
    property var layer: 1

    signal ok()
    signal cancel()

    function init(_from, _to, _layer) {
        from = _from
        to = _to
        idEdit.text = _layer
        idEdit.lastValue = parseInt(_layer)
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
            text: qsTr("Enter layer number") + "(" + idRoot.from + "-" + idRoot.to + ")"
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_10
            font.family: Constants.labelFontFamilyBold
        }

        BasicDialogTextField{
            id: idEdit
            width: parent.width
            height: 28*screenScaleFactor
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            text: ""
            property string lastValue
            validator: RegExpValidator { regExp: /^[0-9]*$/ }

            onTextChanged: {
                if (text == "") {
                    lastValue = ""
                    return;
                }

                let currentValue = parseInt(text)
                if (currentValue < idRoot.from || currentValue > idRoot.to) {
                    text = lastValue
                } else {
                    lastValue = currentValue
                }
            }
        }

        Grid
        {
            width : basicComButton.width + basicComButton2.width + spacing
            height: 30*screenScaleFactor
            columns: 2
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter

            BasicDialogButton {
                id: basicComButton
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("OK")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                enabled: idEdit.text

                onSigButtonClicked:
                {
                    idRoot.layer = parseInt(idEdit.text)
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
