import QtQuick 2.0
import QtQuick.Controls 2.12
import CrealityUI 1.0
import QtQml.Models 2.13
import "qrc:/CrealityUI"
BasicDialog
{
    id: idDialog
    title: qsTr("Image Config")
    width: 460 * screenScaleFactor
    height: 410 * screenScaleFactor
    titleHeight : 30 * screenScaleFactor
    signal accept()
    signal cancel()

    property alias myHeight: idHeight.text
    property alias myWidth: idWidth.text
    property alias myBase: idBase.text
    property alias myCmbCurrentIndex :idHigherCmb.currentIndex
    property alias myColorModel :idColorModel.text
    property alias mySmothing: idSmothing.text

    onVisibleChanged: {
        if(visible){
            idHigherCmb.model = supPattern
        }else{
            idHigherCmb.model = null
        }
    }

    function initTextVelue()
    {
        idHeight.text = "2.2"
        idWidth.text = "140"
        idBase.text = "0.35"
        idHigherCmb.currentIndex = 0
        idColorModel.text = "Linear"
        idSmothing.text = "9"
    }
    Rectangle
    {
        width: parent.width
        y:30 * screenScaleFactor
        height: 360 * screenScaleFactor
        color: "transparent"
        anchors.centerIn: parent.Center
        Grid
        {//by TCJ
            id:idGrid
            columns: 2
            rows: 6
            spacing: 10 * screenScaleFactor
            x:50 * screenScaleFactor
            y:40 * screenScaleFactor
            StyledLabel {
                text: qsTr("Base(mm)")
                /*color: "black"*/
                height : 30 * screenScaleFactor
                width : 160 * screenScaleFactor
                font.pointSize : Constants.labelFontPointSize_9
                verticalAlignment: Qt.AlignVCenter
            }
            BasicDialogTextField
            {
                id :idBase
                height : 30 * screenScaleFactor
                width : 140 * screenScaleFactor
                text: "0.35"
                //                validator: RegExpValidator { regExp: /(\d{1,6})([.,]\d{1,3})?$/ }
                //dayu 0 de xiao shu xiaoshu zhineng 4 wei
                validator: RegExpValidator { regExp: /(^[1-9](\d+)?(\.\d{1,4})?$)|(^\d\.\d{1,4}$)/}

                onEditingFinished:
                {
                }
            }
            StyledLabel {
                text: qsTr("Height(mm)")
                /*color: "black"*/
                height : 30 * screenScaleFactor
                width : 160 * screenScaleFactor
                font.pointSize : Constants.labelFontPointSize_9
                verticalAlignment: Qt.AlignVCenter
            }
            BasicDialogTextField
            {
                id :idHeight
                height : 30 * screenScaleFactor
                width : 140 * screenScaleFactor
                text: "2.2"
                validator: RegExpValidator { regExp: /(^[1-9](\d+)?(\.\d{1,4})?$)|(^\d\.\d{1,4}$)/}
                onEditingFinished:
                {
                }
            }

            StyledLabel {
                text: qsTr("Width(mm)")
                /*color: "black"*/
                height : 30 * screenScaleFactor
                width : 160 * screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_9  // panelFontSize
                verticalAlignment: Qt.AlignVCenter
            }
            BasicDialogTextField
            {
                id :idWidth
                height : 30 * screenScaleFactor
                width : 140 * screenScaleFactor
                text: "140"
                validator: RegExpValidator { regExp: /(^[1-9](\d+)?(\.\d{1,4})?$)|(^\d\.\d{1,4}$)/}
                onEditingFinished:
                {
                }
            }
            StyledLabel {
                text: ""
                /*color: "black"*/
                height : 30 * screenScaleFactor
                width : 160 * screenScaleFactor
                font.pointSize : Constants.labelFontPointSize_9  // panelFontSize
                verticalAlignment: Qt.AlignVCenter
            }

            ListModel {
                id:supPattern
                ListElement { text: qsTr("Darker is higher"); }
                ListElement { text: qsTr("Lighter is higher"); }
            }

            BasicCombobox {
                id:idHigherCmb
                height : 30 * screenScaleFactor
                width: 140 * screenScaleFactor
                font.pointSize : Constants.labelFontPointSize_9
                currentIndex: 0
                popPadding : 2 * screenScaleFactor
                model: supPattern
                property var modelText: "0"
                onModelTextChanged: {

                }
                onCurrentIndexChanged:
                {
                }

            }
            StyledLabel {
                text: qsTr("Color model")
                /*color: "black"*/
                height : 30 * screenScaleFactor
                width : 160 * screenScaleFactor
                font.pointSize : Constants.labelFontPointSize_9
                verticalAlignment: Qt.AlignVCenter
            }
            BasicDialogTextField
            {
                id :idColorModel
                height : 30 * screenScaleFactor
                width : 140 * screenScaleFactor
                color: "white"
                text: "Linear"
                enabled: false
            }

            StyledLabel {
                text: qsTr("Smothing")
                /*color: "black"*/
                height : 30 * screenScaleFactor
                width : 160 * screenScaleFactor
                font.pointSize : Constants.labelFontPointSize_9
                verticalAlignment: Qt.AlignVCenter
            }
            BasicDialogTextField
            {
                id :idSmothing
                height : 30 * screenScaleFactor
                width : 140 * screenScaleFactor
                text: "9"
                placeholderText : "Input 0-18"
                validator: RegExpValidator { regExp: /^([0-9]|(1[0-8]))$/ }
            }
        }
        StyledLabel {
            id: idInputPrompt
            x:50 * screenScaleFactor
            anchors.top: idGrid.bottom
            anchors.topMargin: 10 * screenScaleFactor
            text: qsTr("Please make sure the input is not empty !!")
            color: "red"
            height : 30 * screenScaleFactor
            width : 160 * screenScaleFactor
            visible : (idHeight.text === ""
                       ||idWidth.text === ""
                       || idBase.text === ""
                       || idSmothing.text === "") ? true : false
            font.pointSize : Constants.labelFontPointSize_9
        }
        Grid
        {
            columns: 2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20 * screenScaleFactor
            anchors.right: parent.right
            anchors.rightMargin: 20 * screenScaleFactor
            spacing: 15 * screenScaleFactor
            BasicDialogButton
            {
                width: 80 * screenScaleFactor
                height: 30 * screenScaleFactor
                text: qsTr("Yes")
                btnRadius:5
                btnBorderW:0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                onSigButtonClicked:
                {
                    if(idHeight.text === ""
                            ||idWidth.text === ""
                            || idBase.text === ""
                            || idSmothing.text === "")
                    {

                        return
                    }
                    accept()
                    close()
                }
            }

            BasicDialogButton
            {
                width: 80 * screenScaleFactor
                height: 30 * screenScaleFactor
                text: qsTr("Cancel")
                btnRadius:5
                btnBorderW:0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                onSigButtonClicked:
                {
                    cancel()
                    close()
                }
            }
            
        }
    }
}
