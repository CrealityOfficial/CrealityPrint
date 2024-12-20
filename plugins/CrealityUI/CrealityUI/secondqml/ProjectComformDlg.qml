import QtQuick 2.0
import QtQuick.Controls 2.12
import CrealityUI 1.0
import QtQml.Models 2.13
import QtQuick.Layouts 1.15
import "qrc:/CrealityUI"
BasicDialog
{
    id: idDialog
    title: qsTr("Drop project file")
    width: 460 * screenScaleFactor
    height: 280 * screenScaleFactor
    titleHeight : 30 * screenScaleFactor
    signal accept(var option)
    signal cancel()
    property var selectedOption: 1
    ColumnLayout {
        anchors.top: parent.top
         anchors.topMargin: 50 * screenScaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 21* screenScaleFactor
     height: 150 * screenScaleFactor
     BasicRadioButton {
        id: option1
        Layout.preferredWidth: 200 * screenScaleFactor
        Layout.preferredHeight: 40 * screenScaleFactor
         text: qsTr("Open as project")
         onClicked: {
                selectedOption = 0
            }
     }
     BasicRadioButton {
        id: option2
        Layout.preferredWidth: 200 * screenScaleFactor
        Layout.preferredHeight: 40 * screenScaleFactor
        checked: true
        text: qsTr("Import geometry only")
        onClicked: {
                selectedOption = 1;
            }
     }

 }
    

    RowLayout
        {
            width: 200 * screenScaleFactor
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20 * screenScaleFactor
            anchors.right: parent.right
            anchors.rightMargin: 20 * screenScaleFactor
            spacing: 15 * screenScaleFactor
            BasicDialogButton
            {
                Layout.preferredWidth: 80* screenScaleFactor
                Layout.preferredHeight: 30* screenScaleFactor
                text: qsTr("OK")
                btnRadius:5
                btnBorderW:0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                onSigButtonClicked:
                {
                    console.log("selectedOption",selectedOption)
                    accept(selectedOption)
                    close()
                }
            }

            BasicDialogButton
            {
                Layout.preferredWidth: 80* screenScaleFactor
                Layout.preferredHeight: 30* screenScaleFactor
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