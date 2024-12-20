import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import "../qml"
Item {
    signal vectorSelected()
    signal blackSelected()
    signal graySelected()
    signal ditheringImage()
    signal thresholdChanged(var value)
    signal originalImageShow(var value)
    signal reverseImage(var value)
    signal contrastValueChanged(var value)
    signal brightnessValueChanged(var value)
    signal whiteValueChanged(var value)
    signal flipModelValueChanged(var value)
    signal m4modeChanged(var value)
   // sinnal 
    width: 262
    //height: parent.height
    implicitHeight:60
    //anchors.leftMargin: 23
	
	function initState()
	{
        idBlackWhiteBtn.sigBtnClicked()
	}

    Column{
        width: parent.width
        height: parent.height
        Rectangle{
            width: parent.width
            height: 10
            color: "transparent"
        }
        Row{
            width: parent.width
            height: 60
            spacing: 9
            LaserImageBtn{
                id: idBlackWhiteBtn
                btnText: qsTr("Black")
                imgSrc: "qrc:/UI/photo/laser_image_black.png"
                onSigBtnClicked:{
                    idBlackWhiteBtn.btnSelected = true
                    idLineGrayBtn.btnSelected = false
                    idJitterGrayBtn.btnSelected = false
                    idVectorBtn.btnSelected = false
                    
					idM4modeValue.visible = false
                    idFlipModelValue.visible = true
                    idBlackWhiteValue.visible = true
                    idContrastValue.visible = false
                    idPackingDensityValue.visible = false
                    idBrightnessValue.visible = false
                    idWhiteValue.visible = false
                    idDensityValue.visible = true

                    blackSelected()
                }
            }
            LaserImageBtn{
                id: idLineGrayBtn
                btnText: qsTr("Line Gray")
                imgSrc: "qrc:/UI/photo/laser_image_line_gray.png"
                onSigBtnClicked:{
                    idBlackWhiteBtn.btnSelected = false
                    idLineGrayBtn.btnSelected = true
                    idJitterGrayBtn.btnSelected = false
                    idVectorBtn.btnSelected = false

					idM4modeValue.visible = false
                    idFlipModelValue.visible = true
                    idBlackWhiteValue.visible = false
                    idContrastValue.visible = true
                    idPackingDensityValue.visible = false
                    idBrightnessValue.visible = true
                    idWhiteValue.visible = true
                    idDensityValue.visible = true

                    graySelected()
                }
            }
            LaserImageBtn{
                id: idJitterGrayBtn
                btnText: qsTr("Jitter Gray")
                imgSrc: "qrc:/UI/photo/laser_image_Jitter_gray.png"
                onSigBtnClicked:{
                    idBlackWhiteBtn.btnSelected = false
                    idLineGrayBtn.btnSelected = false
                    idJitterGrayBtn.btnSelected = true
                    idVectorBtn.btnSelected = false

					idM4modeValue.visible = false
                    idFlipModelValue.visible = true
                    idBlackWhiteValue.visible = true
                    idContrastValue.visible = false
                    idPackingDensityValue.visible = false
                    idBrightnessValue.visible = false
                    idWhiteValue.visible = false
                    idDensityValue.visible = true

                    ditheringImage()
                }
            }
            LaserImageBtn{
                id: idVectorBtn
                btnText: qsTr("Vector")
                imgSrc: "qrc:/UI/photo/laser_image_vector.png"
                onSigBtnClicked:{
                    idBlackWhiteBtn.btnSelected = false
                    idLineGrayBtn.btnSelected = false
                    idJitterGrayBtn.btnSelected = false
                    idVectorBtn.btnSelected = true

					idM4modeValue.visible = false
                    idFlipModelValue.visible = true
                    idBlackWhiteValue.visible = true
                    idContrastValue.visible = false
                    idPackingDensityValue.visible = false
                    idBrightnessValue.visible = false
                    idWhiteValue.visible = false
                    idDensityValue.visible = false

                    vectorSelected()
                }
            }
        }
        Rectangle{
            width: parent.width
            height: 20
            color: "transparent"
        }
        Column{
            width: parent.width
            height: 60
            spacing: 10
            StyleCheckBox {
                width: 100
                height: 16
                text: qsTr("Original Image Show")
                checked: selShape ? selShape.originalShow : true
                // fontSize: Constants.labelFontPointSize_12
                onCheckedChanged:
                {
                    if(!selShape)
                        return;
                    selShape.originalShow = !checked
                    originalImageShow(checked)
                }
            }
            StyleCheckBox {
                width: 48
                height: 16
                text: qsTr("Reverse")
                checked: selShape ? selShape.reverse : true
                // fontSize: Constants.labelFontPointSize_12
                onCheckedChanged:
                {
                    if(!selShape)
                        return;
                    selShape.reverse = !checked
                    reverseImage(checked)
                }
            }

            StyleCheckBox {
			    id:idM4modeValue
                width: 48
                height: 16
                fontSize: 12
                text: "M4"
                checked: selShape ? selShape.m4mode : true

                onCheckedChanged:
                {
                    if(!selShape)
                        return;
                    selShape.m4mode = !checked
                    m4modeChanged(checked)
                }
            }
        }
        Rectangle{
            width: parent.width
            height: 3
            color: "transparent"
        }
        Column{
            width: parent.width
            height:  200 + 20
            spacing: 2
            Row{
                id: idFlipModelValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("Flip Model")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Qt.AlignVCenter
                }
                BasicCombobox{
                    width: 120
                    height: 28
                    font.pointSize: Constants.labelFontPointSize_10
                    currentIndex: selShape ? selShape.filpModelValue : 0

                    model: ListModel {
                        id: modelType
                        ListElement {key:"NoFlip"; modelData: qsTr("No Flip");}
                        ListElement {key:"Vertical"; modelData: qsTr("Vertical");}
                        ListElement {key:"Horizontal"; modelData: qsTr("Horizontal");}
                        ListElement {key:"Simultaneously"; modelData: qsTr("Simultaneously");}
                    }
                     onCurrentIndexChanged: {
                         if(!selShape)
                             return;
                         selShape.filpModelValue = currentIndex
                         flipModelValueChanged(currentIndex)
                     }
                }
            }
            Row{
                id: idBlackWhiteValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("Threshold")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Qt.AlignVCenter
                }
                StyledSlider
                {
                    anchors{
                        ///horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    id : control
                    value: selShape ? selShape.threshold : 0
                    width: 74
                    maximumValue:255
                    minimumValue:1
                    sliderHeight: 2
                    height: 25
                    onValueChanged:
                    { 
                        if(!selShape)
                            return;
                        selShape.threshold = value
                        thresholdChanged(value)
                    }
                }
                BasicDialogTextField {
                    width: 41
                    height: 28
                    horizontalAlignment: Text.AlignVCenter
                    text: control.value
                    onTextChanged: {
                        if(!selShape)
                            return;
                        if(Number(text) > 255)
                        {
                            selShape.threshold = 255
                        }
                        else if(Number(text) < 1)
                        {
                            selShape.threshold = 1
                        }
                        else
                        {
                            selShape.threshold = Number(text)
                        }
                    }
                }
            }
            Row{
                id: idContrastValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("Contrast")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Qt.AlignVCenter
                }
                StyledSlider
                {
                    anchors{
                        ///horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    id : control1
                    value: selShape ? selShape.contrastValue : 0
                    width: 74
                    maximumValue:100
                    minimumValue:1
                    sliderHeight: 2
                    height: 25
                    onValueChanged:
                    { 
                        if(!selShape)
                            return;
                        selShape.contrastValue = value
                        contrastValueChanged(value)
                    }
                }
                BasicDialogTextField {
                    width: 41
                    height: 28
                    horizontalAlignment: Text.AlignVCenter
                    text: control1.value
                    onTextChanged: {
                        if(!selShape)
                            return;
                        if(Number(text) > 100)
                        {
                            selShape.contrastValue = 100
                        }
                        else if(Number(text) < 1)
                        {
                            selShape.contrastValue = 1
                        }
                        else
                        {
                            selShape.contrastValue = Number(text)
                        }
                    }
                }
            }
            Row{
                id: idPackingDensityValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("Sparse infill density")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Qt.AlignVCenter
                }
                StyledSlider
                {
                    anchors{
                        ///horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    id : control2
                    value: 127
                    width: 74
                    maximumValue:255
                    minimumValue:1
                    sliderHeight: 2
                    height: 25
                    onValueChanged:
                    { 
                    }
                }
                BasicDialogTextField {
                    width: 41
                    height: 28
                    horizontalAlignment: Text.AlignVCenter
                    text: control2.value
                    onTextChanged: {}
                }
            }
            Row{
                id: idBrightnessValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("Brightness")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Qt.AlignVCenter
                }
                StyledSlider
                {
                    anchors{
                        ///horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    id : control3
                    value: selShape ? selShape.brightnessValue : 0
                    width: 74
                    maximumValue:100
                    minimumValue:1
                    sliderHeight: 2
                    height: 25
                    onValueChanged:
                    {
                        if(!selShape)
                            return;
                        selShape.brightnessValue = value 
                        brightnessValueChanged(value)
                    }
                }
                BasicDialogTextField {
                    width: 41
                    height: 28
                    horizontalAlignment: Text.AlignVCenter
                    text: control3.value
                    onTextChanged: {
                        if(!selShape)
                            return;
                        if(Number(text) > 100)
                        {
                            selShape.brightnessValue = 100
                        }
                        else if(Number(text) < 1)
                        {
                            selShape.brightnessValue = 1
                        }
                        else
                        {
                            selShape.brightnessValue = Number(text)
                        }
                    }
                }
            }
            Row{
                id: idWhiteValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("White")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Qt.AlignVCenter
                }
                StyledSlider
                {
                    anchors{
                        ///horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    id : control4
                    value: selShape ? selShape.whiteValue : 0
                    width: 74
                    maximumValue:255
                    minimumValue:1
                    sliderHeight: 2
                    height: 25
                    onValueChanged:
                    { 
                        if(!selShape)
                            return;
                        selShape.whiteValue = value 
                        whiteValueChanged(value)
                    }
                }
                BasicDialogTextField {
                    width: 41
                    height: 28
                    horizontalAlignment: Text.AlignVCenter
                    text: control4.value
                    onTextChanged: {
                        if(!selShape)
                            return;
                        if(Number(text) > 255)
                        {
                            selShape.whiteValue = 255
                        }
                        else if(Number(text) < 1)
                        {
                            selShape.whiteValue = 1
                        }
                        else
                        {
                            selShape.whiteValue = Number(text)
                        }
                    }
                }
            }
            Row{
                id: idDensityValue
                width: parent.width
                height: 28
                spacing: 5
                StyledLabel{
                    width: 110
                    height: 28
                    text: qsTr("Density")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Qt.AlignVCenter
                }
                BasicDialogTextField {
                    width: 120
                    height: 28
                    horizontalAlignment: Text.AlignLeft
                    unitChar:"dot/mm"
                    text: selShape ? selShape.densityValue : ""
                    onTextChanged: {
                        if(!selShape)
                            return;
                        if(text == "")
                        {
                        }
                        else if(parseInt(text) > 10)
                        {
                            selShape.densityValue=10
                        }
                        else if(parseInt(text) < 1)
                        {
                             selShape.densityValue=1
                        }
                        else{
                            selShape.densityValue=Number(text)
                        }
                    }
                }
            }
        }
    }
}
