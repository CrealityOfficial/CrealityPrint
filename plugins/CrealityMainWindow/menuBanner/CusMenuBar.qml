import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.5
import QtQml 2.13
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"

Rectangle {
    id: root
    width: parent.width
    height: 32 * screenScaleFactor

    gradient: Gradient {
        GradientStop {  position: 0.0; color: Constants.themeColor_secondary  }
        GradientStop {  position: 1.0; color: Constants.themeColor_primary }
    }

    property var mainWindow: ""
    property bool isMaxed: true

    signal closeWindow()
    signal changeServer(int serverIndex)

    Rectangle {
        id: idBottom
        height: 1
        width: root.width
        color: Constants.themeColor_secondary
        anchors.top: root.bottom
    }

    Image {
        id: logoImage
        anchors.left: root.left
        anchors.leftMargin: 12
        anchors.verticalCenter: root.verticalCenter
        height: 24 * screenScaleFactor
        width: 24 * screenScaleFactor
        source: "qrc:/res/img/logo.svg"
        objectName: "blankItem"
    }

    CusMenuBanner {
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

        CusButton {
            id: closeBtn
            anchors.verticalCenter: parent.verticalCenter
            width: 46 * screenScaleFactor
            height: 32 * screenScaleFactor
        }

        CusButton {
            id: maxBtn
            anchors.verticalCenter: parent.verticalCenter
            width: closeBtn.width
            height: closeBtn.height
        }

        CusButton{
            id:minBtn;
            anchors.verticalCenter: parent.verticalCenter
            width: closeBtn.width
            height: closeBtn.height
        }
    }
}
