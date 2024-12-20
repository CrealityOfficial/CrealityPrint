import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Item {
    id: root
    height: 30
    property string title: ""
    property var value: 1000
    property var from: 100
    property var to: 5000
    property var unit: ''
    property var stepSize: 1
    property alias sliderPressed: idSlider.pressed

    CusText {
        id: idTitle
        anchors.left: parent.left
        anchors.right: idSlider.left
        height: parent.height
        fontText: title
        fontColor: Constants.textColor
        hAlign: 0
        fontPointSize: Constants.labelFontPointSize_9
    }

    onValueChanged: {
        let realValue = (root.value / 1).toFixed(2)
        if (realValue != idSpinBox.tempValue) {
            idSpinBox.realValue = realValue
            idSpinBox.tempValue = realValue
        }
    }

    Slider {
        id: idSlider
        height: parent.height
        anchors.right: idSpinBox.left
        // anchors.left: idTitle.right
        anchors.margins: 10
        width: 130 * screenScaleFactor

        from: root.from
        to: root.to
        value: root.value
        focusPolicy:Qt.NoFocus
        background: Rectangle {
            x: idSlider.leftPadding
            y: idSlider.topPadding + idSlider.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 4
            width: idSlider.availableWidth
            height: implicitHeight
            radius: 2
            color:idSlider.enabled? Constants.lpw_BtnBorderColor : Constants.currentTheme? "#f3f3f4" :"#3b7252"

            Rectangle {
                width: idSlider.visualPosition * parent.width
                height: parent.height
                color: (idSlider.enabled ? Qt.rgba(0.09, 0.80, 0.37, 1.0) : Constants.currentTheme? "#f3f3f4" :"#3b7252")
                radius: 2
            }
        }

        handle: Rectangle {
            x: idSlider.leftPadding + idSlider.visualPosition * (idSlider.availableWidth - width)
            y: idSlider.topPadding + idSlider.availableHeight / 2 - height / 2
            implicitWidth: 18
            implicitHeight: 18
            radius: 9

            color: (idSlider.enabled ? Qt.rgba(0.09, 0.80, 0.37) :Constants.currentTheme? "#b9f0cf" :"#3b7252")
            border.color: Constants.currentTheme? "#b2ffd0" :"#37644b" //(idSlider.enabled ? Qt.rgba(0.49, 0.91, 0.93) : Qt.rgba(0.35, 0.48, 0.49))
            border.width: 2
        }

        onValueChanged: {
            if (root.value != value) {
                root.value = value
        }
    }
    }
    CusStyledSpinBox {
        id: idSpinBox
        anchors.right: parent.right
        width: 60 * screenScaleFactor
        height: parent.height

        stepSize: root.stepSize * Math.pow(10, decimals)
        from: root.from * Math.pow(10, decimals)
        to: root.to * Math.pow(10, decimals)
        decimals: 2
        unitchar: root.unit
        font.pointSize: Constants.labelFontPointSize_9
        textValidator: RegExpValidator {
            regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
        }

        property var tempValue

        isBindingValue : false
        onTextContentChanged: {
            if (root.value != result) {
                root.value = result
                tempValue = result
            }
        }

        onTextEditing: {
            if (root.value != result) {
                tempValue = result
                root.value = result
            }
        }

        Component.onCompleted: {
            tempValue = realValue
        }
    }

}
