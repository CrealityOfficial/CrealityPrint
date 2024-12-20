import QtQuick 2.10
import QtQuick.Controls 2.0

Rectangle {
//    width: 190
//    height: 100
    width : 252 * screenScaleFactor
    height : 390* screenScaleFactor
    radius: 5* screenScaleFactor
    color: Constants.itemBackgroundColor

    property  var control
    readonly property int checkboxWidth: 15* screenScaleFactor
    readonly property int margin: 10* screenScaleFactor
    property bool addSuportMode: true

    MouseArea{//捕获鼠标点击空白地方的事件
        anchors.fill: parent
    }

    StyledLabel {
       id: panel_name
       anchors.top: parent.top
       anchors.topMargin: margin
       anchors.horizontalCenter: parent.horizontalCenter
       text: qsTr("Support")
       color: Constants.textColor
       font.pointSize: 9
    }

    Rectangle {
        id: auto_support_rect
        visible: true
        width: (parent.width - 20)* screenScaleFactor
        height: 180* screenScaleFactor
        anchors.top: panel_name.bottom
        anchors.topMargin: margin * 2* screenScaleFactor
        anchors.horizontalCenter: parent.horizontalCenter
        //color: "gray"
        color: Constants.itemChildBackgroundColor
        border.color:Constants.rectBorderColor

        StyledLabel {
            id: auto_support
            text: qsTr("Auto Support")
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_6
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: margin
            anchors.leftMargin: margin
        }
        StyledLabel {
            id: support_grid
            text: qsTr("Support Grid")
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_6
            width: 100* screenScaleFactor
            anchors.top: auto_support.bottom
            anchors.topMargin: margin * 2
            anchors.left: auto_support.left
        }

        SizeTextField {
            id: support_grid_text
            implicitWidth: 100* screenScaleFactor
            implicitHeight: 28* screenScaleFactor
            anchors.left: support_grid.right
            anchors.verticalCenter:  support_grid.verticalCenter
            anchors.leftMargin: margin
            text: qsTr("0.1")
            placeholderText : "[0.1,20.0]"
//            validator:RegExpValidator {
//                regExp:/^(20?)|(^[1-9](.[0-9])?)$|^(0(.[1-9])?)$/
//                    ///^[0-9]\d*\.\d$/
//            }
            onActiveFocusChanged:
            {
                if(text < 0.1)text = 0.1
                if(text >20.0 ) text = 20
            }
            onEditingFinished:
            {
                if(text < 0.1) text = 0.1
                if(text >20.0 ) text = 20
            }
            onTextChanged:
            {
                if(text < 0) text = 0.1
                if(text >20.0 ) text = 20
                control.changeSize(text)
            }
        }

        StyledLabel {
            id: overhang_angle
            color: Constants.textColor
            width: 100* screenScaleFactor
            font.pointSize: Constants.labelFontPointSize_6
            anchors.top: support_grid.bottom
            anchors.topMargin: margin * 2* screenScaleFactor
            anchors.left: support_grid.left
            text: qsTr("Maximum Angle")
        }

        SizeTextField {
            id: overhang_angle_text
            implicitWidth: support_grid_text.implicitWidth
            implicitHeight: support_grid_text.implicitHeight
            text: qsTr("30")
            unitchar: qsTr("(°)")//qsTr("度(°)")
            anchors.verticalCenter:  overhang_angle.verticalCenter
            anchors.left: overhang_angle.right
            anchors.leftMargin: margin
        }

        CheckBox {
            id: platform_checkbox
            text: "<font color='#E3EBEE'>" + qsTr("Only add support to the plate") + "</font>"
            font.pointSize: Constants.labelFontPointSize_10
            font.family: Constants.labelFontFamily
            indicator.width: checkboxWidth
            indicator.height: checkboxWidth
            anchors.top: overhang_angle_text.bottom
            anchors.topMargin: 0
        }

        BasicButton
        {
            id: generate_support
            height: 32* screenScaleFactor
            width: parent.width
            btnRadius:5
            text : qsTr("Automatic Support")
            defaultBtnBgColor : "#FFFFFF"
            hoveredBtnBgColor : "#3492FF"
            btnTextColor : down || hovered ? "#FFFFFF":"3492FF"

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: platform_checkbox.bottom
            anchors.topMargin: 0
            enabled:  support_grid_text.text > 0.0 ? true :false
            onSigButtonClicked:  {
                control.autoAddSupport(parseFloat(support_grid_text.text),parseFloat(overhang_angle_text.text),platform_checkbox.checked);
            }
        }
    }

    Rectangle {
        id: manual_support
        width: auto_support_rect.width
        height: 80* screenScaleFactor
        anchors.top: auto_support_rect.bottom
        anchors.topMargin: margin * 2
        anchors.horizontalCenter: parent.horizontalCenter
//        color: "gray"
        color: Constants.itemChildBackgroundColor
        border.color:Constants.rectBorderColor

        StyledLabel {
            id: custom_support
            text: qsTr("Custom Support")
            color: Constants.textColor
            font.pointSize: Constants.labelFontPointSize_6
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: margin
            anchors.leftMargin: margin
        }

        BasicButton {
            id: add_support
            text: qsTr("Add")
            defaultBtnBgColor : enabled ? "white" : "#3492FF"
            hoveredBtnBgColor : "#3492FF"
            btnTextColor : down || hovered ? "#FFFFFF":"3492FF"

            width: (parent.width - 2 * margin) / 2
            height: 32* screenScaleFactor
            btnRadius:5
            anchors.left: parent.left
            anchors.top: custom_support.bottom
            anchors.topMargin: margin
            enabled: !addSuportMode
            onSigButtonClicked: {
                if(support_grid_text.text ==="" || support_grid_text.text ==="0")
                {
                    support_grid_text.text = 0.1
                }
                control.addMode();
                addSuportMode=true;
            }
        }

        BasicButton {
            id: delete_support
            text: qsTr("Delete")
            enabled: addSuportMode
            defaultBtnBgColor : enabled ? "white" : "#3492FF"
            hoveredBtnBgColor : "#3492FF"
            btnTextColor : down || hovered ? "#FFFFFF":"3492FF"

            width: add_support.width
            height: add_support.height
            anchors.right:  parent.right
            anchors.top: add_support.top
            btnRadius:5
            onSigButtonClicked: {
                control.deleteMode();
                addSuportMode=false;
            }
        }
    }

    BasicButton {
        id: clear_support
        text: qsTr("Clear All Supports")
        btnRadius:5
        defaultBtnBgColor : "#FFFFFF"
        hoveredBtnBgColor : "#3492FF"
        btnTextColor : down || hovered ? "#FFFFFF":"3492FF"

        width: manual_support.width
        height: add_support.height
        anchors.top: manual_support.bottom
        anchors.topMargin: margin
        anchors.left: manual_support.left
        onSigButtonClicked: {
            control.removeAll();
        }
    }
}
