import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "../qml"
import "../components"

BasicDialogV4 {
    id: root

    property string msgText: "message contect"
    property string otherMsg: ""
    property bool acceptBtnVisible: true
    property bool cancelBtnVisible: true
    property bool ignoreBtnVisible: false
    property var okBtnText: qsTr("Ok")
    property var ignoreBtnText: qsTr("Ignore")
    property var cancelBtnText: qsTr("Cancel")
    property var messageType: 0
    property var isInfo: false
    property var labelPointSize: 9

    signal sigOkButtonClicked()
    signal sigIgnoreButtonClicked()
    signal sigCancelButtonClicked()

    width: 500 * screenScaleFactor
    height: {
        const line_count = msgText.split('\n').length
        return (160 + (line_count - 1) * 15) * screenScaleFactor
    }
    titleHeight: 30 * screenScaleFactor
    title: qsTr("Message")
    maxBtnVis: false

    onVisibleChanged: {}

    bdContentItem: ColumnLayout {
        spacing: 10 * screenScaleFactor

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 10 * screenScaleFactor
            Layout.leftMargin: 10 * screenScaleFactor
            Layout.rightMargin: 10 * screenScaleFactor

            Item {
                Layout.fillWidth: true
            }

            Image {
                Layout.minimumWidth: sourceSize.width
                Layout.maximumWidth: Layout.minimumWidth
                Layout.minimumHeight: sourceSize.height
                Layout.maximumHeight: Layout.minimumHeight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                source: messageType == 0 || isInfo ? "qrc:/UI/photo/UploadSuccess.png"
                      : messageType == 1           ? "qrc:/UI/photo/upload_msg.png"
                                                   : "qrc:/UI/photo/upload_success_image.png"
            }

            StyledLabel {
                Layout.minimumWidth: contentWidth
                Layout.maximumWidth: root.width - 80 * screenScaleFactor
                Layout.minimumHeight: contentHeight
                Layout.maximumHeight: Layout.minimumHeight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.pointSize: labelPointSize
                wrapMode: Text.WordWrap
                text: msgText
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.bottomMargin: 10 * screenScaleFactor
            Layout.leftMargin: 10 * screenScaleFactor
            Layout.rightMargin: 10 * screenScaleFactor

            spacing: 10 * screenScaleFactor

            Item {
                Layout.fillWidth: true
            }

            BasicButton {
                Layout.minimumWidth: 125 * screenScaleFactor
                Layout.maximumWidth: Layout.minimumWidth
                Layout.minimumHeight: 28 * screenScaleFactor
                Layout.maximumHeight: Layout.minimumHeight
                btnRadius: height / 2
                btnBorderW: 0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                visible: acceptBtnVisible
                text: okBtnText
                onSigButtonClicked: {
                    sigOkButtonClicked()
                    root.close()
                }
            }

            BasicButton {
                Layout.minimumWidth: 125 * screenScaleFactor
                Layout.maximumWidth: Layout.minimumWidth
                Layout.minimumHeight: 28 * screenScaleFactor
                Layout.maximumHeight: Layout.minimumHeight
                btnRadius: height / 2
                btnBorderW: 0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                visible: ignoreBtnVisible
                text: ignoreBtnText
                onSigButtonClicked: {
                    sigIgnoreButtonClicked()
                    root.close()
                }
            }

            BasicButton {
                Layout.minimumWidth: 125 * screenScaleFactor
                Layout.maximumWidth: Layout.minimumWidth
                Layout.minimumHeight: 28 * screenScaleFactor
                Layout.maximumHeight: Layout.minimumHeight
                btnRadius: height / 2
                btnBorderW: 0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                visible: cancelBtnVisible
                text: cancelBtnText
                onSigButtonClicked: {
                    sigCancelButtonClicked()
                    root.close()
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
