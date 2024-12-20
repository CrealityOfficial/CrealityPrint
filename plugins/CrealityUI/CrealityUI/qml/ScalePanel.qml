import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.13
import "../secondqml"

LeftPanelDialog {
    id:control
    width: 300* screenScaleFactor
    height: 240* screenScaleFactor
    title: qsTr("Scale")
    property var msale
    property var size:msale ? msale.orgSize : ""
    property bool lockCheck: msale ? msale.uniformCheck : true
    enum XYZType
    {
        X_Type =0,
        Y_Type,
        Z_Type
    }

    //    visible: msale.message

    enum XYZScaleType
    {
        X_Type =0,
        Y_Type,
        Z_Type,
        XYZ_Type
    }
    enum XYZSizeType
    {
        X_Size =0,
        Y_Size,
        Z_Size,
        XYZ_X_Size,
        XYZ_Y_Size,
        XYZ_Z_Size
    }

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }
    /*BasicDialogV4 {
        id: warningDialog
        width: 500* screenScaleFactor
        height: 200* screenScaleFactor
        title: qsTr("Warning")
        maxBtnVis: false
        bdContentItem:Rectangle {
            color: Constants.lpw_bgColor
            ColumnLayout{
                anchors.centerIn: parent
                spacing: 30* screenScaleFactor
                StyledLabel{
                    color: Constants.textColor
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.preferredWidth: 450 * screenScaleFactor
                    text: qsTr("You are unlocking the uniform ratio, which means the model will not return to its original size. Do you want to proceed?")
                }

                RowLayout{
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20* screenScaleFactor
                    BasicDialogButton {
                        text: qsTr("OK")
                        Layout.minimumWidth: 120 * screenScaleFactor
                        Layout.fillHeight: true
                        btnRadius: height / 2
                        btnBorderW: 1 * screenScaleFactor
                        btnTextColor: Constants.manager_printer_button_text_color
                        borderColor: Constants.manager_printer_button_border_color
                        defaultBtnBgColor: Constants.manager_printer_button_default_color
                        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                        selectedBtnBgColor: Constants.manager_printer_button_checked_color

                        onSigButtonClicked: {
                            if(warningDialog.visible)
                                warningDialog.close()
                        }
                    }
                    BasicDialogButton {
                        text: qsTr("Cancel")
                        Layout.minimumWidth: 120 * screenScaleFactor
                        Layout.fillHeight: true
                        btnRadius: height / 2
                        btnBorderW: 1 * screenScaleFactor
                        btnTextColor: Constants.manager_printer_button_text_color
                        borderColor: Constants.manager_printer_button_border_color
                        defaultBtnBgColor: Constants.manager_printer_button_default_color
                        hoveredBtnBgColor: Constants.manager_printer_button_checked_color
                        selectedBtnBgColor: Constants.manager_printer_button_checked_color

                        onSigButtonClicked: {
                            warningDialog.close()
                            cusContent.cusCheckBox.checked = true
                        }
                    }
                }
            }
        }
    }*/

    Item {
        property alias x_scale: x_scale
        property alias y_scale: y_scale
        property alias z_scale: z_scale
        property alias x_length: x_length
        property alias y_length: y_length
        property alias z_length: z_length
        property alias cusCheckBox: uniformScalingCheckbox
        property real maxEnlarge: 1000//允许缩放的最大倍数
        anchors.fill: control.panelArea
        id: cusContent
        Column{
            anchors.top: parent.top
            anchors.topMargin: 20* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10* screenScaleFactor

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10* screenScaleFactor
                StyledLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 15* screenScaleFactor
                    leftPadding: 0/*10*/
                    rightPadding: 10//
                    color: "#FB0202"
                    text: "X:"
                    font.pointSize: Constants.labelFontPointSize_9
                }
                LeftToolSpinBox {
                    id:x_length
                    anchors.verticalCenter: parent.verticalCenter
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    realStepSize: 5
                    // realFrom:(size.x/10000) < 0.01 ? 0.01 : size.x/10000
                    realFrom:(size) ? 0.001 : size.x * 0.00107
                    realTo:(size) ? 9999 : size.x * cusContent.maxEnlarge
                    realValue: msale ? msale.size.x.toFixed(2) : 1
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    property var tmpVal: msale ? msale.size.x.toFixed(2) : 1
                    onValueEdited:{
                        console.log("====scale.x_length.value = " + realValue)
                        if(!msale || realValue < realFrom)return ;
                        if(lockCheck)
                        {
                            msale.setQmlSize(realValue,ScalePanel.XYZSizeType.XYZ_X_Size) //xyz_scale
                        }
                        else
                        {
                            msale.setQmlSize(realValue,ScalePanel.XYZSizeType.X_Size) //x_scale
                        }
                        //                        realValue = Qt.binding(function(){return msale.size.x.toFixed(2) });

                    }

                    onRealValueChanged: {
                        kernel_model_selector.sizeChanged()
                        console.log("111111111111111")
                    }

                    onTmpValChanged:
                    {
                        console.log("tmpVal xxx=",tmpVal)
                        realValue = tmpVal
                    }
                }
                LeftToolSpinBox {
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    id:x_scale
                    realStepSize:0.5
                    realFrom:0.1
                    realTo:cusContent.maxEnlarge * 100
                    realValue: tmpVal>realTo?realTo:tmpVal // msale ? msale.scale.x.toFixed(2) * 100 : 100
                    unitchar : "%"
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    property var tmpVal: msale ?( msale.scale.x * 100).toFixed(2) : 100
                    onValueEdited:{
                        if(!msale || realValue < 1)return ;
                        console.log("===scale.x_scale.value = " + realValue)

                        if(lockCheck)
                        {
                            msale.setQmlScale(realValue / 100,ScalePanel.XYZScaleType.XYZ_Type) //xyz_scale
                        }
                        else
                        {
                            msale.setQmlScale(realValue / 100,ScalePanel.XYZScaleType.X_Type) //x_scale
                        }
                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                    onRealValueChanged: {
                        kernel_model_selector.sizeChanged()
                    }
                }
            }
            Row {
                spacing: 10* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 15* screenScaleFactor
                    leftPadding: 0/*10*/
                    rightPadding: 10//
                    color: "#00FD00"
                    text: "Y:"
                    font.pointSize: Constants.labelFontPointSize_9
                }
                LeftToolSpinBox {
                    id:y_length
                    anchors.verticalCenter: parent.verticalCenter
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    realStepSize: 5
                    // realFrom:(size.y/10000) < 0.01 ? 0.01 : size.y/10000
                    realFrom:(size) ? 0.001 : size.y * 0.00107
                    realTo:(size) ? 9999 : size.y*cusContent.maxEnlarge
                    realValue: msale ? msale.size.y.toFixed(2) : 1
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    property var tmpVal: msale ? msale.size.y.toFixed(2) : 1
                    onValueEdited:{
                        console.log("====scale.y_length.value = " + realValue)
                        if(!msale || realValue < realFrom)return ;
                        if(lockCheck)
                        {
                            msale.setQmlSize(realValue,ScalePanel.XYZSizeType.XYZ_Y_Size) //xyz_scale
                        }
                        else
                        {
                            msale.setQmlSize(realValue,ScalePanel.XYZSizeType.Y_Size) //x_scale
                        }
                        realValue = Qt.binding(function(){return msale.size.y.toFixed(2) });

                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }
                LeftToolSpinBox {
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    id:y_scale
                    realStepSize:0.5
                    realFrom:0.1
                    realTo:cusContent.maxEnlarge*100
                    realValue: tmpVal>realTo?realTo:tmpVal  // msale ? msale.scale.y * 100: 100
                    unitchar : "%"
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    property var tmpVal: msale ? (msale.scale.y * 100).toFixed(2): 100
                    onValueEdited:{
                        if(!msale || realValue < 1)return ;
                        if(lockCheck)
                        {
                            msale.setQmlScale(realValue / 100,ScalePanel.XYZScaleType.XYZ_Type) //xyz_scale
                        }
                        else
                        {
                            msale.setQmlScale(realValue / 100,ScalePanel.XYZScaleType.Y_Type) //y_scale
                        }
                        //                        realValue = Qt.binding(function(){return (msale.scale.y * 100).toFixed(2)});

                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }
            }
            Row {
                spacing: 10* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    width: 15* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    leftPadding: 0/*10*/
                    rightPadding: 10//
                    color: "#008FFD"
                    text: "Z:"
                    font.pointSize: Constants.labelFontPointSize_9
                }
                LeftToolSpinBox {
                    id:z_length
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    realStepSize: 5
                    // realFrom:(size.z/10000) < 0.01 ? 0.01 : size.z/10000
                    realFrom:(size) ? 0.001 : size.z * 0.00107
                    realTo:(size) ? 9999 : size.z*cusContent.maxEnlarge
                    realValue: msale ? msale.size.z.toFixed(2) : 1
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    property var tmpVal: msale ? msale.size.z.toFixed(2) : 1
                    onValueEdited:{
                        console.log("z_length")
                        if(!msale || realValue < realFrom)return ;
                        console.log("====scale.z_length.value = " + realValue)
                        if(lockCheck)
                        {
                            msale.setQmlSize(realValue,ScalePanel.XYZSizeType.XYZ_Z_Size) //xyz_scale
                        }
                        else
                        {
                            msale.setQmlSize(realValue,ScalePanel.XYZSizeType.Z_Size) //x_scale
                        }
                        //                        realValue = Qt.binding(function(){return msale.size.z.toFixed(2) });

                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }
                LeftToolSpinBox {
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    id:z_scale
                    realStepSize:0.5
                    realFrom:0.1
                    realTo:cusContent.maxEnlarge*100
                    realValue: tmpVal>realTo?realTo:tmpVal//msale ? msale.scale.z * 100 : 100
                    unitchar : "%"
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    property var tmpVal: msale ? (msale.scale.z * 100).toFixed(2) : 100
                    onValueEdited:{
                        if(!msale || realValue < 1)return ;
                        console.log("===scale.z_scale.value = " + realValue)
                        if(lockCheck)
                        {
                            msale.setQmlScale(realValue / 100,ScalePanel.XYZScaleType.XYZ_Type) //xyz_scale
                        }
                        else
                        {
                            msale.setQmlScale(realValue / 100,ScalePanel.XYZScaleType.Z_Type) //z_scale
                        }
                        //                        realValue = Qt.binding(function(){return (msale.scale.z * 100).toFixed(2)});

                    }
                    onTmpValChanged:
                    {
                        realValue = tmpVal
                    }
                }
            }
            StyleCheckBox {
                id: uniformScalingCheckbox
                checked: lockCheck
                text: qsTr("Uniform")

                onCheckedChanged:
                {
                    if(msale)
                        msale.uniformCheck = checked
                }

                onClicked: {
                    //if(!checked)
                    //    warningDialog.visible = true
                }
            }

            BasicButton{
                width: 258* screenScaleFactor
                height: 30* screenScaleFactor
                text : qsTr("Reset")
                btnRadius:15* screenScaleFactor
                btnBorderW:1
                anchors.horizontalCenter: parent.horizontalCenter
                borderColor: Constants.lpw_BtnBorderColor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                    msale.reset()
                }
            }
        }
    }
}
