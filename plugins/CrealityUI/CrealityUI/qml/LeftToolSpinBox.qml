import QtQuick 2.12
import QtQuick.Controls 2.12

SpinBox {
    id: control
    editable: true

    property var unitchar: "mm"

    property int decimals: 2
    property real factor: Math.pow(10, decimals)
    property var realValue: 1
    property real realStepSize: 1.0
    property real realFrom: 0.0
    property real realTo: 100.0
    property var orgValue:1
    property alias  textObj: numbTxt
    property var wheelEnable: true

    property bool bResetOtgValue: false
    //lisugui 2020-10-14 control signal block
    property bool bSignalsBlcked: false


    signal valueEdited
    signal textPressed
    //height: 30
    font.pointSize: 9
    implicitWidth: 180
    implicitHeight: 28
    value: realValue*factor
    stepSize: realStepSize*factor
    to : realTo*factor
    from : realFrom*factor
    function setRealValue(v)
    {
        console.log("v=" + parseFloat(v).toFixed(2))
        realValue = parseFloat(v).toFixed(2)
    }
    property bool bZeroInputEanbled: true
    property alias textValidator: numbTxt.validator
    contentItem: TextInput {
        id:numbTxt
        text: control.textFromValue(realValue, control.locale)
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
        color: Constants.textColor//"white"/*"#21be2b"*/
        selectionColor: "#21be2b"
        selectedTextColor: "#ffffff"
        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter
        focus:  activeFocus
        validator: DoubleValidator {
            bottom: control.from / control.factor
            top: control.to / control.factor
            notation: DoubleValidator.StandardNotation
            decimals: control.decimals
        }
        MouseArea{
            enabled: wheelEnable
            anchors.fill: parent
            onClicked: {
                numbTxt.selectAll()
                numbTxt.forceActiveFocus()
                textPressed()
            }
            onWheel: {

                var datl = wheel.angleDelta.y/120;
                if(datl>0){
                    control.increase()
                }else{
                    control.decrease()
                }
                orgValue = realValue
                realValue = control.value/factor
                valueEdited()
            }
        }

        onTextChanged: {
            if (text.includes("+")) {
                text = text.replace("+", "")
            }
            if(text == "NaN")
            {
                if(bResetOtgValue)
                {
                    text =control.textFromValue(orgValue, control.locale)
                }
                else
                {
                    text =control.textFromValue(control.realFrom, control.locale)
                }

            }
        }
        onAccepted:
        {
            console.log("******onAccepted******")
            if(text==="" && bResetOtgValue)
            {
                text = orgValue
            }
            if(text <realFrom)realValue =control.textFromValue(control.realFrom, control.locale)
            else if(text > realTo) realValue =control.textFromValue(control.realTo, control.locale)
            var locale_lang = Qt.locale("zh_CN");
            realValue = Number.fromLocaleString(locale_lang, text)// control.value/factor
            orgValue = realValue
            text = Qt.binding(function(){ return control.textFromValue(realValue, control.locale)})
            valueEdited()
        }
        onActiveFocusChanged:
        {
            if(!activeFocus)
            {
                if(text==="" && bResetOtgValue)
                {
                    text = orgValue
                }
                if(text <realFrom)realValue =control.textFromValue(control.realFrom, control.locale)
                else if(text > realTo) realValue =control.textFromValue(control.realTo, control.locale)
                if(orgValue == realValue)return
                else
                {
                    var locale_lang = Qt.locale("zh_CN");
                    realValue = Number.fromLocaleString(locale_lang, text)// control.value/factor
                    orgValue = realValue
                    text = Qt.binding(function(){ return control.textFromValue(realValue, control.locale)})
                    valueEdited()
                }
            }
        }
    }

    StyledLabel
    {
        id: idUnitchar
        text: unitchar
        width: contentWidth
        color: "#999999"
        visible: unitchar.length > 0 ? true : false
        anchors.bottom: control.bottom
        anchors.bottomMargin: 8//4
        anchors.right: idUp.left
        anchors.rightMargin: 4
        font.pointSize: 9
    }
    up.indicator: Rectangle {
        id:idUp
        x: control.mirrored ? 0 : parent.width - width - 1
        y:1
        implicitWidth: control.height -2
        implicitHeight: parent.height/2 - 1
        color: "transparent"
        z:2
        Image {
            width: sourceSize.width* screenScaleFactor
            height: sourceSize.height* screenScaleFactor
            source: control.up.pressed || control.up.hovered? Constants.upBtnImgSource_d:Constants.upBtnImgSource
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    down.indicator: Rectangle {
        x: control.mirrored ? 0:parent.width - width - 1
        y:parent.height/2
        implicitWidth: control.height -2 //16
        implicitHeight: parent.height/2 - 1
        color: "transparent"
        z:2
        Image {
            width: sourceSize.width* screenScaleFactor
            height: sourceSize.height* screenScaleFactor
            source: control.down.pressed || control.down.hovered ? Constants.downBtnImgSource_d : Constants.downBtnImgSource
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.centerIn: parent
        }
    }

    background: Rectangle {
        implicitWidth: control.width
        border.color: control.hovered ? Constants.textRectBgHoveredColor : Constants.dialogItemRectBgBorderColor
        color: Constants.dialogItemRectBgColor//"#4B4B4B"
        radius: 5
    }
    onValueModified: {
        focus = true
        orgValue = realValue
        realValue = control.value/factor
        valueEdited()
    }

    textFromValue: function(value, locale) {
        return parseFloat(value).toFixed(decimals);
    }
    valueFromText: function(text, locale) {
        var locale_lang = Qt.locale("zh_CN");
        realValue = Number.fromLocaleString(locale_lang, text)
        return realValue*factor;
    }
}
