import QtQuick 2.0
import QtQuick.Controls 2.12
import CrealityUI 1.0
import "qrc:/CrealityUI"
BasicDialog
{
    id: idDialog
    width: 400*screenScaleFactor
    height: 155*screenScaleFactor//200
    titleHeight : 30*screenScaleFactor
    title: qsTr("Message")

    signal sigNumber(var clonenum)
    StyledLabel {
        id: idLabel
        x:60*screenScaleFactor//30
        y:20*screenScaleFactor + titleHeight
//        width:80*screenScaleFactor
        height:30*screenScaleFactor
        text: qsTr("Clone Number:")
        font.pointSize: Constants.labelFontPointSize_9
        color:Constants.textColor
    }

    SizeTextField/*StyledTextInput*/ {
        id: idInput
        anchors.left: idLabel.right
        anchors.leftMargin: 30*screenScaleFactor//10
        anchors.verticalCenter: idLabel.verticalCenter
        width: 150*screenScaleFactor
        height: 30*screenScaleFactor
        color: Constants.textColor
        text: "1"
        validator: IntValidator {bottom: 1; top: 99;}
        unitchar:""
        font.pointSize: Constants.labelFontPointSize_9
        horizontalAlignment: Qt.LeftToRight

        onAccepted: {
            basicComButton2.sigButtonClicked()
        }

        onTextChanged: {
            let error = text == "" || parseInt(text) == 0;
            idErrorTips.visible = error;
            basicComButton2.enabled = !error;
        }

        Rectangle
        {
            anchors.fill: parent
            color: Constants.dialogItemRectBgColor
            z:-1
            //x:-5
            border.width: 1
            border.color: Constants.dialogItemRectBgBorderColor
        }
    }

    Text {
        id: idErrorTips
        anchors.top: idInput.bottom
        anchors.topMargin: 5
        anchors.left: idInput.left
        visible: false

        color: Constants.warningColor
        text: qsTr("The number cannot be empty")
    }


    BasicButton {
        id: basicComButton
        width: 100*screenScaleFactor
        height: 28*screenScaleFactor
        text: qsTr("Cancel")
        btnRadius:3*screenScaleFactor
        btnBorderW:0*screenScaleFactor
        defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
        anchors.top: idLabel.bottom
        anchors.topMargin: 20*screenScaleFactor
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: 5*screenScaleFactor
        onSigButtonClicked:
        {
            idDialog.close();
        }
    }

    BasicButton {
        id: basicComButton2
        width: 100*screenScaleFactor
        height: 28*screenScaleFactor
        text: qsTr("Yes")
        btnRadius:3*screenScaleFactor
        btnBorderW:0
        defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
        anchors.top: basicComButton.top
        anchors.right: parent.horizontalCenter
        anchors.rightMargin: 5*screenScaleFactor
        onSigButtonClicked:
        {
            idDialog.close();
            sigNumber(idInput.text)
        }
    }

}
