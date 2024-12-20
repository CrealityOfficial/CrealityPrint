import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import "../qml"
Item {

    signal thresholdChanged(var value)
    signal originalImageShow(var value)
    signal reverseImage(var value)
    signal flipModelValueChanged(var value)
    width: 262
    //height: parent.height
    implicitHeight:60
    anchors.leftMargin: 23

    function initState()
    {
        console.log("~~~~initState~~~~~")
    }

    Column{
        width: parent.width
        height: parent.height
        spacing: 5
        Rectangle{
            width: parent.width
            height: 10
            color: "transparent"
        }

        Column{
            width: parent.width
            //height: 46
            spacing: 5
            StyleCheckBox {
                width: 100
                height: 18
                text: qsTr("Original Image Show")
                checked: selShape ? selShape.originalShow : true
                // fontSize: Constants.labelFontPointSize_12
                onCheckedChanged:
                {
                    selShape.originalShow = !checked
                    originalImageShow(checked)
                }
            }
            StyleCheckBox {
                width: 48
                height: 18
                text: qsTr("Reverse")
                checked: selShape ? selShape.reverse : true
                // fontSize: Constants.labelFontPointSize_12
                onCheckedChanged:
                {
                    selShape.reverse = !checked
                    reverseImage(checked)
                }
            }
            Row{
                id: idFlipModelValue
                width: parent.width
                height: 28
                spacing: 25
                StyledLabel{
                    width: 90
                    height: 28
                    text: qsTr("Flip Model")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Qt.AlignVCenter
                }
                BasicCombobox{
                    width: 120
                    height: 28
                    //font.pixelSize: 14
                    currentIndex: selShape ? selShape.filpModelValue : 0

                    model: ListModel {
                        id: modelType
                        ListElement {key:"NoFlip"; modelData: qsTr("No Flip");}
                        ListElement {key:"Vertical"; modelData: qsTr("Vertical");}
                        ListElement {key:"Horizontal"; modelData: qsTr("Horizontal");}
                        ListElement {key:"Simultaneously"; modelData: qsTr("Simultaneously");}
                    }
                    onCurrentIndexChanged: {
                        selShape.filpModelValue = currentIndex
                        flipModelValueChanged(currentIndex)
                    }
                }
            }

            Row{
                id: idBlackWhiteValue
                width: parent.width
                height: 28
                spacing: 10
                StyledLabel{
                    width: 105
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
                    width: 70
                    maximumValue:255
                    minimumValue:1
                    sliderHeight: 2
                    height: 25
                    onValueChanged:
                    {
                        selShape.threshold = value
                        thresholdChanged(value)
                    }
                }
                BasicDialogTextField {
                    width: 40
                    height: 28
                    horizontalAlignment: Text.AlignVCenter
                    text: control.value
                    onTextChanged: {
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
                            if(selShape)
                                selShape.threshold = Number(text)
                        }
                    }
                }
            }
        }

        // Column{
        //     width: parent.width
        //     height:  200 + 20
        //     spacing: 5


        // }

    }

}
