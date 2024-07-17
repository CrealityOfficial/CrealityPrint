import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15

// import CrealityUI 1.0

// import "qrc:/CrealityUI"

Window {
    id: root

    x: (Screen.width - root.width) / 2
    y: (Screen.height - root.height) / 2
    width: 860 * screenScaleFactor
    height: 480 * screenScaleFactor

    modality: Qt.ApplicationModal
    flags: Qt.SplashScreen
    color: "transparent"

    Image {
        id: image
        anchors.fill: parent
        sourceSize.width: width
        sourceSize.height: height
        source: "qrc:/scence3d/res/splash.png"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 211 * screenScaleFactor
        anchors.bottomMargin: 29 * screenScaleFactor
        anchors.leftMargin: 33 * screenScaleFactor
        anchors.rightMargin: 631 * screenScaleFactor

        spacing: 30 * screenScaleFactor

        Text {
            id: description_text

            readonly property string introduction: qsTranslate(cxkernel_const.translateContext,
                                                               "offical_brief_introduction")

            visible: introduction !== "offical_brief_introduction"

            Layout.fillWidth: true
            Layout.fillHeight: true
            verticalAlignment: Qt.AlignTop
            horizontalAlignment: Qt.AlignHCenter

            wrapMode: Text.WordWrap
            // font: Constants.font
            color: "#666666"
            text: introduction
        }

        Text {
            id: version_text

            Layout.fillWidth: true
            Layout.minimumHeight: contentHeight
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignHCenter

            wrapMode: Text.NoWrap
            // font: Constants.font
            color: "#666666"
            text: "%1: %2".arg(qsTr("Version")).arg(cxkernel_const.version)
        }
    }
}
