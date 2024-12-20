import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import ".."
import "../qml"

LeftPanelDialog {
    id:idRoot
    
    width: 280 * screenScaleFactor
    height: 421 * screenScaleFactor
    title: qsTr("Support")

    //property var com

    property alias support_grid_text:support_grid_text
    property alias overhang_angle_text:overhang_angle_text
    property alias platform_checkbox:platform_checkbox

    //property var com
    property var control
    property var defaultSupportGird: "10"

    readonly property int margin: 10
    readonly property int checkboxWidth: 15
    readonly property int textfileWidth: 60
    readonly property int panelFontSize: 18
    readonly property int resetButtonHeight: 23
    readonly property int resetButtonWidth: 160

    //signal sliceClicked()

    Item {
        anchors.fill: parent.panelArea
        anchors.margins: 10 * screenScaleFactor

        CusText{
            id:spTxt
            fontText: qsTr("Support Parameter")
            fontPointSize: 12
            fontWidth: 280 * screenScaleFactor
            fontColor: Constants.themeTextColor
            fontWeight: Font.Medium
            anchors.top: parent.top
            anchors.topMargin: 4 * screenScaleFactor
            hAlign: 0
        }

        RowLayout {
            id:sgRL
            width: parent.width
            height: 22 * screenScaleFactor
            spacing: 5
            anchors.top: spTxt.bottom
            anchors.topMargin: 8 * screenScaleFactor
            CusText{
                id: support_grid
                fontText: qsTr("Support Grid")
                fontColor: Constants.themeTextColor
                Layout.alignment :  Qt.AlignVCenter
                fontWidth: 120* screenScaleFactor
                hAlign: 0
                fontPointSize: 9
            }

            Item{
                Layout.fillWidth: true
            }

            SizeTextField {
                id: support_grid_text
                Layout.preferredWidth: 120 * screenScaleFactor
                Layout.preferredHeight: 22 * screenScaleFactor
                Layout.alignment :  Qt.AlignVCenter
                fontPointSize: 9
                text: idRoot.defaultSupportGird
//                    borderHoverColor: "#00a3ff"
//                    borderNormalColor: "#6e6e72"
                placeholderText : qsTr("Side length")

                onActiveFocusChanged:
                {
                    if(text < idRoot.defaultSupportGird) text = idRoot.defaultSupportGird
                    if(text >20.0 ) text = 20
                }

                onEditingFinished:
                {
                    if(text < idRoot.defaultSupportGird) text = idRoot.defaultSupportGird
                    if(text >20.0 ) text = 20
                }

                onTextChanged:
                {
                    if(text < 0) text = idRoot.defaultSupportGird
                    if(text >20.0 ) text = 20
                    supportUICpp.changeSize(text)
                }
            }
        }

        RowLayout {
            id:msRL
            anchors.top: sgRL.bottom
            anchors.topMargin: 2 * screenScaleFactor
            width: parent.width
            spacing: 5 * screenScaleFactor

            CusText{
                id: overhang_angle
                fontText: qsTr("Maximum Angle")
                fontColor: Constants.themeTextColor
                Layout.alignment :  Qt.AlignVCenter
                fontPointSize: 9
                fontWidth: 120* screenScaleFactor
                hAlign: 0
            }

            Item{
                Layout.fillWidth: true
            }

            SizeTextField {
                id: overhang_angle_text
                Layout.preferredWidth: 120 * screenScaleFactor
                Layout.preferredHeight: 22 * screenScaleFactor
                fontPointSize: 9
//                    borderHoverColor:"#6e6e72"
//                    borderNormalColor:"#00a3ff"
                text: qsTr("30")
                placeholderText: qsTr("Enter Angle")//
                unitchar: qsTr("(°)")//qsTr("度(°)")
                Layout.alignment :  Qt.AlignVCenter
                onTextChanged:
                {
                    if(idRoot.control)
                        supportUICpp.changeAngle(text)
                }
            }
        }

        StyleCheckBox {
            id:platform_checkbox
            anchors.top: msRL.bottom
            anchors.topMargin: 6 * screenScaleFactor
            width: 240 * screenScaleFactor
            height: 14 * screenScaleFactor
            text: qsTr("Only add support to the plate")
        }

        Item {
            id:lin2
            width: parent.width
            height : 2
            anchors.top: platform_checkbox.bottom
            anchors.topMargin: 20
            Rectangle {
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColor
                }
            }
        }

        CusText {
            id:asTxt
            anchors.top: lin2.bottom
            anchors.topMargin: 10
            fontWidth: 280 * screenScaleFactor
            fontText: qsTr("Automatic Support")
            fontPointSize: 12
            fontColor: Constants.themeTextColor
            fontWeight: Font.Medium
            hAlign: 0
        }

        BasicButton {
            id: generate_support
            height: 28 * screenScaleFactor
            width: 258 * screenScaleFactor
            anchors.top: asTxt.bottom
            anchors.topMargin: 9 * screenScaleFactor
            btnRadius: height/2
            text : qsTr("Automatic Support")
            borderColor: Constants.lpw_BtnBorderColor
            defaultBtnBgColor: Constants.leftToolBtnColor_normal
            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            enabled:  support_grid_text.text > 0.0&& overhang_angle_text.text.length!==0 ? true :false
            onSigButtonClicked:  {
                var ret = supportUICpp.hasSupport()
                if(ret)
                {
                    idHasSupport.visible = true
                }
                else{
                    supportUICpp.autoAddSupport(parseFloat(support_grid_text.text),parseFloat(overhang_angle_text.text),platform_checkbox.checked);
                }
            }
        }

        Item {
            id:line3
            anchors.top: generate_support.bottom
            anchors.topMargin: 20
            width: parent.width
            height : 2
            Rectangle {
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColor
                }
            }
        }

        CusText{
            id:csTxt
            anchors.top: line3.bottom
            anchors.topMargin: 10
            fontWidth: 280 * screenScaleFactor
            fontText: qsTr("Custom Support")
            fontPointSize: 12
            fontColor: Constants.themeTextColor
            fontWeight: Font.Medium
            hAlign: 0
        }

        Row {
            id:addRow
            anchors.top: csTxt.bottom
            anchors.topMargin: 6 * screenScaleFactor
            spacing: 10
            BasicButton {
                id: add_support
                text: qsTr("Add")
                width: 126 * screenScaleFactor
                height: 28 * screenScaleFactor
                btnRadius:height/2
                pointSize: 9
                borderColor: Constants.lpw_BtnBorderColor
                borderHoverColor: "transparent"
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    if(support_grid_text.text ==="" || support_grid_text.text ==="0")
                    {
                        support_grid_text.text = 0.1
                    }
                    supportUICpp.addMode();
                    enabled=false;
                    delete_support.enabled=true;
                }
            }

            BasicButton {
                id: delete_support
                text: qsTr("Delete")
                width: add_support.width
                height: add_support.height
                btnRadius:14 * screenScaleFactor
                pointSize: 9
                borderColor: Constants.lpw_BtnBorderColor
                borderHoverColor: "transparent"
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    supportUICpp.deleteMode();
                    enabled=false;
                    add_support.enabled=true;
                }
            }
        }

        Item {
            id:line4
            width: parent.width
            height : 2
            anchors.top: addRow.bottom
            anchors.topMargin: 20
            Rectangle {
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColor
                }
            }
        }

        Rectangle {
            width:parent.width
            height:59 * screenScaleFactor
            anchors.top: line4.bottom
            anchors.topMargin: 10 * screenScaleFactor
            color:"transparent"
            BasicButton {
                id: clear_support
                anchors.top: parent.top
                anchors.topMargin: 10 * screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Clear All Supports")
                btnRadius:14*screenScaleFactor
                width: 258 * screenScaleFactor
                height: 28 * screenScaleFactor
                pointSize: 9 * screenScaleFactor
                borderColor: Constants.lpw_BtnBorderColor
                borderHoverColor: "transparent"
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    supportUICpp.removeAll();
                }
            }
        }
    }

    UploadMessageDlg {
        id: idHasSupport
        visible: false
        messageType: 1
        msgText: qsTr("Do you want to clear existing  supports before create new supports?")

        onSigOkButtonClicked:
        {
            idHasSupport.visible = false
            supportUICpp.removeAll();
            supportUICpp.autoAddSupport(parseFloat(support_grid_text.text),parseFloat(overhang_angle_text.text),platform_checkbox.checked);
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
