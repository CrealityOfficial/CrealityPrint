import QtQuick 2.0
import QtQuick.Controls 2.12
import CrealityUI 1.0
import "qrc:/CrealityUI"
BasicDialog
{
    id: idDialog
    width: 400
    height: 150
    titleHeight : 30
    property int spacing: 5
    title: qsTr("New Project")
    property alias projectName: idProjectInput.text
    signal sigProject(var name)
    StyledLabel {//by TCJ
        id: idLabel
        x:30
        y:20 + titleHeight
        width:120
        height:30
        text: qsTr("Project Name:")
        font.pointSize: Constants.labelFontPointSize_16
        color:"white"//"black"
		verticalAlignment: Qt.AlignVCenter
		horizontalAlignment: Qt.AlignRight
    }

    BasicDialogTextField {
        id: idProjectInput
        anchors.left: idLabel.right
        anchors.leftMargin: 10
        anchors.top: idLabel.top
        width: 200
        height: 30
        color: "white"
        text: "Untitled"
        font.pointSize: Constants.labelFontPointSize_16
        verticalAlignment: Qt.AlignVCenter
        onTextEdited:{}
    }

    BasicDialogButton {
        id: basicComButton
        width: 100
        height: 32
        text: qsTr("Cancel")
        anchors.top: idLabel.bottom
        anchors.topMargin: 20
        anchors.left: parent.horizontalCenter
        anchors.leftMargin: 15
        //btnRadius:6
        onSigButtonClicked:
        {
            idDialog.close();
        }
    }
    BasicDialogButton {
        id: basicComButton2
        width: 100
        height: 32
        text: qsTr("Yes")
        anchors.top: basicComButton.top
        anchors.right: parent.horizontalCenter
        anchors.rightMargin: 15
        //btnRadius:6
        onSigButtonClicked:
        {
            idDialog.close();
            sigProject(idProjectInput.text)
        }
    }
}
