import QtQuick 2.10
import QtQuick.Controls 2.0
//import CrealityUI 1.0
//import "qrc:/CrealityUI"
import "../qml"
BasicDialog {
    id: informationDlg
    width: 483
    height: 247
    title: qsTr("Close information")
    //radius : 5
   // color: Constants.itemBackgroundColor
    signal sigSaveFile()
    signal sigNoSaveFile()
    signal delDefaulltProject()
    Column
    {
        spacing: 20
        y:60
        height: parent.height -60
        width: parent.width
        Image {
            id: imageRepair
            anchors.horizontalCenter: parent.horizontalCenter
            width: 64
            height: 54
            source: "qrc:/UI/images/warning.png"
            fillMode: Image.PreserveAspectFit       //拉伸是缩放 总是显示完整图片
        }
        StyledLabel {
           id: panel_name
           x: 65
           y: 100
           verticalAlignment: Label.AlignTop
           horizontalAlignment: Label.AlignHCenter
           text: qsTr("Save Project Before Exiting?")
           anchors.horizontalCenter: parent.horizontalCenter
           font.pointSize: Constants.labelFontPointSize_6
        }
        Item {
            height: 2
            x:10
            y:10
            width: parent.width - 20
            BasicSeparator
            {
                anchors.fill: parent
            }
        }

        Row
        {
            leftPadding: 57
            spacing: 20
            width : parent.width
            height: 30
            BasicButton
            {
                id: idYes
                text: qsTr("Save")
                width: 110
                height: 28

                onSigButtonClicked:
                {
                    close();
                    sigSaveFile();
                }
            }

            BasicButton
            {
                id: no_support
                text: qsTr("Exit")
                width: 110
                height: 28

                onSigButtonClicked:
                {
                    close();
                    sigNoSaveFile()
                    delDefaulltProject()
                }
            }
            BasicButton
            {
                id: cancel
                text: qsTr("Cancel")
                width: 110
                height: 28

                onSigButtonClicked:
                {
                    close();
                }
            }
        }
    }
}
