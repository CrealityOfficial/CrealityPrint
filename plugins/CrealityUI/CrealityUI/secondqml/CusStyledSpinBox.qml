import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import "../qml"

SpinBox {
    property string unitchar: ""
    property color bgColor: "transparent"
    property color borderColor: Constants.rectBorderColor
    property color itemHighlightColor:Constants.cmbPopupColor_pressed//"#42BDD8"
    property color itemNormalColor: Constants.cmbPopupColor//"#E3EBEE"
    //    property color indicatorColor: "#383C3E"     //勾选的指示器的颜色
    property color itemBorderColor: Constants.dialogItemRectBgBorderColor
    property color itemBorderColor_H: Constants.textRectBgHoveredColor  //"#1e9be2"
    property int decimals: 2
    property real factor: Math.pow(10, decimals)
    property alias textValidator: numbTxt.validator
    property real realValue: 0
    property real realFrom: from /factor
    property real realTo: to /factor
    property var overMax: false
    property bool isBindingValue: true
    property bool needTextEditing: true
    signal textContentChanged(real result)
    signal textEditing(real result)

    value: 0
    id: control
    implicitWidth: 80* screenScaleFactor
    implicitHeight: 28* screenScaleFactor

    to: 99999999
    leftPadding: 5
    rightPadding: 16
    topPadding: 1
    bottomPadding: 1

    editable: true
    opacity: enabled? 1 : 0.7
    hoverEnabled: true
    valueFromText: function(text, locale) {
        var local_lang = Qt.locale("zh_CN");
        let realValue = Number.fromLocaleString(local_lang, text)
        return realValue;
    }

    textFromValue: function(value, locale) {
        return parseFloat(value).toFixed(decimals);
    }

    onRealValueChanged: {
        value = (realValue * factor).toFixed(0)
    }

    background: Rectangle{
        radius: 5
        color: bgColor
        border.width: 1
        // border.color:overMax ? "red" : control.hovered ? "#009cff" : borderColor
        border.color: overMax ? "red" : control.hovered ? itemBorderColor_H : borderColor
        // border.color: "red"
    }

    contentItem: Item{
        id: cntItem
        property real unitcharWidth: 10
        RowLayout{
            anchors.fill: parent
            TextInput {
                id:numbTxt
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: (control.value/factor).toFixed(decimals)
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                horizontalAlignment: TextInput.AlignLeft
                verticalAlignment: TextInput.AlignVCenter
                color: Constants.textColor
                validator: control.textValidator

                // validator: RegExpValidator {
                //     regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                // }

                onTextChanged: {
                    if(text > control.realTo || text < control.realFrom)
                    {
                        overMax = true
                    }
                    else
                    {
                        overMax = false
                    }
                    idUp.enabled = +text < control.realTo
                    idDown.enabled =  +text > control.realFrom
                }

                function format(txt) {
                    let value = 0
                    if(!isBindingValue)
                    {
                        if(control.valueFromText(txt, control.locale) < control.from / factor)
                        {
                            value = control.textFromValue((control.from).toFixed(decimals), control.locale)
                        }
                        else if(control.valueFromText(txt, control.locale) > control.to / factor)
                        {
                            value = control.textFromValue((control.to).toFixed(decimals), control.locale)
                        }
                        else{
                            value = control.valueFromText(txt, control.locale)*factor.toFixed(decimals)
                        }
                    }
                    else 
                    {
                        if(control.valueFromText(txt, control.locale) < control.from / factor)
                        {
                            value = (control.from / factor.toFixed(decimals))
                        }
                        else if(control.valueFromText(txt, control.locale) > control.to / factor)
                        {
                            value = (control.to / factor.toFixed(decimals))
                        }
                        else
                        {
                            console.log("***control.value***=",control.value)
                            value = ((control.valueFromText(txt, control.locale)).toFixed(decimals))
                        }
                    }
                    return value

                }

                onTextEdited: {
                    if(!needTextEditing)return
                    let value;
                    if (text == "")
                        value = 0
                    else
                        value = format(text)

                    if(!isBindingValue)
                    {
                        // control.value = value
                        // numbTxt.text = Qt.binding(function() { return (control.value/factor).toFixed(decimals) })
                        control.textEditing((value/factor).toFixed(decimals))
                    }
                    else
                    {
                        control.textEditing(value)
                        //两次输入相同值也能刷新显示一次 text
                        // numbTxt.text = Qt.binding(function() { return (control.value/factor).toFixed(decimals) })
                    }
                }

                onEditingFinished: {
                    let value;
                    if (text == "")
                        value = 0
                    else
                        value = format(text)
                        
                    if(!isBindingValue)
                    {
                        control.value = value
                        numbTxt.text = Qt.binding(function() { return (control.value/factor).toFixed(decimals) })
                        control.textContentChanged((control.value/factor).toFixed(decimals))
                    }
                    else 
                    {
                        control.textContentChanged(value)
                        //两次输入相同值也能刷新显示一次 text
                        numbTxt.text = Qt.binding(function() { return (control.value/factor).toFixed(decimals) })
                    }
                }

                MouseArea{
                    anchors.fill: parent
                    onClicked: {
                        numbTxt.selectAll()
                        numbTxt.forceActiveFocus()
                    }
//                    onDoubleClicked:
//                    {
//                        numbTxt.selectAll()
//                        numbTxt.forceActiveFocus()
//                    }
                    onWheel: {
                        var datl = wheel.angleDelta.y/120;
                        if(datl>0){
                            control.increase()
                        }else{
                            control.decrease()
                        }
                        control.textContentChanged((control.value/factor).toFixed(decimals))
                    }
                }
                onActiveFocusChanged:
                {
                    if(!activeFocus)
                    {
                        if(text === "")
                        {
                            numbTxt.text = Qt.binding(function() { return (control.value/factor).toFixed(decimals) })
                        }
                    }
                }
            }
            Text{
                Layout.preferredWidth: 10
                Layout.rightMargin: 8
                text:unitchar
                visible: unitchar !== ""
                font.pointSize: Constants.labelFontPointSize_9
                color: Constants.textColor
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    up.indicator: Item {
        id:idUp
        x: control.mirrored ? 0 : parent.width - width -1
        y:1
        height: parent.height/2 -1
        implicitWidth: 15
        implicitHeight: parent.height/2 - 1
        z:2
        Image {
            width: sourceSize.width
            height: sourceSize.height
            source: control.up.pressed || control.up.hovered ? Constants.upBtnImgSource_d:Constants.upBtnImgSource
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        enabled: control.enabled
        opacity: control.opacity
    }

    down.indicator: Item {
        id:idDown
        x: control.mirrored ? 0:parent.width - width -1
        y:parent.height/2
        height: parent.height/2 -1
        implicitWidth: 15
        implicitHeight: parent.height/2 - 1
        z:2
        Image {
            width: sourceSize.width
            height: sourceSize.height
            source: control.down.pressed || control.down.hovered ? Constants.downBtnImgSource_d : Constants.downBtnImgSource
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.centerIn: parent
        }
        enabled:control.enabled
        opacity: control.opacity
    }

//    validator: DoubleValidator {
//        notation:DoubleValidator.StandardNotation
//        bottom: Math.min(control.from, control.to)
//        top:  Math.max(control.from, control.to)
//    }
    validator: IntValidator {
          locale: control.locale.name
          bottom: Math.min(control.from, control.to)
          top: Math.max(control.from, control.to)
      }

    onValueModified: {
        control.textContentChanged((control.value/factor).toFixed(decimals))
    }
}

