import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.13
import "../secondqml"
LeftPanelDialog {
    id : control_1
    title: qsTr("Clone")
    width: 300* screenScaleFactor
    height: 168* screenScaleFactor
    readonly property int margin: 10
    property var control
    
    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }

    Item {
        anchors.fill: control_1.panelArea

        Column{
            anchors.top: parent.top
            anchors.topMargin: 30* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 30* screenScaleFactor
            RowLayout
            {
                spacing: 10* screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                width: (parent.width - 40)
                StyledLabel
                {
                    id: cloneNum
                    Layout.preferredWidth: 60* screenScaleFactor
                    text: qsTr("Clone Number:")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                    Layout.alignment: Qt.AlignVCenter
                }

                Item{
                    Layout.fillWidth: true
                }

                BasicDialogTextField {
                    id: inputCloneNum
                    Layout.preferredWidth: 100* screenScaleFactor
                    Layout.preferredHeight: 28* screenScaleFactor
                    radius: 5
                    text: "1"
                    font.pointSize: 9
                    validator: IntValidator {bottom: 1; top: 99;}//by TCJ
                    Layout.alignment: Qt.AlignVCenter

                    onAccepted: {
                        add_support.sigButtonClicked()
                    }
                }
            }

            BasicButton {
                id: add_support
                text: qsTr("Add")
                btnRadius:14
                btnBorderW:1
                anchors.horizontalCenter: parent.horizontalCenter
                borderColor: Constants.lpw_BtnBorderColor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                width: 258* screenScaleFactor
                height: 28* screenScaleFactor
                pointSize: 9
                onSigButtonClicked:
                {
                    if(control.isSelect())
                    {
                        var cnt = parseInt(inputCloneNum.text)
                        control.clone(cnt)
                    }else{
                        control.open()
                    }
                }
            }
        }
    }
}
