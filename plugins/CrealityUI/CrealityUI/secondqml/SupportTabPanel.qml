import QtQuick 2.10
import QtQuick.Controls 2.0
import ".."
import "../qml"
Rectangle {
    //    width: 190
    //    height: 100
    id : idRoot
    anchors.fill: parent
    
    radius: 0//5
    color: "red"//"#4b4b4d"
    border.width: 1
//    border.color: Constants.rightPanelRectBgBorderColor

    property var com
    property  var control
    readonly property int checkboxWidth: 15 * screenScaleFactor
    readonly property int textfileWidth: 60 * screenScaleFactor
    readonly property int margin: 10 * screenScaleFactor
    readonly property int panelFontSize: 18
    readonly property int resetButtonHeight: 23 * screenScaleFactor
    readonly property int resetButtonWidth: 160 * screenScaleFactor

    property var defaultSupportGird: ""

    signal sliceClicked()

    function execute(){
    }

    //property bool addSuportMode: false/*true*/
    Grid
    {
        rows: 12
        spacing:20
        y:20 * screenScaleFactor
        x:10 * screenScaleFactor
        width: parent.width - 20
        BasicGroupBox {

            id : idParameterGroup
            width: parent.width
            height : 130
            title: qsTr("Support Parameter")
            Row
            {
                y:10
                spacing: 5
                StyledLabel {
                    id: support_grid
                    text: qsTr("Support Grid")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_6
                    width: 100 * screenScaleFactor
                    height: 28 * screenScaleFactor
                }

                SizeTextField {
                    id: support_grid_text
                    width: 130 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    text: defaultSupportGird
                    placeholderText : qsTr("Side length")

                    onActiveFocusChanged:
                    {
                        if(text < defaultSupportGird) text = defaultSupportGird
                        if(text >20.0 ) text = 20
                    }
                    onEditingFinished:
                    {
                        if(text < defaultSupportGird) text = defaultSupportGird
                        if(text >20.0 ) text = 20
                    }
                    onTextChanged:
                    {
                        if(text < 0) text = defaultSupportGird
                        if(text >20.0 ) text = 20
                        if(control)
                            control.changeSize(text)
                    }
                }
            }

            Row
            {
                y:40 * screenScaleFactor
                spacing: 5
                StyledLabel {
                    id: overhang_angle
                    color: Constants.textColor
                    width: 100 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    font.pointSize: Constants.labelFontPointSize_6
                    text: qsTr("Maximum Angle")
                }

                SizeTextField {
                    id: overhang_angle_text
                    width: 130 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    text: qsTr("30")
                    placeholderText: qsTr("Enter Angle")//
                    unitchar: qsTr("(°)")//qsTr("度(°)")

                    onTextChanged:
                    {
                        if(control)
                            control.changeAngle(text)
                    }
                }

            }
            StyleCheckBox {
                id:platform_checkbox
                y:75 * screenScaleFactor
                width: 240 * screenScaleFactor
                height: 28 * screenScaleFactor
                text: qsTr("Only add support to the plate")

            }

        }

        Item {
            width: idRoot.width -3
            height : 2 * screenScaleFactor
            
            Rectangle {
                // anchors.left: idCol.left
                // anchors.leftMargin: -10
                x: -9 * screenScaleFactor
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColorDark
                }
            }
        }

        BasicGroupBox
        {
            width: parent.width
            height : 60 * screenScaleFactor
            title:  qsTr("Automatic Support")
            BasicButton
            {
                id: generate_support
                //y:5
                height: 28 * screenScaleFactor
                width: 230 * screenScaleFactor
                btnRadius:3
                text : qsTr("Automatic Support")
                //                defaultBtnBgColor : "#FFFFFF"
                //                hoveredBtnBgColor : "#3492FF"
                //                btnTextColor : down || hovered ? "#FFFFFF":"3492FF"

                //                anchors.horizontalCenter: parent.horizontalCenter
                //                anchors.top: platform_checkbox.bottom
                //                anchors.topMargin: 0
                enabled:  support_grid_text.text > 0.0&& overhang_angle_text.text.length!==0 ? true :false
                onSigButtonClicked:  {
                    var ret = control.hasSupport()
                    if(ret)
                    {
                        idHasSupport.visible = true
                    }
                    else{
                        control.autoAddSupport(parseFloat(support_grid_text.text),parseFloat(overhang_angle_text.text),platform_checkbox.checked);
                    }
                }
            }
        }

        Item {
            width: idRoot.width-3// -20
            height : 2
            
            Rectangle {
                // anchors.left: idCol.left
                // anchors.leftMargin: -10
                x: -9
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColorDark
                }
            }
        }

        BasicGroupBox
        {
            title: qsTr("Custom Support")
            width: parent.width
            height: 60 * screenScaleFactor
            Row
            {
                //y:5
                spacing: 10 * screenScaleFactor
                BasicButton {
                    id: add_support
                    text: qsTr("Add")
                    width: 110 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    btnRadius:3
                    /*enabled: !addSuportMode*/
                    onSigButtonClicked: {
                        if(support_grid_text.text ==="" || support_grid_text.text ==="0")
                        {
                            support_grid_text.text = 0.1
                        }
                        control.addMode();
                        /*addSuportMode=true;*///by TCJ
                        enabled=false;
                        delete_support.enabled=true;
                    }
                }

                BasicButton {
                    id: delete_support
                    text: qsTr("Delete")
                    /*enabled: addSuportMode*/
                    width: add_support.width
                    height: add_support.height
                    btnRadius:3
                    onSigButtonClicked: {
                        control.deleteMode();
                        /*addSuportMode=false;*/
                        enabled=false;
                        add_support.enabled=true;
                    }
                }
            }
        }

        Item {
            width: idRoot.width -3
            height : 2
            
            Rectangle {
                // anchors.left: idCol.left
                // anchors.leftMargin: -10
                x: -9
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColorDark
                }
            }
        }

        Rectangle{
            width:240 * screenScaleFactor
            height:30 * screenScaleFactor
            color:"transparent"
            BasicButton {
                id: clear_support
                x: 10
                text: qsTr("Clear All Supports")
                btnRadius:3
                width: 230
                height: 28
                onSigButtonClicked: {
                    control.removeAll();
                }
            }
        }

        Item {
            width: idRoot.width -3
            height : 2
            
            Rectangle {
                // anchors.left: idCol.left
                // anchors.leftMargin: -10
                x: -9
                width:parent.width > parent.height ?  parent.width : 2
                height: parent.width > parent.height ?  2 : parent.height
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width > parent.height ? parent.width : 1
                    height: parent.width > parent.height ? 1 : parent.height
                    color: Constants.splitLineColorDark
                }
            }
        }

//        Rectangle{
//            width:240
//            height:80
//            color:"transparent"
//            BasicButton {
//                id: idbackbtn
//                x: 10
//                text: qsTr("Back to Printer Config")
//                btnRadius:3
//                width: 230
//                height: 28
//                onSigButtonClicked: {
//                    //control.onSliceClicked(false)
//                    control.backToPrinterConfig()
//                }
//            }
//        }

        

        // Rectangle{
        //     width:240
        //     height:60
        //     color:"transparent"
        // }

        // BasicGroupBox
        // {
        //     width: parent.width
        //     height : 50
        //     BasicButton {
        //         id: clear_support
        //         text: qsTr("Clear All Supports")
        //         btnRadius:3
        //         width: 240
        //         height: 28
        //         onSigButtonClicked: {
        //             control.removeAll();
        //         }
        //     }
        // }
//        Rectangle{
//            width: 250
//            height: 70
//            color:"transparent"
//            x:20
//            SliceExecute
//            {
//                id:idSliceBtn
//                text : qsTr("Start Slice")
//                anchors.horizontalCenter: parent.horizontalCenter
//                width:230
//                height:48
//                btnRadius : 3
//                enabled : true
//                onSigButtonClicked: {
//                    control.onSliceClicked(true)
//                }
//                onEnabledChanged:
//                {
//                    if(enabled == false)
//                    {
//                        control.onSliceClicked(false)
//                    }
//                }
//            }
//        }
    }
    UploadMessageDlg{
        id: idHasSupport
        visible: false
        //cancelBtnVisible: false
        messageType:1
        msgText: qsTr("Do you want to clear existing  supports before create new supports?")

        onSigOkButtonClicked:
        {
            idHasSupport.visible = false
            control.removeAll();
            control.autoAddSupport(parseFloat(support_grid_text.text),parseFloat(overhang_angle_text.text),platform_checkbox.checked);
        }
    }
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
