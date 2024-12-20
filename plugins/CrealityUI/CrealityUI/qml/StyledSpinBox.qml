import QtQuick 2.15
import QtQuick.Controls 2.15

SpinBox {
    id: control
    editable: true

    property int radius: 0
    property int decimals: 2

    property var unitchar: "mm"
    property var realValue: 0

    property real factor: Math.pow(10, decimals)
    property real realTo: 100.0
    property real realFrom: 0.0
    property real orgValue: 0
    property real realStepSize: 1.0

    property alias textObj: numbTxt
    property alias unitColor: idUnitchar.color

    signal valueEdited
    signal textPressed
    signal contentChanged
    //height: 30
    implicitHeight: 20
    value: realValue*factor
    stepSize: realStepSize*factor
    to : realTo*factor
    from : realFrom*factor
    function setRealValue(v)
    {
        realValue = v
        value = realValue*factor
    }

    contentItem: TextInput {//by TCJ
        id: numbTxt
        text: control.textFromValue(control.value, control.locale)
        anchors.left: control.left
        anchors.leftMargin: 10
        anchors.right: idUnitchar.right
        anchors.rightMargin: idUnitchar.width
        anchors.top: control.top
        anchors.topMargin: 0
        anchors.bottom: control.bottom
        anchors.bottomMargin: 0

        width: control.width - idUp.width
        height: control.height
        font: control.font
        color: Constants.textColor
        selectionColor: "#21be2b"
        selectedTextColor: color
        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter

        validator: DoubleValidator {
            bottom: control.from * control.factor
            top: control.to * control.factor
            notation: DoubleValidator.StandardNotation
            decimals: control.decimals
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onClicked: {
                numbTxt.selectAll()
                numbTxt.forceActiveFocus()
                textPressed()
            }

            onWheel: {
                var datl = wheel.angleDelta.y/120;
                if(datl>0){
                    control.value+=stepSize
                }else{
                    control.value-=stepSize
                }
                orgValue = realValue
                realValue = control.value/factor
                valueEdited()
            }

            onExited: {
                if(numbTxt.text === "") numbTxt.text = 5
                if(numbTxt.text >= realFrom)
                {
                    let tempValue = valueFromText(numbTxt.text,control.locale)
                    if(realValue > control.realTo){
                        numbTxt.text = control.textFromValue(control.to, control.locale)
                    }else{
                        numbTxt.text = control.textFromValue(tempValue, control.locale)
                    }
                    realValue = numbTxt.text
                }
            }
        }

        onEditingFinished: {
            if (text === "") {
                return
            }

            if (text >= realFrom) {
                orgValue = realValue
                valueFromText(text,control.locale)
                valueEdited()
            }
        }

        onTextChanged: {
            if (text.includes("+")) {
                text = text.replace("+", "")
            }
            contentChanged()
        }
    }

    StyledLabel
    {
        id: idUnitchar
        width: contentWidth
        visible: unitchar.length > 0 ? true : false

        anchors.right: control.right
        anchors.rightMargin: 30 * screenScaleFactor
        anchors.verticalCenter: control.verticalCenter

        text: unitchar
        color: "#999999"
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: Constants.labelFontPointSize_9
    }
    up.indicator: Rectangle {
        id:idUp

        x: control.mirrored ? 0 : parent.width - width - 1 * screenScaleFactor
        y: 1 * screenScaleFactor
        z: 2

        implicitWidth: 25 * screenScaleFactor
        implicitHeight: parent.height / 2 - 1 * screenScaleFactor
        color: "transparent"

        enabled: control.enabled

        Rectangle {
            anchors.centerIn: parent
            width: 16 * screenScaleFactor
            height: 10 * screenScaleFactor
            radius: 3 * screenScaleFactor
            color: "transparent"
            // color: control.up.pressed || control.up.hovered ? Constants.cmbIndicatorRectColor_pressed_basic
            //                                                 : Constants.cmbIndicatorRectColor_basic
            Image {
                anchors.centerIn: parent
                width: 7 * screenScaleFactor
                height: 5 * screenScaleFactor
                source: control.up.pressed || control.up.hovered ? Constants.upBtnImgSource_d
                                                                 : Constants.upBtnImgSource
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    down.indicator: Rectangle {
        x: control.mirrored ? 0 : parent.width - width - 1 * screenScaleFactor
        y: parent.height / 2
        z: 2

        implicitWidth: 25 * screenScaleFactor
        implicitHeight: parent.height / 2 - 1 * screenScaleFactor
        color: "transparent"

        enabled: control.enabled

        Rectangle {
            anchors.centerIn: parent
            width: 16 * screenScaleFactor
            height: 10 * screenScaleFactor
            radius: 3 * screenScaleFactor
            color: "transparent"
            Image {
                anchors.centerIn: parent
                width: 7 * screenScaleFactor
                height: 5 * screenScaleFactor
                source: control.down.pressed || control.down.hovered ? Constants.downBtnImgSource_d
                                                                     : Constants.downBtnImgSource
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    background: Rectangle {//by TCJ
        implicitWidth: control.width
        border.color: control.hovered ? Constants.textRectBgHoveredColor: Constants.dialogItemRectBgBorderColor
        color: Constants.dialogItemRectBgColor
        radius: control.radius
    }
    onValueModified: {
        orgValue = realValue
        realValue = control.value/factor
        valueEdited()
    }

    textFromValue: function(value, locale) {
        return parseFloat(value*1.0/factor).toFixed(decimals);
    }
    valueFromText: function(text, locale) {
        var locale_lang = Qt.locale("zh_CN");
        realValue = Number.fromLocaleString(locale_lang, text)
        return realValue*factor;
    }
}
