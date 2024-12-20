import QtQuick 2.0
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import QtQml 2.13

import ".."
import "../qml"


Popup {
    id: mouseOptPopup
    padding: 0
    margins: 0
    background: null

    width: 340 * screenScaleFactor
    height: 278 * screenScaleFactor
    // closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    closePolicy: Popup.NoAutoClose

    Connections {
        target: kernel_parameter_manager
        function onFunctionTypeChanged(){
            mouseOptPopup.close()
        }
    }

    Connections {
        target: kernel_kernel
        function onCurrentPhaseChanged(){
            mouseOptPopup.close()
        }
    }

    contentItem: Rectangle {
        radius: 5
        visible: true
        id: mouseOptRect

        border.width: 1
        border.color: Constants.currentTheme ? "#D6D6DC" : "#56565C"
        color: Constants.currentTheme ? "#F2F2F5" : "#363638"

        ColumnLayout {
            spacing: 0
            anchors.fill: parent

            CusPopViewTitle {
                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                Layout.preferredHeight: 24 * screenScaleFactor
                Layout.alignment: Qt.AlignHCenter

                radius: 5
                leftTop: true
                rightTop: true
                maxBtnVis: false
                clickedable: false
                //closeBtnVis: false
                title: qsTr("Model List")
                color: mouseOptRect.color
                fontColor: Constants.themeTextColor
                borderColor: mouseOptRect.border.color
                closeBtnNColor: "transparent"//color
                closeBtnWidth: 8 * screenScaleFactor
                closeBtnHeight: 8 * screenScaleFactor
                titleLeftMargin: 6 *screenScaleFactor
                closeIcon: "qrc:/UI/photo/closeBtn.svg"
                closeIcon_d: "qrc:/UI/photo/closeBtn_d.svg"

                onCloseClicked: mouseOptPopup.close()
            }

            ColumnLayout {
                spacing: 10 * screenScaleFactor
                Layout.leftMargin: 11 * screenScaleFactor
                Layout.rightMargin: 11 * screenScaleFactor

                Item{
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20 * screenScaleFactor
                    CusText{
                        hAlign: 0
                        fontText: qsTr("Name")
                        fontColor: Constants.themeTextColor
                        fontPointSize: Constants.labelFontPointSize_9
                        anchors.left: parent.left
                        anchors.leftMargin: 20 * screenScaleFactor
                        anchors.bottom: parent.bottom
                    }

                    CusText{
                        hAlign: 2
                        fontText: qsTr("Filament")
                        fontColor: Constants.themeTextColor
                        fontPointSize: Constants.labelFontPointSize_9
                        anchors.right: parent.right
                        anchors.rightMargin: 50 * screenScaleFactor
                        anchors.bottom: parent.bottom
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 318 * screenScaleFactor
                    Layout.preferredHeight: 204 * screenScaleFactor
                    Layout.bottomMargin: 10 * screenScaleFactor

                    radius: 5
                    clip: true
                    border.width: 1
                    border.color: mouseOptRect.border.color
                    color: mouseOptRect.color

                    ModelListView {
                        id: modelListView

                        width: parent.width - 2 * screenScaleFactor
                        height: parent.height - 2 * screenScaleFactor
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 5 * screenScaleFactor
                    }
                }
            }
        }
    }
}
