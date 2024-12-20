//import CrealityUI 1.0
//import "qrc:/CrealityUI"
import ".."
import "../qml"
import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12

BasicDialog {
    id: informationDlg

    property alias messageText: idPanelname.text
    property alias width_x: informationDlg.width
    property alias width_y: informationDlg.height
    property alias yesBtnText: add_support.text
    property alias noBtnText: cancel_support.text
    property alias ignoreBtnText: ignore_support.text
    property alias checkedText: label1.text
    property alias checkedFlag: idPopup.checked
    property var isInfo: true
    property var btnCount: 1
    property var showChecked: false

    //radius : 5
    // color: Constants.itemBackgroundColor
    signal accept()
    signal cancel()
    signal ignore()
    signal closeDlg()

    function showSingleBtn() {
        cancel_support.visible = false;
        ignore_support.visible = false;
        btnCount = 1;
        informationDlg.width = 315 * screenScaleFactor;
    }

    function showDoubleBtn() {
        cancel_support.visible = true;
        ignore_support.visible = false;
        btnCount = 2;
        informationDlg.width = 480 * screenScaleFactor;
    }

    function showTripleBtn() {
        cancel_support.visible = true;
        ignore_support.visible = true;
        btnCount = 3;
        informationDlg.width = 480 * screenScaleFactor;
    }

    function showCheckBox(isShow) {
        showChecked = isShow;
        checkedFlag = false;
    }

    width: 315 * screenScaleFactor
    height: 190 * screenScaleFactor
    title: qsTr("Message")
    onDialogClosed: {
        closeDlg();
    }

    Image {
        id: imageRepair

        x: parent.width / 2 - width / 2
        // width: 52/*60*/
        // height: 44/*50*/
        height: sourceSize.height
        width: sourceSize.width
        source: isInfo ? "qrc:/UI/images/information.png" : "qrc:/UI/images/warning.png"
        fillMode: Image.PreserveAspectFit //拉伸是缩放 总是显示完整图片
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: idPanelname.contentWidth * -0.5
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -30
    }

    Text {
        id: idPanelname

        x: 65 * screenScaleFactor
        //y: 100
        anchors.left: imageRepair.right
        anchors.leftMargin: 10* screenScaleFactor
        anchors.verticalCenter: imageRepair.verticalCenter
        text: qsTr("Save and exit ?")
        color: Constants.textColor
        width:300* screenScaleFactor
        font.family: Constants.mySystemFont.name
        font.pointSize: Constants.labelFontPointSize_9 // panelFontSize
        wrapMode: Text.WordWrap
    }

    // Item {
    //     id: idSeparator

    //     anchors.left: parent.left
    //     anchors.leftMargin: 20 * screenScaleFactor
    //     anchors.top: imageRepair.bottom
    //     anchors.topMargin: 20 * screenScaleFactor
    //     width: parent.width - 20 * screenScaleFactor
    //     height: 2 * screenScaleFactor

    //     BasicSeparator {
    //         width: parent.width - 20 * screenScaleFactor
    //         height: 2 * screenScaleFactor
    //     }

    // }

    RowLayout {
        spacing: 5 * screenScaleFactor

        anchors.bottom: add_support.top
        anchors.horizontalCenter: parent.horizontalCenter

        StyleCheckBox {
            // onCheckedChanged:{}
            // onClicked: {}

            id: idPopup

            // anchors.left: idSeparator.left
            // anchors.top: idSeparator.top
            // anchors.topMargin: 10 * screenScaleFactor
            Layout.minimumHeight: 17 * screenScaleFactor
            Layout.minimumWidth: 17 * screenScaleFactor
            checked: false
            visible: showChecked
        }

        StyledLabel {
            id: label1

            // anchors.top: idPopup.top
            // anchors.topMargin: 2 * screenScaleFactor
            // anchors.left: idPopup.right
            // anchors.leftMargin: 5 * screenScaleFactor
            Layout.minimumHeight: 50 * screenScaleFactor
            Layout.minimumWidth: 300 * screenScaleFactor
            text: qsTr("Next time it will not pop up, remember that?")
            wrapMode: Text.WordWrap
            font.pointSize: Constants.labelFontPointSize_9
            visible: showChecked
        }
    }

    //by TCJ
    BasicDialogButton {
        //x: parent.width/2-width

        id: add_support

        text: qsTr("yes")
        //        btnTextColor : "#E3EBEE"
        //        hoveredBtnBgColor:  Constants.hoveredColor
        //        defaultBtnBgColor : Constants.buttonColor
        width: parent.width > 250 * screenScaleFactor ? 110 * screenScaleFactor : (parent.width - 30 * screenScaleFactor) / 2
        height: 30 * screenScaleFactor
        btnRadius: height / 2
        btnBorderW: 0
        defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 18 //10
        anchors.left: parent.left
        anchors.leftMargin: informationDlg.width / 2 - (add_support.width * btnCount + 25 * screenScaleFactor * (btnCount - 1)) / 2
        onSigButtonClicked: {
            close();
            accept();
        }
    }

    BasicDialogButton {
        id: cancel_support

        text: qsTr("no")
        width: add_support.width
        height: 30 * screenScaleFactor
        anchors.left: add_support.right
        anchors.top: add_support.top
        anchors.leftMargin: 15 * screenScaleFactor
        btnRadius: height / 2
        btnBorderW: 0
        defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
        onSigButtonClicked: {
            close();
            cancel();
        }
    }

    BasicDialogButton {
        id: ignore_support

        text: qsTr("ignore")
        width: add_support.width
        height: 30 * screenScaleFactor
        anchors.left: cancel_support.right
        anchors.top: cancel_support.top
        anchors.leftMargin: 15 * screenScaleFactor
        btnRadius: height / 2
        btnBorderW: 0
        defaultBtnBgColor: Constants.profileBtnColor
        hoveredBtnBgColor: Constants.profileBtnHoverColor
        onSigButtonClicked: {
            close();
            ignore();
        }
    }

}
