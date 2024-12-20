import QtQuick 2.13
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Item {
    id : __rootPanel
    property var curSelectPrintingIndex: -1
    property var devicesMap: undefined   //已打开的设备ID和数据映射表
    property var maxDeviceDataCount: 650
    property var sourceTheme: themeModel.get(Constants.currentTheme)
    property var errorCode

    function setRealEntryPanel(value)
    {
        if(value) lanPrintingBtnClicked()
        __lanPrinterPanel.setRealEntry(value)
    }

    function setCurGcodeFileName(fileName)
    {
        __lanPrinterPanel.curGcodeFileName = fileName
    }

    function setCurGcodeFilePath(filePath)
    {
        __lanPrinterPanel.curGcodeFilePath = filePath
    }

    function lanPrintingBtnClicked()
    {
        curSelectPrintingIndex = -1
        __testLoad.sourceComponent = undefined
        idRefreshDetailTimer.stop();
    }

    Component.onCompleted: __repeaterModel.clear()

    MouseArea
    {
        anchors.fill: parent
        focus: __rootPanel.visible
    }

    ListModel
    {
        id: __repeaterModel
    }

    Component
    {
        //设备详情页面，被共用的单例页面
        id: __test
        LanPrinterDetail {

        }
    }

    Timer {
        repeat: true
        interval: 1000
        id: idRefreshTimer
        triggeredOnStart: true

        onTriggered: {
            let devIds = Object.keys(devicesMap)
            if (devIds.length === 0 ||
                    curSelectPrintingIndex < 0)
                return;

            for (let i = 0; i < devIds.length; ++i) {
                let devId = devIds[i]
                let item = printerListModel.cSourceModel.getDeviceItem(devId)  //获取设备
                var curBedTemp = item ? item.pcBedTemp : ""
                var curNozzleTemp = item ? item.pcNozzleTemp : ""
                let data = ""
                if(item && item.nozzleCount > 1){
                    var curNozzle2Temp = item.nozzle2Temp
                    var curChamberTemp = item.chamberTemp
                    data = [curBedTemp, curNozzleTemp, curNozzle2Temp, curChamberTemp]
                }else if(item && item.printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408){
                    data = [curBedTemp, curNozzleTemp, item.chamberTemp]
                }else{
                    data = [curBedTemp, curNozzleTemp]
                }
                devicesMap[devId].push(data)

                if (devicesMap[devId].length > maxDeviceDataCount)
                    devicesMap[devId].shift() // 超出最大缓存

                if (__testLoad.itemPrinterID === devId)
                    __testLoad.item.addChartData(data)
            }
        }
    }

    ColumnLayout
    {
        width: parent.width
        height: parent.height
        spacing: 0

        Rectangle
        {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 32 * screenScaleFactor
            color: Constants.lanPrinter_panel_background

            z: 1

            layer.enabled: Constants.currentTheme

            layer.effect: DropShadow {
                radius: 3
                spread: 0
                samples: 7
                color: Constants.dropShadowColor
                verticalOffset: 3 * screenScaleFactor
            }

            ScrollView{
                anchors.fill: parent
                clip: true
                ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 1 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1 * screenScaleFactor

                    //默认的“局域网打印”按钮
                    Button {
                        id: lanPrinter_list_btn
                        width: 100 * screenScaleFactor
                        height: 30 * screenScaleFactor
                        property bool isSelected: curSelectPrintingIndex < 0

                        background: Rectangle {
                            radius: 5
                            border.width: 1
                            border.color: Constants.lanPrinter_panel_border
                            color: (lanPrinter_list_btn.isSelected || lanPrinter_list_btn.hovered)
                                   ? Constants.lanPrinter_panel_btn_hovered : Constants.lanPrinter_panel_btn_default
                        }

                        contentItem: Text {
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("LAN Printing")
                            color: (lanPrinter_list_btn.isSelected || lanPrinter_list_btn.hovered)
                                   ? Constants.lanPrinter_panel_weight_txt : Constants.lanPrinter_panel_light_txt
                        }

                        onClicked: lanPrintingBtnClicked()
                    }
                    Timer {
                        id: idRefreshDetailTimer
                        interval: 1000; running: false; repeat: false
                        onTriggered: {
                            __testLoad.item.stopPlayVideo()
                            __testLoad.item.updateShowData(devicesMap[__testLoad.itemPrinterID])
                            __testLoad.item.startPlayVideo()
                        }
                    }
                    Timer {
                        id: idEnableDetailTimer
                        interval: 1000; running: false; repeat: false
                        onTriggered: {
                            lanPrinter_list_btn.enabled = true;
                        }
                    }
                    //可变数量的打印机按钮
                    Repeater {
                        model: __repeaterModel
                        delegate: Button {
                            id: lanPrinter_detail_btn
                            height: 30 * screenScaleFactor
                            width: detailBtnRow.width + detailBtnRow.spacing * 2
                            property bool isSelected: (curSelectPrintingIndex === index)

                            background: Rectangle {
                                radius: 5
                                border.width: 1
                                border.color: Constants.lanPrinter_panel_border
                                color: (lanPrinter_detail_btn.isSelected || lanPrinter_detail_btn.hovered)
                                       ? Constants.lanPrinter_panel_btn_hovered : Constants.lanPrinter_panel_btn_default
                            }

                            Row {
                                spacing: 10 * screenScaleFactor
                                id: detailBtnRow
                                height: parent.height
                                anchors.centerIn: parent

                                Text {
                                    height: parent.height
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    text: Key_PrinterName
                                    color: (lanPrinter_detail_btn.isSelected || lanPrinter_detail_btn.hovered)
                                           ? Constants.lanPrinter_panel_weight_txt : Constants.lanPrinter_panel_light_txt
                                }

                                Image {
                                    width: 8 * screenScaleFactor
                                    height: 8 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: Constants.currentTheme ? "qrc:/UI/photo/lanPrinterSource/closeBtn_light.svg" : "qrc:/UI/photo/lanPrinterSource/closeBtn_dark.svg"

                                    MouseArea {
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                                        onClicked: {
                                            //if(__testLoad.status!=Loader.Ready)
                                            delete devicesMap[__testLoad.itemPrinterID]
                                            curSelectPrintingIndex = -1
                                            __testLoad.sourceComponent = undefined
                                            __repeaterModel.remove(index)
                                        }
                                    }
                                }
                            }

                            onClicked: {
                                console.log(__testLoad.sourceComponent)
                                if(__testLoad.status!=Loader.Ready && __testLoad.sourceComponent!=null) return
                                if(isSelected) return
                                curSelectPrintingIndex = index
                                console.log("select"+index)
                                console.log("select"+__testLoad.sourceComponent)
                                if(__testLoad.sourceComponent === null)
                                {
                                    lanPrinter_list_btn.enabled = false;
                                    __testLoad.itemPrinterID = Key_PrinterID
                                    __testLoad.itemPrinterType = Key_PrinterType
                                    __testLoad.sourceComponent = __test
                                    
                                }
                                else {
                                    __testLoad.itemPrinterID = Key_PrinterID
                                    __testLoad.item.deviceID = Key_PrinterID

                                    __testLoad.itemPrinterType = Key_PrinterType
                                    __testLoad.item.deviceType = Key_PrinterType
                                    idRefreshDetailTimer.restart()
                                    
                                }
                            }
                        }
                    }
                }
            }
        }

        //内容
        Item
        {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width

            z: 0

            LanPrinterListLocal
            {
                id: __lanPrinterPanel
                anchors.fill: parent

                function isModelContains(id) {
                    var result = -1
                    for(var index = 0; index < __repeaterModel.count; ++index)
                    {
                        var macAddress = __repeaterModel.get(index).Key_PrinterID
                        if(macAddress === id) result = index
                    }
                    return result
                }

                onClickDetail:
                {
                    var result = isModelContains(printerID)
                    curSelectPrintingIndex = (result < 0) ? __repeaterModel.count : result
                    if(result < 0) __repeaterModel.append({ "Key_PrinterID": printerID, "Key_PrinterName": printerName, "Key_PrinterType": printerType })

                    __testLoad.itemPrinterID = printerID
                    __testLoad.itemPrinterType = printerType
                    __testLoad.sourceComponent = __test
                }
                onClickDelete: {
                    delete devicesMap[__testLoad.itemPrinterID]
                    curSelectPrintingIndex = -1
                    __testLoad.sourceComponent = undefined
                    for(var index = 0; index < __repeaterModel.count; ++index)
                    {
                        var macAddress = __repeaterModel.get(index).Key_PrinterID
                        if(macAddress === printerID)  __repeaterModel.remove(index)
                    }

                }
                Loader
                {
                    id: __testLoad
                    property var itemPrinterID: ""
                    property var itemPrinterType: ""

                    focus: true
                    active: true
                    anchors.fill: parent
                    visible: status == Loader.Ready
                    onStatusChanged: {
                        console.log("__testLoad"+__testLoad.status)
                    }
                    onLoaded: {
                        item.deviceID = itemPrinterID
                        item.deviceType = itemPrinterType
                        if (devicesMap === undefined)
                            devicesMap = new Object
                        if (devicesMap[itemPrinterID] === undefined)
                            devicesMap[itemPrinterID] = []
                        item.updateShowData(devicesMap[itemPrinterID])
                        item.startPlayVideo()
                        idEnableDetailTimer.restart()
                    }
                }
            }
        }
    }

    ListModel {
        id: themeModel
        ListElement {
            //Dark Theme
            shadow_color: "#333333"
            background_color: "#1F1F21"
            background_border: "#0F0F10"

            title_background_color: "#37373B"
            popup_background_color: "#28282B"

            label_background_color: "#28282B"
            label_border_color: "#515157"

            tooltip_background_color: "#414143"
            tooltip_border_color: "#626266"

            image_popup_background: "#37373B"
            image_popup_border: "#0F0F10"

            video_popup_background: "#28282B"
            video_popup_border: "#0F0F10"
            video_popup_title: "#37373B"

            dialog_title_color: "#6E6E73"
            dialog_shadow_color: "#333333"
            dialog_border_color: "#262626"
            dialog_background_color: "#4B4B4D"

            btn_border_color: "transparent"
            btn_default_color: "#515157"
            btn_hovered_color: "#67676D"

            innerBtn_border_color: "#59595D"
            innerBtn_default_color: "#59595D"
            innerBtn_hovered_color: "#6E6E73"

            ignoreBtn_border_color: "#606064"
            ignoreBtn_default_color: "#28282B"
            ignoreBtn_hovered_color: "#38383C"

            switch_border_color: "transparent"
            switch_indicator_color: "#28282B"
            switch_checked_background: "#144F71"
            switch_unchecked_background: "#424246"

            text_normal_color: "#FFFFFF"
            text_camera_color: "#B1B1B9"
            text_status_color: "#FCE100"
            text_method_color: "#1E9BE2"
            text_special_color: "#7F7F80"

            scrollbar_color: "#737378"
            print_progressbar_color: "#404044"
            import_progressbar_color: "#28282B"
            textedit_selection_color: "#1E9BE2"

            right_submenu_border: "transparent"
            right_submenu_background: "#FFFFFF"
            right_submenu_text_color: "#414143"
            right_submenu_selection: "#A5DBF9"

            item_hovered_color: "#414145"
            item_checked_border: "#48484C"
            item_splitline_color: "#414145"
            item_crossline_color: "#414145"
            item_rectangle_border: "#4F4F54"

            imgpreview_color: "#19191B"
            imgpreview_border: "transparent"
            imgpreview_source: "qrc:/UI/photo/lanPrinterSource/gridbg_dark.png"

            img_upBtn: "qrc:/UI/photo/lanPrinterSource/upBtn.svg"
            img_downBtn: "qrc:/UI/photo/lanPrinterSource/downBtn.svg"
            img_closeBtn: "qrc:/UI/photo/lanPrinterSource/closeBtn_dark.svg"
            img_upBtn_hover: "qrc:/UI/photo/lanPrinterSource/upBtn_hover.svg"
            img_downBtn_hover: "qrc:/UI/photo/lanPrinterSource/downBtn_hover.svg"

            img_refresh: "qrc:/UI/photo/lanPrinterSource/refresh_dark.svg"
            img_enlarge: "qrc:/UI/photo/lanPrinterSource/enlarge_detail_dark.svg"
            img_upArrow: "qrc:/UI/photo/lanPrinterSource/upArrow.svg"
            img_downArrow: "qrc:/UI/photo/lanPrinterSource/downArrow.svg"

            img_warning: "qrc:/UI/photo/lanPrinterSource/addPrinter_warning.png"
            img_infomation: "qrc:/UI/photo/lanPrinterSource/addPrinter_infomation.png"
            img_successful: "qrc:/UI/photo/lanPrinterSource/addPrinter_successful.png"

            img_label_idle: "qrc:/UI/photo/lanPrinterSource/idle_detail_dark.svg"
            img_label_offline: "qrc:/UI/photo/lanPrinterSource/offline_detail_dark.svg"
            img_camera_connection: "qrc:/UI/photo/lanPrinterSource/camera_connection_dark.svg"

            img_fans_open: "qrc:/UI/photo/lanPrinterSource/fans_open.svg"
            img_fans_close: "qrc:/UI/photo/lanPrinterSource/fans_close.svg"
            img_led_open: "qrc:/UI/photo/lanPrinterSource/led_open.svg"
            img_led_close: "qrc:/UI/photo/lanPrinterSource/led_close.svg"

            axis_plusIcon: "qrc:/UI/photo/lanPrinterSource/axis_plusIcon.svg"



            fan_block_bgColor: "#212124"
        }
        ListElement {
            //Light Theme
            shadow_color: "#BBBBBB"
            background_color: "#F2F2F5"
            background_border: "#D6D6DC"

            title_background_color: "#FFFFFF"
            popup_background_color: "#FFFFFF"

            label_background_color: "#FFFFFF"
            label_border_color: "#D6D6DC"

            tooltip_background_color: "#FFFFFF"
            tooltip_border_color: "#D6D6DC"

            image_popup_background: "#E9E9EF"
            image_popup_border: "transparent"

            video_popup_background: "#FFFFFF"
            video_popup_border: "transparent"
            video_popup_title: "#E9E9EF"

            dialog_title_color: "#FFFFFF"
            dialog_shadow_color: "#BBBBBB"
            dialog_border_color: "transparent"
            dialog_background_color: "#FFFFFF"

            btn_border_color: "#D6D6DC"
            btn_default_color: "#FFFFFF"
            btn_hovered_color: "#E8E8ED"

            innerBtn_border_color: "#CBCBCC"
            innerBtn_default_color: "#ECECEC"
            innerBtn_hovered_color: "#C2C2C5"

            ignoreBtn_border_color: "#D9D9DE"
            ignoreBtn_default_color: "#FFFFFF"
            ignoreBtn_hovered_color: "#FFFFFF"

            switch_border_color: "#D6D6DC"
            switch_indicator_color: "#D6D6DC"
            switch_checked_background: "#CBECFF"
            switch_unchecked_background: "#FFFFFF"

            text_normal_color: "#333333"
            text_camera_color: "#919198"
            text_status_color: "#FC7700"
            text_method_color: "#1E9BE2"
            text_special_color: "#666666"

            scrollbar_color: "#C7C7CE"
            print_progressbar_color: "#E5E5E5"
            import_progressbar_color: "#E5E5E5"
            textedit_selection_color: "#98DAFF"

            right_submenu_border: "#D6D6DC"
            right_submenu_background: "#FFFFFF"
            right_submenu_text_color: "#333333"
            right_submenu_selection: "#A5DBF9"

            item_hovered_color: "#D6D6DC"
            item_checked_border: "#D6D6DC"
            item_splitline_color: "#D6D6DC"
            item_crossline_color: "#E5E5E5"
            item_rectangle_border: "#D6D6DC"

            imgpreview_color: "#FFFFFF"
            imgpreview_border: "#D6D6DC"
            imgpreview_source: "qrc:/UI/photo/lanPrinterSource/gridbg_light.png"

            img_upBtn: "qrc:/UI/photo/lanPrinterSource/upBtn.svg"
            img_downBtn: "qrc:/UI/photo/lanPrinterSource/downBtn.svg"
            img_closeBtn: "qrc:/UI/photo/lanPrinterSource/closeBtn_light.svg"
            img_upBtn_hover: "qrc:/UI/photo/lanPrinterSource/upBtn_hover.svg"
            img_downBtn_hover: "qrc:/UI/photo/lanPrinterSource/downBtn_hover.svg"

            img_refresh: "qrc:/UI/photo/lanPrinterSource/refresh_light.svg"
            img_enlarge: "qrc:/UI/photo/lanPrinterSource/enlarge_detail_light.svg"
            img_upArrow: "qrc:/UI/photo/lanPrinterSource/upArrow.svg"
            img_downArrow: "qrc:/UI/photo/lanPrinterSource/downArrow.svg"

            img_warning: "qrc:/UI/photo/lanPrinterSource/addPrinter_warning.png"
            img_infomation: "qrc:/UI/photo/lanPrinterSource/addPrinter_infomation.png"
            img_successful: "qrc:/UI/photo/lanPrinterSource/addPrinter_successful.png"

            img_label_idle: "qrc:/UI/photo/lanPrinterSource/idle_detail_light.svg"
            img_label_offline: "qrc:/UI/photo/lanPrinterSource/offline_detail_light.svg"
            img_camera_connection: "qrc:/UI/photo/lanPrinterSource/camera_connection_light.svg"
            fan_block_bgColor: "#ffffff"

            axis_plusIcon: "qrc:/UI/photo/lanPrinterSource/axis_plusIcon_light.svg"
        }
    }

    function showMessage(msgType, ipAddr, deviceType){
        idMessageDialog.showMessageDialog(msgType)
        idMessageDialog.ipAddress = ipAddr
        idMessageDialog.deviceType = deviceType
    }

    LanPrinterDialog {
        visible: false
        id: idMessageDialog
        width: 500 * screenScaleFactor
        height: 152 * screenScaleFactor
        shadowColor: sourceTheme.dialog_shadow_color
        borderColor: sourceTheme.dialog_border_color
        titleBgColor: sourceTheme.dialog_title_color
        titleFtColor: sourceTheme.text_normal_color
        backgroundColor: sourceTheme.dialog_background_color

        property int curMessageType: -1
        property bool showCancelBtn: true
        property string curMessageText: ""
        property string ipAddress
        property var deviceType
        property var cobjectName

        function showMessageDialog(msgType, errorCodeMsg, objectName) {
            switch(msgType)
            {
            case LanPrinterDetail.MessageType.Pause:
                title = qsTranslate("LanPrinterDetail", "Pause printing")
                curMessageText =qsTranslate("LanPrinterDetail", "Whether to pause printing?")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.Continue:
                title = qsTranslate("LanPrinterDetail", "Continue printing")
                curMessageText = qsTranslate("LanPrinterDetail", "Whether to continue printing?")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.Stop:
                title = qsTranslate("LanPrinterDetail", "Stop printing")
                curMessageText = qsTranslate("LanPrinterDetail", "Whether to stop printing?")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.NotExist:
                title = qsTranslate("LanPrinterDetail", "Restart printing")
                curMessageText = qsTranslate("LanPrinterDetail", "G-code file has been deleted and cannot be printed")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.ObjectDelete:
                title = qsTranslate("LanPrinterDetail", "Object deletion")
                curMessageText = qsTranslate("LanPrinterDetail", "Whether to continue deleting the selected object")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.ObjectNotDelete:
                title = qsTranslate("LanPrinterDetail", "Object deletion")
                curMessageText = qsTranslate("LanPrinterDetail", "Exclusion is not supported when only a single model is left")
                showCancelBtn = false
                break;
            case LanPrinterDetail.MessageType.CancelBrokenMaterial:
                title = qsTranslate("LanPrinterDetail", "Stop printing")
                curMessageText = qsTranslate("LanPrinterDetail", "Printing will stop shortly.")
                showCancelBtn = true
                break;
            default:
                break;
            }
            cobjectName = objectName
            errorCode = errorCodeMsg;
            curMessageType = msgType
            show()
        }

        function excuteMessageCommand() {
            switch(curMessageType)
            {
            case LanPrinterDetail.MessageType.Pause:
                if(ipAddress !== "")
                {
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_PAUSE, " ", deviceType)
                }
                break;
            case LanPrinterDetail.MessageType.Continue:
                if(ipAddress !== "")
                {
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_CONTINUE, " ", deviceType)
                }
                break;
            case LanPrinterDetail.MessageType.Stop:
                if(ipAddress !== "")
                {
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_STOP, " ", deviceType)
                }
                break;
            case LanPrinterDetail.MessageType.ObjectDelete:
                if(ipAddress !== "")
                {
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_EXCLUDEOBJECTS, cobjectName, deviceType)

                }
                break;
            case LanPrinterDetail.MessageType.ObjectNotDelete:
                break;
            case LanPrinterDetail.MessageType.CancelBrokenMaterial:
                if(ipAddress === "") return
                if(errorCode == 20000)
                {
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "0", deviceType)
                }
                else {
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_STOP, " ", deviceType)
                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", deviceType)
                }
                break;
            default:
                break;
            }
            curMessageType = -1
            close()
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
                        color: sourceTheme.text_normal_color
                        text: idMessageDialog.curMessageText
                    }
                }

                Row {
                    spacing: 10 * screenScaleFactor
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        width: 124 * screenScaleFactor
                        height: 28 * screenScaleFactor

                        border.width: 1
                        border.color: sourceTheme.innerBtn_border_color
                        color: btnConfirmArea.containsMouse ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color
                        radius: height / 2

                        Text {
                            anchors.centerIn: parent
                            verticalAlignment: Text.AlignVCenter
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            text: qsTranslate("LanPrinterDetail", "Confirm")
                            color: sourceTheme.text_normal_color
                        }

                        MouseArea {
                            id: btnConfirmArea
                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                            onClicked: idMessageDialog.excuteMessageCommand()
                        }
                    }

                    Rectangle {
                        width: 124 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        visible: idMessageDialog.showCancelBtn

                        border.width: 1
                        border.color: sourceTheme.innerBtn_border_color
                        color: btnCancelArea.containsMouse ? sourceTheme.innerBtn_hovered_color : sourceTheme.innerBtn_default_color
                        radius: height / 2

                        Text {
                            anchors.centerIn: parent
                            verticalAlignment: Text.AlignVCenter
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            text: qsTranslate("LanPrinterDetail", "Cancel")
                            color: sourceTheme.text_normal_color
                        }

                        MouseArea {
                            id: btnCancelArea
                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                            onClicked: idMessageDialog.close()
                        }
                    }
                }
            }
        }
    }

}
