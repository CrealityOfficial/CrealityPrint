import QtQuick 2.10
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.12

import QtGraphicalEffects 1.13

import "."
import "../qml"

Item {
    id: root

    property string groupId: ""
    property string groupName: ""
    property string groupImage: ""
    property int    modelCount: 0
    property string authorName: ""
    property string authorHead: ""
    property int    totalPrice: 0
    property bool   collected: false
    property int    createdTimestamp: 0

    property bool imageVisible: true

    signal sigButtonClicked(var group_id)
    signal sigButtonDownClicked(var group_id)
    signal sigDownloadedTip()
    signal sigLoginTip()

    function onResetDownloadButton() {
        download_button.visible = true
        download_button.checked = false
    }

    function onDownloadFinished() {
        download_button.visible = false
    }

    width: 286 * screenScaleFactor
    height: 360 * screenScaleFactor

    Button {
        id: btnModelItem
        anchors.fill: parent

        onClicked: {
            if (root.totalPrice !== 0) {
                Qt.openUrlExternally(cxkernel_cxcloud.modelGroupUrlHead + root.groupId)
                return
            }

            sigButtonClicked(groupId)
        }

        background: Item {}

        contentItem: Rectangle {
            id: button_content

            x: 0
            y: parent.hovered ? 0 : 10 * screenScaleFactor
            width: root.width
            height: root.height

            Behavior on y {
                NumberAnimation {
                    duration: 100
                }
            }

            radius: 10 * screenScaleFactor
            border.width: 3 * screenScaleFactor
            border.color: parent.hovered ? "#1E9BE2" : Constants.themeColor_primary

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: parent.border.width

                BaseCircularImage {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.minimumWidth: parent.width
                    Layout.minimumHeight: Layout.minimumWidth
                    radiusImg: button_content.radius - 5
                    isPreserveAspectCrop: true
                    isLeftBottomRadius: false
                    isRightBottomRadius: false
                    img_src: imageVisible ? groupImage : null
                }

                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.margins: 10 * screenScaleFactor

                    Label {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true

                        text: groupName
                        elide: Text.ElideRight
                        font.weight: Font.Bold
                        font.family: Constants.labelFontFamily
                        font.pointSize: Constants.labelFontPointSize_12
                        color: "#333333"
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignBottom
                        Layout.fillWidth: true

                        BaseCircularImage {
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.minimumWidth: 24 * screenScaleFactor
                            Layout.minimumHeight: Layout.minimumWidth

                            radiusImg: button_content.radius
                            img_src: imageVisible ? authorHead : null
                        }

                        Label {
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.leftMargin: 2 * screenScaleFactor
                            Layout.fillWidth: true

                            text: authorName
                            elide: Text.ElideRight
                            font.family: Constants.labelFontFamily
                            font.pointSize: Constants.labelFontPointSize_9
                            color: "#666666"
                        }

                        Button {
                            id: download_button

                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            Layout.leftMargin: 10 * screenScaleFactor
                            Layout.minimumWidth: 40 * screenScaleFactor
                            Layout.minimumHeight: 24 * screenScaleFactor

                            visible: root.totalPrice === 0

                            BasicTooltip {
                                id: undoTooptip
                                visible: download_button.hovered && String(text).length
                                text: qsTr("Download")
                            }

                            background: Rectangle {
                                anchors.fill: parent
                                radius: height / 2
                                color: download_button.hovered ? "#00A3FF" : "#E2F5FF"

                                Image {
                                    anchors.centerIn: parent
                                    width: 14 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    source: download_button.checked ? "qrc:/UI/photo/loading_g.gif"
                                                                    : download_button.hovered ? "qrc:/UI/photo/model_library_download_h.png"
                                                                                              : "qrc:/UI/photo/model_library_download_l.png"
                                }
                            }

                            onClicked: {
                                if (!cxkernel_cxcloud.downloadService.checkModelGroupDownloadable(groupId)) {
                                    sigDownloadedTip()
                                    return
                                }

                                if (!cxkernel_cxcloud.accountService.logined) {
                                    cxkernel_cxcloud.downloadService.makeModelGroupDownloadPromise(groupId)
                                    sigLoginTip()
                                    return
                                }

                                sigButtonDownClicked(groupId)
                            }
                        }

                        Button {
                            id: buy_button

                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            Layout.leftMargin: 10 * screenScaleFactor
                            Layout.minimumWidth: 60 * screenScaleFactor
                            Layout.minimumHeight: 24 * screenScaleFactor

                            visible: root.totalPrice !== 0

                            background: Rectangle {
                                anchors.fill: parent
                                radius: height / 2
                                color: "#FF6131"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8 * screenScaleFactor
                                    anchors.rightMargin: 8 * screenScaleFactor

                                    spacing: 4 * screenScaleFactor

                                    Image {
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                        Layout.minimumWidth: 14 * screenScaleFactor
                                        Layout.minimumHeight: 14 * screenScaleFactor
                                        sourceSize.width: 14 * screenScaleFactor
                                        sourceSize.height: 14 * screenScaleFactor
                                        source: "qrc:/UI/photo/coin.png"
                                    }

                                    Text {
                                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignLeft

                                        color: "#FFFFFF"
                                        text: root.totalPrice
                                        font.pointSize: Constants.labelFontPointSize_9
                                        font.family: Constants.labelFontFamily
                                        font.weight: Font.Bold
                                    }
                                }
                            }

                            onClicked: {
                                Qt.openUrlExternally(cxkernel_cxcloud.modelGroupUrlHead + root.groupId)
                            }
                        }
                    }
                }
            }
        }
    }

    DropShadow {
        id: idDropShadow
        anchors.fill: btnModelItem
        visible: btnModelItem.hovered ? true : false
        radius: button_content.radius
        samples: 30
        source: btnModelItem
        color: Constants.dropShadowColor
    }
}
