import QtQuick 2.12
import QtQuick.Controls 2.12
import "../"
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
    property real orgValue:1
    property alias  textObj: numbTxt
    property var wheelEnable: true
    property string bgColor: "#4B4B4B"
    property string borderColor: "#6e6e72"
    property string borderHoverColor: "#009CFF"
    //lisugui 2020-10-14 control signal block
    property bool bSignalsBlcked: false

    property string upBtnImgSource: "qrc:/res/img/upBtn.svg"
    property string upBtnImgSource_d: "qrc:/res/img/upBtn_d.svg"
    property string downBtnImgSource: "qrc:/res/img/downBtn.svg"
    property string downBtnImgSource_d: "qrc:/res/img/downBtn_d.svg"


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
        validator: RegExpValidator {
            regExp:   /(\d{1,4})([.,]\d{1,2})?$/
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
            if(text == "NaN")
                text =control.textFromValue(control.realFrom, control.locale)
        }

        onAccepted:
        {
            console.log("******onAccepted******")
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

    Text
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
            width: sourceSize.width
            height: sourceSize.height
            source: control.up.pressed || control.up.hovered? upBtnImgSource_d:upBtnImgSource
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
            width: sourceSize.width
            height: sourceSize.height
            source: control.down.pressed || control.down.hovered ? downBtnImgSource_d : downBtnImgSource
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.centerIn: parent
        }
    }

    background: Rectangle {
        implicitWidth: control.width
        border.color: control.hovered ? borderHoverColor : borderColor
        color: bgColor//"#4B4B4B"
        radius: 5
    }
    onValueModified: {
        focus = true
        orgValue = realValue
        realValue = control.value/factor
        valueEdited()
    }
    validator: DoubleValidator {
        notation:DoubleValidator.StandardNotation
        bottom: Math.min(control.from, control.to)
        top:  Math.max(control.from, control.to)
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
