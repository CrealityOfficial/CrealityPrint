import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import "../qml"
Column {
    signal familyChanged(var text)
    //signal fontStyleChanged(var text)
    //signal fontTextChanged(var text)
    property alias fontText: idTextArea.text
    property alias fontFamily: idFontFamily.currentText
    property alias fontSize: idFontSize.text
    property var fontStyle //:idFontStyles.currentText
    width: 300* screenScaleFactor
    height: 130* screenScaleFactor
    spacing:5* screenScaleFactor
    Rectangle{
        width: parent.width
        height: 5* screenScaleFactor
        color: "transparent"
    }
    Row{
        spacing:25* screenScaleFactor
        StyledLabel {
            text: qsTr("Text")
            font.pointSize: Constants.labelFontPointSize_10
            width: 90* screenScaleFactor
            height: 28* screenScaleFactor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        BasicDialogTextField {
            id:idTextArea
            width: 120* screenScaleFactor
            height: 28* screenScaleFactor
            baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
            placeholderText: qsTr("Please input text")
            text: "Creality"
            onTextChanged: {
                fontTextChanged(text)
            }
        }
    }
    Row{
        spacing:25
        StyledLabel {
            text: qsTr("Font Family")
            font.pointSize: Constants.labelFontPointSize_10
            width: 90* screenScaleFactor
            height: 28* screenScaleFactor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        BasicCombobox {
            id:idFontFamily
            width: 120* screenScaleFactor
            height: 28* screenScaleFactor
            model: control.getFontFamilys()
            onCurrentTextChanged:  {
                //idFontStyles.model = control.getFontFamilysStyles(currentText);
                //familyChanged(text)
            }
        }
    }

    Row{
        spacing:25* screenScaleFactor
        StyledLabel {
            text: qsTr("Font Size")
            font.pointSize: Constants.labelFontPointSize_10
            width: 90* screenScaleFactor
            height: 28* screenScaleFactor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        BasicDialogTextField {
            id:idFontSize
            width: 120* screenScaleFactor
            height: 28* screenScaleFactor
            unitChar:"pt"
            text: "30"
        }
    }

    // Row{
    //     spacing:25
    //     StyledLabel {
    //         text: qsTr("Font Style")
    //         font.pixelSize: 14
    // 		width: 90
    //         height: 28
    // 		verticalAlignment: Qt.AlignVCenter
    // 		horizontalAlignment: Qt.AlignLeft
    //     }
    //     BasicCombobox {
    //         id:idFontStyles
    //         width: 120
    //         height: 28
    //         onCurrentTextChanged: {
    //             //fontStyleChanged(currentText)
    //         }
    //     }
    // }
}
