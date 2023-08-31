import QtQuick 2.0
import QtQuick.Window 2.1
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.12
Window {
    id: splash
    color: "transparent"
    title: qsTr("Splash Window")
    modality: Qt.ApplicationModal
    flags: Qt.SplashScreen
    property int timeoutInterval: 2000
    signal timeout
    x: (Screen.width - splash.width) / 2
    y: (Screen.height - splash.height) / 2

    width: 876 * screenScaleFactor//splashImage.width
    height: 480 * screenScaleFactor//splashImage.height
    //property alias versionText: idVersion.text

    readonly property string context: cxkernel_const.translateContext
    readonly property string introduction: qsTranslate(context, "offical_brief_introduction")
    readonly property bool introductionVisible: introduction !== "offical_brief_introduction"

    Rectangle{
        id: mainWindowRext
        color: "transparent"
        anchors.fill: parent
        Image {
            id: splashImage
            source: "qrc:/scence3d/res/splash.png"
            width: 876 * screenScaleFactor
            height: 480 * screenScaleFactor
            // MouseArea {
            //     anchors.horizontalCenter: parent.horizontalCenter
            //     anchors.verticalCenter: parent.verticalCenter
            //     onClicked: Qt.quit()
            // }

            TextEdit {//by TCJ
                anchors.left: parent.left
                anchors.leftMargin: 20* screenScaleFactor
                anchors.top: parent.top
                anchors.topMargin: 200* screenScaleFactor
                id: idTextLine4
                visible: splash.introductionVisible
                text: splash.introduction
                color: "#a3c2d4"
                font.pointSize: Constants.labelFontPointSize_9
                font.family: "Microsoft YaHei UI"//"Source Han Sans CN"
                width: 174 * screenScaleFactor
                height: 60 * screenScaleFactor
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.AutoText
            }
        }
    }
    Component.onCompleted: visible = true
}
