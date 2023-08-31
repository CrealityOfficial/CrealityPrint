import QtQml 2.13
import QtQuick 2.13
import QtQuick.Window 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Rectangle {
    //id: root
    width: parent.width
    height: parent.height
    color: sourceTheme.background_color
    signal clickDetail(var printerID, var printerName, var printerType)

    property int themeType: -1
    property bool realEntry: false
    property bool buttonState: true
    property bool editGroupReadOnly: true

    property var ipAddrList: ""
    property var sourceTheme: ""
    property string curGcodeFileName: ""
    property string skipWebUrl: "https://www.crealitycloud.cn/post-detail/639857d26a069d622233f814"
    property bool oneClickPrintAble: true
    function setRealEntry(value)
    {
        realEntry = value
        stateControl.currentIndex = LanPrinterListLocal.StateFliterType.Fliter_All
    }

    function visualHeight(height)
    {
        height -= 10 * screenScaleFactor
        var item = (138 + 10) * screenScaleFactor
        var result = Math.floor(height / item) * item
        return result
    }

    enum StateFliterType
    {
        Fliter_All,
        Fliter_Idle,
        Fliter_Printing
    }

    enum PrintControlType
    {
        PRINT_START,
        PRINT_WITH_CALIBRATION,
        PRINT_PAUSE,
        PRINT_CONTINUE,
        PRINT_STOP,
        CONTROL_MOVE_X,
        CONTROL_MOVE_Y,
        CONTROL_MOVE_Z,
        CONTROL_MOVE_XY0,
        CONTROL_MOVE_Z0,
        CONTROL_CMD_FAN,
        CONTROL_CMD_LED,
        CONTROL_CMD_BEDTEMP,
        CONTROL_CMD_NOZZLETEMP,
        CONTROL_CMD_FEEDRATEPCT,
        CONTROL_CMD_AUTOHOME,
        CONTROL_CMD_FANCASE,
        CONTROL_CMD_FANAUXILIARY,
        CONTROL_CMD_RESTART_KLIPPER,
        CONTROL_CMD_RESTART_FIRMWARE,
        CONTROL_CMD_REPOPLRSTATUS,
        CONTROL_CMD_CLEANERR,
        CONTROL_CMD_1STNOZZLETEMP,
        CONTROL_CMD_2NDNOZZLETEMP,
        CONTROL_CMD_CHAMBERTEMP,
        CONTROL_CMD_MODELFANPCT,
        CONTROL_CMD_CASEFANPCT,
        CONTROL_CMD_AUXILIARYFANPCT
    }

    enum RemotePrinterType
    {
        REMOTE_PRINTER_TYPE_LAN,
        REMOTE_PRINTER_TYPE_OCTOPRINT,
        REMOTE_PRINTER_TYPE_KLIPPER,
        REMOTE_PRINTER_TYPE_KLIPPER4408
    }

    ListModel {
        id: themeModel
        ListElement {
            //Dark Theme
            background_color: "#363638"

            dialog_title_color: "#6E6E73"
            dialog_shadow_color: "#333333"
            dialog_border_color: "#262626"
            dialog_background_color: "#4B4B4D"

            btn_border_color: "transparent"
            btn_default_color: "#59595D"
            btn_hovered_color: "#6E6E73"

            innerBtn_default_color: "#59595D"
            innerBtn_hovered_color: "#6E6E73"
            innerBox_checked_border: "#FFFFFF"

            ignoreBtn_border_color: "#606064"
            ignoreBtn_default_color: "#28282B"
            ignoreBtn_hovered_color: "#38383C"

            text_light_color: "#CBCBCC"
            text_weight_color: "#FFFFFF"
            text_method_color: "#1E9BE2"

            box_default_color: "#363638"
            box_highlight_color: "#1E9BE2"
            box_border_default: "#6E6E72"
            box_border_highlight: "#1E9BE2"

            scrollbar_color: "#7E7E84"
            progressbar_color: "#343435"
            text_discover_color: "#00A3FF"
            textedit_selection_color: "#1E9BE2"

            add_printer_border: "#59595D"
            add_printer_border_bg: "#6E6E72"
            add_printer_background: "#4B4B4D"

            right_menu_selection: "#A5DBF9"
            right_menu_background: "#FFFFFF"
            right_menu_text_color: "#414143"
            right_menu_border_color: "transparent"

            item_border_real: "#414143"
            item_border_color: "#626266"
            item_checked_color: "#1E9BE2"
            item_checked_border: "#89898D"
            item_crossline_color: "#515153"
            item_searchImg_color: "#6B6B6E"
            item_deviceImg_color: "#6C6C6F"
            item_deviceImg_border: "transparent"
            item_background_color: "#414143"
            label_background_color: "#515155"

            item_select_border: "#a9a9ac"

            img_submenu: "qrc:/UI/photo/submenu.png"
            img_downBtn: "qrc:/UI/photo/downBtn.svg"
            img_downBtn_d: "qrc:/UI/photo/downBtn_d.svg"
            img_checkIcon: "qrc:/UI/photo/checkIcon.png"
            img_edit_device: "qrc:/UI/photo/editDevice.png"
            img_add_printer: "qrc:/UI/photo/lanPrinterSource/plusSign.svg"
            img_camera_status: "qrc:/UI/photo/camera_%1.png"

            img_print_time: "qrc:/UI/photo/print_total_time.png"
            imt_print_leftTime: "qrc:/UI/photo/print_left_time.png"
            img_print_progress: "qrc:/UI/photo/print_progress.png"

            img_print_speed: "qrc:/UI/photo/print_speed.png"
            img_bed_temperature: "qrc:/UI/photo/bed_temperature.png"
            img_nozzle_temperature: "qrc:/UI/photo/nozzle_temperature.png"

            img_warning: "qrc:/UI/photo/lanPrinterSource/addPrinter_warning.png"
            img_infomation: "qrc:/UI/photo/lanPrinterSource/addPrinter_infomation.png"
            img_successful: "qrc:/UI/photo/lanPrinterSource/addPrinter_successful.png"
        }
        ListElement {
            //Light Theme
            background_color: "#F2F2F5"

            dialog_title_color: "#FFFFFF"
            dialog_shadow_color: "#BBBBBB"
            dialog_border_color: "transparent"
            dialog_background_color: "#FFFFFF"

            btn_border_color: "#D6D6DC"
            btn_default_color: "#FFFFFF"
            btn_hovered_color: "#E8E8ED"

            innerBtn_default_color: "#ECECEC"
            innerBtn_hovered_color: "#C2C2C5"
            innerBox_checked_border: "#1E9BE2"

            ignoreBtn_border_color: "#D9D9DE"
            ignoreBtn_default_color: "#FFFFFF"
            ignoreBtn_hovered_color: "#FFFFFF"

            text_light_color: "#666666"
            text_weight_color: "#333333"
            text_method_color: "#333333"

            box_default_color: "#FFFFFF"
            box_highlight_color: "#98DAFF"
            box_border_default: "#D6D6DC"
            box_border_highlight: "#98DAFF"

            scrollbar_color: "#C7C7CE"
            progressbar_color: "#E5E5E5"
            text_discover_color: "#1E9BE2"
            textedit_selection_color: "#98DAFF"

            add_printer_border: "#CBCBCC"
            add_printer_border_bg: "#CBCBCC"
            add_printer_background: "#FFFFFF"

            right_menu_selection: "#A5DBF9"
            right_menu_background: "#FFFFFF"
            right_menu_text_color: "#333333"
            right_menu_border_color: "#D6D6DC"

            item_border_real: "#D6D6DC"
            item_border_color: "#D6D6DC"
            item_checked_color: "#1E9BE2"
            item_checked_border: "#D6D6DC"
            item_crossline_color: "#E5E5E5"
            item_searchImg_color: "#FFFFFF"
            item_deviceImg_color: "#EEEEEE"
            item_deviceImg_border: "#D6D6DC"
            item_background_color: "#FFFFFF"
            label_background_color: "#B6B6BD"

            item_select_border: "#00d2ff"

            img_submenu: "qrc:/UI/photo/submenu.png"
            img_downBtn: "qrc:/UI/photo/downBtn.svg"
            img_downBtn_d: "qrc:/UI/photo/downBtn.svg"
            img_checkIcon: "qrc:/UI/photo/checkIcon.png"
            img_edit_device: "qrc:/UI/photo/editDevice.png"
            img_add_printer: "qrc:/UI/photo/lanPrinterSource/plusSign.svg"
            img_camera_status: "qrc:/UI/photo/camera_%1.png"

            img_print_time: "qrc:/UI/photo/print_total_time.png"
            imt_print_leftTime: "qrc:/UI/photo/print_left_time.png"
            img_print_progress: "qrc:/UI/photo/print_progress.png"

            img_print_speed: "qrc:/UI/photo/print_speed.png"
            img_bed_temperature: "qrc:/UI/photo/bed_temperature.png"
            img_nozzle_temperature: "qrc:/UI/photo/nozzle_temperature.png"

            img_warning: "qrc:/UI/photo/lanPrinterSource/addPrinter_warning.png"
            img_infomation: "qrc:/UI/photo/lanPrinterSource/addPrinter_infomation.png"
            img_successful: "qrc:/UI/photo/lanPrinterSource/addPrinter_successful.png"
        }
    }

    Binding on themeType {
        value: Constants.currentTheme
    }

    onVisibleChanged: {
        if(visible)
        {
            printerListModel.cSourceModel.findDeviceList()//发现设备
            historyFileListModel.findHistoryFileList()//历史记录
            gcodeFileListModel.cSourceModel.findGcodeFileList()//文件列表
            videoListModel.findVideoFileList()//延时摄影文件列表
        }
    }

    onThemeTypeChanged: sourceTheme = themeModel.get(themeType)//切换主题

    onCurGcodeFileNameChanged: printerListModel.cSourceModel.batch_reset()

    Timer {
        id: checkTimer
        interval: 10000
        onTriggered: idAddPrinterPopup.curPopupType = 1//Warning
    }

    Connections {
        target: printerListModel.cSourceModel
        onSigConnectedIpSuccessed: {
            if(checkTimer.running)
            {
                checkTimer.stop()
                idAddPrinterPopup.curPopupType = 2//Successful
            }
        }
        onRowsInserted: {
            let rowCount = first + 1
            idEmptyText.visible = (rowCount <= 0)
        }
        onRowsRemoved: {
            let rowCount = last + 1
            idEmptyText.visible = (rowCount <= 0)
        }
    }

    Connections{
        target: kernel_kernel
        onCurrentPhaseChanged:{
            console.log("_____________________num = ", kernel_kernel.currentPhase)
            if(kernel_kernel.currentPhase === 0){
                idDeleteGroupDlg.visible = false
                idAddPrinterScan.visible = false
                idAddPrinterManual.visible = false
                idAddPrinterPopup.visible = false
            }
        }
    }

    LanPrinterDialog {
        id: idDeleteGroupDlg
        title: qsTr("Warning")
        width: 500 * screenScaleFactor
        height: 152 * screenScaleFactor
        shadowColor: sourceTheme.dialog_shadow_color
        borderColor: sourceTheme.dialog_border_color
        titleBgColor: sourceTheme.dialog_title_color
        titleFtColor: sourceTheme.text_weight_color
        backgroundColor: sourceTheme.dialog_background_color

        visible: false

        property string curDeleteText: ""
        function openDialog(currentText) {
            curDeleteText = currentText
            show()
        }

        bdContentItem: Item {
            Column {
                anchors.centerIn: parent
                spacing: 20 * screenScaleFactor

                Row {
                    spacing: 5 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: sourceTheme.img_warning
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        verticalAlignment: Text.AlignVCenter
                        font.weight: Font.Normal
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        text: qsTr("After deleting the current group, the device will move into the default group")
                        color: sourceTheme.text_weight_color
                    }
                }

                Row {
                    spacing: 10 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        radius: height / 2
                        width: 124 * screenScaleFactor
                        height: 28 * screenScaleFactor

                        border.width: 1
                        border.color: sourceTheme.add_printer_border
                        color: deleteAccept.containsMouse ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color

                        Text {
                            anchors.centerIn: parent
                            verticalAlignment: Text.AlignVCenter
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            text: qsTr("Yes")
                            color: sourceTheme.text_weight_color
                        }

                        MouseArea {
                            id: deleteAccept
                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                            onClicked: {
                                var groupNames = deviceControl.model
                                var delGroupName = idDeleteGroupDlg.curDeleteText

                                groupNames.pop(delGroupName)
                                deviceControl.model = groupNames
                                printerListModel.cSourceModel.deleteGroup(delGroupName)

                                idDeleteGroupDlg.close()
                            }
                        }
                    }

                    Rectangle {
                        radius: height / 2
                        width: 124 * screenScaleFactor
                        height: 28 * screenScaleFactor

                        border.width: 1
                        border.color: sourceTheme.add_printer_border
                        color: deleteReject.containsMouse ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color

                        Text {
                            anchors.centerIn: parent
                            verticalAlignment: Text.AlignVCenter
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            text: qsTr("No")
                            color: sourceTheme.text_weight_color
                        }

                        MouseArea {
                            id: deleteReject
                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                            onClicked: idDeleteGroupDlg.close()
                        }
                    }
                }
            }
        }
    }

    LanPrinterDialog {
        id: idAddPrinterScan
        title: qsTr("Add printer")
        width: 900 * screenScaleFactor
        height: 500 * screenScaleFactor
        shadowColor: sourceTheme.dialog_shadow_color
        borderColor: sourceTheme.dialog_border_color
        titleBgColor: sourceTheme.dialog_title_color
        titleFtColor: sourceTheme.text_weight_color
        backgroundColor: sourceTheme.dialog_background_color

        visible: false
        modality: Qt.NonModal
        extraBtnVisible: true

        property real timerCount: 5
        property bool maskEnabled: false
        property bool firstVisible: false

        function refreshDevice(trigger)
        {
            extraBtnEnabled = !trigger
            extraBtnTimer.running = trigger
            idAddPrinterScan.maskEnabled = trigger
            if(trigger) searchMacListModel.searchDevice()//刷新列表
        }

        Timer {
            id: extraBtnTimer
            repeat: true
            interval: 1000
            onTriggered: {
                if(--idAddPrinterScan.timerCount <= 0)
                {
                    idAddPrinterScan.refreshDevice(false)
                    idAddPrinterScan.timerCount = 5
                }
            }
        }

        onVisibleChanged: {
            if(visible && !firstVisible)
            {
                firstVisible = true
                searchMacListModel.searchDeviceList()//扫描添加
                refreshDevice(true)
            }
        }

        onSigExtraBtnClicked: refreshDevice(true)

        bdContentItem: Item
        {
            Column {
                anchors.fill: parent
                anchors.margins: 20 * screenScaleFactor
                spacing: 10 * screenScaleFactor

                //Header
                Item {
                    width: parent.width
                    height: 14 * screenScaleFactor

                    Row {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 20 * screenScaleFactor

                        Row {
                            width: 130 * screenScaleFactor
                            spacing: 4 * screenScaleFactor

                            Button {
                                checkable: true
                                width: 14 * screenScaleFactor
                                height: 14 * screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter

                                Image {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.leftMargin: 3 * screenScaleFactor
                                    anchors.topMargin: 4 * screenScaleFactor

                                    width: 9 * screenScaleFactor
                                    height: 6 * screenScaleFactor
                                    source: sourceTheme.img_checkIcon
                                    visible: parent.checked
                                }

                                background: Rectangle {
                                    radius: 3
                                    border.width: parent.checked ? 0 : 1
                                    border.color: sourceTheme.item_checked_border
                                    color: parent.checked ? sourceTheme.item_checked_color : sourceTheme.item_background_color
                                }

                                onClicked: searchMacListModel.selectAllDevice(checked)
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: sourceTheme.text_light_color
                                text: qsTr("All")
                            }
                        }

                        Text {
                            width: 200 * screenScaleFactor
                            height: 14 * screenScaleFactor

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_10
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_light_color
                            text: qsTr("Device Name")
                        }

                        Text {
                            width: 200 * screenScaleFactor
                            height: 14 * screenScaleFactor

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_10
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_light_color
                            text: qsTr("Printer Model")
                        }
                    }

                    Text {
                        width: 100 * screenScaleFactor
                        height: 14 * screenScaleFactor

                        anchors.right: parent.right
                        anchors.rightMargin: 14 * screenScaleFactor
                        anchors.verticalCenter: parent.verticalCenter

                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_10
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        color: sourceTheme.text_light_color
                        text: qsTr("Ip address")
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1 * screenScaleFactor
                    color: sourceTheme.item_crossline_color
                }

                ListView {
                    clip: true
                    id: scanListView
                    width: parent.width
                    height: 350 * screenScaleFactor
                    spacing: 10 * screenScaleFactor
                    ScrollBar.vertical: ScrollBar {
                        visible: scanListView.contentHeight > scanListView.height
                        contentItem: Rectangle {
                            radius: width / 2
                            color: sourceTheme.scrollbar_color
                            implicitWidth: 6 * screenScaleFactor
                            implicitHeight: 100 * screenScaleFactor
                        }
                    }

                    model: searchMacListModel
                    delegate: ItemDelegate
                    {
                        id: scanItem
                        background: null
                        width: parent.width
                        height: 96 * screenScaleFactor
                        property int curHasSelect: mItem.pcHasSelect
                        property bool klipperType: printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER

                        onClicked: mItem.pcHasSelect = !mItem.pcHasSelect

                        Row {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 20 * screenScaleFactor

                            Row {
                                width: 130 * screenScaleFactor
                                height: 96 * screenScaleFactor
                                spacing: 20 * screenScaleFactor

                                Rectangle {
                                    width: 14 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    radius: 3
                                    border.width: scanItem.curHasSelect ? 0 : 1
                                    border.color: sourceTheme.item_checked_border
                                    color: scanItem.curHasSelect ? sourceTheme.item_checked_color : sourceTheme.item_background_color

                                    Image {
                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.leftMargin: 3 * screenScaleFactor
                                        anchors.topMargin: 4 * screenScaleFactor

                                        width: 9 * screenScaleFactor
                                        height: 6 * screenScaleFactor
                                        source: sourceTheme.img_checkIcon
                                        visible: scanItem.curHasSelect
                                    }
                                }

                                Rectangle {
                                    width: 96 * screenScaleFactor
                                    height: 96 * screenScaleFactor

                                    radius: 5
                                    border.width: 1
                                    border.color: sourceTheme.item_deviceImg_border
                                    color: sourceTheme.item_searchImg_color

                                    Image {
                                        anchors.centerIn: parent
                                        width: (scanItem.klipperType ? 60 : 64) * screenScaleFactor
                                        height: (scanItem.klipperType ? 35 : 70) * screenScaleFactor
                                        source: scanItem.klipperType ? "qrc:/UI/photo/addPrinter_SonicPad.png" :
                                                (printerModel != "" ? `qrc:/UI/photo/machineImage/machineImage_${kernel_add_printer_model.getModelNameByCodeName(printerModel)}.png` : "qrc:/UI/photo/crealityIcon.png")
                                    }
                                }
                            }

                            Item {
                                width: 200 * screenScaleFactor
                                height: 96 * screenScaleFactor

                                Text {
                                    id: scanDeviceName
                                    width: 200 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    elide: Text.ElideMiddle
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_10
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.text_weight_color
                                    text: scanItem.klipperType ? "Sonic Pad" : kernel_add_printer_model.getModelNameByCodeName(deviceName)

                                    MouseArea {
                                        id: scanDeviceNameArea
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        acceptedButtons: Qt.NoButton
                                    }
                                }

                                ToolTip {
                                    margins: 6 * screenScaleFactor
                                    padding: 6 * screenScaleFactor
                                    x: -10 * screenScaleFactor; y: 0

                                    contentItem: Text {
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_10
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_weight_color
                                        text: scanDeviceName.text
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: sourceTheme.item_border_color
                                        color: sourceTheme.item_background_color
                                        radius: 5
                                    }

                                    visible: scanDeviceNameArea.containsMouse && scanDeviceName.truncated//elide set
                                }
                            }

                            Item {
                                width: 200 * screenScaleFactor
                                height: 96 * screenScaleFactor

                                Text {
                                    id: scanPrinterModel
                                    width: 200 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    elide: Text.ElideMiddle
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    lineHeightMode: Text.FixedHeight
                                    lineHeight: 24 * screenScaleFactor
                                    color: sourceTheme.text_weight_color
                                    text: kernel_add_printer_model.getModelNameByCodeName(printerModel)

                                    MouseArea {
                                        id: scanPrinterModelArea
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        acceptedButtons: Qt.NoButton
                                    }
                                }

                                ToolTip {
                                    margins: 6 * screenScaleFactor
                                    padding: 6 * screenScaleFactor
                                    x: -10 * screenScaleFactor; y: 0

                                    contentItem: Text {
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_weight_color
                                        text: scanPrinterModel.text
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: sourceTheme.item_border_color
                                        color: sourceTheme.item_background_color
                                        radius: 5
                                    }

                                    visible: scanPrinterModelArea.containsMouse && scanPrinterModel.truncated//elide set
                                }
                            }
                        }

                        Item {
                            width: 100 * screenScaleFactor
                            height: 96 * screenScaleFactor

                            anchors.right: parent.right
                            anchors.rightMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                anchors.centerIn: parent
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignRight
                                verticalAlignment: Text.AlignVCenter
                                color: sourceTheme.text_weight_color
                                text: ipAddr
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1 * screenScaleFactor
                    color: sourceTheme.item_crossline_color
                }

                Button {
                    width: 124 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    background: Rectangle {
                        border.width: 1
                        border.color: sourceTheme.add_printer_border
                        color: parent.hovered ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color
                        radius: height / 2
                    }

                    contentItem: Text {
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        text: qsTr("Add")
                        color: sourceTheme.text_weight_color
                    }

                    onClicked: searchMacListModel.addSearchDevice()
                }
            }

            Rectangle {
                anchors.fill: parent
                visible: idAddPrinterScan.maskEnabled
                opacity: Constants.currentTheme ? 0.7 : 0.5
                color: Constants.currentTheme ? "white" : "black"

                MouseArea {
                    hoverEnabled: true
                    anchors.fill: parent
                    onWheel: wheel.acccpted = true
                }
            }

            Text {
                anchors.centerIn: parent
                visible: idAddPrinterScan.maskEnabled

                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: sourceTheme.text_weight_color
                text: qsTr("Searching for devices") + `... (${idAddPrinterScan.timerCount}s)`
            }
        }
    }

    LanPrinterDialog {
        id: idAddPrinterManual
        title: qsTr("Add printer")
        width: 500 * screenScaleFactor
        height: 263 * screenScaleFactor
        shadowColor: sourceTheme.dialog_shadow_color
        borderColor: sourceTheme.dialog_border_color
        titleBgColor: sourceTheme.dialog_title_color
        titleFtColor: sourceTheme.text_weight_color
        backgroundColor: sourceTheme.dialog_background_color

        visible: false
        modality: Qt.NonModal

        bdContentItem: Item
        {
            Row {
                id: rowPrinterType
                property int curPrinterType: -1

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.topMargin: 20 * screenScaleFactor
                anchors.leftMargin: 68 * screenScaleFactor
                spacing: 10 * screenScaleFactor

                RadioButton {
                    checked: false
                    indicator: null
                    width: 178 * screenScaleFactor
                    height: 90 * screenScaleFactor

                    Component.onCompleted: checked = true

                    onCheckedChanged: if(checked) rowPrinterType.curPrinterType = LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_LAN

                    background: Rectangle {
                        radius: 5
                        border.width: parent.checked ? 2 : (parent.hovered ? 1 : 1)
                        border.color: parent.checked ? "#1E9BE2" : (parent.hovered ?sourceTheme.innerBox_checked_border : sourceTheme.box_border_default)
                        color: sourceTheme.item_background_color
                    }

                    contentItem: Item
                    {
                        anchors.fill: parent

                        Image {
                            anchors.top: parent.top
                            anchors.topMargin: 10 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter
                            source: "qrc:/UI/photo/addPrinter_Printer.png"
                        }

                        Text {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 5 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_weight_color
                            text: qsTr("Printer(WiFi)")
                        }
                    }
                }

                RadioButton {
                    checked: false
                    indicator: null
                    width: 178 * screenScaleFactor
                    height: 90 * screenScaleFactor

                    onCheckedChanged: if(checked) rowPrinterType.curPrinterType = LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER

                    background: Rectangle {
                        radius: 5
                        border.width: parent.checked ? 2 : (parent.hovered ? 1 : 1)
                        border.color: parent.checked ?"#1E9BE2" : (parent.hovered ? sourceTheme.innerBox_checked_border : sourceTheme.box_border_default)
                        color: sourceTheme.item_background_color
                    }

                    contentItem: Item
                    {
                        anchors.fill: parent

                        Image {
                            anchors.top: parent.top
                            anchors.topMargin: 16 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter
                            source: "qrc:/UI/photo/addPrinter_SonicPad.png"
                        }

                        Text {
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 5 * screenScaleFactor
                            anchors.horizontalCenter: parent.horizontalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_weight_color
                            text: qsTr("Sonic Pad")
                        }
                    }
                }
            }

            Text {
                anchors.right: rowIpAddress.left
                anchors.rightMargin: 6 * screenScaleFactor
                anchors.verticalCenter: rowIpAddress.verticalCenter

                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                verticalAlignment: Text.AlignVCenter
                color: sourceTheme.text_weight_color
                text: qsTr("IP") + ":"
            }

            Row {
                id: rowIpAddress
                anchors.top: rowPrinterType.bottom
                anchors.left: rowPrinterType.left
                anchors.topMargin: 20 * screenScaleFactor
                spacing: 4 * screenScaleFactor

                Repeater {
                    id: editRepeater
                    function getPasteContent() {
                        ipAddrList = printerListModel.cSourceModel.pasteContent()
                        //console.log("getPasteContent()", ipAddrList)
                        //判断是否完整ip
                        let length = ipAddrList.length
                        for(let index=0; (index < length && length !== 1); ++index)
                        {
                            let ipAddress = ipAddrList[index]
                            editRepeater.itemAt(index).myEditText = ipAddress
                        }
                    }
                    function getIpAddressText() {
                        var result = ""
                        for(let index=0; index<model.count; ++index)
                        {
                            var data = model.get(index).data
                            var split = model.get(index).split
                            if(data !== "") {result += `${data}${split}`;continue}

                            return undefined
                        }
                        return result
                    }
                    function cursorBack(index, cursorLeft) {
                        if(index > 0)
                        {
                            //console.log("cursorBack", index)
                            let focusItem = itemAt(index - 1).myEditIpAddress
                            focusItem.forceActiveFocus(); focusItem.cursorPosition = cursorLeft ? 0 : focusItem.length
                        }
                    }
                    function cursorNext(index, cursorLeft) {
                        if(index < count - 1)
                        {
                            //console.log("cursorNext", index)
                            let focusItem = itemAt(index + 1).myEditIpAddress
                            focusItem.forceActiveFocus(); focusItem.cursorPosition = cursorLeft ? 0 : focusItem.length
                        }
                    }

                    model: ListModel {
                        ListElement {data: ""; split: "."}
                        ListElement {data: ""; split: "."}
                        ListElement {data: ""; split: "."}
                        ListElement {data: ""; split: ""}
                    }

                    delegate: Row {
                        spacing: 4 * screenScaleFactor
                        property alias myEditText: editIpAddress.text
                        property alias myEditIpAddress: editIpAddress

                        TextField {
                            id: editIpAddress
                            width: 83 * screenScaleFactor
                            height: 28 * screenScaleFactor

                            Keys.enabled: true
                            selectByMouse: true
                            activeFocusOnPress: true
                            selectionColor: sourceTheme.textedit_selection_color
                            selectedTextColor : color
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignHCenter
                            color: sourceTheme.text_weight_color
                            font.weight: Font.Normal
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            validator: RegExpValidator{regExp: new RegExp("[\\d]{1,2}|[1][\\d][\\d]|[2][0-4][\\d]|[2][5][0-5]")}
                            //onEditingFinished: focus = false

                            background: Rectangle {
                                radius: 5
                                color: sourceTheme.add_printer_background
                                border.color: parent.activeFocus ? "#1E9BE2" : sourceTheme.add_printer_border_bg
                                border.width: 1
                            }

                            onTextChanged: {
                                editRepeater.model.setProperty(index, "data", text)
                                if(length >= 3) editRepeater.cursorNext(index, false)
                            }

                            onPressed: {
                                if(event.button === Qt.RightButton && editIpAddress.activeFocus)
                                {
                                    pasteMenu.openPasteMenu(event.x, event.y)
                                }
                            }

                            Keys.onPressed: {
                                switch(event.key)
                                {
                                    case Qt.Key_Enter:
                                    case Qt.Key_Return:
                                        addConfirmBtn.clicked()
                                    break;
                                    case Qt.Key_Tab:
                                    case Qt.Key_Period:
                                        editRepeater.cursorNext(index, false)
                                    break;
                                    case Qt.Key_Left:
                                        if(cursorPosition == 0) editRepeater.cursorBack(index, false)
                                    break;
                                    case Qt.Key_Right:
                                        if(cursorPosition == length) editRepeater.cursorNext(index, true)
                                    break;
                                    case Qt.Key_Backspace:
                                        if(cursorPosition == 0 && length <= 0) editRepeater.cursorBack(index, false)
                                    break;
                                    case Qt.Key_V:
                                        if(event.modifiers & Qt.ControlModifier) editRepeater.getPasteContent()
                                    break;
                                }
                            }
                        }

                        Popup {
                            id: pasteMenu
                            padding: 0
                            background: null
                            function openPasteMenu(mouseX, mouseY){
                                x = mouseX
                                y = mouseY
                                open()
                            }

                            contentItem: Button
                            {
                                background: Rectangle {
                                    width: 83 * screenScaleFactor
                                    height: 28 * screenScaleFactor
                                    color: parent.hovered ? sourceTheme.right_menu_selection : sourceTheme.right_menu_background
                                    radius: 5
                                }

                                contentItem: Text {
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: sourceTheme.right_menu_text_color
                                    text: qsTr("Paste")
                                }

                                onClicked: {
                                    editRepeater.getPasteContent()
                                    pasteMenu.close()
                                }
                            }
                        }

                        Text {
                            anchors.verticalCenter: editIpAddress.verticalCenter
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            font: editIpAddress.font
                            color: editIpAddress.color
                            text: split
                        }
                    }
                }
            }

            Text {
                anchors.left: rowIpAddress.left
                anchors.top: rowIpAddress.bottom
                anchors.topMargin: 6 * screenScaleFactor

                font.weight: Font.Bold
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                verticalAlignment: Text.AlignVCenter
                color: sourceTheme.text_discover_color
                text: qsTr("How to discover the IP address") + "?"

                MouseArea {
                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: Qt.openUrlExternally(skipWebUrl)
                }
            }

            Button {
                id: addConfirmBtn
                width: 124 * screenScaleFactor
                height: 28 * screenScaleFactor

                anchors.top: rowIpAddress.bottom
                anchors.topMargin: 26 * screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter

                background: Rectangle {
                    border.width: 1
                    border.color: sourceTheme.add_printer_border
                    color: parent.hovered ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color
                    radius: height / 2
                }

                contentItem: Text {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    text: qsTr("Add")
                    color: sourceTheme.text_weight_color
                }

                onClicked: {
                    var ipAddr = editRepeater.getIpAddressText()
                    if(ipAddr !== undefined)
                    {
                        checkTimer.start()
                        parent.forceActiveFocus()

                        idAddPrinterManual.close()
                        idAddPrinterPopup.show()
                        idAddPrinterPopup.curPopupType = 0//Connecting
                        printerListModel.cSourceModel.createConnect(ipAddr, rowPrinterType.curPrinterType)
                    }
                }
            }
        }
    }

    LanPrinterDialog {
        id: idAddPrinterPopup
        title: qsTr("Add printer")
        width: 500 * screenScaleFactor
        height: 152 * screenScaleFactor
        shadowColor: sourceTheme.dialog_shadow_color
        borderColor: sourceTheme.dialog_border_color
        titleBgColor: sourceTheme.dialog_title_color
        titleFtColor: sourceTheme.text_weight_color
        backgroundColor: sourceTheme.dialog_background_color

        visible: false

        property int curPopupType: 0 //0:Connecting 1:Warning 2:Successful
        function getSourceImage() {
            var sourceImage = ""
            switch(curPopupType)
            {
                case 0:
                    sourceImage = sourceTheme.img_infomation;
                    break;
                case 1:
                    sourceImage = sourceTheme.img_warning;
                    break;
                case 2:
                    sourceImage = sourceTheme.img_successful;
                    break;
            }
            return sourceImage
        }
        function getSourceText() {
            var sourceText = ""
            switch(curPopupType)
            {
                case 0:
                    sourceText = qsTr("Connecting") + "...";
                    break;
                case 1:
                    sourceText = qsTr("The printer could not be found") + "!";
                    break;
                case 2:
                    sourceText = qsTr("The printer added successfully") + "!";
                    break;
            }
            return sourceText
        }

        bdContentItem: Item
        {
            Column {
                anchors.centerIn: parent
                spacing: 20 * screenScaleFactor

                Row {
                    spacing: 10 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    Image {
                        source: idAddPrinterPopup.getSourceImage()
                    }

                    Text {
                        font.weight: Font.Normal
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        verticalAlignment: Text.AlignVCenter
                        text: idAddPrinterPopup.getSourceText()
                        color: sourceTheme.text_weight_color
                    }
                }

                Rectangle {
                    id: confirmBtn
                    radius: height / 2
                    width: 124 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    visible: idAddPrinterPopup.curPopupType
                    anchors.horizontalCenter: parent.horizontalCenter

                    border.width: 1
                    border.color: sourceTheme.add_printer_border
                    color: popupConfirmArea.containsMouse ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color

                    Text {
                        anchors.centerIn: parent
                        verticalAlignment: Text.AlignVCenter
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        text: qsTr("OK")
                        color: sourceTheme.text_weight_color
                    }

                    MouseArea {
                        id: popupConfirmArea
                        hoverEnabled: true
                        anchors.fill: parent
                        cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                        onClicked: {
                            idAddPrinterManual.close()
                            idAddPrinterPopup.close()
                            if(checkTimer.running) checkTimer.stop()
                        }
                    }
                }
            }
        }
    }

    //Condition Fliter
    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20 * screenScaleFactor
        anchors.leftMargin: 20 * screenScaleFactor
        spacing: 30 * screenScaleFactor

        Row {
            spacing: 3 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Normal
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_10
                color: sourceTheme.text_weight_color
                text: qsTr("Device group") + ":"
            }

            ComboBox {
                id: deviceControl
                property int viewPadding: 5
                property int extraMargin: 2
                property int showMaxCount: 10//3

                width: 160 * screenScaleFactor
                height: 28 * screenScaleFactor

                onCurrentIndexChanged: {
                    let changeIndex = (remoteControl.currentIndex != 2) ? !remoteControl.currentIndex : 2
                    if(currentIndex != -1) printerListModel.multiConditionFilter(stateControl.currentIndex, currentIndex+1, changeIndex)
                }

                Component.onCompleted: model = printerListModel.cSourceModel.getGroups()

                delegate: ItemDelegate {
                    width: deviceControl.width
                    height: deviceControl.height

                    contentItem: Rectangle {
                        //radius: 5
                        anchors.fill: parent
                        color: (deviceControl.highlightedIndex == index) ? sourceTheme.box_highlight_color : sourceTheme.box_default_color

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_10
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_weight_color
                            elide: Text.ElideRight
                            text: modelData
                        }

                        Image {
                            anchors.right: parent.right
                            anchors.rightMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            source: "qrc:/UI/photo/delete_group.png"
                            visible: index !== 0 && deviceControl.highlightedIndex == index

                            MouseArea {
                                hoverEnabled: true
                                anchors.fill: parent
                                cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                onClicked: idDeleteGroupDlg.openDialog(modelData)
                            }
                        }
                    }
                }

                background: Rectangle {
                    color: sourceTheme.box_default_color
                    border.color: deviceControl.popup.visible ? sourceTheme.box_highlight_color : sourceTheme.box_border_default
                    border.width: 1
                    radius: 5
                }

                indicator: Item {
                    width: 7 * screenScaleFactor
                    height: 4 * screenScaleFactor
                    visible: editGroupReadOnly
                    anchors.right: deviceControl.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 12 * screenScaleFactor

                    Image {
                        anchors.fill: parent
                        source: (controlPopupArea.containsMouse || deviceControl.popup.visible) ? sourceTheme.img_downBtn_d : sourceTheme.img_downBtn

                        MouseArea {
                            id: controlPopupArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }

                contentItem: TextField {
                    id: editGroupName
                    anchors.fill: parent
                    clip: true
                    leftPadding: 8.3
                    rightPadding: 8.3
                    maximumLength: 16
                    readOnly: editGroupReadOnly
                    enabled: !editGroupReadOnly
                    selectByMouse: !editGroupReadOnly
                    activeFocusOnPress: !editGroupReadOnly
                    selectionColor: sourceTheme.textedit_selection_color
                    selectedTextColor: color
                    verticalAlignment: TextInput.AlignVCenter
                    //horizontalAlignment: Text.AlignHCenter
                    color: sourceTheme.text_weight_color
                    font.weight: Font.Normal
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_10
                    text: enabled ? parent.editText : parent.displayText
                    background: null

                    Keys.onEnterPressed: editGroupReadOnly = true
                    Keys.onReturnPressed: editGroupReadOnly = true
                    onActiveFocusChanged: if(!activeFocus) editGroupReadOnly = true
                    onEditingFinished: {
                        if(text !== "" && text !== parent.currentText)
                        {
                            var groupNames = parent.model
                            var currentIndex = parent.currentIndex
                            var elementIndex = groupNames.indexOf(text)
                            var newGroupName = (elementIndex < 0) ? text : `${text}(1)`

                            //console.log("newGroupName", newGroupName)
                            groupNames.splice(currentIndex, 1, newGroupName)
                            parent.model = groupNames;parent.currentIndex = currentIndex
                            printerListModel.cSourceModel.editGroupName(currentIndex, newGroupName)
                        }
                    }
                }

                popup: Popup {
                    property alias itemHeight: deviceControl.height
                    property alias itemPadding: deviceControl.viewPadding
                    property alias btnExtraMargin: deviceControl.extraMargin
                    property alias itemMaxCount: deviceControl.showMaxCount

                    property int itemCount: deviceControl.delegateModel.count
                    property int count: itemCount < itemMaxCount ? itemCount : itemMaxCount

                    id: devPopup
                    width: deviceControl.width
                    y: deviceControl.height + 1// offset
                    leftPadding: 1; rightPadding: 1
                    bottomPadding: 0; topPadding: itemPadding

                    implicitHeight: deviceControl.delegateModel?
                                        count*itemHeight + btnExtraMargin + itemHeight + itemPadding*2
                                      :itemHeight + itemPadding*2

                    contentItem: Item {
                        Column {
                            width: parent.width
                            spacing: 2
                            ListView {
                                clip: true
                                width: parent.width
                                implicitHeight: contentHeight < devPopup.itemMaxCount*devPopup.itemHeight?
                                                    contentHeight : devPopup.itemMaxCount*devPopup.itemHeight
                                //snapMode: ListView.SnapToItem
                                model: deviceControl.popup.visible?deviceControl.delegateModel:null
                                ScrollBar.vertical: ScrollBar {
                                    //id: deviceBar
                                    visible: deviceControl.delegateModel ? (devPopup.itemCount > devPopup.itemMaxCount) : false
                                }
                            }
                            Rectangle {
                                radius: 5
                                width: 155 * screenScaleFactor
                                height: devPopup.itemHeight
                                anchors.left: parent.left
                                anchors.leftMargin: 2

                                border.width: 1
                                border.color: sourceTheme.btn_border_color
                                color: btnNewGroupArea.containsMouse ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color

                                Text {
                                    anchors.centerIn: parent
                                    verticalAlignment: Text.AlignVCenter
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_10
                                    elide: Text.ElideMiddle
                                    text: qsTr("New group")
                                    color: sourceTheme.text_weight_color
                                }

                                MouseArea {
                                    id: btnNewGroupArea
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        var groupNames = deviceControl.model
                                        var itemCount = groupNames.length + 1
                                        var currentIndex = deviceControl.currentIndex

                                        if(itemCount <= devPopup.itemMaxCount)
                                        {
                                            var addGroupName = `Group${itemCount}`
                                            groupNames.push(addGroupName)
                                            deviceControl.model = groupNames
                                            deviceControl.currentIndex = currentIndex
                                            printerListModel.cSourceModel.addGroup(addGroupName)
                                        }

                                        //deviceControl.popup.visible = false
                                    }
                                }
                            }
                        }
                    }

                    background: Rectangle {
                        color: sourceTheme.box_default_color
                        border.color: deviceControl.popup.visible ? sourceTheme.box_highlight_color : sourceTheme.box_border_default
                        border.width: 1
                        radius: 5
                    }
                }
            }

            Item {
                width: 10 * screenScaleFactor - 2 * parent.spacing
                height: parent.height
            }

            Rectangle {
                id: editGroupRect
                color: "transparent"
                width: 14 * screenScaleFactor
                height: 14 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter

                Image {
                    anchors.centerIn: parent
                    source: sourceTheme.img_edit_device
                }

                MouseArea {
                    id: editGroupArea
                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                    onClicked:
                    {
                        if(deviceControl.model && editGroupReadOnly) {
                            editGroupReadOnly = false
                            editGroupName.forceActiveFocus()
                        }
                    }
                }
            }
        }

        Row {
            spacing: 3 * screenScaleFactor

            Text {
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Normal
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_10
                color: sourceTheme.text_weight_color
                text: qsTr("State") + ":"
            }

            ComboBox {
                id: stateControl
                property int viewPadding: 5
                //property int extraMargin: 2
                property int showMaxCount: 3

                width: 160 * screenScaleFactor
                height: 28 * screenScaleFactor

                displayText: qsTr(currentText)

                onCurrentIndexChanged: {
                    let changeIndex = (remoteControl.currentIndex != 2) ? !remoteControl.currentIndex : 2
                    if(currentIndex != -1) printerListModel.multiConditionFilter(currentIndex, deviceControl.currentIndex+1, changeIndex)
                }

                model: [QT_TR_NOOP("All"), QT_TR_NOOP("Idle"), QT_TR_NOOP("Printing")]

                delegate: ItemDelegate {
                    width: stateControl.width
                    height: stateControl.height

                    contentItem: Rectangle {
                        //radius: 5
                        anchors.fill: parent
                        color: (stateControl.highlightedIndex == index) ? sourceTheme.box_highlight_color : sourceTheme.box_default_color

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_10
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_weight_color
                            elide: Text.ElideRight
                            text: qsTr(modelData)
                        }
                    }
                }

                background: Rectangle {
                    color: sourceTheme.box_default_color
                    border.color: stateControl.popup.visible ? sourceTheme.box_highlight_color : sourceTheme.box_border_default
                    border.width: 1
                    radius: 5
                }

                indicator: Item {
                    width: 7 * screenScaleFactor
                    height: 4 * screenScaleFactor
                    anchors.right: stateControl.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 12 * screenScaleFactor

                    Image {
                        anchors.fill: parent
                        source: (statePopupArea.containsMouse || stateControl.popup.visible) ? sourceTheme.img_downBtn_d : sourceTheme.img_downBtn

                        MouseArea {
                            id: statePopupArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }

                contentItem: Text {
                    padding: 8
                    font.weight: Font.Normal
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    text: stateControl.displayText
                    color: sourceTheme.text_weight_color
                }

                popup: Popup {
                    property alias itemHeight: stateControl.height
                    property alias itemPadding: stateControl.viewPadding
                    //property alias btnExtraMargin: stateControl.extraMargin
                    property alias itemMaxCount: stateControl.showMaxCount
                    property int itemCount: stateControl.delegateModel.count

                    id: staPopup
                    width: stateControl.width
                    y: stateControl.height + 1//offset
                    leftPadding: 1; rightPadding: 1
                    bottomPadding: 0; topPadding: itemPadding

                    implicitHeight: itemCount*itemHeight + itemPadding*2

                    contentItem: Item {
                        Column {
                            width: parent.width
                            spacing: 0
                            ListView {
                                clip: true
                                width: parent.width
                                implicitHeight: contentHeight < staPopup.itemMaxCount*staPopup.itemHeight?
                                                    contentHeight : staPopup.itemMaxCount*staPopup.itemHeight
                                //snapMode: ListView.SnapToItem
                                model: stateControl.popup.visible?stateControl.delegateModel:null
                                ScrollBar.vertical: ScrollBar {
                                    //id: deviceBar
                                    visible: stateControl.delegateModel ? (staPopup.itemCount > staPopup.itemMaxCount) : false
                                }
                            }
                        }
                    }

                    background: Rectangle {
                        color: sourceTheme.box_default_color
                        border.color: stateControl.popup.visible ? sourceTheme.box_highlight_color : sourceTheme.box_border_default
                        border.width: 1
                        radius: 5
                    }
                }
            }
        }

        Row {
            spacing: 3 * screenScaleFactor

            Text {
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Normal
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_10
                color: sourceTheme.text_weight_color
                text: qsTr("Device Type") + ":"
            }

            ComboBox {
                id: remoteControl
                property int viewPadding: 5
                //property int extraMargin: 2
                property int showMaxCount: 3

                width: 160 * screenScaleFactor
                height: 28 * screenScaleFactor

                displayText: qsTr(currentText)

                onCurrentIndexChanged: {
                    let changeIndex = (currentIndex != 2) ? !currentIndex : 2
                    if(currentIndex != -1) printerListModel.multiConditionFilter(stateControl.currentIndex, deviceControl.currentIndex+1, changeIndex)
                }

                model: [QT_TR_NOOP("All"), QT_TR_NOOP("Printer"), QT_TR_NOOP("Sonic Pad")]

                delegate: ItemDelegate {
                    width: remoteControl.width
                    height: remoteControl.height

                    contentItem: Rectangle {
                        //radius: 5
                        anchors.fill: parent
                        color: (remoteControl.highlightedIndex == index) ? sourceTheme.box_highlight_color : sourceTheme.box_default_color

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_10
                            verticalAlignment: Text.AlignVCenter
                            color: sourceTheme.text_weight_color
                            elide: Text.ElideRight
                            text: qsTr(modelData)
                        }
                    }
                }

                background: Rectangle {
                    color: sourceTheme.box_default_color
                    border.color: remoteControl.popup.visible ? sourceTheme.box_highlight_color : sourceTheme.box_border_default
                    border.width: 1
                    radius: 5
                }

                indicator: Item {
                    width: 7 * screenScaleFactor
                    height: 4 * screenScaleFactor
                    anchors.right: remoteControl.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 12 * screenScaleFactor

                    Image {
                        anchors.fill: parent
                        source: (remotePopupArea.containsMouse || remoteControl.popup.visible) ? sourceTheme.img_downBtn_d : sourceTheme.img_downBtn

                        MouseArea {
                            id: remotePopupArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }

                contentItem: Text {
                    padding: 8
                    font.weight: Font.Normal
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_10
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    text: remoteControl.displayText
                    color: sourceTheme.text_weight_color
                }

                popup: Popup {
                    property alias itemHeight: remoteControl.height
                    property alias itemPadding: remoteControl.viewPadding
                    //property alias btnExtraMargin: remoteControl.extraMargin
                    property alias itemMaxCount: remoteControl.showMaxCount
                    property int itemCount: remoteControl.delegateModel.count

                    id: remotePopup
                    width: remoteControl.width
                    y: remoteControl.height + 1//offset
                    leftPadding: 1; rightPadding: 1
                    bottomPadding: 0; topPadding: itemPadding

                    implicitHeight: itemCount*itemHeight + itemPadding*2

                    contentItem: Item {
                        Column {
                            width: parent.width
                            spacing: 0
                            ListView {
                                clip: true
                                width: parent.width
                                implicitHeight: contentHeight < remotePopup.itemMaxCount*remotePopup.itemHeight?
                                                    contentHeight : remotePopup.itemMaxCount*remotePopup.itemHeight
                                //snapMode: ListView.SnapToItem
                                model: remoteControl.popup.visible?remoteControl.delegateModel:null
                                ScrollBar.vertical: ScrollBar {
                                    //id: deviceBar
                                    visible: remoteControl.delegateModel ? (remotePopup.itemCount > remotePopup.itemMaxCount) : false
                                }
                            }
                        }
                    }

                    background: Rectangle {
                        color: sourceTheme.box_default_color
                        border.color: remoteControl.popup.visible ? sourceTheme.box_highlight_color : sourceTheme.box_border_default
                        border.width: 1
                        radius: 5
                    }
                }
            }
        }
    }

    //Add Printer
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 20 * screenScaleFactor
        anchors.rightMargin: 20 * screenScaleFactor
        spacing: 10 * screenScaleFactor

        Rectangle {
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor

            border.width: 1
            border.color: sourceTheme.btn_border_color
            color: scanAddArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
            radius: height / 2

            Row {
                height: parent.height
                anchors.centerIn: parent
                spacing: 4 * screenScaleFactor

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: sourceTheme.img_add_printer
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    color: sourceTheme.text_weight_color
                    text: qsTr("Scan Add")
                }
            }

            MouseArea {
                id: scanAddArea
                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: idAddPrinterScan.show()
            }
        }

        Rectangle {
            width: 100 * screenScaleFactor
            height: 28 * screenScaleFactor

            border.width: 1
            border.color: sourceTheme.btn_border_color
            color: manualAddArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
            radius: height / 2

            Row {
                height: parent.height
                anchors.centerIn: parent
                spacing: 4 * screenScaleFactor

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: sourceTheme.img_add_printer
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    color: sourceTheme.text_weight_color
                    text: qsTr("Manual Add")
                }
            }

            MouseArea {
                id: manualAddArea
                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: idAddPrinterManual.show()
            }
        }
    }

    //Scroll View
    Rectangle {
        color: "transparent"
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 60 * screenScaleFactor
        anchors.leftMargin: 20 * screenScaleFactor
        anchors.rightMargin: 20 * screenScaleFactor
        anchors.bottomMargin: 60 * screenScaleFactor
        width: parent.width - anchors.leftMargin - anchors.rightMargin
        height: parent.height - anchors.topMargin - anchors.bottomMargin

        Text {
            id: idEmptyText
            anchors.centerIn: parent
            font.weight: Font.Medium
            font.family: Constants.mySystemFont.name
            font.pointSize: Constants.labelFontPointSize_12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: sourceTheme.text_weight_color
            text: qsTr("Device list is empty, you can add devices by scanning or manually.")
        }

        BasicScrollView {
            clip: true
            id: idScrollView
            width: parent.width
            height: visualHeight(parent.height)
            hpolicyVisible: contentWidth > width
            vpolicyVisible: contentHeight > height
            hpolicyindicator: Rectangle {
                radius: height / 2
                color: sourceTheme.scrollbar_color
                implicitWidth: 180 * screenScaleFactor
                implicitHeight: 6 * screenScaleFactor
            }
            vpolicyindicator: Rectangle {
                radius: width / 2
                color: sourceTheme.scrollbar_color
                implicitWidth: 6 * screenScaleFactor
                implicitHeight: 180 * screenScaleFactor
            }

            Column {
                anchors.fill: parent
                spacing: 10 * screenScaleFactor

                Repeater {
                    model: printerListModel
                    delegate: Rectangle {
                        id: rootItem
                        width: idScrollView.vpolicyVisible ? idScrollView.width - 14 * screenScaleFactor : idScrollView.width
                        height: 138 * screenScaleFactor
                        enabled: curTransProgress <= 0 || curTransProgress == 100
                        opacity: (enabled && isDeviceOnline) ? 1.0 : 0.5
                        color: sourceTheme.item_background_color
                        border.color: hoveredStyle&& !curBtnSelect ? sourceTheme.item_select_border :( (realEntry && curBtnSelect) ? checkedColor : sourceTheme.item_border_real)
                        border.width: 2
                        radius: 5

                        property bool hoveredStyle: rootItem.hovered  && realEntry  && isDeviceOnline && !fatalErrorCode
                        property bool hovered: false
                        property int currentState: printerState
                        property int curBtnSelect: mItem.pcHasSelect
                        property int curTransProgress: mItem.gCodeTransProgress

                        //property bool btnChecked: false
                        property bool isSonicPad: printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER
                        property bool isKlipper_4408: printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408
                        property bool isDeviceIdle: (printerState != 1 && printerState != 5)
                        property bool isDeviceOnline: (printerStatus == 1) //在线状态
                        property bool isStopping: (printerState == 4) //打印停止

                        property bool editReadOnly: true
                        //property bool validMachine: printerModel === kernel_parameter_manager.currentMachineName()
                        property bool fatalErrorCode: (errorCode >= 1 && errorCode <= 100) || errorCode == 2000 || errorCode == 2001
                        property bool controlEnabled: !isSonicPad && !fatalErrorCode && (isDeviceIdle || (!isDeviceIdle && printerMethod == 1))

                        property string checkedColor: sourceTheme.item_checked_color
                        property string cameraImagePath: sourceTheme.img_camera_status
                        property string previewImage: (isKlipper_4408 && !isDeviceIdle) ? `http://${ipAddr}:80/downloads/original/current_print_image.png` : `image://preview/icons/${printerID}`

                        function second2String(sec) {
                            var hours = Math.floor(sec / 3600)
                            var minutes = Math.floor(sec % 3600 / 60)
                            var seconds = Math.floor(sec % 3600 % 60)
                            return `${hours}h ${minutes}m ${seconds}s`
                        }
                        function getPrinterMethodText(method) {
                            var methodText = ""
                            switch(method)
                            {
                            case 0:
                                methodText = qsTr("TF Card Printing")
                                break;
                            case 1:
                                methodText = qsTr("WLAN Printing")
                                break;
                            case 2:
                                methodText = qsTr("CrealityCloud Printing")
                                break;
                            default:
                                break;
                            }
                            return methodText
                        }
                        function getPrinterMethodSource(method) {
                            var methodSource = []
                            switch(method)
                            {
                            case 0:
                                methodSource[0] = "qrc:/UI/photo/lanPrinterSource/tfCard_Method.svg"
                                methodSource[1] = Qt.size(13 * screenScaleFactor, 14 * screenScaleFactor)
                                break;
                            case 1:
                                methodSource[0] = "qrc:/UI/photo/lanPrinterSource/localNetwork_Method.svg"
                                methodSource[1] = Qt.size(14 * screenScaleFactor, 14 * screenScaleFactor)
                                break;
                            case 2:
                                methodSource[0] = "qrc:/UI/photo/lanPrinterSource/crealityCloud_Method.png"
                                methodSource[1] = Qt.size(15 * screenScaleFactor, 14 * screenScaleFactor)
                                break;
                            default:
                                break;
                            }
                            return methodSource
                        }

                        onCurrentStateChanged: {
                            if(curBtnSelect && currentState == 1) mItem.pcHasSelect = false
                            if(curTransProgress == 100 && (currentState == 1 || currentState == 3)) mItem.gCodeTransProgress = 0 //打印成功 or 失败
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onEntered:{
                                rootItem.hovered = true
                            }
                            onExited: {
                                rootItem.hovered = false
                            }
                            onClicked: {
                                if(mouse.button === Qt.LeftButton)
                                {
                                    rootItem.forceActiveFocus()
                                     mItem.pcHasSelect = !mItem.pcHasSelect
                                     oneClickPrintAble = !printerListModel.cSourceModel.checkOnePrint()
                                }
                                else {
                                    contentMenu.openMenu(mouseX, mouseY)
                                }
                            }
                        }

                        Menu {
                            id: contentMenu
                            padding: 1 * screenScaleFactor
                            function openMenu(mouseX, mouseY){
                                x = mouseX
                                y = mouseY
                                open()
                            }

                            Menu {
                                id: subMenu
                                title: qsTr("Move to")
                                padding: 1 * screenScaleFactor
                                enabled: groupsInstantiator.count > 0

                                Instantiator {
                                    id: groupsInstantiator
                                    model: deviceControl.model
                                    onObjectAdded: subMenu.insertItem(index, object)
                                    onObjectRemoved: subMenu.removeItem(object)

                                    delegate: MenuItem {
                                        enabled: groupNo != (index+1)
                                        implicitWidth: 160 * screenScaleFactor
                                        implicitHeight: 28 * screenScaleFactor

                                        onTriggered: {
                                            printerListModel.cSourceModel.editDeviceGroup(printerID, index+1)
                                            //contentMenu.close()
                                        }

                                        contentItem: Text {
                                            elide: Text.ElideRight
                                            opacity: enabled ? 1.0 : 0.5
                                            leftPadding: 8 * screenScaleFactor
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            horizontalAlignment: Text.AlignLeft
                                            verticalAlignment: Text.AlignVCenter
                                            color: sourceTheme.right_menu_text_color
                                            text: modelData
                                        }

                                        background: Rectangle {
                                            color: parent.highlighted ? sourceTheme.right_menu_selection : sourceTheme.right_menu_background
                                            border.color: "transparent"
                                            border.width: 0
                                        }
                                    }
                                }

                                background: Rectangle {
                                    implicitWidth: 160 * screenScaleFactor
                                    implicitHeight: groupsInstantiator.count * 28 * screenScaleFactor

                                    color: sourceTheme.right_menu_background
                                    border.color: sourceTheme.right_menu_border_color
                                    border.width: 1
                                }
                            }

                            Action {
                                text: qsTr("Remove device")
                                onTriggered: printerListModel.cSourceModel.deleteConnect(printerID)
                            }

                            delegate: MenuItem {
                                id: menuItem
                                indicator: null
                                implicitWidth: 160 * screenScaleFactor
                                implicitHeight: 28 * screenScaleFactor

                                arrow: Image {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: sourceTheme.img_submenu
                                    visible: menuItem.subMenu
                                }

                                contentItem: Text {
                                    elide: Text.ElideRight
                                    opacity: enabled ? 1.0 : 0.5
                                    leftPadding: 8 * screenScaleFactor
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.right_menu_text_color
                                    text: menuItem.text
                                }

                                background: Rectangle {
                                    color: menuItem.highlighted ? sourceTheme.right_menu_selection : sourceTheme.right_menu_background
                                    border.color: "transparent"
                                    border.width: 0
                                }
                            }

                            background: Rectangle {
                                implicitWidth: 160 * screenScaleFactor
                                implicitHeight: 56 * screenScaleFactor

                                color: sourceTheme.right_menu_background
                                border.color: sourceTheme.right_menu_border_color
                                border.width: 1
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            spacing: 5 * screenScaleFactor
                            enabled: rootItem.isDeviceOnline

                            Item {
                                Layout.fillHeight: true
                                Layout.preferredWidth: 50 * screenScaleFactor

                                Rectangle {
                                    id: checkButton
                                    anchors.centerIn: parent
                                    width: 14 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    visible: realEntry  && isDeviceOnline && !fatalErrorCode
                                    radius: 3
                                    border.width:  curBtnSelect ? 0 : 1
                                    border.color: sourceTheme.item_checked_border
                                    color: curBtnSelect ? checkedColor : sourceTheme.item_background_color

                                    Image {
                                        visible: curBtnSelect
                                        width: 9 * screenScaleFactor
                                        height: 6 * screenScaleFactor
                                        anchors.top: checkButton.top
                                        anchors.left: checkButton.left
                                        anchors.leftMargin: 3 * screenScaleFactor
                                        anchors.topMargin: 4 * screenScaleFactor
                                        source: sourceTheme.img_checkIcon
                                    }

                                    MouseArea {
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                        onClicked: {
                                            mItem.pcHasSelect = !mItem.pcHasSelect
                                            oneClickPrintAble = !printerListModel.cSourceModel.checkOnePrint()
                                            parent.forceActiveFocus()
                                        }
                                    }
                                }

                                Canvas {
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.topMargin: rootItem.border.width
                                    anchors.leftMargin: rootItem.border.width

                                    width: 50 * screenScaleFactor
                                    height: 50 * screenScaleFactor
                                    visible: !rootItem.isDeviceOnline
                                    property var backgroundColor: sourceTheme.label_background_color

                                    onBackgroundColorChanged: requestPaint()

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        //Draw path
                                        ctx.beginPath()
                                        ctx.moveTo(0, height)
                                        ctx.lineTo(0, rootItem.radius)
                                        ctx.arcTo(0, 0, rootItem.radius, 0, rootItem.radius)
                                        ctx.lineTo(width, 0)
                                        ctx.closePath()
                                        ctx.fillStyle = backgroundColor; ctx.fill()
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        anchors.verticalCenterOffset: -8 * screenScaleFactor
                                        anchors.horizontalCenterOffset: -8 * screenScaleFactor

                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Normal
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: sourceTheme.text_weight_color
                                        text: qsTr("Offline")
                                        rotation: -45
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 20 * screenScaleFactor - parent.spacing * 2
                            }

                            //deviceImg
                            Rectangle {
                                radius: 5
                                Layout.preferredWidth: 96 * screenScaleFactor
                                Layout.preferredHeight: 96 * screenScaleFactor

                                color: sourceTheme.item_deviceImg_color
                                border.color: sourceTheme.item_deviceImg_border
                                border.width: 1

                                Image {
                                    anchors.centerIn: parent
                                    width: 64 * screenScaleFactor
                                    height: 70 * screenScaleFactor
                                    source: (printerModel != "")?`qrc:/UI/photo/machineImage/machineImage_${kernel_add_printer_model.getModelNameByCodeName(printerModel)}.png`:"qrc:/UI/photo/crealityIcon.png"
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 20 * screenScaleFactor - parent.spacing * 2
                            }

                            ColumnLayout {
                                spacing: 15 * screenScaleFactor

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    RowLayout {
                                        spacing: 10 * screenScaleFactor
                                        visible: !rootItem.fatalErrorCode

                                        Rectangle {
                                            color: "transparent"
                                            Layout.preferredWidth: 11 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            Image {
                                                //id: cameraImage
                                                anchors.centerIn: parent
                                                source: cameraImagePath.arg(hasCamera?"online":"offline")
                                            }

                                            ToolTip {
                                                x: -10 * screenScaleFactor
                                                y: -implicitHeight - 6 * screenScaleFactor
                                                margins: 6 * screenScaleFactor
                                                padding: 6 * screenScaleFactor

                                                contentItem: Text {
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    verticalAlignment: Text.AlignVCenter
                                                    color: sourceTheme.text_weight_color
                                                    text: qsTr("Camera connected")
                                                }

                                                background: Rectangle {
                                                    radius: 5
                                                    width: 124 * screenScaleFactor
                                                    height: 28 * screenScaleFactor

                                                    color: sourceTheme.item_background_color
                                                    border.color: sourceTheme.item_border_color
                                                    border.width: 1
                                                }

                                                visible: hasCamera && cameraArea.containsMouse
                                            }

                                            MouseArea {
                                                id: cameraArea
                                                hoverEnabled: true
                                                anchors.fill: parent
                                            }
                                        }

                                        Rectangle {
                                            Layout.preferredWidth: 1 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            color: sourceTheme.item_crossline_color
                                            visible: idleLayout.visible || methodLayout.visible
                                        }

                                        RowLayout {
                                            id: idleLayout
                                            spacing: 5 * screenScaleFactor
                                            visible: rootItem.isDeviceOnline && rootItem.isDeviceIdle

                                            Image {
                                                width: 14 * screenScaleFactor
                                                height: 14 * screenScaleFactor
                                                source: `qrc:/UI/photo/lanPrinterSource/idle_detail_${themeType?"light":"dark"}.svg`
                                            }

                                            Text {
                                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                                Layout.preferredHeight: 14 * screenScaleFactor

                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                verticalAlignment: Text.AlignVCenter
                                                color: sourceTheme.text_weight_color
                                                text: qsTr("Printing has stopped")
                                            }
                                        }

                                        RowLayout {
                                            id: methodLayout
                                            spacing: 5 * screenScaleFactor
                                            visible: rootItem.isDeviceOnline && !rootItem.isDeviceIdle

                                            Image {
                                                Layout.preferredWidth: sourceSize.width
                                                Layout.preferredHeight: sourceSize.height
                                                property var methodSource: getPrinterMethodSource(printerMethod)

                                                source: methodSource[0]
                                                sourceSize: methodSource[1]
                                            }

                                            Text {
                                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                                Layout.preferredHeight: 14 * screenScaleFactor

                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                verticalAlignment: Text.AlignVCenter
                                                color: sourceTheme.text_method_color
                                                text: getPrinterMethodText(printerMethod)
                                            }
                                        }
                                    }

                                    RowLayout {
                                        id: errorLayout
                                        visible: errorCode != 0
                                        spacing: 10 * screenScaleFactor
                                        Layout.alignment: Qt.AlignRight
                                        property bool isRepoPlrStatus: errorCode == 2000
                                        property bool isMaterialStatus: errorCode == 2001
                                        property color displayTextColor: fatalErrorCode ? "#FD265A" : (themeType ? "#333333" : "#FCE100")

                                        onVisibleChanged: if(!visible) errorTooltip.close()

                                        Item {
                                            Layout.preferredWidth: 20 * screenScaleFactor
                                            Layout.preferredHeight: 20 * screenScaleFactor

                                            MouseArea {
                                                id: imageArea
                                                hoverEnabled: true
                                                anchors.fill: parent
                                                onContainsMouseChanged: if(containsMouse && !errorTooltip.opened) errorTooltip.open()

                                                Image {
                                                    anchors.centerIn: parent
                                                    source: fatalErrorCode ? "qrc:/UI/photo/lanPrinterSource/printer_error_icon.svg" : "qrc:/UI/photo/lanPrinterSource/printer_warning_icon.svg"
                                                }
                                            }

                                            ToolTip {
                                                id: errorTooltip
                                                x: -10 * screenScaleFactor
                                                y: -implicitHeight - 10 * screenScaleFactor

                                                padding: 0
                                                margins: 0

                                                contentItem: MouseArea {
                                                    id: tooltipArea
                                                    hoverEnabled: true
                                                    implicitWidth: rowContent.width + 32 * screenScaleFactor
                                                    implicitHeight: rowContent.height

                                                    Row {
                                                        id: rowContent
                                                        height: 65 * screenScaleFactor
                                                        spacing: 10 * screenScaleFactor
                                                        anchors.horizontalCenter: parent.horizontalCenter

                                                        Column {
                                                            width: childrenRect.width
                                                            spacing: 10 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter

                                                            Text {
                                                                font.weight: Font.Bold
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                verticalAlignment: Text.AlignVCenter
                                                                color: errorLayout.displayTextColor
                                                                text: qsTr("Error code:") + `E${String(errorCode).padStart(4, '0')}`
                                                                visible: !(errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus)
                                                            }

                                                            Text {
                                                                font.weight: Font.Bold
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                verticalAlignment: Text.AlignVCenter
                                                                color: errorLayout.displayTextColor
                                                                text: (errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus) ? errorMessage : (qsTr("Description:") + "Key" + errorKey + errorMessage) 
                                                            }
                                                        }

                                                        Button {
                                                            width: 60 * screenScaleFactor
                                                            height: 24 * screenScaleFactor
                                                            visible: !fatalErrorCode

                                                            anchors.verticalCenter: parent.verticalCenter

                                                            background: Rectangle {
                                                                border.width: 1
                                                                border.color: sourceTheme.ignoreBtn_border_color
                                                                color: parent.hovered ? sourceTheme.ignoreBtn_hovered_color : sourceTheme.ignoreBtn_default_color
                                                                radius: height / 2
                                                            }

                                                            contentItem: Text
                                                            {
                                                                font.weight: Font.Bold
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                horizontalAlignment: Text.AlignHCenter
                                                                verticalAlignment: Text.AlignVCenter
                                                                color: sourceTheme.text_weight_color
                                                                text: qsTr("Ignore")
                                                            }

                                                            onClicked:
                                                            {
                                                                printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", printerType)
                                                            }
                                                        }
                                                    }
                                                }

                                                background: Rectangle {
                                                    color: sourceTheme.item_background_color
                                                    border.color: sourceTheme.item_border_color
                                                    border.width: 1
                                                    radius: 5
                                                }

                                                timeout: (imageArea.containsMouse || tooltipArea.containsMouse) ? -1 : 1000
                                            }
                                        }

                                        Button {
                                            Layout.preferredWidth: 73 * screenScaleFactor
                                            Layout.preferredHeight: 24 * screenScaleFactor
                                            visible: (errorCode >= 101 && errorCode <= 200) || errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus

                                            background: Rectangle {
                                                border.width: 1
                                                border.color: sourceTheme.btn_border_color
                                                color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                                radius: height / 2
                                            }

                                            contentItem: Text
                                            {
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_weight_color
                                                text: qsTr("Continue")
                                            }

                                            onClicked:
                                            {
                                                if(errorLayout.isRepoPlrStatus)
                                                {
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "1", printerType)
                                                }
                                                else {
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.PRINT_CONTINUE, " ", printerType)
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", printerType)
                                                }
                                            }
                                        }

                                        Button {
                                            Layout.preferredWidth: 73 * screenScaleFactor
                                            Layout.preferredHeight: 24 * screenScaleFactor
                                            visible: (errorCode >= 101 && errorCode <= 200) || errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus

                                            background: Rectangle {
                                                border.width: 1
                                                border.color: sourceTheme.btn_border_color
                                                color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                                radius: height / 2
                                            }

                                            contentItem: Text
                                            {
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_weight_color
                                                text: qsTr("Cancel")
                                            }

                                            onClicked:
                                            {
                                                if(errorLayout.isRepoPlrStatus)
                                                {
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "0", printerType)
                                                }
                                                else {
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.PRINT_STOP, " ", printerType)
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", printerType)
                                                }
                                            }
                                        }

                                        Button {
                                            Layout.preferredWidth: 73 * screenScaleFactor
                                            Layout.preferredHeight: 24 * screenScaleFactor
                                            visible: (errorCode >= 1 && errorCode <= 100)

                                            background: Rectangle {
                                                border.width: 1
                                                border.color: sourceTheme.btn_border_color
                                                color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                                radius: height / 2
                                            }

                                            contentItem: Text
                                            {
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_weight_color
                                                text: qsTr("Reboot")
                                            }

                                            onClicked:
                                            {
                                                printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RESTART_KLIPPER, " ", printerType)
                                                printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RESTART_FIRMWARE, " ", printerType)
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Text {
                                        id: deviceText
                                        Layout.preferredWidth: 80 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Device Name") + ":"
                                        color: sourceTheme.text_light_color
                                    }

                                    Rectangle {
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 200 * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor
                                        Layout.fillWidth: true

                                        color: sourceTheme.item_background_color
                                        border.color: sourceTheme.item_border_color
                                        border.width: 1
                                        radius: 5

                                        TextField {
                                            id: editDeviceName
                                            anchors.fill: parent
                                            property var delayShow: false
                                            property bool widthExceed: contentWidth > (width - leftPadding - rightPadding)

                                            Binding on text {
                                                when: !editDeviceName.activeFocus && !editDeviceName.delayShow
                                                value: kernel_add_printer_model.getModelNameByCodeName(mItem.pcDeviceName)
                                            }

                                            Timer {
                                                repeat: false
                                                interval: 10000
                                                id: deviceNameDelayShow
                                                onTriggered: editDeviceName.delayShow = false
                                            }

                                            leftPadding: 8.3 * screenScaleFactor
                                            rightPadding: 8.3 * screenScaleFactor
                                            maximumLength: 48
                                            readOnly: editReadOnly
                                            enabled: !editReadOnly
                                            selectByMouse: !editReadOnly
                                            activeFocusOnPress: !editReadOnly
                                            selectionColor: sourceTheme.textedit_selection_color
                                            selectedTextColor: color
                                            verticalAlignment: TextInput.AlignVCenter
                                            //horizontalAlignment: Text.AlignHCenter
                                            color: sourceTheme.text_weight_color
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            background: null

                                            Keys.onEnterPressed: rootItem.forceActiveFocus()
                                            Keys.onReturnPressed: rootItem.forceActiveFocus()
                                            onActiveFocusChanged: if(!activeFocus) editReadOnly = true
                                            onEditingFinished: {
                                                editDeviceName.delayShow = true; deviceNameDelayShow.start()
                                                printerListModel.cSourceModel.editDeviceName(printerID, displayText)
                                            }
                                        }

                                        MouseArea {
                                            id: hoverArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            acceptedButtons: Qt.NoButton
                                        }

                                        ToolTip.text: editDeviceName.displayText
                                        ToolTip.visible: hoverArea.containsMouse && editDeviceName.widthExceed && !editDeviceName.activeFocus
                                    }

                                    Rectangle {
                                        color: "transparent"
                                        Layout.preferredWidth: 14 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        Image {
                                            anchors.centerIn: parent
                                            source: sourceTheme.img_edit_device
                                        }

                                        MouseArea {
                                            id: editButtonArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                            onClicked: {
                                                if(editReadOnly) {
                                                    editReadOnly = false
                                                    editDeviceName.forceActiveFocus()
                                                }
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Text {
                                        Layout.preferredWidth: 80 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: sourceTheme.text_light_color
                                        text: qsTr("Ip address") + " : "
                                    }

                                    Text {
                                        Layout.preferredWidth: 120 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        property string number: rootItem.isSonicPad ? `(${multiNumber})` : " "

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_weight_color
                                        text: ipAddr + number

                                        MouseArea {
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: printerListModel.cSourceModel.copyContent(ipAddr)
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            Rectangle {
                                color: sourceTheme.item_crossline_color
                                Layout.preferredWidth: 1 * screenScaleFactor
                                Layout.preferredHeight: 96 * screenScaleFactor
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            //gcodeImg
                            Rectangle {
                                radius: 5
                                Layout.preferredWidth: 62 * screenScaleFactor
                                Layout.preferredHeight: 62 * screenScaleFactor

                                color: sourceTheme.item_background_color
                                border.color: sourceTheme.item_border_color
                                border.width: 1

                                Image {
                                    id: gcodeImage
                                    anchors.centerIn: parent
                                    width: 48 * screenScaleFactor
                                    height: 48 * screenScaleFactor
                                    visible: !rootItem.isDeviceIdle

                                    function reload() {
                                        source = ""
                                        source = rootItem.previewImage
                                    }

                                    cache: false
                                    mipmap: true
                                    smooth: true
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    sourceSize: Qt.size(width, height)
                                    source: rootItem.previewImage
                                }
                            }

                            ColumnLayout {
                                spacing: 10 * screenScaleFactor

                                RowLayout {
                                    spacing: 5 * screenScaleFactor

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        visible: (curTransProgress >= 0)

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_light_color
                                        text: qsTr("File:")
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 200 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        Text {
                                            id: id_GcodeName
                                            property var idleModelName: (curTransProgress > 0 && curTransProgress < 100) ? " " : sentFileName
                                            property var realGcodeName: !rootItem.isDeviceIdle ? gCodeName : idleModelName
                                            property var showErrorText: "<br>" + "<font color='#FD265A'>" + qsTr("Upload Failed") + "<br>" +
                                                                        qsTr("Description:") + qsTranslate("Cpr_ErrorMsg", transErrorMessage) + "</font>"

                                            anchors.fill: parent
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            horizontalAlignment: Text.AlignLeft
                                            verticalAlignment: Text.AlignVCenter
                                            color: sourceTheme.text_light_color
                                            elide: Text.ElideMiddle
                                            text: (curTransProgress < 0) ? showErrorText : realGcodeName

                                            MouseArea {
                                                id: id_GcodeNameArea
                                                hoverEnabled: true
                                                anchors.fill: parent
                                                acceptedButtons: Qt.NoButton
                                                cursorShape: containsMouse ? Qt.ArrowCursor : Qt.PointingHandCursor
                                            }
                                        }

                                        ToolTip.text: id_GcodeName.text
                                        ToolTip.visible: id_GcodeNameArea.containsMouse && id_GcodeName.truncated//elide set
                                    }
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor
                                    visible: (curTransProgress != 0) && rootItem.isDeviceIdle//空闲页面传输中

                                    Rectangle {
                                        radius: 1
                                        color: sourceTheme.progressbar_color

                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 72 * screenScaleFactor
                                        Layout.maximumWidth: 120 * screenScaleFactor
                                        Layout.preferredHeight: 2 * screenScaleFactor

                                        Rectangle {
                                            width: parent.width * curTransProgress / 100.0
                                            height: 2 * screenScaleFactor
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            color: "#1E9BE2"
                                            radius: 1
                                        }
                                    }

                                    Text {
                                        visible: curTransProgress >= 0
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_12
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_weight_color
                                        text: `${curTransProgress}%`
                                    }

                                    Button {
                                        Layout.preferredWidth: 100 * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor
                                        visible: curTransProgress < 0

                                        background: Rectangle {
                                            border.width: 1
                                            border.color: sourceTheme.btn_border_color
                                            color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                            radius: height / 2
                                        }

                                        contentItem: Text
                                        {
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_weight_color
                                            text: qsTr("Retry")
                                        }

                                        onClicked: printerListModel.cSourceModel.reloadFile(printerID)
                                    }
                                }

                                RowLayout {
                                    spacing: 6 * screenScaleFactor
                                    visible: realEntry && isDeviceIdle && isKlipper_4408

                                    Button {
                                        Layout.preferredWidth: 14 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        property int enableSelfTest: mItem.enableSelfTest

                                        background: Rectangle {
                                            border.width: parent.enableSelfTest ? 0 : 1
                                            border.color: sourceTheme.item_checked_border
                                            color: parent.enableSelfTest ? checkedColor : sourceTheme.item_background_color
                                            radius: 3
                                        }

                                        Image {
                                            width: 9 * screenScaleFactor
                                            height: 6 * screenScaleFactor
                                            source: sourceTheme.img_checkIcon
                                            visible: parent.enableSelfTest

                                            anchors.top: parent.top
                                            anchors.left: parent.left
                                            anchors.leftMargin: 3 * screenScaleFactor
                                            anchors.topMargin: 4 * screenScaleFactor
                                        }

                                        onClicked: mItem.enableSelfTest = !mItem.enableSelfTest
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Normal
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        horizontalAlignment: Text.AlignLeft
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_weight_color
                                        text: qsTr("Print calibration")
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            Rectangle {
                                color: sourceTheme.item_crossline_color
                                Layout.preferredWidth: 1 * screenScaleFactor
                                Layout.preferredHeight: 96 * screenScaleFactor
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            ColumnLayout {
                                spacing: 15 * screenScaleFactor

                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 14
                                        Layout.preferredHeight: 14
                                        source: sourceTheme.img_print_time
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_light_color
                                        text: qsTr("Print time") + ": "
                                    }
                                }

                                RowLayout {
                                    spacing: 7 * screenScaleFactor

                                    Image {
                                        Layout.leftMargin: 1
                                        Layout.preferredWidth: 12
                                        Layout.preferredHeight: 14
                                        source: sourceTheme.imt_print_leftTime
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_light_color
                                        text: qsTr("Time left") + ": "
                                    }
                                }

                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 14
                                        Layout.preferredHeight: 14
                                        source: sourceTheme.img_print_progress
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_light_color
                                        text: qsTr("Print progress")
                                    }
                                }
                            }

                            ColumnLayout {
                                spacing: 15 * screenScaleFactor

                                Text {
                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                    Layout.preferredHeight: 14 * screenScaleFactor

                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.text_weight_color
                                    text: second2String(totalPrintTime)
                                }

                                Text {
                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                    Layout.preferredHeight: 14 * screenScaleFactor

                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.text_weight_color
                                    text: second2String(leftTime)
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Rectangle {
                                        radius: 1
                                        color: sourceTheme.progressbar_color

                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 72 * screenScaleFactor
                                        Layout.maximumWidth: 120 * screenScaleFactor
                                        Layout.preferredHeight: 2 * screenScaleFactor

                                        Rectangle {
                                            width: parent.width * curPrintProgress / 100.0
                                            height: 2 * screenScaleFactor
                                            anchors.left: parent.left
                                            anchors.top: parent.top
                                            color: "#1E9BE2"
                                            radius: 1
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_light_color
                                        text: curPrintProgress + "%"
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            Rectangle {
                                color: sourceTheme.item_crossline_color
                                Layout.preferredWidth: 1 * screenScaleFactor
                                Layout.preferredHeight: 96 * screenScaleFactor
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2

                            }
                            ColumnLayout {
                                spacing: 6 * screenScaleFactor
                                visible: screenScaleFactor <= 1
                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 14
                                        Layout.preferredHeight: 14
                                        source: sourceTheme.img_print_speed
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Printing speed") + "："
                                        color: sourceTheme.text_light_color
                                    }
                                }

                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Item {
                                        Layout.preferredWidth: 14
                                        Layout.preferredHeight: 14

                                        Image {
                                            anchors.centerIn: parent
                                            source: sourceTheme.img_bed_temperature
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Hot bed temp") + "："
                                        color: sourceTheme.text_light_color
                                    }
                                }

                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Item {
                                        Layout.preferredWidth: 14
                                        Layout.preferredHeight: 14

                                        Image {
                                            anchors.centerIn: parent
                                            source: sourceTheme.img_nozzle_temperature
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Nozzle temp") + "："
                                        color: sourceTheme.text_light_color
                                    }
                                }
                            }

                            ColumnLayout {
                                spacing: 6 * screenScaleFactor
                                visible: screenScaleFactor <= 1
                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Rectangle {
                                        id: printSpeedBox
                                        enabled: controlEnabled
                                        opacity: enabled ? 1.0 : 0.5

                                        property int minValue: 10
                                        property int maxValue: 300
                                        property bool delayShow: false
                                        property alias boxValue: editPrintSpeedBox.text

                                        function abnormal() {
                                            if(boxValue == "" || boxValue < minValue)
                                            {
                                                boxValue = minValue
                                                mItem.pcPrintSpeed = parseInt(boxValue)
                                            }
                                        }
                                        function decrease() {
                                            if(boxValue != "" && boxValue > minValue)
                                            {
                                                var value = parseInt(boxValue) - 1
                                                boxValue = value
                                            }
                                        }
                                        function increase() {
                                            if(boxValue != "" && boxValue < maxValue)
                                            {
                                                var value = parseInt(boxValue) + 1
                                                boxValue = value
                                            }
                                        }
                                        function specifyPcPrintSpeed() {
                                            delayShow = true
                                            printSpeedDelayShow.start()
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_FEEDRATEPCT, boxValue, printerType)
                                        }

                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 60 * screenScaleFactor
                                        Layout.maximumWidth: 140 * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor

                                        color: sourceTheme.item_background_color
                                        border.color: sourceTheme.item_border_color
                                        border.width: 1
                                        radius: 5

                                        Binding on boxValue {
                                            when: (!editPrintSpeedBox.activeFocus && !printSpeedBox.delayShow)
                                            value: mItem.pcPrintSpeed
                                        }

                                        Timer {
                                            repeat: false
                                            interval: 10000
                                            id: printSpeedDelayShow
                                            onTriggered: printSpeedBox.delayShow = false
                                        }

                                        TextField {
                                            id: editPrintSpeedBox
                                            width: parent.width
                                            height: parent.height
                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter

                                            validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1-2][\\d][\\d]|300")}
                                            selectByMouse: true
                                            selectionColor: sourceTheme.textedit_selection_color
                                            selectedTextColor: color
                                            leftPadding: 6 * screenScaleFactor
                                            rightPadding: 4 * screenScaleFactor
                                            verticalAlignment: TextInput.AlignVCenter
                                            horizontalAlignment: TextInput.AlignLeft
                                            color: sourceTheme.text_weight_color
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            background: null

                                            Keys.onUpPressed: printSpeedBox.increase()
                                            Keys.onDownPressed: printSpeedBox.decrease()
                                            Keys.onEnterPressed: rootItem.forceActiveFocus()
                                            Keys.onReturnPressed: rootItem.forceActiveFocus()
                                            onActiveFocusChanged: printSpeedBox.abnormal()
                                            onEditingFinished: printSpeedBox.specifyPcPrintSpeed()
                                        }

                                        Text {
                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.leftMargin: editPrintSpeedBox.contentWidth + editPrintSpeedBox.leftPadding + 4 * screenScaleFactor

                                            font: editPrintSpeedBox.font
                                            color: editPrintSpeedBox.color
                                            verticalAlignment: Text.AlignVCenter
                                            text: "%"
                                        }

                                        Image {
                                            property string upBtnImage: upButtonArea_0.containsMouse?"upBtn_d":"upBtn"
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.rightMargin: 8 * screenScaleFactor
                                            anchors.topMargin: 8 * screenScaleFactor
                                            visible: editPrintSpeedBox.activeFocus
                                            source: `qrc:/UI/photo/${upBtnImage}.svg`

                                            MouseArea {
                                                id: upButtonArea_0
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: printSpeedBox.increase()
                                            }
                                        }

                                        Image {
                                            property string downBtnImg: downButtonArea_0.containsMouse?"downBtn_d":"downBtn"
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            anchors.rightMargin: 8 * screenScaleFactor
                                            anchors.bottomMargin: 7 * screenScaleFactor
                                            visible: editPrintSpeedBox.activeFocus
                                            source: `qrc:/UI/photo/${downBtnImg}.svg`

                                            MouseArea {
                                                id: downButtonArea_0
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: printSpeedBox.decrease()
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.preferredWidth: 60 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                    }
                                }

                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Rectangle {
                                        id: bedDstTempBox
                                        enabled: controlEnabled
                                        opacity: enabled ? 1.0 : 0.5

                                        property int minValue: 0
                                        property int maxValue: 200
                                        property bool delayShow: false
                                        property alias boxValue: editBedDstTempBox.text

                                        function decrease() {
                                            if(boxValue != "" && boxValue > minValue)
                                            {
                                                var value = parseInt(boxValue) - 1
                                                boxValue = value
                                            }
                                        }
                                        function increase() {
                                            if(boxValue != "" && boxValue < maxValue)
                                            {
                                                var value = parseInt(boxValue) + 1
                                                boxValue = value
                                            }
                                        }
                                        function specifyPcBedDstTemp() {
                                            delayShow = true
                                            bedDstTempDelayShow.start()
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_BEDTEMP, boxValue, printerType)
                                        }

                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 60 * screenScaleFactor
                                        Layout.maximumWidth: 140 * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor

                                        color: sourceTheme.item_background_color
                                        border.color: sourceTheme.item_border_color
                                        border.width: 1
                                        radius: 5

                                        Binding on boxValue {
                                            when: (!editBedDstTempBox.activeFocus && !bedDstTempBox.delayShow)
                                            value: mItem.pcBedDstTemp
                                        }

                                        Timer {
                                            repeat: false
                                            interval: 10000
                                            id: bedDstTempDelayShow
                                            onTriggered: bedDstTempBox.delayShow = false
                                        }

                                        TextField {
                                            id: editBedDstTempBox
                                            width: parent.width
                                            height: parent.height
                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter

                                            validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1][\\d][\\d]|200")}
                                            selectByMouse: true
                                            selectionColor: sourceTheme.textedit_selection_color
                                            selectedTextColor: color
                                            leftPadding: 6 * screenScaleFactor
                                            rightPadding: 4 * screenScaleFactor
                                            verticalAlignment: TextInput.AlignVCenter
                                            horizontalAlignment: TextInput.AlignLeft
                                            color: sourceTheme.text_weight_color
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            background: null

                                            Keys.onUpPressed: bedDstTempBox.increase()
                                            Keys.onDownPressed: bedDstTempBox.decrease()
                                            Keys.onEnterPressed: rootItem.forceActiveFocus()
                                            Keys.onReturnPressed: rootItem.forceActiveFocus()
                                            //onActiveFocusChanged: mItem.pcBedDstTempFocus = activeFocus
                                            onEditingFinished: bedDstTempBox.specifyPcBedDstTemp()
                                        }

                                        Text {
                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.leftMargin: editBedDstTempBox.contentWidth + editBedDstTempBox.leftPadding + 4 * screenScaleFactor

                                            verticalAlignment: Text.AlignVCenter
                                            font: editBedDstTempBox.font
                                            color: editBedDstTempBox.color
                                            text: "°C"
                                        }

                                        Image {
                                            property string upBtnImage: upButtonArea_1.containsMouse?"upBtn_d":"upBtn"
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.rightMargin: 8 * screenScaleFactor
                                            anchors.topMargin: 8 * screenScaleFactor
                                            visible: editBedDstTempBox.activeFocus
                                            source: `qrc:/UI/photo/${upBtnImage}.svg`

                                            MouseArea {
                                                id: upButtonArea_1
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: bedDstTempBox.increase()
                                            }
                                        }

                                        Image {
                                            property string downBtnImg: downButtonArea_1.containsMouse?"downBtn_d":"downBtn"
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            anchors.rightMargin: 8 * screenScaleFactor
                                            anchors.bottomMargin: 7 * screenScaleFactor
                                            visible: editBedDstTempBox.activeFocus
                                            source: `qrc:/UI/photo/${downBtnImg}.svg`

                                            MouseArea {
                                                id: downButtonArea_1
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: bedDstTempBox.decrease()
                                            }
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: 60 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: `/ ${bedTemp.toFixed(2)} °C`
                                        color: sourceTheme.text_light_color
                                    }
                                }

                                RowLayout {
                                    spacing: 6 * screenScaleFactor

                                    Rectangle {
                                        id: nozzleDstTempBox
                                        enabled: controlEnabled
                                        opacity: enabled ? 1.0 : 0.5

                                        property int minValue: 0
                                        property int maxValue: 300
                                        property bool delayShow: false
                                        property alias boxValue: editNozzleDstTempBox.text

                                        function decrease() {
                                            if(boxValue != "" && boxValue > minValue)
                                            {
                                                var value = parseInt(boxValue) - 1
                                                boxValue = value
                                            }
                                        }
                                        function increase() {
                                            if(boxValue != "" && boxValue < maxValue)
                                            {
                                                var value = parseInt(boxValue) + 1
                                                boxValue = value
                                            }
                                        }
                                        function specifyPcNozzleDstTemp() {
                                            delayShow = true
                                            nozzleDstTempDelayShow.start()
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_NOZZLETEMP, boxValue, printerType)
                                        }

                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 60 * screenScaleFactor
                                        Layout.maximumWidth: 140 * screenScaleFactor
                                        Layout.preferredHeight: 28 * screenScaleFactor

                                        color: sourceTheme.item_background_color
                                        border.color: sourceTheme.item_border_color
                                        border.width: 1
                                        radius: 5

                                        Binding on boxValue {
                                            when: (!editNozzleDstTempBox.activeFocus && !nozzleDstTempBox.delayShow)
                                            value: mItem.pcNozzleDstTemp
                                        }

                                        Timer {
                                            repeat: false
                                            interval: 10000
                                            id: nozzleDstTempDelayShow
                                            onTriggered: nozzleDstTempBox.delayShow = false
                                        }

                                        TextField {
                                            id: editNozzleDstTempBox
                                            width: parent.width
                                            height: parent.height
                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter

                                            validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1-2][\\d][\\d]|300")}
                                            selectByMouse: true
                                            selectionColor: sourceTheme.textedit_selection_color
                                            selectedTextColor: color
                                            leftPadding: 6 * screenScaleFactor
                                            rightPadding: 4 * screenScaleFactor
                                            horizontalAlignment: TextInput.AlignLeft
                                            verticalAlignment: TextInput.AlignVCenter
                                            color: sourceTheme.text_weight_color
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            background: null

                                            Keys.onUpPressed: nozzleDstTempBox.increase()
                                            Keys.onDownPressed: nozzleDstTempBox.decrease()
                                            Keys.onEnterPressed: rootItem.forceActiveFocus()
                                            Keys.onReturnPressed: rootItem.forceActiveFocus()
                                            //onActiveFocusChanged: mItem.pcNozzleDstTempFocus = activeFocus
                                            onEditingFinished: nozzleDstTempBox.specifyPcNozzleDstTemp()
                                        }

                                        Text {
                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.leftMargin: editNozzleDstTempBox.contentWidth + editNozzleDstTempBox.leftPadding + 4 * screenScaleFactor

                                            font: editNozzleDstTempBox.font
                                            color: editNozzleDstTempBox.color
                                            verticalAlignment: Text.AlignVCenter
                                            text: "°C"
                                        }

                                        Image {
                                            property string upBtnImage: upButtonArea_2.containsMouse?"upBtn_d":"upBtn"
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            anchors.rightMargin: 8 * screenScaleFactor
                                            anchors.topMargin: 8 * screenScaleFactor
                                            visible: editNozzleDstTempBox.activeFocus
                                            source: `qrc:/UI/photo/${upBtnImage}.svg`

                                            MouseArea {
                                                id: upButtonArea_2
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: nozzleDstTempBox.increase()
                                            }
                                        }

                                        Image {
                                            property string downBtnImg: downButtonArea_2.containsMouse?"downBtn_d":"downBtn"
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            anchors.rightMargin: 8 * screenScaleFactor
                                            anchors.bottomMargin: 7 * screenScaleFactor
                                            visible: editNozzleDstTempBox.activeFocus
                                            source: `qrc:/UI/photo/${downBtnImg}.svg`

                                            MouseArea {
                                                id: downButtonArea_2
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                onClicked: nozzleDstTempBox.decrease()
                                            }
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: 60 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: `/ ${nozzleTemp.toFixed(2)} °C`
                                        color: sourceTheme.text_light_color
                                    }
                                }
                            }

                            Item {
                                visible: screenScaleFactor <= 1
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            Rectangle {
                                visible: screenScaleFactor <= 1
                                color: sourceTheme.item_crossline_color
                                Layout.preferredWidth: 1 * screenScaleFactor
                                Layout.preferredHeight: 96 * screenScaleFactor
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 0
                                Layout.maximumWidth: 50 * screenScaleFactor - parent.spacing * 2
                            }

                            ColumnLayout {
                                id: skipBtnLayout
                                spacing: 1 * screenScaleFactor

                                Button {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 50 * screenScaleFactor
                                    Layout.maximumWidth: 100 * screenScaleFactor
                                    Layout.preferredHeight: 28 * screenScaleFactor
                                    visible: !rootItem.isSonicPad

                                    contentItem: Text
                                    {
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: sourceTheme.text_weight_color
                                        text: qsTr("Details")
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                        radius: 14 * screenScaleFactor
                                    }

                                    onClicked: clickDetail(printerID, `${editDeviceName.text}_${ipAddr}`, printerType)
                                }

                                Button {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 80 * screenScaleFactor
                                    Layout.maximumWidth: 100 * screenScaleFactor
                                    Layout.preferredHeight: 28 * screenScaleFactor
                                    visible: rootItem.isSonicPad

                                    contentItem: Row
                                    {
                                        spacing: 4 * screenScaleFactor

                                        Item {
                                            width: 16 * screenScaleFactor
                                            height: 16 * screenScaleFactor

                                            Image {
                                                anchors.centerIn: parent
                                                source: "qrc:/UI/photo/icon_fluidd.png"
                                            }
                                        }

                                        Text {
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_weight_color
                                            text: qsTr("Fluidd")
                                        }
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                        radius: 14 * screenScaleFactor
                                    }

                                    onClicked: Qt.openUrlExternally(`http://${ipAddr}:${fluiddPort}`)
                                }

                                Button {
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 80 * screenScaleFactor
                                    Layout.maximumWidth: 100 * screenScaleFactor
                                    Layout.preferredHeight: 28 * screenScaleFactor
                                    visible: rootItem.isSonicPad

                                    contentItem: Row
                                    {
                                        spacing: 4 * screenScaleFactor

                                        Item {
                                            width: 16 * screenScaleFactor
                                            height: 16 * screenScaleFactor

                                            Image {
                                                anchors.centerIn: parent
                                                source: "qrc:/UI/photo/icon_mainsail.png"
                                            }
                                        }

                                        Text {
                                            horizontalAlignment: Text.AlignHCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_weight_color
                                            text: qsTr("Mainsail")
                                        }
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                        radius: 14 * screenScaleFactor
                                    }

                                    onClicked: Qt.openUrlExternally(`http://${ipAddr}:${mainsailPort}/allPrinters`)
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }
                        }
                    }
                }
            }
        }
    }

    //Buttons
    Row {
        spacing: 10 * screenScaleFactor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32 * screenScaleFactor
        anchors.horizontalCenter: parent.horizontalCenter
        visible: stateControl.currentIndex !== LanPrinterListLocal.StateFliterType.Fliter_Printing

        Button {
            width: 200 * screenScaleFactor
            height: 36 * screenScaleFactor
            visible: realEntry

            background: Rectangle {
                border.width: 1
                border.color: sourceTheme.btn_border_color
                color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                radius: height / 2
            }

            contentItem: Text
            {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                color: sourceTheme.text_weight_color
                text: qsTr("Send G-code")
            }

            onClicked: printerListModel.cSourceModel.batch_command(curGcodeFileName, true)

        }

        Button {
            width: 200 * screenScaleFactor
            height: 36 * screenScaleFactor
            visible: realEntry && oneClickPrintAble

            background: Rectangle {
                border.width: 1
                border.color: sourceTheme.btn_border_color
                color: parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                radius: height / 2
            }

            contentItem: Text
            {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                color: sourceTheme.text_weight_color
                text: qsTr("One-click Printing")
            }

            onClicked: printerListModel.cSourceModel.batch_command(curGcodeFileName, false)
        }
    }
}
