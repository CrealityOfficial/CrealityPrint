import QtQuick 2.0
import QtQuick.Controls 2.5
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

Column {
    property var oldX
    property var oldY
    property var oldWidth: 0
    property var oldHeight: 0
    property var oldRotation
    property var selShape: null
    property var com

    spacing: 10 * screenScaleFactor

    Rectangle {
        width: parent.width
        height: 5 * screenScaleFactor
        color: "transparent"
    }

    Row {
        spacing: 5 * screenScaleFactor

        StyledLabel {
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            text: qsTr("Move")
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        StyledLabel {
            width: 10 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            text: "X"
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
        }

        CusStyledSpinBox {
            id: idMoveX

            width: 138 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            stepSize: 1 * Math.pow(10, decimals)
            from: 0 * Math.pow(10, decimals)
            to: (selShape ? (gGlScene.width - selShape.textWidth) : (gGlScene.width - 100)) * Math.pow(10, decimals)
            unitchar: ("mm")
            realValue: {
                if (selShape)
                    return (selShape.x / 1).toFixed(2);
                else
                    return 0;
            }
            onTextContentChanged: {
                if (selShape) {
                    oldX = selShape.x;
                    selShape.x = result;
                }
            }

            textValidator: RegExpValidator {
                regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
            }

        }

    }

    Row {
        spacing: 5 * screenScaleFactor

        StyledLabel {
            width: 100 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            text: ""
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        StyledLabel {
            width: 10 * screenScaleFactor
            height: 28 * screenScaleFactor
            text: "Y"
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
        }

        CusStyledSpinBox {
            id: idMoveY

            width: 138 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            stepSize: 1 * Math.pow(10, decimals)
            from: 0 * Math.pow(10, decimals)
            to: (selShape ? (gGlScene.height - selShape.textHeight) : (gGlScene.height - 100)) * Math.pow(10, decimals)
            unitchar: ("mm")
            realValue: {
                if (selShape)
                    return (selShape.y / 1).toFixed(2);
                else
                    return 0;
            }
            onTextContentChanged: {
                if (selShape) {
                    oldX = selShape.xy;
                    selShape.y = result;
                }
            }

            textValidator: RegExpValidator {
                regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
            }

        }

    }

    Row {
        spacing: 5 * screenScaleFactor
        visible: false

        StyledLabel {
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor
            text: qsTr("Size")
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        StyledLabel {
            width: 10 * screenScaleFactor
            height: 28 * screenScaleFactor
            text: "X"
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
        }

        CusStyledSpinBox {
            id: idScaleX

            width: 138 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            stepSize: 1 * Math.pow(10, decimals)
            from: 0 * Math.pow(10, decimals)
            to: 9999 * Math.pow(10, decimals)
            unitchar: ("mm")
            realValue: {
                if (selShape)
                    return (selShape.width).toFixed(2);
                else
                    return 0;
            }
            onTextContentChanged: {
                if (selShape) {
                    oldWidth = selShape.width;
                    selShape.width = (Number(result).toFixed(2)); //*3
                }
            }

            textValidator: RegExpValidator {
                regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
            }

        }

    }

    Row {
        spacing: 5 * screenScaleFactor
        visible: false

        StyledLabel {
            width: 100 * screenScaleFactor
            text: ""
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        StyledLabel {
            width: 10 * screenScaleFactor
            height: 28 * screenScaleFactor
            text: "Y"
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
        }

        CusStyledSpinBox {
            id: idScaleY

            width: 138 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            stepSize: 1 * Math.pow(10, decimals)
            from: 0 * Math.pow(10, decimals)
            to: 9999 * Math.pow(10, decimals)
            unitchar: ("mm")
            realValue: {
                if (selShape) {
                    return (selShape.height).toFixed(2);
                } else {
                    return 0;
                }
            }
            onTextContentChanged: {
                if (selShape) {
                    oldHeight = selShape.height;
                    selShape.height = (Number(result).toFixed(2)); //*3
                }
            }

            textValidator: RegExpValidator {
                regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
            }

        }

    }

    Row {
        spacing: 5 * screenScaleFactor

        StyledLabel {
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor
            text: qsTr("Rotate")
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        StyledLabel {
            width: 10 * screenScaleFactor
            text: ""
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
        }

        CusStyledSpinBox {
            id: idRotate

            width: 138 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            stepSize: 1 * Math.pow(10, decimals)
            from: -360 * Math.pow(10, decimals)
            to: 360 * Math.pow(10, decimals)
            unitchar: "(°)"
            realValue: {
                if (selShape)
                    return selShape.rotation.toFixed(2);
                else
                    return 0;
            }
            onTextContentChanged: {
                if (selShape) {
                    oldRotation = selShape.rotation.toFixed(2);
                    selShape.rotation = Number(result).toFixed(2);
                }
            }

            textValidator: RegExpValidator {
                regExp: /[+-]?(\d|[1-9]\d|[1-2]\d{2}|3[0-5]\d|360)(\.\d{0,2})?/
            }

        }

    }

    Row {
        spacing: 5 * screenScaleFactor

        StyledLabel {
            height: 28 * screenScaleFactor
            width: 100 * screenScaleFactor
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_9
            text: qsTr("Thickness")
            leftPadding: 10 * screenScaleFactor
            rightPadding: 10 * screenScaleFactor
            font.family: Constants.labelFontFamily
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        StyledLabel {
            width: 10 * screenScaleFactor
            text: ""
            font.pointSize: Constants.labelFontPointSize_9
            color: Constants.textColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignRight
        }

        CusStyledSpinBox {
            id: z_pos

            width: 138 * screenScaleFactor
            height: 28 * screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_9
            stepSize: 1 * Math.pow(10, decimals)
            from: 0 * Math.pow(10, decimals)
            to: 100 * Math.pow(10, decimals)
            decimals: 2
            realValue: com ? com.text_thickness * factor : 1
            unitchar: ("mm")
            isBindingValue: false
            value: com ? com.text_thickness * factor : 1
            onTextContentChanged: {
                if (com)
                    com.text_thickness = result;

            }

            textValidator: RegExpValidator {
                regExp: /[\+ \-]?(\d{1,4})([.,]\d{1,2})?$/ ///^([\+ \-]?(\d{1,4})([.,]\d{1,3})?$/
            }

        }

    }

    Row {
        id: idTextSideRow

        spacing: 5 * screenScaleFactor

        // visible: Qt.platform.os === "windows"
        StyledLabel {
            height: 28 * screenScaleFactor
            width: 100 * screenScaleFactor
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_9
            text: qsTr("Letter Mode")
            leftPadding: 10 * screenScaleFactor
            rightPadding: 10 * screenScaleFactor
            font.family: Constants.labelFontFamily
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter
        }

        ButtonGroup {
            id: idTextSideGroup
        }

        BasicRadioButton {
            id: idTextInside

            text: qsTr("Inside")
            font.pointSize: Constants.labelFontPointSize_9
            ButtonGroup.group: idTextSideGroup
            checked: com ? com.text_side : true // == -1 ? true:false
            onClicked: {
                com.text_side = true;
            }
        }

        BasicRadioButton {
            id: idTextOutside

            text: qsTr("Outside")
            font.pointSize: Constants.labelFontPointSize_9
            ButtonGroup.group: idTextSideGroup
            checked: com ? !com.text_side : false //== 1 ? true:false
            onClicked: {
                com.text_side = false;
            }
        }

    }

}
