import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"
LeftPanelDialog
{
    id:idControl
    width: 300* screenScaleFactor
    height: 250* screenScaleFactor
    title: qsTr("Letter")
    property var com
    property alias idPlatform: idPlatform

    function execute()
    {
        console.log("execute")
        idPlatform.bottonSelected=true;
        com.startLetter()
    }

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }

    Item {
        anchors.fill: idControl.panelArea

        Column
        {
            anchors.top: parent.top
            anchors.topMargin: 20* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10* screenScaleFactor
            Row{
                id:idTextRow
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    id:idTextLabel
                    color: Constants.textColor
                    text: qsTr("Text:")
                    height: 30* screenScaleFactor
                    width: 75* screenScaleFactor
                    font.pointSize: 9
                    verticalAlignment: Qt.AlignVCenter
                }
                BasicDialogTextField {
                    id: idTextField
                    color: Constants.textColor
                    radius: 5
                    height : 28* screenScaleFactor
                    width : 153* screenScaleFactor
                    text: com.text
                    font.pointSize: 9
                    baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                    onTextChanged: {
                        com.text = idTextField.text
                    }
                }
            }

            Row
            {
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    id:idFontLabel
                    color: Constants.textColor
                    text: qsTr("Font:")
                    height: 30* screenScaleFactor
                    width : 75* screenScaleFactor
                    font.pointSize: 9
                    verticalAlignment: Qt.AlignVCenter
                }

                CXComboBox {
                    id: idFont
                    objectName: "idFontComboboxObject"
                    height : 28* screenScaleFactor
                    width : 153* screenScaleFactor
                    font.pointSize: 9
                    model: com.font_list
                    currentIndex: com.cur_font_index
                    onActivated: {
                        com.cur_font_index = idFont.currentIndex
                    }
                }
            }

            Row{
                id:idHeightRow
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    id:idHeightLabel
                    color: Constants.textColor
                    text: qsTr("Height:")
                    height: 30* screenScaleFactor
                    width: 75* screenScaleFactor
                    font.pointSize: 9
                    verticalAlignment: Qt.AlignVCenter
                }
                LeftToolSpinBox
                {
                    id: idHeight
                    height : 28* screenScaleFactor
                    width : 153* screenScaleFactor
                    realStepSize:0.5
                    realFrom:0
                    realTo:9999
                    font.pointSize: 9
                    realValue: com.height
                    onValueEdited:
                    {
                        com.height = idHeight.realValue
                    }
                }
            }

            Row{
                id:idThicknessRow
                anchors.horizontalCenter: parent.horizontalCenter
                StyledLabel {
                    id:idThicknessLabel
                    color: Constants.textColor
                    text: qsTr("Thickness:")
                    height: 30* screenScaleFactor
                    width: 75* screenScaleFactor
                    font.pointSize: 9
                    verticalAlignment: Qt.AlignVCenter
                }
                LeftToolSpinBox
                {
                    id: idThickness
                    height : 28* screenScaleFactor
                    width : 153* screenScaleFactor
                    realStepSize:0.5
                    realFrom:0
                    realTo:9999
                    font.pointSize: 9
                    realValue: com.text_thickness
                    onValueEdited:
                    {
                        com.text_thickness = idThickness.realValue
                    }
                }
            }

            Row{
                id:idTextSideRow
                spacing:20* screenScaleFactor
                visible : false
                ButtonGroup { id: idTextSideGroup }
                BasicRadioButton {
                    id: idTextInside
                    text: qsTr("Inside")
                    ButtonGroup.group: idTextSideGroup
                    //anchors.left: parent.left
                    //anchors.leftMargin: 30
                    font.pointSize: 9
                    font.bold: false
                    checked: com.text_side == -1 ? true:false
                    onClicked:
                    {
                        com.text_side = -1
                    }
                }
                BasicRadioButton {
                    id: idTextOutside
                    text: qsTr("Outside")
                    ButtonGroup.group: idTextSideGroup
                    checked: com.text_side == 1 ? true:false
                    //anchors.right: parent.right
                    //anchors.rightMargin: 50
                    font.pointSize: 9
                    onClicked:
                    {
                        com.text_side = 1
                    }
                }
            }

            BasicButton {
                id : idPlatform
                property var bottonSelected: true
                width: 258* screenScaleFactor
                height: 28* screenScaleFactor
                text : bottonSelected ? qsTr("Cancel Lettering") : qsTr("Start Lettering")
                btnRadius:14
                btnBorderW:1
                borderColor: Constants.lpw_BtnBorderColor
                borderHoverColor: Constants.lpw_BtnBorderHoverColor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                anchors.horizontalCenter: parent.horizontalCenter
                pointSize: 9
                onSigButtonClicked:
                {
                    if(!bottonSelected)
                        com.startLetter()
                    else
                    {
                        com.endLetter()
                    }
                    bottonSelected =!bottonSelected
                }
            }
        }
    }

    BasicButton {
        id : idImportFont
        width: 190* screenScaleFactor
        height: 32* screenScaleFactor
        text : qsTr("import font")
        btnRadius:3
        btnBorderW:1
        pointSize: 9
        borderColor: Constants.lpw_BtnBorderColor
        borderHoverColor: Constants.lpw_BtnBorderHoverColor
        defaultBtnBgColor: Constants.lpw_BtnColor
        hoveredBtnBgColor: Constants.lpw_BtnHoverColor
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter

        onSigButtonClicked:
        {
            com.loadFont();
        }
    }
}
