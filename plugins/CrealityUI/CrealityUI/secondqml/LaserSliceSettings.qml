import QtQuick 2.0
import QtQuick.Controls 2.5
import "../qml"
Item {
    width: 262
    signal genLaserGcode()
    //property var settingsModel
    //height: parent.height
    implicitHeight:250 - 170
    anchors.leftMargin: 23
    Column{
        spacing:2//5
        Item{
            width: parent.width
            height : 10
        }
		BasicButton
        {
            id: control
            width: 230
            height:40
            text: qsTr("Generate GCode")
            btnTextColor: "#FFFFFF"
			defaultBtnBgColor : "#1E9BE2"//"#3cd5ed"
            btnRadius: 3
            btnBorderW: 0
            pointSize: Constants.labelFontPointSize_9
            onSigButtonClicked:
            {
                genLaserGcode()
            }
        }
    }
}
