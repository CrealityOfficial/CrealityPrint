import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.5

import ".."
import "../secondqml"

Rectangle {
    id: root
    gradient: Gradient {
        GradientStop {  position: 0.0; color: Constants.headerBackgroundColor  }
        GradientStop {  position: 1.0; color: Constants.headerBackgroundColorEnd }
    }

    property var mainWindow: ""
    property bool isMaxed: frameLessView.isMax

    signal requestFirstConfig();
    signal closeWindow()
    signal changeServer(int serverIndex)

    Item {
        id: blankItem
        objectName: "blankItem"
        anchors {
            left: idMenuBar.right
            leftMargin: 4* screenScaleFactor
            right: idControlButtonsRow.left
            rightMargin: 4* screenScaleFactor
            top: parent.top
            topMargin: 4* screenScaleFactor
            bottom: parent.bottom
        }
        Component.onCompleted: {
            frameLessView.setTitleItem(blankItem)
        }
    }
    function startWizardComp() {
        var pRoot = rootLoader
        while (pRoot.parent !== null) {
            pRoot = pRoot.parent
        }
        wizardComp.createObject(pRoot, {model: rootLoader.item.wizardModel })
    }

    function startFirstConfig() {
        requestFirstConfig()
        // idStartFirstConfig.visible = true
    }

    Rectangle {
        id: idBottom
        height: 1
        width: root.width
        color: Constants.mainViewSplitLineColor
        anchors.top: root.bottom
    }

    Image {
        id: logoImage
        anchors.left: root.left
        anchors.leftMargin: 12* screenScaleFactor
        anchors.verticalCenter: root.verticalCenter
        height: 24 * screenScaleFactor
        width: 24 * screenScaleFactor
        source: "qrc:/scence3d/res/logo.png"
    }

    BasicMenuBarStyle {
        id: idMenuBar
        width: contentWidth
        height: root.height
        objectName: "mybasicmenubar"

        anchors.left: logoImage.right
        anchors.leftMargin: 10 * screenScaleFactor
        anchors.verticalCenter: parent.verticalCenter

        background: Item
        {
            anchors.fill: parent
        }
    }

    Row {
        id: idControlButtonsRow
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 1
        spacing: 1
        layoutDirection: Qt.RightToLeft
        objectName: "controlButtonsRow"

        TitleBarBtn {
            id: closeBtn
            anchors.verticalCenter: parent.verticalCenter
            width: 46 * screenScaleFactor
            height: 32 * screenScaleFactor
            hoverBgColor: "#FD265A"
            borderwidth:0
            buttonRadius:0
            iconWidth: 12*  screenScaleFactor
            iconHeight: 12 *  screenScaleFactor
            normalIconSource: "qrc:/UI/photo/closeBtn_main.svg"

            onClicked: {
                frameLessView.close()
            }
        }

        TitleBarBtn {
            property string maxPath: "qrc:/UI/photo/maxBtn_main.svg"
            property string normalPath: "qrc:/UI/photo/normalBtn_main.svg"
            id: maxBtn
            anchors.verticalCenter: parent.verticalCenter
            width: closeBtn.width
            height: closeBtn.height
            borderwidth: 0
            buttonRadius: 0
            normalIconSource: isMaxed ? normalPath : maxPath
            iconWidth: 12 * screenScaleFactor
            iconHeight: 12 * screenScaleFactor

            onClicked: {
                if(isMaxed)
                {
//                    isMaxed = false
                    frameLessView.showNormal()
                }
                else {
//                    isMaxed = true
                    frameLessView.showMaximized()
                }
            }
        }

        TitleBarBtn{
            id:minBtn;
            anchors.verticalCenter: parent.verticalCenter
            width: closeBtn.width
            height: closeBtn.height
            borderwidth:0
            buttonRadius:0
            normalIconSource: "qrc:/UI/photo/minBtn_main.svg"
            iconWidth: 12 * screenScaleFactor
            iconHeight: 2 * screenScaleFactor

            onClicked: {
                frameLessView.showMinimized()
            }
        }
    }

    // StartFirstConfig {
    //     visible: false
    //     id: idStartFirstConfig
    // }

    Loader {
        id:rootLoader
        source: "../secondqml/CusWizardHome.qml"
    }

    Component {
        id: wizardComp
        CusWizard {
            id: cusWizard
            anchors.fill: parent
            currentIndex: 0
            onWizardFinished: {
                destroy(cusWizard)
                showAddPrinterDlg()
            }
        }
    }
}
