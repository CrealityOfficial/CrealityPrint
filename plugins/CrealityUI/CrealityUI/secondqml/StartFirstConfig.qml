import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Window {
    id: root
    color: "transparent"
    width: 500 * screenScaleFactor
    height: 196 * screenScaleFactor
    modality: Qt.ApplicationModal
    flags: Qt.FramelessWindowHint | Qt.Dialog

    property var translator: null

    property real borderWidth: 1 * screenScaleFactor
    property real shadowWidth: 5 * screenScaleFactor
    property real titleHeight: 30 * screenScaleFactor
    property real borderRadius: 5 * screenScaleFactor

    property string shadowColor: Constants.currentTheme ? "#BBBBBB" : "#333333"
    property string borderColor: Constants.currentTheme ? "#D6D6DC" : "#262626"
    property string titleFtColor: Constants.currentTheme ? "#333333" : "#FFFFFF"
    property string titleBgColor: Constants.currentTheme ? "#E9E9EF" : "#6E6E73"
    property string backgroundColor: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"

    function updateLanguage(index) {
        if (kernel_global_const.cxcloudEnabled) {
            translator = languageServerModel.get(index)
            curTitleText.text = translator.title_text
            textLanguage.text = translator.text_language
            textServer.text = translator.text_server
            textConfirm.text = translator.text_confirm
            idServerComboBox.model = translator.cbox_server_model
        } else {
            translator = languageModel.get(index)
            curTitleText.text = translator.title_text
            textLanguage.text = translator.text_language
            textConfirm.text = translator.text_confirm
        }
    }

    /* 保存并完成第一次配置 */
    function saveAndFinishConfig() {
        cxkernel_cxcloud.serverType = idServerComboBox.currentIndex
        kernel_ui.changeLanguage(idLanguageComboBox.currentIndex)
        kernel_ui.firstStartConfigFinished()
        kernel_ui.requestQmlDialog("idAddPrinterDlgNew")
        root.hide()
    }

    Component.onCompleted: updateLanguage(kernel_ui.currentLanguage)

    ListModel {
        id: languageServerModel
        ListElement {
            title_text: "Please select language and server"
            text_language: "Language:"
            text_server: "Locale:"
            text_confirm: "OK"
            cbox_server_model:
            [
                ListElement { modelData: "China" },
                ListElement { modelData: "Asia-Pascific" },
                ListElement { modelData: "Europe" },
                ListElement { modelData: "North America" },
                ListElement { modelData: "South America" },
                ListElement { modelData: "Others"}
            ]
        }
        ListElement {
            title_text: "请选择语言和区域"
            text_language: "选择语言："
            text_server: "选择区域："
            text_confirm: "确定"
            cbox_server_model:
            [
                ListElement { modelData: "中国" },
                ListElement { modelData: "亚太" },
                ListElement { modelData: "欧洲" },
                ListElement { modelData: "北美" },
                ListElement { modelData: "南美" },
                ListElement { modelData: "其他 "}
            ]
        }
        ListElement {
            title_text: "請選擇語言和區域"
            text_language: "選擇語言："
            text_server: "選擇區域："
            text_confirm: "確定"
            cbox_server_model:
            [
                ListElement { modelData: "中國" },
                ListElement { modelData: "亞太" },
                ListElement { modelData: "歐洲" },
                ListElement { modelData: "北美" },
                ListElement { modelData: "南美" },
                ListElement { modelData: "其他"}
            ]
        }
    }

    ListModel {
        id: languageModel
        ListElement {
            title_text: "Please select language"
            text_language: "Language:"
            text_confirm: "OK"
        }
        ListElement {
            title_text: "请选择语言"
            text_language: "选择语言："
            text_confirm: "确定"
        }
        ListElement {
            title_text: "請選擇語言"
            text_language: "選擇語言："
            text_confirm: "確定"
        }
    }

    Rectangle {
        //id: rect
        anchors.fill: parent
        anchors.margins: shadowWidth

        border.width: borderWidth
        border.color: borderColor
        color: backgroundColor
        radius: borderRadius

        CusRoundedBg {
            id: cusTitle
            width: parent.width - 2 * borderWidth
            height: titleHeight

            anchors.top: parent.top
            anchors.topMargin: borderWidth
            anchors.horizontalCenter: parent.horizontalCenter

            leftTop: true
            rightTop: true
            color: titleBgColor
            radius: borderRadius

            MouseArea {
                anchors.fill: parent
                property point clickPos: "0,0"

                onPressed: clickPos = Qt.point(mouse.x, mouse.y)

                onPositionChanged: {
                    var cursorPos = WizardUI.cursorGlobalPos()
                    root.x = cursorPos.x - clickPos.x
                    root.y = cursorPos.y - clickPos.y
                }
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 10 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4 * screenScaleFactor

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: "qrc:/scence3d/res/logo.png"
                    sourceSize: Qt.size(16, 16)
                }

                Text {
                    id: curTitleText
                    anchors.verticalCenter: parent.verticalCenter

                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    verticalAlignment: Text.AlignVCenter
                    color: titleFtColor
                }
            }

            CusButton {
                id: closeBtn
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor

                rightTop: true
                allRadius: false
                normalColor: "transparent"
                hoveredColor: "#E81123"

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    anchors.centerIn: parent
                    width: 10 * screenScaleFactor
                    height: 10 * screenScaleFactor
                    source: parent.isHovered ? "qrc:/UI/photo/closeBtn_d.svg" : "qrc:/UI/photo/closeBtn.svg"
                }

                onClicked: idAskSaveDlg.show()
            }
        }
        
        UploadMessageDlg {
            id: idAskSaveDlg
            visible: false
            cancelBtnVisible: true
            ignoreBtnVisible: false
            okBtnText: "OK"
            cancelBtnText: "Cancel"
            msgText: "Do you want to save the current settings?"
            onSigOkButtonClicked: {
                idAskSaveDlg.hide()
                saveAndFinishConfig()
            }

            onSigCancelButtonClicked: {
                idAskSaveDlg.hide()
            }
        }

        Item {
            width: parent.width
            height: parent.height - cusTitle.height

            anchors.top: cusTitle.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            Column {
                id: contentColumn
                anchors.top: parent.top
                anchors.topMargin: 30 * screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 10 * screenScaleFactor

                Row {
                    spacing: 20 * screenScaleFactor

                    Text {
                        id: textLanguage
                        width: 65 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        verticalAlignment: Text.AlignVCenter
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color: Constants.currentTheme ? "#333333" : "#CBCBCC"
                    }

                    ComboBox {
                        id: idLanguageComboBox
                        width: 375 * screenScaleFactor
                        height: 28 * screenScaleFactor

                        onCurrentIndexChanged: updateLanguage(currentIndex)
                        currentIndex: kernel_ui.currentLanguage

                        model: ["English", "简体中文", "繁體中文","한국어/Korean"]

                        delegate: ItemDelegate {
                            width: idLanguageComboBox.width
                            height: idLanguageComboBox.height
                            contentItem: Rectangle {
                                anchors.fill: parent
                                color: (idLanguageComboBox.highlightedIndex == index) ? "#1E9BE2" : Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                                    text: modelData
                                }
                            }
                        }

                        indicator: Image {
                            width: 7 * screenScaleFactor
                            height: 4 * screenScaleFactor
                            source: (parent.hovered || parent.popup.opened)  ? "qrc:/UI/photo/downBtn_d.svg" : "qrc:/UI/photo/downBtn.svg"

                            anchors.right: parent.right
                            anchors.rightMargin: 10 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        contentItem: Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                            text: idLanguageComboBox.displayText
                        }

                        background: Rectangle {
                            color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                            border.color: Constants.currentTheme ? "#D6D6DC" : "#6E6E72"
                            border.width: 1
                        }

                        popup: Popup {
                            y: idLanguageComboBox.height
                            width: idLanguageComboBox.width
                            implicitHeight: contentItem.implicitHeight + bottomPadding

                            leftPadding: 1; rightPadding: 1
                            bottomPadding: 1; topPadding: 0

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: idLanguageComboBox.delegateModel
                            }

                            background: Rectangle {
                                color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#6E6E72"
                                border.width: 1
                            }
                        }
                    }
                }

                Row {
                    spacing: 20 * screenScaleFactor

                    visible: kernel_global_const.cxcloudEnabled

                    Text {
                        id: textServer
                        width: 65 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter
                        verticalAlignment: Text.AlignVCenter
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color: Constants.currentTheme ? "#333333" : "#CBCBCC"
                    }

                    ComboBox {
                        id: idServerComboBox
                        width: 375 * screenScaleFactor
                        height: 28 * screenScaleFactor

                        currentIndex: cxkernel_cxcloud.serverType

                        delegate: ItemDelegate {
                            width: idServerComboBox.width
                            height: idServerComboBox.height
                            contentItem: Rectangle {
                                anchors.fill: parent
                                color: (idServerComboBox.highlightedIndex == index) ? "#1E9BE2" : Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                                    text: modelData
                                }
                            }
                        }

                        indicator: Image {
                            width: 7 * screenScaleFactor
                            height: 4 * screenScaleFactor
                            source: (parent.hovered || parent.popup.opened) ? "qrc:/UI/photo/downBtn_d.svg" : "qrc:/UI/photo/downBtn.svg"

                            anchors.right: parent.right
                            anchors.rightMargin: 10 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        contentItem: Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                            text: idServerComboBox.displayText
                        }

                        background: Rectangle {
                            color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                            border.color: Constants.currentTheme ? "#D6D6DC" : "#6E6E72"
                            border.width: 1
                        }

                        popup: Popup {
                            y: idServerComboBox.height
                            width: idServerComboBox.width
                            implicitHeight: contentItem.implicitHeight + bottomPadding

                            leftPadding: 1; rightPadding: 1
                            bottomPadding: 1; topPadding: 0

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: idServerComboBox.delegateModel
                            }

                            background: Rectangle {
                                color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#6E6E72"
                                border.width: 1
                            }
                        }
                    }
                }
            }

            Button {
                width: 160 * screenScaleFactor
                height: 28 * screenScaleFactor

                anchors.top: contentColumn.bottom
                anchors.topMargin: 20 * screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter

                background: Rectangle {
                    border.width: 1
                    border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                    color: parent.hovered ? (Constants.currentTheme ? "#E8E8ED" : "#59595D") : (Constants.currentTheme ? "#FFFFFF" : "#6E6E73")
                    radius: height / 2
                }

                contentItem: Text
                {
                    id: textConfirm
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    color: Constants.currentTheme ? "#333333" : "#DBDBDC"
                }

                onClicked: saveAndFinishConfig()
            }
        }

        layer.enabled: true

        layer.effect: DropShadow {
            radius: 8
            spread: 0.2
            samples: 17
            color: shadowColor
        }
    }
}
