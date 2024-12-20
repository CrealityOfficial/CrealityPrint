import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"
Column {
    signal familyChanged(var text)
    //signal fontStyleChanged(var text)
    //signal fontTextChanged(var text)
    property alias fontText: idTextArea.text
    property alias fontFamily: idFontFamily.currentText
    property alias fontSize: idFontSize.realValue

    spacing:10

    Rectangle{
        width: parent.width
        height: 5 * screenScaleFactor
        color: "transparent"
    }

    Row{
        spacing: 10 * screenScaleFactor
        StyledLabel {
            text: qsTr("Text")
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_9
        }
        BasicDialogTextField {
            id:idTextArea
            width: 145 * screenScaleFactor
            height: 28 * screenScaleFactor
            baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
            placeholderText: qsTr("Please input text")
            font.pointSize: Constants.labelFontPointSize_9
            text: gTextPara.textName
            onTextChanged: {
                if (text === gTextPara.textName)
                    return;
                fontTextChanged(text)
                gTextPara.textName = text
            }
        }
    }
    Row{
        spacing: 10 * screenScaleFactor
        StyledLabel {
            text: qsTr("Font Family")
            font.pointSize: Constants.labelFontPointSize_9
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
            color: Constants.textColor
        }
        CXComboBox
        {
            id: idFontFamily
            width: 145 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            model: /*com ? com.font_list :  */Qt.fontFamilies()
            currentIndex:0
            showCount : 7

            function setFontFamily(familyName) {
                gTextPara.familyName = familyName
                if(currentText.toLowerCase().indexOf("normal") !== -1)
                {
                    curSharp.fontfamily = Constants.labelFontFamily
                }
            }

            onCurrentTextChanged: {
                setFontFamily(currentText)
            }
            
            Component.onCompleted:
            {
                setFontFamily(currentText)
//                currentIndex = count >  253 ? 253 : 0
                currentIndex = indexOfValue(qsTr("Arial"))
            }
        }
    }

    Row{
        spacing: 10 * screenScaleFactor
        StyledLabel {
            text: qsTr("Font Size")
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_9
        }
        CusStyledSpinBox {
            id: idFontSize
            width: 145 * screenScaleFactor
            height: 28  * screenScaleFactor
            stepSize:1 * Math.pow(10, decimals)
            from:20 * Math.pow(10, decimals)
            to:100 * Math.pow(10, decimals)
            decimals: 0
            realValue: gTextPara.textSize
            unitchar: ("pt")
            font.pointSize: Constants.labelFontPointSize_9
            textValidator: RegExpValidator {
                regExp:   /(\d{1,4})?$/  ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
            }
            isBindingValue : false
            onTextContentChanged: {
                gTextPara.textSize = result
            }
        }
    }
}
