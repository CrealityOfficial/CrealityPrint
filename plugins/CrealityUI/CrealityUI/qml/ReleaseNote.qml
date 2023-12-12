import QtQuick 2.12
import QtQuick.Controls 2.5

import CrealityUI 1.0

import "qrc:/CrealityUI"
import ".."
import "../qml"
BasicDialog {
    id: root

    width: 580 * screenScaleFactor
    height: 392 * screenScaleFactor
    title : qsTr("Check For Updates")

    //  closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

    property var service: cxkernel_cxcloud.upgradeService

    signal requestNoNewVersionTip

    function autochecked() { service.autoCheck = true }
    function unautochecked() { service.autoCheck = false }
    function skipVersion() { service.skippedVersion = version }
    function exec() {
        manualCheck = true
        service.checkUpgradeable()
    }

    property string projectName: service.onlineVersionName
    property string releaseNote: service.onlineVersionDescription
    property string version: service.onlineVersion
    property string url: service.onlineVersionDownloadUrl
    property bool autocheck: service.autoCheck
    property bool upgradable: service.upgradable

    property bool manualCheck: false

    Component.onCompleted: {
        if (root.autocheck && root.upgradable) {
            root.show()
        }
    }

    Connections {
        target: root.service

        onCheckUpgradableFinished: {
            if (root.upgradable) {
                root.show()
            } else {
                if (root.manualCheck) {
                    root.requestNoNewVersionTip()
                }
            }
        }
    }

    Grid{
        x: 5*screenScaleFactor
        y: 35*screenScaleFactor
        width: parent.width -10
        rows: 3
        //spacing: 5
        Row{
            id:idCtl
            width: parent.width
            height: 68*screenScaleFactor
            spacing: 5*screenScaleFactor
            leftPadding: 21*screenScaleFactor
            Image {
                id: idImage
                width: 26*screenScaleFactor
                height: 26*screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/UI/photo/updateTip.png"
            }
            Text {
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                id: idTips
                width: 295*screenScaleFactor
                height: 30*screenScaleFactor
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Detected a new verion of ")+ projectName + " " + version+qsTr(", Is it updated immediately?")
                font.pointSize: 10//9
                color: Constants.textColor
                anchors.verticalCenter: parent.verticalCenter
            }
            //            Button {
            //                width: 60
            //                height: 30
            //                font.family: Constants.labelFontFamily
            //                font.weight: Constants.labelFontWeight
            //                text: qsTr("Yes")
            //                onClicked: {
            //                    Qt.openUrlExternally(url);
            //                }
            //            }
            Rectangle
            {
                width: 82*screenScaleFactor
                height: 30*screenScaleFactor
                color: "transparent"
            }
            BasicDialogButton
            {
                width: 110*screenScaleFactor
                height: 28*screenScaleFactor
                text: qsTr("Yes")
                btnRadius:14*screenScaleFactor
                btnBorderW:0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                //y: 2
                anchors.verticalCenter: parent.verticalCenter
                onSigButtonClicked:
                {
                    console.log("curversion url:"+ url)
                    Qt.openUrlExternally(url);
                }
            }
        }
        Row {
            id:idReleaseNoteText
            width: parent.width
            height: 224*screenScaleFactor
            ScrollView {
                id: view
                //anchors.fill: parent
                anchors.top: parent.top
                anchors.bottom : parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 21
                anchors.rightMargin: 21
                TextArea {
                    id:control
                    width: parent.width
                    height: 182*screenScaleFactor
                    readOnly:true
                    //textFormat: TextEdit.RichText
                    //placeholderText: qsTr("Enter description")
                    text: releaseNote
                    color:Constants.textColor
                    //text: "text\ntext\ntext\ntext\ntext\ntext\ntext\ntext\ntext\ntext\ntext\ntext\ntext\n"
                    font.pointSize: 10//12
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    //ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                }
                background: Rectangle {
                    implicitWidth: 538*screenScaleFactor
                    implicitHeight: 224*screenScaleFactor
                    radius: 5
                    border.color: control.enabled ? "#7F7F7F" : "transparent"
                    color: "transparent"//"#7F7F7F"
                }
            }
        }
        Row {
            width: parent.width -10
            height: 68*screenScaleFactor
            spacing: 5
            leftPadding: 21
            StyleCheckBox{
                width: 160*screenScaleFactor
                height: 30*screenScaleFactor
                checked: autocheck
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Antomatically check the lastest version at startup")
                onCheckedChanged:
                {
                    if(checked)
                    {
                        autochecked()
                    }
                    else{
                        unautochecked()
                    }
                }
            }

            StyledLabel{
                id: urlLabel
                property string openUrl: cxkernel_cxcloud.serverType ? "https://www.crealitycloud.com/post-detail/6454af4f58f57701c94a56f9" : "https://www.crealitycloud.cn/post-detail/6454a92b716e5ffc532cec31"
                width : 250*screenScaleFactor
                height: 28*screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Check out the changelog")
                color: Constants.leftBtnBgColor_selected
                font.underline: mouseArea.containsMouse
                horizontalAlignment: Text.AlignHCenter
                MouseArea{
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:(containsMouse? Qt.PointingHandCursor: Qt.ArrowCursor);
                    onClicked: Qt.openUrlExternally(urlLabel.openUrl)
                }
            }

            BasicDialogButton
            {
                id : idVersionBtn
                width : 110*screenScaleFactor
                height: 28*screenScaleFactor
                //y: 3
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Skip this version")
                btnRadius:14*screenScaleFactor
                btnBorderW:0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                onSigButtonClicked:
                {
                    skipVersion()
                    root.close()
                }
            }
        }
    }
}
