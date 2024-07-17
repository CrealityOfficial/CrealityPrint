import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.3 as File
import "qrc:/CrealityUI"
import ".."
import "../qml"
import "../components"
DockItem {
    id: root
    width: 756 * screenScaleFactor
    height: (Constants.languageType? 450 : 400) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Preferences")

    BasicMessageDialog {
        id: swtich_engine_dialog
        title: qsTr("Switch the slice engine type")
        messageText: qsTr(
            "Switch the slice engine type to \"%1\" ? The application will be restart immediately."
            .arg(swtich_engine_combobox.currentText))
        yesBtnText: qsTr("Yes")
        noBtnText: qsTr("No")
        Component.onCompleted: {
            showDoubleBtn()
            width = 580 * screenScaleFactor
        }
        onAccept: {
            kernel_slice_flow.setEngineTypeAndRestart(swtich_engine_combobox.currentIndex)
        }
        onCancel: {
            swtich_engine_combobox.currentIndex = kernel_slice_flow.engineType
            swtich_engine_dialog.hide()
        }
    }

    Grid{
        width: parent.width
        height: parent.height
        anchors.left: parent.left
        anchors.leftMargin: 40* screenScaleFactor
        anchors.right: parent.right
        anchors.rightMargin: 40* screenScaleFactor
        anchors.top: parent.top
        anchors.topMargin: 80* screenScaleFactor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70* screenScaleFactor
        columns: 2
        // rows: 7
        rowSpacing: 10

        StyledLabel{
            activeFocusOnTab: false
            width: 230* screenScaleFactor
            height: 30* screenScaleFactor
            text:qsTr("Language")
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        CXComboBox {
            id:preferencesCombox
            property var actionModel:  actionCommands.getOpt("Language").subMenuActionmodel
            property var curIndex:  0
            width: 450*screenScaleFactor
            height: 30*screenScaleFactor
            model: actionModel
            textRole: "actionNameRole"
            currentIndex: curIndex //actionChecked
            Component.onCompleted: {
                let rowCount = actionModel.rowCount()
                for(let i =0; i<rowCount;i++){
                    if(actionModel.data(actionModel.index(i,0),265)){
                        curIndex = i
                        break
                    }
                }
            }
            delegate: ItemDelegate
            {
                property bool currentItem: preferencesCombox.highlightedIndex === index
                width: preferencesCombox.width
                height : preferencesCombox.popHeight

                contentItem: Rectangle
                {
                    anchors.fill: parent
                    Text {
                        id:myText
                        anchors.left: iconImg.left
                        anchors.leftMargin: hasImg ? iconImg.width +5*screenScaleFactor : 0
                        x:5* screenScaleFactor
                        height: preferencesCombox.popHeight
                        width: parent.width - 5*screenScaleFactor
                        text: actionNameRole
                        color: preferencesCombox.textColor
                        font: preferencesCombox.font
                        elide: Text.ElideMiddle
                        verticalAlignment: Text.AlignVCenter
                        Component.onCompleted: {
                            if(actionChecked) preferencesCombox.curIndex = index

                        }
                    }

                    color: preferencesCombox.highlightedIndex === index ? preferencesCombox.itemHighlightColor
                                                              : preferencesCombox.itemNormalColor

                }
                hoverEnabled: preferencesCombox.hoverEnabled
                onClicked: {
                    model.actionItem.execute()
                    cxkernel_cxcloud.profileService.updateProfileLanguage()
                }
            }

        }

        StyledLabel{
            activeFocusOnTab: false
            width: 230* screenScaleFactor
            height: 30* screenScaleFactor
            text:qsTr("Login area")
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        CXComboBox {
            width: 450*screenScaleFactor
            height: 30*screenScaleFactor
            model: cxkernel_cxcloud.serverListModel
            currentIndex: cxkernel_cxcloud.serverType
            textRole: "model_name"
            displayText: qsTr(currentText)
            onActivated: {
                if (currentIndex === -1) {
                    return
                }

                const old_type = cxkernel_cxcloud.toRealServerType(cxkernel_cxcloud.serverType)
                const new_type = cxkernel_cxcloud.toRealServerType(currentIndex)

                if (old_type == new_type) {
                    cxkernel_cxcloud.serverType = currentIndex
                    currentIndex = Qt.binding(_ => cxkernel_cxcloud.serverType)
                    return
                }

                standaloneWindow.cloudContext.tipDialog.tipRestartAppliction(
                    onAccepted => {
                        cxkernel_cxcloud.serverType = currentIndex
                        kernel_kernel.restartApplication()
                    },
                    onCanceled => {
                        currentIndex = Qt.binding(_ => cxkernel_cxcloud.serverType)
                    }
                )
            }
        }

        // StyledLabel{
        //     activeFocusOnTab: false
        //     width: 230* screenScaleFactor
        //     height: 30* screenScaleFactor
        //     text:"单位"
        //     font.pointSize:Constants.labelFontPointSize_10
        //     color: Constants.themeTextColor
        //     verticalAlignment: Qt.AlignVCenter
        //     horizontalAlignment: Qt.AlignLeft
        // }
        // CXComboBox {
        //     width: 450*screenScaleFactor
        //     height: 30*screenScaleFactor
        //     model:["公有制","英制"]
        // }

        // StyledLabel{
        //     activeFocusOnTab: false
        //     width: 230* screenScaleFactor
        //     height: 30* screenScaleFactor
        //     text: qsTr("Synchronize user presets (printer/filament/process)")
        //     font.pointSize:Constants.labelFontPointSize_10
        //     color: Constants.themeTextColor
        //     verticalAlignment: Qt.AlignVCenter
        //     horizontalAlignment: Qt.AlignLeft
        // }
        // CheckBox{
        //     implicitHeight: 30* screenScaleFactor
        //     implicitWidth: 16* screenScaleFactor
        //     id: synchronizeControl
        //     checked: kernel_global_const.readRegister("cloud_service/sync_user_param") == "1"
        //     indicator:Item {
        //         implicitWidth: 16* screenScaleFactor
        //         implicitHeight: 16* screenScaleFactor
        //         anchors.verticalCenter: parent.verticalCenter
        //         Rectangle {
        //             implicitWidth: parent.width
        //             implicitHeight: parent.width
        //             anchors.verticalCenter: parent.verticalCenter
        //             radius: 3
        //             opacity: synchronizeControl.enabled ? 1 : 0.7
        //             border.color: synchronizeControl.checked || synchronizeControl.hovered ? Constants.textRectBgHoveredColor
        //                                                              : Constants.dialogItemRectBgBorderColor
        //             border.width: 1
        //             color: synchronizeControl.checked ?  Constants.themeGreenColor : "transparent"
        //             Image {
        //                 width: 9* screenScaleFactor
        //                 height: 6* screenScaleFactor
        //                 anchors.centerIn: parent
        //                 source: synchronizeControl.checked ? "qrc:/UI/images/check2.png" : ""
        //                 visible: synchronizeControl.checked
        //             }
        //         }
        //     }
        //     onClicked: kernel_global_const.writeRegister("cloud_service/sync_user_param", Number(checked))
        // }

        StyledLabel{
            activeFocusOnTab: false
            width: 230* screenScaleFactor
            height: 30* screenScaleFactor
            text: qsTr("Automatically update system defaults")
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        CheckBox{
            implicitHeight: 30* screenScaleFactor
            implicitWidth: 16* screenScaleFactor
            id: updateControl
            checked: cxkernel_cxcloud.profileService.autoCheckUpdateable
            indicator:Item {
                implicitWidth: 16* screenScaleFactor
                implicitHeight: 16* screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    implicitWidth: parent.width
                    implicitHeight: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 3
                    opacity: updateControl.enabled ? 1 : 0.7
                    border.color: updateControl.checked || updateControl.hovered ? Constants.textRectBgHoveredColor
                                                                     : Constants.dialogItemRectBgBorderColor
                    border.width: 1
                    color: updateControl.checked ? Constants.themeGreenColor : "transparent"
                    Image {
                        width: 9* screenScaleFactor
                        height: 6* screenScaleFactor
                        anchors.centerIn: parent
                        source: updateControl.checked ? "qrc:/UI/images/check2.png" : ""
                        visible: updateControl.checked
                    }
                }
            }
            onClicked: {
                cxkernel_cxcloud.profileService.autoCheckUpdateable = checked
                if (cxkernel_cxcloud.profileService.autoCheckUpdateable) {
                    cxkernel_cxcloud.profileService.checkOfficalProfileUpdateable()
                }
            }
        }

        StyledLabel{
            activeFocusOnTab: false
            width: 230* screenScaleFactor
            height: 30* screenScaleFactor
            text: qsTr("Enable dark theme mode")
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        CheckBox{
            property var actionModel: actionCommands.getOpt("Theme color change").subMenuActionmodel
            implicitHeight: 30* screenScaleFactor
            implicitWidth: 16* screenScaleFactor
            id: themeControl
            checked: actionModel.data(actionModel.index(0,0),265)
            indicator:Item {
                implicitWidth: 16* screenScaleFactor
                implicitHeight: 16* screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    implicitWidth: parent.width
                    implicitHeight: parent.width
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 3* screenScaleFactor
                    opacity: themeControl.enabled ? 1 : 0.7
                    border.color: themeControl.checked || themeControl.hovered ? Constants.textRectBgHoveredColor
                                                                     : Constants.dialogItemRectBgBorderColor
                    border.width: 1
                    color: themeControl.checked ?  Constants.themeGreenColor : "transparent"
                    Image {
                        width: 9* screenScaleFactor
                        height: 6* screenScaleFactor
                        anchors.centerIn: parent
                        source: themeControl.checked ? "qrc:/UI/images/check2.png" : ""
                        visible: themeControl.checked
                    }
                }
            }
            onClicked: actionModel.data(actionModel.index(+!checked,0),262).execute()
        }

        StyledLabel{
            activeFocusOnTab: false
            width: 230* screenScaleFactor
            height: 30* screenScaleFactor
            text: qsTr("Model download")
            font.pointSize:Constants.labelFontPointSize_10
            color: Constants.themeTextColor
            verticalAlignment: Qt.AlignVCenter
            horizontalAlignment: Qt.AlignLeft
        }
        Row {
            width: 230* screenScaleFactor
            height: 30* screenScaleFactor

            CusFolderField
            {
                width: 450*screenScaleFactor
                height: 30*screenScaleFactor
                text:  kernel_preference_manager.downloadModelPath
                font.family: Constants.labelFontFamilyBold
                font.weight: Font.Bold
                rightPadding : image.width + 20 * screenScaleFactor
                onClicked:
                {
                    console.log("aaaaa")
                    fileDialog.itemObj = this
                    fileDialog.folder = "file:///"+ (text)
                    fileDialog.visible = true
                }
                function callbackText(path)
                {
                    text = path
                    kernel_preference_manager.downloadModelPath = path
                }
            }
        }

        // StyledLabel{
        //     activeFocusOnTab: false
        //     width: 230* screenScaleFactor
        //     height: 30* screenScaleFactor
        //     text:qsTr("Engine Type")
        //     font.pointSize:Constants.labelFontPointSize_10
        //     color: Constants.themeTextColor
        //     verticalAlignment: Qt.AlignVCenter
        //     horizontalAlignment: Qt.AlignLeft
        // }
        // CXComboBox {
        //     id: swtich_engine_combobox
        //     width: 450*screenScaleFactor
        //     height: 30*screenScaleFactor
        //     model: [ "Cura", "Orca" ]
        //     currentIndex: kernel_slice_flow.engineType
        //     onCurrentIndexChanged: {
        //         if (root.visible) {
        //             swtich_engine_dialog.show()
        //         }
        //     }
        // }
    }

    File.FileDialog {
        id: fileDialog
        title: "Please choose a file"
        property var itemObj: null

        selectFolder : true
        selectExisting:true
        onAccepted: {
            console.log("You chose folder: " + fileDialog.fileUrl.toString().replace("file:///",""))
            itemObj.callbackText(fileDialog.fileUrl.toString().replace("file:///",""))
        }
        onRejected: {
            console.log("FileDialog Canceled")
            visible = false
        }
        Component.onCompleted: visible = false
    }
}
