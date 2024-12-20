import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
//import ".."
import CrealityUI 1.0
import "../qml"
import ".."

BasicDialogNoWin_V4 {
    property var mtranslate
    property int labelWidth: 15* screenScaleFactor
    property string currentColor: "#FFFFFF"
    id: control
    title:""
    color:"#4b4b4d"
    titleColor: "#4b4b4d"
    width: 235* screenScaleFactor
    height: 198* screenScaleFactor
    shadowWidth: 1
    allRadius: true
    radius: 10
    titleHeight: 20* screenScaleFactor
    closeBtnVis: true
    ListModel{
        id: colorModel
        ListElement{btnColor:"#2161AF"}
        ListElement{btnColor:"#57AFB2"}
        ListElement{btnColor:"#F7B32D"}
        ListElement{btnColor:"#E33D4A"}
        ListElement{btnColor:"#C088AD"}
        ListElement{btnColor:"#5D88BE"}
        ListElement{btnColor:"#5ABD0E"}
        ListElement{btnColor:"#E17239"}
        ListElement{btnColor:"#F74E46"}
        ListElement{btnColor:"#874AF9"}
        ListElement{btnColor:"#50C2EC"}
        ListElement{btnColor:"#8DC15A"}
        ListElement{btnColor:"#C3977A"}
        ListElement{btnColor:"#CD7776"}
        ListElement{btnColor:"#9086BA"}
        ListElement{btnColor:"#FFFFFF"}
        ListElement{btnColor:"#D3D3D3"}
        ListElement{btnColor:"#9E9E9E"}
        ListElement{btnColor:"#5A5A5A"}
        ListElement{btnColor:"#000000"}
    }

    bdContentItem:Rectangle {
        color:"transparent"//control.color
        Flow{
            id: colorBtnFlow
            width: parent.width
            height: parent.width
            padding: 13
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 15
            Repeater{
                id:repeater
                model:colorModel
                delegate: colorBtn
            }
        }
    }

    ExclusiveGroup { id: tabPositionGroup }
    Component{
        id:colorBtn
        RadioButton{
            exclusiveGroup: tabPositionGroup
            checked: currentColor === model.btnColor
            style: RadioButtonStyle {
                indicator: Rectangle {
                    implicitWidth: 24* screenScaleFactor
                    implicitHeight: 24* screenScaleFactor
                    radius: width/2
                    color: model.btnColor
                    border.color: "#262626"
                    border.width: control.checked ? 1 : 0
                    Image{
                        width: 14* screenScaleFactor
                        height: 10* screenScaleFactor
                        visible: control.checked
                        anchors.centerIn: parent
                        source: "qrc:/UI/images/check.png"
                    }
                }
            }
            onCheckedChanged: {
                if(checked)
                {
                    currentColor = model.btnColor
                    control.visible = false
                }
            }
        }
    }
}
