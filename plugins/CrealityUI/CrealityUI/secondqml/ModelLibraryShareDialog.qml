import QtQuick 2.0
import QtQuick 2.10
import QtQuick.Controls 2.12
//import QtQuick.Controls 2.0 as QQC2
//import QtQuick.Layouts 1.3
import QtQuick.Window 2.0
import QtGraphicalEffects 1.12

import ".."
import "../qml"
Window {
    id: eo_askDialog
    width: 398 + 10
    height: 138 + 10
    flags: Qt.FramelessWindowHint | Qt.Dialog
   // A model window prevents other windows from receiving input events. Possible values are Qt.NonModal (the default), Qt.WindowModal, and Qt.ApplicationModal.
    modality: Qt.ApplicationModal
    color:"transparent" //Constants.themeColor

    property string contentBackground: Constants.dialogContentBgColor
    property var bgImage: "qrc:/UI/photo/mode_library_share_bg.png"
    property var modelId: ""

    /* 追踪某个控件(在target控件下方显示) */
    function trackTo(target) {
        if (!target)
            return

        let globalPos = target.mapToGlobal(0, 0)
        x = globalPos.x - width / 2
        y = globalPos.y + target.height

        visible = true
    }

    function setId(id) {
        modelId = id
        idShareLink.text = "  " + cxkernel_cxcloud.modelGroupUrlHead + id
    }

    Image{
        width: parent.width
        height: parent.height
        mipmap: true
        smooth: true
        cache: false
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: bgImage
    }

    Column {
        x: 21 + 10
        y: 36
        spacing: 10
        Row {
            spacing: 9
            StyledLabel {
                width: 37 * screenScaleFactor
                height: 30 * screenScaleFactor
                font.pointSize: Constants.labelFontPointSize_9
                text: qsTr("Link: ")
                color: "#333333"
                verticalAlignment: Text.AlignVCenter
            }
            StyledLabel {
                id: idShareLink
                width: 304 * screenScaleFactor
                height: 30 * screenScaleFactor
                font.pointSize: Constants.labelFontPointSize_9
                text: ""
                color: "#333333"
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.width: 1
                    border.color: "#E7E7E7"
                }
            }
        }
        Row {
            x: 45
            spacing: 24
            BasicButton {
                width: 140 * screenScaleFactor
                height: 36 * screenScaleFactor
                text: qsTr("Copy Link")
                btnRadius: 18 * screenScaleFactor
                btnBorderW: 0
                pointSize: Constants.labelFontPointSize_9
                // defaultBtnBgColor: "#BBBBBB"
                // hoveredBtnBgColor: "#1E9BE2"
                btnTextColor: "white"
                defaultBtnBgColor: "#1E9BE2"
                hoveredBtnBgColor: "#1EB6E2"
                onSigButtonClicked: {
                    cxkernel_cxcloud.modelLibraryService.shareModelGroup(eo_askDialog.modelId)
                    eo_askDialog.visible = false
                }
            }
            BasicButton {
                width: 140 * screenScaleFactor
                height: 36 * screenScaleFactor
                text: qsTr("Cancel")
                btnRadius: 18 * screenScaleFactor
                btnBorderW: 0
                pointSize: Constants.labelFontPointSize_9
                btnTextColor: "white"
                defaultBtnBgColor: "#BBBBBB"
                hoveredBtnBgColor: "#1E9BE2"

                onSigButtonClicked: {
                    eo_askDialog.visible = false
                }
            }
        }
    }

}
