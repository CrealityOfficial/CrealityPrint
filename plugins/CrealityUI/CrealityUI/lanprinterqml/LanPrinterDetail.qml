import QtQuick 2.13
import QtCharts 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
import com.cxsw.SceneEditor3D 1.0
import Qt.labs.platform 1.1

import "../qml"
import "../secondqml"

Rectangle {
    id: root
    visible: true
    width: parent.width
    height: parent.height
    color: sourceTheme.background_color

    property int themeType: -1
    property var sourceTheme: ""

    property var deviceID: ""
    property var deviceType: ""
    property var deviceItem: ""

    property int errorKey: deviceItem ? deviceItem.errorKey : 0
    property int errorCode: deviceItem ? deviceItem.errorCode : 0
    property int printerState: deviceItem ? deviceItem.pcPrinterState : 0
    property int printerStatus: deviceItem ? deviceItem.pcPrinterStatus : 0
    property int printerMethod: deviceItem ? deviceItem.pcPrinterMethod : 0
    property int curImportProgress: deviceItem ? deviceItem.gCodeImportProgress : 0
    property int maxTryTimes : 0    // 加载图片失败时，自动重新加载的最大次数

    property bool hasCamera: deviceItem ? deviceItem.pcHasCamera : false
    property bool zAutohome: deviceItem ? (isKlipper_4408 ? deviceItem.zAutohome : true) : false
    property bool xyAutohome: deviceItem ? (isKlipper_4408 ? deviceItem.xyAutohome : true) : false
    property bool isDeviceIdle: (printerState != 1 && printerState != 5)
    property bool isDeviceOnline: (printerStatus == 1) //在线状态
    property bool isWlanPrinting: (printerMethod === 1 || isKlipper_4408) && isDeviceOnline && !fatalErrorCode
    property bool controlEnabled: (isDeviceIdle && isDeviceOnline && !fatalErrorCode)
    property bool fatalErrorCode: (errorCode >= 1 && errorCode <= 100) || errorCode == 2000 || errorCode == 2001
    property bool isKlipper_4408: deviceType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408
    property bool isDoubleNozzle : (deviceItem && deviceItem.nozzleCount > 1)
    property bool needLoadModelItem : false // 当前打印的预览图加载完了再加载ModelItem(列表),qml的Image不能多个同时加载，同时加载存在图片混淆的问题，用这个标志位做加载顺序的处理 */

    property string ipAddress: deviceItem ? deviceItem.pcIpAddr : ""
    property string curGcodeName: deviceItem ? deviceItem.pcGCodeName : ""
    property string errorMessage: deviceItem ? deviceItem.errorMessage : ""
//    property string previewImage: (isKlipper_4408 && !isDeviceIdle) ? `http://${ipAddress}:80/downloads/original/current_print_image.png` : `image://preview/icons/${deviceID}`

    property string previewImage: root.isDeviceIdle ? "" : `http://${ipAddress}:80/downloads/original/current_print_image.png`
    property string defaultImage: Constants.currentTheme ? "qrc:/UI/images/default_preview_light.svg" : "qrc:/UI/images/default_preview_dark.svg"
    property string modelItemImagePrefix: needLoadModelItem ? "http://" + ipAddress : ""

    onDeviceIDChanged: {
        imagePreview_min.init()
    }

    onPrinterStateChanged: {
        imagePreview_min.load()
    }

    function second2String(sec) {
        var hours = Math.floor(sec / 3600)
        var minutes = Math.floor(sec % 3600 / 60)
        var seconds = Math.floor(sec % 3600 % 60)
        return `${hours}h ${minutes}m ${seconds}s`
    }

    function startPlayVideo() {
        if(ipAddress !== "" && hasCamera)
        {
            var urlStr = "rtsp://" + ipAddress + "/ch0_0"
            cameraItem.showPopup = true
            if(deviceType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_LAN)
                urlStr = "rtsp://" + ipAddress + "/ch0_0"
            else
                urlStr = "http://" + ipAddress + ":8080/?action=stream"
            idVideoPlayer.start(urlStr)
        }
        else {
            idVideoPlayer.setUrl("")
            cameraItem.showPopup = false
        }
    }

    function stopPlayVideo() {
        idVideoPlayer.stop()
    }

    function addChartData(data, timeOffset = 0) {
        chartView.addData(data, timeOffset)
    }

    function initChartView(initData) {
        chartView.setupCustomPlot(initData)
        idRefreshTimer.restart()
    }

    function updateShowData(initData) {
        needLoadModelItem = false

        if(!isKlipper_4408)
            idSwipeview.currentIndex = LanPrinterDetail.ListViewType.GcodeListView

        deviceItem = printerListModel.cSourceModel.getDeviceItem(deviceID)  //获取设备
        historyFileListModel.switchDataSource(deviceID, deviceType)
        gcodeFileListModel.cSourceModel.switchDataSource(deviceID, deviceType)
        videoListModel.switchDataSource(deviceID, deviceType)
        initChartView(initData)
        imagePreview_min.load()
    }

    function widthProvider(value) {
        var w = 0
        switch(value) {
        case LanPrinterDetail.ListViewFieldType.FieldFileName:
            w = 300 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldFileSize:
            w = 150 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldLayerHeight:
        case LanPrinterDetail.ListViewFieldType.FieldInterval:
            w = 150 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldFileTime:
            w = 150 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldMaterial:
            w = 150 * screenScaleFactor;
            break;
        }
        return w
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

    enum MessageType
    {
        Pause,
        Continue,
        Stop,
        NotExist
    }

    enum ListViewType
    {
        GcodeListView,
        HistoryListView,
        VideoListView
    }

    enum ListViewFieldType
    {
        FieldFileName,
        FieldFileSize,
        FieldLayerHeight,
        FieldInterval,
        FieldFileTime,
        FieldMaterial
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
        }
    }

    Binding on themeType {
        //when: visible
        value: Constants.currentTheme
    }

    onThemeTypeChanged: sourceTheme = themeModel.get(themeType)//切换主题

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

        function showMessageDialog(msgType) {
            switch(msgType)
            {
            case LanPrinterDetail.MessageType.Pause:
                title = qsTr("Pause printing")
                curMessageText = qsTr("Whether to pause printing?")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.Continue:
                title = qsTr("Continue printing")
                curMessageText = qsTr("Whether to continue printing?")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.Stop:
                title = qsTr("Stop printing")
                curMessageText = qsTr("Whether to stop printing?")
                showCancelBtn = true
                break;
            case LanPrinterDetail.MessageType.NotExist:
                title = qsTr("Restart printing")
                curMessageText = qsTr("G-code file has been deleted and cannot be printed")
                showCancelBtn = false
                break;
            default:
                break;
            }
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
                            text: qsTr("Confirm")
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
                            text: qsTr("Cancel")
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

    LanPrinterDialog {
        visible: false
        id: idVideoDialog
        title: qsTr("Camera")
        width: 1330 * screenScaleFactor
        height: 810 * screenScaleFactor
        titleHeight: 40 * screenScaleFactor
        shadowColor: sourceTheme.dialog_shadow_color
        borderColor: sourceTheme.video_popup_border
        titleBgColor: sourceTheme.video_popup_title
        titleFtColor: sourceTheme.text_normal_color
        backgroundColor: sourceTheme.video_popup_background

        titleSource: "qrc:/UI/photo/lanPrinterSource/cameraIcon.svg"
        titleImageSize: Qt.size(11 * screenScaleFactor, 14 * screenScaleFactor)
        titleImgVisible: true

        onVisibleChanged: idVideoPlayer.state = visible ? "maxState" : "minState"

        bdContentItem: Item {

        }
    }

    Popup {
        id: idPreviewImagePopup
        width: 932 * screenScaleFactor
        height: 539 * screenScaleFactor

        x: (root.width - width) / 2
        y: (root.height - height) / 2

        background: null

        Rectangle {
            anchors.fill: parent
            anchors.margins: 5 * screenScaleFactor//shadow

            border.width: 1
            border.color: sourceTheme.image_popup_border
            color: sourceTheme.image_popup_background
            radius: 5

            Rectangle {
                anchors.fill: parent
                anchors.margins: 10 * screenScaleFactor

                color: sourceTheme.imgpreview_color
                radius: 5

                Row {
                    spacing: 20 * screenScaleFactor
                    layoutDirection: Qt.RightToLeft

                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: 20 * screenScaleFactor
                    anchors.rightMargin: 20 * screenScaleFactor

                    MouseArea {
                        width: 12 * screenScaleFactor
                        height: 12 * screenScaleFactor

                        anchors.verticalCenter: parent.verticalCenter

                        Image {
                            mipmap: true
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            source: sourceTheme.img_closeBtn
                        }

                        hoverEnabled: true
                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: idPreviewImagePopup.close()
                    }

                    MouseArea {
                        width: 14 * screenScaleFactor
                        height: 14 * screenScaleFactor
                        visible: imagePreview_max.status == Image.Error

                        anchors.verticalCenter: parent.verticalCenter

                        Image {
                            anchors.fill: parent
                            source: sourceTheme.img_refresh
                        }

                        hoverEnabled: true
                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: imagePreview_max.reload()
                    }
                }

                Image {
                    id: imagePreview_max
                    function load() {
                        reloadTimes = 2

                        source = ""
                        source = root.previewImage
                    }

                    function refresh() {
                        source = ""
                        source = root.previewImage
                    }

                    cache: false
                    mipmap: true
                    smooth: true
                    asynchronous: true
                    anchors.centerIn: parent
                    fillMode: Image.PreserveAspectFit
                    source: ""
                    property int reloadTimes: 0 // 切换设备时，界面需要刷新的东西比较多，有时会把预览图的刷新卡掉，所以要加个重载操作，确保预览图能加载进去

                    onStatusChanged: {
                        /* 加载不成功时使用默认图标 */
                        if (status === Image.Error) {
                            if (reloadTimes) {
                                reloadTimes--
                                refresh()
                            }
                        }
                    }

                }
            }

            layer.enabled: true

            layer.effect: DropShadow {
                radius: 8
                spread: 0.2
                samples: 17
                color: sourceTheme.shadow_color
            }
        }

        closePolicy: Popup.NoAutoClose | Popup.CloseOnEscape
    }

    QMLPlayer {
        visible: true
        id: idVideoPlayer

        state: "minState"

        states: [
            State {
                name: "minState"
                PropertyChanges {
                    target: idVideoPlayer
                    parent: cameraPopup
                    width: cameraPopup.width - 2 * x
                    height: cameraPopup.height - 2 * y
                    x: 20 * screenScaleFactor
                    y: 10 * screenScaleFactor
                }
            },
            State {
                name: "maxState"
                PropertyChanges {
                    target: idVideoPlayer
                    parent: idVideoDialog.cloader
                    width: 1280 * screenScaleFactor
                    height: 720 * screenScaleFactor
                    x: 20 * screenScaleFactor
                    y: 20 * screenScaleFactor
                }
            }
        ]

        Column {
            anchors.centerIn: parent
            visible: !root.hasCamera
            spacing: 20 * screenScaleFactor

            Image {
                width: 62 * screenScaleFactor
                height: 62 * screenScaleFactor
                source: sourceTheme.img_camera_connection
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                font.weight: Font.Medium
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: sourceTheme.text_camera_color
                text: ipAddress + " " + qsTr("Rejected our connection request.")
            }
        }

        Component.onDestruction: stop()
    }

    BasicScrollView {
        clip: true
        width: root.width
        height: root.height
        anchors.centerIn: parent
        vpolicyVisible: contentHeight > height
        vpolicyindicator: Rectangle {
            color: "#737378"
            radius: width / 2
            implicitWidth: 6 * screenScaleFactor
            implicitHeight: 630 * screenScaleFactor
        }

        RowLayout {
            spacing: 0
            width: root.width
            height: root.height

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 10 * screenScaleFactor
                Layout.maximumWidth: 20 * screenScaleFactor
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 10 * screenScaleFactor
                Layout.bottomMargin: 10 * screenScaleFactor
                spacing: 16 * screenScaleFactor

                ColumnLayout {
                    spacing: 0
                    id: printerItem
                    property bool showPopup: true
                    property bool isPrinting: printerState == 1
                    property bool isStopping: printerState == 4
                    property bool isSuspending: printerState == 5

                    CusRoundedBg {
                        leftTop: true
                        rightTop: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.title_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20 * screenScaleFactor
                            anchors.rightMargin: 20 * screenScaleFactor
                            spacing: 10 * screenScaleFactor

                            Image {
                                Layout.preferredWidth: 14 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/printerIcon.svg"
                            }

                            Text {
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                text: qsTr("Printing Infomation")
                                color: sourceTheme.text_normal_color
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor - 2 * parent.spacing
                                Layout.fillHeight: true
                            }

                            Rectangle {
                                Layout.preferredWidth: 100 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                visible: !root.isDeviceOnline

                                border.width: 1
                                border.color: sourceTheme.label_border_color
                                color: sourceTheme.label_background_color
                                radius: 5

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 5 * screenScaleFactor

                                    Image {
                                        width: 18 * screenScaleFactor
                                        height: 14 * screenScaleFactor
                                        source: sourceTheme.img_label_offline
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_status_color
                                        text: qsTr("Offline")
                                    }
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: childrenRect.width + 24 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                visible: root.isDeviceOnline && root.isDeviceIdle

                                border.width: 1
                                border.color: sourceTheme.label_border_color
                                color: sourceTheme.label_background_color
                                radius: 5

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 5 * screenScaleFactor

                                    Image {
                                        width: 14 * screenScaleFactor
                                        height: 14 * screenScaleFactor
                                        source: sourceTheme.img_label_idle
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_normal_color
                                        text: qsTr("Printing has stopped")
                                    }
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: childrenRect.width + 24 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                visible: root.isDeviceOnline && !root.isDeviceIdle

                                border.width: 1
                                border.color: sourceTheme.label_border_color
                                color: sourceTheme.label_background_color
                                radius: 5

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 5 * screenScaleFactor

                                    Image {
                                        width: sourceSize.width
                                        height: sourceSize.height
                                        anchors.verticalCenter: parent.verticalCenter
                                        property var methodSource: getPrinterMethodSource(printerMethod)

                                        source: methodSource[0]
                                        sourceSize: methodSource[1]
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_method_color
                                        text: getPrinterMethodText(printerMethod)
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
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
                                                        color: sourceTheme.text_normal_color
                                                        text: qsTr("Ignore")
                                                    }

                                                    onClicked:
                                                    {
                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", deviceType)
                                                    }
                                                }
                                            }
                                        }

                                        background: Rectangle {
                                            color: sourceTheme.tooltip_background_color
                                            border.color: sourceTheme.tooltip_border_color
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
                                        color: sourceTheme.text_normal_color
                                        text: qsTr("Continue")
                                    }

                                    onClicked:
                                    {
                                        if(ipAddress === "") return
                                        if(errorLayout.isRepoPlrStatus)
                                        {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "1", deviceType)
                                        }
                                        else {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_CONTINUE, " ", deviceType)
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", deviceType)
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
                                        color: sourceTheme.text_normal_color
                                        text: qsTr("Cancel")
                                    }

                                    onClicked:
                                    {
                                        if(ipAddress === "") return
                                        if(errorLayout.isRepoPlrStatus)
                                        {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "0", deviceType)
                                        }
                                        else {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_STOP, " ", deviceType)
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", deviceType)
                                        }
                                    }
                                }

                                Button {
                                    Layout.preferredWidth: 73 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor
                                    visible: errorCode >= 1 && errorCode <= 100

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
                                        color: sourceTheme.text_normal_color
                                        text: qsTr("Reboot")
                                    }

                                    onClicked:
                                    {
                                        if(ipAddress !== "")
                                        {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RESTART_KLIPPER, " ", deviceType)
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RESTART_FIRMWARE, " ", deviceType)
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                radius: height / 2
                                opacity: enabled ? 1.0 : 0.5
                                enabled: root.isWlanPrinting
                                visible: errorCode == 0 && (printerItem.isPrinting || printerItem.isSuspending)

                                border.width: 1
                                border.color: sourceTheme.btn_border_color
                                color: btnStateArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color

                                Layout.preferredWidth: 120 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor

                                Row {
                                    height: parent.height
                                    anchors.centerIn: parent
                                    spacing: 10 * screenScaleFactor

                                    Image {
                                        width: 8 * screenScaleFactor
                                        height: 10 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: `qrc:/UI/photo/lanPrinterSource/${printerItem.isSuspending?"continue":"pause"}_detail.svg`
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: printerItem.isSuspending?qsTr("Continue"):qsTr("Pause")
                                        color: sourceTheme.text_normal_color
                                    }
                                }

                                MouseArea {
                                    id: btnStateArea
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        var msgType = printerItem.isSuspending ? LanPrinterDetail.MessageType.Continue : LanPrinterDetail.MessageType.Pause
                                        idMessageDialog.showMessageDialog(msgType)
                                    }
                                }
                            }

                            Rectangle {
                                radius: height / 2
                                opacity: enabled ? 1.0 : 0.5
                                enabled: root.isWlanPrinting
                                visible: errorCode == 0 && (printerItem.isPrinting || printerItem.isSuspending)

                                border.width: 1
                                border.color: sourceTheme.btn_border_color
                                color: btnStopArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color

                                Layout.preferredWidth: 120 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor

                                Row {
                                    height: parent.height
                                    anchors.centerIn: parent
                                    spacing: 10 * screenScaleFactor

                                    Image {
                                        width: 12 * screenScaleFactor
                                        height: 12 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: "qrc:/UI/photo/lanPrinterSource/stop_detail.svg"
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Stop")
                                        color: sourceTheme.text_normal_color
                                    }
                                }

                                MouseArea {
                                    id: btnStopArea
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.Stop)
                                }
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor
                                Layout.preferredHeight: 12 * screenScaleFactor

                                Image {
                                    anchors.centerIn: parent
                                    width: 10 * screenScaleFactor
                                    height: 6 * screenScaleFactor
                                    source: printerItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        printerItem.showPopup = !printerItem.showPopup
                                        printerPopup.visible = printerItem.showPopup
                                    }
                                }
                            }
                        }
                    }

                    CusRoundedBg {
                        id: printerPopup
                        visible: true
                        leftBottom: true
                        rightBottom: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.popup_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.minimumHeight: 120 * screenScaleFactor
                        Layout.maximumHeight: 178 * screenScaleFactor

                        RowLayout {
                            spacing: 0
                            anchors.fill: parent
                            anchors.topMargin: 10 * screenScaleFactor
                            anchors.bottomMargin: 10 * screenScaleFactor

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 10 * screenScaleFactor
                                Layout.maximumWidth: 20 * screenScaleFactor
                            }

                            Rectangle {
                                radius: 5
                                border.width: 1
                                border.color: sourceTheme.imgpreview_border
                                color: sourceTheme.imgpreview_color

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 96 * screenScaleFactor
                                Layout.maximumWidth: 200 * screenScaleFactor
                                Layout.minimumHeight: 96 * screenScaleFactor
                                Layout.maximumHeight: 135 * screenScaleFactor

                                Row {
                                    spacing: 16 * screenScaleFactor
                                    layoutDirection: Qt.RightToLeft

                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.topMargin: 10 * screenScaleFactor
                                    anchors.rightMargin: 10 * screenScaleFactor

                                    MouseArea {
                                        id: maximizeBtn
                                        width: 14 * screenScaleFactor
                                        height: 14 * screenScaleFactor
//                                        visible: !root.isDeviceIdle && imagePreview_min.status == Image.Ready

                                        anchors.verticalCenter: parent.verticalCenter

                                        Image {
                                            anchors.fill: parent
                                            source: sourceTheme.img_enlarge
                                        }

                                        hoverEnabled: true
                                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: {
                                            imagePreview_max.load()
                                            idPreviewImagePopup.open()
                                        }
                                    }

                                    MouseArea {
                                        id: refreshBtn
                                        width: 14 * screenScaleFactor
                                        height: 14 * screenScaleFactor
//                                        visible: imagePreview_min.status == Image.Error && !root.isDeviceIdle

                                        anchors.verticalCenter: parent.verticalCenter

                                        Image {
                                            anchors.fill: parent
                                            source: sourceTheme.img_refresh
                                        }

                                        hoverEnabled: true
                                        cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: imagePreview_min.load()

                                    }
                                }

                                Image {
                                    id: imagePreview_min
                                    width: 96 * screenScaleFactor
                                    height: 96 * screenScaleFactor

                                    function init() {
                                        delayLoadModelItem.stop()
                                        needLoadModelItem = false
                                    }

                                    function load() {
                                        refreshBtn.visible = false
                                        maximizeBtn.visible = false
                                        source = ""
                                        reloadTimes = 0
                                        source = root.previewImage
                                    }

                                    function refresh() {
                                        source = ""
                                        source = root.previewImage
                                    }

                                    cache: false
                                    mipmap: true
                                    smooth: true
                                    asynchronous: true
                                    anchors.centerIn: parent
                                    fillMode: Image.PreserveAspectFit
                                    sourceSize: Qt.size(width, height)
                                    source: ""
                                    property int reloadTimes: 0 // 切换设备时，界面需要刷新的东西比较多，有时会把预览图的刷新卡掉，所以要加个重载操作，确保预览图能加载进去

                                    onStatusChanged: {
                                        if (source == "")
                                            return

                                        if (status === Image.Error) {
                                            if (reloadTimes) {
                                                reloadTimes--
                                                refresh()
                                            } else {
                                                /* 加载不成功时使用默认图标 */
                                                source = defaultImage
                                                refreshBtn.visible = true && !root.isDeviceIdle
                                                maximizeBtn.visible = false
                                            }
                                        } else if (status === Image.Ready){
                                            if (source == defaultImage) {
                                                refreshBtn.visible = !root.isDeviceIdle
                                                maximizeBtn.visible = false
                                                needLoadModelItem = true
                                            } else {
                                                if (reloadTimes) {
                                                    reloadTimes--
                                                    refresh()
                                                } else {
                                                    refreshBtn.visible = false
                                                    maximizeBtn.visible = true && !root.isDeviceIdle
                                                    needLoadModelItem = true
                                                }
                                            }
                                        }
                                    }

//                                    Timer {
//                                        /* 当前打印的预览图加载完了再加载ModelItem（列表）
//                                           qml的Image不能多个同时加载，同时加载存在图片混淆的问题，因此这里需要做加载顺序的处理 */
//                                        id: delayLoadModelItem
//                                        interval: 600
//                                        repeat: false
//                                        onTriggered: {
//                                        }
//                                    }

                                }

                                Image {
                                    width: parent.width
                                    height: parent.height / 2
                                    anchors.bottom: parent.bottom
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    source: sourceTheme.imgpreview_source
                                }


                                Canvas {
                                    id: wave

                                    anchors.fill: parent
                                    anchors.margins: parent.border.width
                                    visible: !root.isDeviceIdle

                                    // 数值范围
                                    property real maxValue: 100
                                    property real minValue: 0
                                    property real curValue: deviceItem?deviceItem.pcCurPrintProgress:0

                                    // 进度
                                    property real curProgress: (curValue - minValue) / (maxValue - minValue) * 100

                                    // 画布

                                    property color waveFontColor: "#78C3ED" // 前波浪颜色
                                    property color waveBackColor: "#D2EBF9" // 后波浪颜色

                                    // 波浪参数
                                    property real waveWidth: 0.05    // 波浪宽度, 数越小越宽
                                    property real waveHeight: 3     // 波浪高度, 数越大越高
                                    property real speed: 0.1        // 波浪速度, 数越大速度越快
                                    property real offset: 0         // 波浪 x 偏移量, 用于动画

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.save()

                                        // 画波浪, 可以导出波浪个数属性
                                        // drawWave(ctx, waveBackColor, 1.5, -1, true)
                                        drawWave(ctx, waveFontColor, 0, 0, false)

                                        ctx.restore()
                                    }

                                    // 画笔， 颜色， x偏移， y偏移，角度值取反
                                    function drawWave(ctx, w_color, x_offset, y_offset, reverse = false) {
                                        //sin曲线
                                        ctx.beginPath()
                                        var x_base = 0
                                        var y_base = height - height * curProgress / 100

                                        //波浪曲线部分，横坐标步进为5px
                                        for (var x_value = 0; x_value <= width + 5; x_value += 5) {
                                            var y_value = waveHeight * Math.sin((reverse ? -1 : 1)
                                                                                * (x_value) * waveWidth + offset + x_offset) + y_offset
                                            ctx.lineTo(x_base + x_value, y_base + y_value)
                                        }

                                        //底部两端围成实心
                                        ctx.lineTo(width, height)
                                        ctx.lineTo(0, height)
                                        ctx.closePath()
                                        ctx.fillStyle = w_color
                                        ctx.globalAlpha  = 0.3
                                        ctx.fill()
                                    }

                                    Timer {
                                        running: wave.visible
                                        repeat: true
                                        interval: 50
                                        onTriggered:{
                                            //波浪移动
                                            wave.offset += wave.speed;
                                            wave.offset %= Math.PI * 2;
                                            wave.requestPaint();
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 10 * screenScaleFactor
                                Layout.maximumWidth: 20 * screenScaleFactor
                            }

                            ColumnLayout {
                                spacing: 20 * screenScaleFactor

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_special_color
                                        text: qsTr("File") + "："
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 150 * screenScaleFactor
                                        Layout.maximumWidth: 300 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        Text {
                                            id: id__GcodeName
                                            anchors.fill: parent
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            verticalAlignment: Text.AlignVCenter
                                            color: sourceTheme.text_normal_color
                                            elide: Text.ElideMiddle
                                            text: curGcodeName
                                            visible: !root.isDeviceIdle

                                            MouseArea {
                                                id: id__GcodeNameArea
                                                hoverEnabled: true
                                                anchors.fill: parent
                                                acceptedButtons: Qt.NoButton
                                            }
                                        }

                                        ToolTip.text: id__GcodeName.text
                                        ToolTip.visible: id__GcodeNameArea.containsMouse && id__GcodeName.truncated//elide set
                                    }
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Layer") + "："
                                        color: sourceTheme.text_special_color
                                    }

                                    Text {
                                        property int curLayer: deviceItem?deviceItem.pcCurPrintLayer:0
                                        property int totalLayer: deviceItem?deviceItem.pcTotalPrintLayer:0

                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        visible: !root.isDeviceIdle

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: `${curLayer} / ${totalLayer}`
                                        color: sourceTheme.text_normal_color
                                    }
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Rectangle {
                                        radius: 1
                                        id: newProgressBar
                                        color: sourceTheme.print_progressbar_color
                                        property real progress: deviceItem?deviceItem.pcCurPrintProgress:0

                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 150 * screenScaleFactor
                                        Layout.maximumWidth: 300 * screenScaleFactor
                                        Layout.preferredHeight: 2 * screenScaleFactor

                                        Rectangle {
                                            width: parent.width * parent.progress / 100.0
                                            height: 2 * screenScaleFactor
                                            visible: !root.isDeviceIdle

                                            anchors.left: parent.left
                                            anchors.verticalCenter: parent.verticalCenter

                                            color: "#1E9BE2"
                                            radius: 1
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        visible: !root.isDeviceIdle

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_12
                                        text: `${newProgressBar.progress}%`
                                        color: sourceTheme.text_normal_color
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 10 * screenScaleFactor
                            }

                            Rectangle {
                                Layout.fillHeight: true
                                Layout.minimumHeight: 50
                                Layout.maximumHeight: 100
                                Layout.preferredWidth: 1 * screenScaleFactor
                                color: sourceTheme.item_crossline_color
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 10 * screenScaleFactor
                            }

                            ColumnLayout {
                                spacing: 20 * screenScaleFactor
                                visible: !root.isDeviceIdle

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Print time") + "："
                                        color: sourceTheme.text_special_color
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: deviceItem?second2String(deviceItem.pcTotalPrintTime):"undefined"
                                        color: sourceTheme.text_normal_color
                                    }
                                }

                                RowLayout {
                                    spacing: 10 * screenScaleFactor

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Time left") + "："
                                        color: sourceTheme.text_special_color
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: deviceItem?second2String(deviceItem.pcLeftTime):"undefined"
                                        color: sourceTheme.text_normal_color
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumWidth: 10 * screenScaleFactor
                                Layout.maximumWidth: 20 * screenScaleFactor
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: 0
                    id: controlItem
                    property bool showPopup: true
                    property string currentUnit: "10"

                    CusRoundedBg {
                        leftTop: true
                        rightTop: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.title_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20 * screenScaleFactor
                            anchors.rightMargin: 20 * screenScaleFactor
                            spacing: 10 * screenScaleFactor

                            Image {
                                Layout.preferredWidth: 14 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/controlIcon.svg"
                            }

                            Text {
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                text: qsTr("Control")
                                color: sourceTheme.text_normal_color
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor
                                Layout.preferredHeight: 12 * screenScaleFactor

                                Image {
                                    anchors.centerIn: parent
                                    width: 10 * screenScaleFactor
                                    height: 6 * screenScaleFactor
                                    source: controlItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        controlItem.showPopup = !controlItem.showPopup
                                        controlPopup.visible = controlItem.showPopup
                                    }
                                }
                            }
                        }
                    }

                    CusRoundedBg {
                        id: controlPopup
                        visible: true
                        leftBottom: true
                        rightBottom: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.popup_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        implicitHeight:colLayoutItem.height + 30 * screenScaleFactor


                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: parent.forceActiveFocus()
                        }

                        ColumnLayout {
                            anchors.topMargin: 10 * screenScaleFactor
                            anchors.bottomMargin: 10 * screenScaleFactor
                            width: parent.width
                            id: colLayoutItem
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight:20 * screenScaleFactor
                                Layout.maximumHeight: 30 * screenScaleFactor
                            }
                            RowLayout {
                                spacing: 0
                                Layout.leftMargin: 30 * screenScaleFactor
                                Layout.rightMargin: 30 * screenScaleFactor
                                Layout.minimumHeight: 140 * screenScaleFactor
                                Layout.maximumHeight: 200 * screenScaleFactor
                                GridLayout {
                                    columns: 4
                                    rowSpacing: 10 * screenScaleFactor
                                    columnSpacing: 10 * screenScaleFactor

                                    Item {
                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled && root.xyAutohome

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: y_plusArea.containsMouse ? "#1E9BE2" : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Column {
                                            width: parent.width
                                            anchors.centerIn: parent
                                            spacing: 5 * screenScaleFactor

                                            Image {
                                                width: 10 * screenScaleFactor
                                                height: 8 * screenScaleFactor
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                source: "qrc:/UI/photo/lanPrinterSource/axis_plusIcon.svg"
                                            }

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: y_plusArea.containsMouse ? "white" : sourceTheme.text_normal_color
                                                text: "Y+"
                                            }
                                        }

                                        MouseArea {
                                            id: y_plusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                var targetValue = parseFloat(idYPostion.text) + curUnit
                                                if(!isKlipper_4408 && targetValue > deviceItem.machineHeight){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? `Y${curUnit} F3000` : targetValue
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_Y, postion, deviceType)
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled && root.zAutohome

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: z_plusArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Column {
                                            width: parent.width
                                            anchors.centerIn: parent
                                            spacing: 5 * screenScaleFactor

                                            Image {
                                                width: 10 * screenScaleFactor
                                                height: 8 * screenScaleFactor
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                source: "qrc:/UI/photo/lanPrinterSource/axis_plusIcon.svg"
                                            }

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "Z+"
                                            }
                                        }

                                        MouseArea {
                                            id: z_plusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                var targetValue = parseFloat(idZPostion.text) + curUnit
                                                if(!isKlipper_4408 && targetValue > deviceItem.machineDepth){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? `Z${curUnit} F600` : targetValue
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_Z, postion, deviceType)
                                            }
                                        }
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled && root.xyAutohome

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: x_minusArea.containsMouse ? "#1E9BE2" : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Row {
                                            height: parent.height
                                            anchors.centerIn: parent
                                            spacing: 5 * screenScaleFactor

                                            Image {
                                                width: 8 * screenScaleFactor
                                                height: 10 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter
                                                source: "qrc:/UI/photo/lanPrinterSource/x_minusIcon.svg"
                                            }

                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: x_minusArea.containsMouse ? "white" : sourceTheme.text_normal_color
                                                text: "X-"
                                            }
                                        }

                                        MouseArea {
                                            id: x_minusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                var targetValue = parseFloat(idXPostion.text) - curUnit
                                                if(!isKlipper_4408 && targetValue < 0){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? `X-${curUnit} F3000` : targetValue
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_X, postion, deviceType)
                                            }
                                        }
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: xy_homeArea.containsMouse ? "#1E9BE2" : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Image {
                                            anchors.centerIn: parent
                                            width: 14 * screenScaleFactor
                                            height: 14 * screenScaleFactor
                                            source: "qrc:/UI/photo/lanPrinterSource/axis_homeIcon.svg"
                                        }

                                        MouseArea {
                                            id: xy_homeArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_XY0, " ", deviceType)
                                            }
                                        }
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled && root.xyAutohome

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: x_plusArea.containsMouse ? "#1E9BE2" : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Row {
                                            spacing: 5
                                            height: parent.height
                                            anchors.centerIn: parent

                                            Image {
                                                width: 8 * screenScaleFactor
                                                height: 10 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter
                                                source: "qrc:/UI/photo/lanPrinterSource/x_plusIcon.svg"
                                            }

                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: x_plusArea.containsMouse ? "white" : sourceTheme.text_normal_color
                                                text: "X+"
                                            }
                                        }

                                        MouseArea {
                                            id: x_plusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                var targetValue = parseFloat(idXPostion.text) + curUnit
                                                if(!isKlipper_4408 && targetValue > deviceItem.machineWidth){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? `X${curUnit} F3000` : targetValue
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_X, postion, deviceType)
                                            }
                                        }
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: z_homeArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Image {
                                            anchors.centerIn: parent
                                            width: 14 * screenScaleFactor
                                            height: 14 * screenScaleFactor
                                            source: "qrc:/UI/photo/lanPrinterSource/axis_homeIcon.svg"
                                        }

                                        MouseArea {
                                            id: z_homeArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_Z0, " ", deviceType)
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled && root.xyAutohome

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: y_minusArea.containsMouse ? "#1E9BE2" : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Column {
                                            width: parent.width
                                            anchors.centerIn: parent
                                            spacing: 5 * screenScaleFactor

                                            Image {
                                                width: 10 * screenScaleFactor
                                                height: 8 * screenScaleFactor
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                source: "qrc:/UI/photo/lanPrinterSource/axis_minusIcon.svg"
                                            }

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: y_minusArea.containsMouse ? "white" : sourceTheme.text_normal_color
                                                text: "Y-"
                                            }
                                        }

                                        MouseArea {
                                            id: y_minusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                var targetValue = parseFloat(idYPostion.text) - curUnit
                                                if(!isKlipper_4408 && targetValue < 0){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? `Y-${curUnit} F3000` : targetValue
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_Y, postion, deviceType)
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor
                                    }

                                    Rectangle {
                                        radius: 5
                                        opacity: enabled ? 1.0 : 0.5
                                        enabled: root.controlEnabled && root.zAutohome

                                        border.width: 1
                                        border.color: sourceTheme.btn_border_color
                                        color: z_minusArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Column {
                                            width: parent.width
                                            anchors.centerIn: parent
                                            spacing: 5 * screenScaleFactor

                                            Image {
                                                width: 10 * screenScaleFactor
                                                height: 8 * screenScaleFactor
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                source: "qrc:/UI/photo/lanPrinterSource/axis_minusIcon.svg"
                                            }

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "Z-"
                                            }
                                        }

                                        MouseArea {
                                            id: z_minusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                var targetValue = parseFloat(idZPostion.text) - curUnit
                                                if(!isKlipper_4408 && targetValue < 0){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? `Z-${curUnit} F600` : targetValue
                                                if(ipAddress !== "") printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_MOVE_Z, postion, deviceType)
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    Layout.minimumWidth: 20 * screenScaleFactor
                                    Layout.maximumWidth: 100 * screenScaleFactor
                                }

                                ColumnLayout {
                                    spacing: 10 * screenScaleFactor

                                    RowLayout {
                                        spacing: 10 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Rectangle {
                                            radius: 5
                                            border.width: 1
                                            border.color: sourceTheme.item_rectangle_border
                                            color: sourceTheme.popup_background_color

                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 83 * screenScaleFactor
                                            Layout.maximumWidth: 110 * screenScaleFactor
                                            Layout.preferredHeight: 28 * screenScaleFactor

                                            Text {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 11 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "X："
                                            }

                                            Text {
                                                id: idXPostion
                                                anchors.right: parent.right
                                                anchors.rightMargin: 11 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: deviceItem?(deviceItem.pcX).toFixed(1):"undefined"
                                            }
                                        }

                                        Rectangle {
                                            radius: 5
                                            border.width: 1
                                            border.color: sourceTheme.item_rectangle_border
                                            color: sourceTheme.popup_background_color

                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 83 * screenScaleFactor
                                            Layout.maximumWidth: 110 * screenScaleFactor
                                            Layout.preferredHeight: 28 * screenScaleFactor

                                            Text {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 11 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "Y："
                                            }

                                            Text {
                                                id: idYPostion
                                                anchors.right: parent.right
                                                anchors.rightMargin: 11 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: deviceItem?(deviceItem.pcY).toFixed(1):"undefined"
                                            }
                                        }

                                        Rectangle {
                                            radius: 5
                                            border.width: 1
                                            border.color: sourceTheme.item_rectangle_border
                                            color: sourceTheme.popup_background_color

                                            Layout.fillWidth: true
                                            Layout.minimumWidth: 84 * screenScaleFactor
                                            Layout.maximumWidth: 110 * screenScaleFactor
                                            Layout.preferredHeight: 28 * screenScaleFactor

                                            Text {
                                                anchors.left: parent.left
                                                anchors.leftMargin: 11 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter
                                                verticalAlignment: Text.AlignVCenter

                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "Z："
                                            }

                                            Text {
                                                id: idZPostion
                                                anchors.right: parent.right
                                                anchors.rightMargin: 11 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: deviceItem?(deviceItem.pcZ).toFixed(1):"undefined"
                                            }
                                        }
                                    }

                                    RowLayout {
                                        spacing: 10 * screenScaleFactor

                                        Repeater {
                                            model: ["0.1", "1", "10", "100"]
                                            delegate: RadioButton {
                                                checkable: true
                                                indicator: null
                                                opacity: enabled ? 1.0 : 0.5
                                                enabled: root.controlEnabled

                                                Layout.fillWidth: true
                                                Layout.minimumWidth: 60 * screenScaleFactor
                                                Layout.maximumWidth: 80 * screenScaleFactor
                                                Layout.preferredHeight: 40 * screenScaleFactor

                                                background: Rectangle {
                                                    radius: 5
                                                    anchors.fill: parent

                                                    border.width: 1
                                                    border.color: sourceTheme.btn_border_color
                                                    color: parent.checked?"#1E9BE2":(parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color)
                                                }

                                                Text {
                                                    anchors.centerIn: parent
                                                    verticalAlignment: Text.AlignVCenter
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    text: `${modelData}mm`
                                                    color: parent.checked ? "white" : sourceTheme.text_normal_color
                                                }

                                                Component.onCompleted: if(index == 2) checked = true//default
                                                onCheckedChanged: if(checked) controlItem.currentUnit = modelData
                                            }
                                        }
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        Layout.minimumWidth: 270 * screenScaleFactor
                                        Layout.maximumWidth: 350 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor
                                        spacing: 10 * screenScaleFactor

                                        Text {
                                            Layout.preferredWidth: contentWidth * screenScaleFactor
                                            Layout.preferredHeight: 28 * screenScaleFactor

                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Normal
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: qsTr("Printing speed") + "："
                                            color: sourceTheme.text_normal_color
                                        }

                                        Rectangle {
                                            id: _printSpeedBox
                                            opacity: enabled ? 1.0 : 0.5
                                            enabled: root.controlEnabled || root.isWlanPrinting

                                            property int minValue: 10
                                            property int maxValue: 300
                                            property var delayShow: false
                                            property var printSpeed: deviceItem?deviceItem.pcPrintSpeed:"undefined"
                                            property alias boxValue: _editPrintSpeedBox.text

                                            function specifyPcPrintSpeed() {
                                                if(ipAddress !== "")
                                                {
                                                    delayShow = true
                                                    printSpeedDelayShow.start()
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_FEEDRATEPCT, boxValue, deviceType)
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

                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 28 * screenScaleFactor

                                            radius: 5
                                            border.width: 1
                                            border.color: sourceTheme.item_rectangle_border
                                            color: sourceTheme.popup_background_color

                                            Binding on boxValue {
                                                when: !_editPrintSpeedBox.activeFocus && !_printSpeedBox.delayShow
                                                value: _printSpeedBox.printSpeed
                                            }

                                            Timer {
                                                repeat: false
                                                interval: 10000
                                                id: printSpeedDelayShow
                                                onTriggered: _printSpeedBox.delayShow = false
                                            }

                                            TextField {
                                                id: _editPrintSpeedBox
                                                width: parent.width
                                                height: parent.height
                                                anchors.left: parent.left
                                                anchors.verticalCenter: parent.verticalCenter

                                                validator: RegExpValidator{regExp: new RegExp("[1-9][\\d]|[1-2][\\d][\\d]|300")}
                                                selectByMouse: true
                                                selectionColor: sourceTheme.textedit_selection_color
                                                selectedTextColor: color
                                                topPadding: 0
                                                bottomPadding: 0
                                                leftPadding: 6 * screenScaleFactor
                                                rightPadding: 4 * screenScaleFactor
                                                verticalAlignment: TextInput.AlignVCenter
                                                horizontalAlignment: TextInput.AlignLeft
                                                color: sourceTheme.text_normal_color
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                background: null

                                                Keys.onUpPressed: _printSpeedBox.increase()
                                                Keys.onDownPressed: _printSpeedBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                //onActiveFocusChanged: mItem.pcPrintSpeedFocus = activeFocus
                                                onEditingFinished: _printSpeedBox.specifyPcPrintSpeed()
                                            }

                                            Text {
                                                anchors.left: parent.left
                                                anchors.verticalCenter: parent.verticalCenter
                                                anchors.leftMargin: _editPrintSpeedBox.contentWidth + _editPrintSpeedBox.leftPadding + 4 * screenScaleFactor

                                                font: _editPrintSpeedBox.font
                                                color: _editPrintSpeedBox.color
                                                verticalAlignment: Text.AlignVCenter
                                                text: "%"
                                            }

                                            Image {
                                                width: 7 * screenScaleFactor
                                                height: 4 * screenScaleFactor

                                                anchors.right: parent.right
                                                anchors.top: parent.top
                                                anchors.rightMargin: 8 * screenScaleFactor
                                                anchors.topMargin: 8 * screenScaleFactor

                                                visible: _editPrintSpeedBox.activeFocus
                                                source: upButtonArea_0.containsMouse ? sourceTheme.img_upBtn_hover : sourceTheme.img_upBtn

                                                MouseArea {
                                                    id: upButtonArea_0
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    onClicked: _printSpeedBox.increase()
                                                }
                                            }

                                            Image {
                                                width: 7 * screenScaleFactor
                                                height: 4 * screenScaleFactor

                                                anchors.right: parent.right
                                                anchors.bottom: parent.bottom
                                                anchors.rightMargin: 8 * screenScaleFactor
                                                anchors.bottomMargin: 7 * screenScaleFactor

                                                visible: _editPrintSpeedBox.activeFocus
                                                source: downButtonArea_0.containsMouse ? sourceTheme.img_downBtn_hover : sourceTheme.img_downBtn

                                                MouseArea {
                                                    id: downButtonArea_0
                                                    anchors.fill: parent
                                                    hoverEnabled: true
                                                    onClicked: _printSpeedBox.decrease()
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight:20 * screenScaleFactor
                                Layout.maximumHeight: 30 * screenScaleFactor
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: flowItem.height
                                Layout.leftMargin: 30 * screenScaleFactor
                                Layout.rightMargin: 30 * screenScaleFactor
                                color: "transparent"
                                Flow {
                                    spacing: 10* screenScaleFactor
                                    width: parent.width
                                    id: flowItem
                                    Rectangle{
                                        width: 207 * screenScaleFactor
                                        height: 109 * screenScaleFactor
                                        color: sourceTheme.fan_block_bgColor
                                        radius: 5 * screenScaleFactor
                                        border.width: 1 * screenScaleFactor
                                        border.color: Constants.currentTheme ? sourceTheme.btn_border_color : "transparent"
                                        ColumnLayout{
                                            y:15* screenScaleFactor
                                            x:15* screenScaleFactor
                                            RowLayout {
                                                spacing: 10 * screenScaleFactor
                                                Text {
                                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                                    Layout.preferredHeight: 14 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    text: qsTr("Fan switch")
                                                    color: sourceTheme.text_normal_color
                                                }

                                                Button {
                                                    id: fanSwitch
                                                    checkable: true
                                                    background: null
                                                    opacity: enabled ? 1.0 : 0.5
                                                    enabled: root.controlEnabled || root.isWlanPrinting

                                                    property var delayShow: false
                                                    property bool fan_checked: deviceItem?deviceItem.fanOpened:false

                                                    Layout.preferredWidth: 50 * screenScaleFactor
                                                    Layout.preferredHeight: 32 * screenScaleFactor

                                                    Binding on checked {
                                                        when: !fanSwitch.delayShow
                                                        value: fanSwitch.fan_checked
                                                    }

                                                    Timer {
                                                        repeat: false
                                                        interval: 10000
                                                        id: fanSwitchDelayShow
                                                        onTriggered: fanSwitch.delayShow = false
                                                    }

                                                    onClicked: {
                                                        var value = checked ? "1" : "0"
                                                        if(ipAddress !== "")
                                                        {
                                                            fanSwitch.delayShow = true
                                                            fanSwitchDelayShow.start()
                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_FAN, value, deviceType)
                                                        }
                                                    }

                                                    indicator: Rectangle {
                                                        radius: height / 2
                                                        anchors.fill: parent

                                                        border.width: fanSwitch.checked ? 0 : 1
                                                        border.color: sourceTheme.switch_border_color
                                                        color: fanSwitch.checked ? sourceTheme.switch_checked_background : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter

                                                            color: fanSwitch.checked ? "#00A3FF" : sourceTheme.switch_indicator_color
                                                            x: fanSwitch.checked ? (parent.width - width - 3 * screenScaleFactor) : 3 * screenScaleFactor

                                                            Behavior on x {
                                                                NumberAnimation { duration: 100 }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            ColumnLayout{
                                                Item {
                                                    Layout.fillHeight: true
                                                    Layout.fillWidth: true
                                                    implicitHeight: 5* screenScaleFactor
                                                }
                                                ColumnLayout{
                                                    RowLayout{
                                                        Image {
                                                            Layout.preferredWidth: 14 * screenScaleFactor
                                                            Layout.preferredHeight: 14 * screenScaleFactor
                                                            source: fanSwitch.checked ? sourceTheme.img_fans_open :sourceTheme.img_fans_close
                                                            id: rotatingImage
                                                            property real rotationAngle: 0
                                                            transform: Rotation {
                                                                origin.x: rotatingImage.width/2
                                                                origin.y: rotatingImage.height/2
                                                                angle: rotatingImage.rotationAngle
                                                            }
                                                            PropertyAnimation {
                                                                target: rotatingImage
                                                                property: "rotationAngle"
                                                                from: 0
                                                                to: 360
                                                                duration: 1000
                                                                running: fanSwitch.checked
                                                                loops: Animation.Infinite
                                                            }
                                                        }
                                                        Text {
                                                            visible: !!deviceItem.fanSpeed && fanSwitch.checked
                                                            Layout.preferredWidth: contentWidth * screenScaleFactor
                                                            Layout.preferredHeight: 14 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            font.family: Constants.mySystemFont.name
                                                            text: qsTr("fan speed") + ":"
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.text_normal_color
                                                        }
                                                        Text {
                                                            visible: !!deviceItem.fanSpeed && fanSwitch.checked
                                                            text:parseInt(fanSpeedId.value*100,10)  + "%"
                                                            font.pointSize: Constants.labelFontPointSize_10
                                                            font.weight: Font.Black
                                                            color: sourceTheme.text_normal_color
                                                            font.family: Constants.mySystemFont.name
                                                            Layout.preferredWidth: contentWidth * screenScaleFactor
                                                            Layout.preferredHeight: 14 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                        }
                                                    }
                                                    Slider{
                                                        visible: !!deviceItem.fanSpeed && fanSwitch.checked
                                                        id:fanSpeedId
                                                        value: deviceItem ? deviceItem.fanSpeed/100 : 0
                                                        background: Rectangle {
                                                            x: fanSpeedId.leftPadding
                                                            y: fanSpeedId.topPadding + fanSpeedId.availableHeight / 2 - height / 2
                                                            implicitWidth: 186* screenScaleFactor
                                                            implicitHeight: 2* screenScaleFactor
                                                            width: fanSpeedId.availableWidth
                                                            height: implicitHeight
                                                            radius: 2* screenScaleFactor
                                                            color: "#bdbebf"

                                                            Rectangle {
                                                                width: fanSpeedId.visualPosition * parent.width
                                                                height: parent.height
                                                                color: "#144F71"
                                                                radius: 2* screenScaleFactor
                                                            }
                                                        }

                                                        handle: Rectangle {
                                                            x: fanSpeedId.leftPadding + fanSpeedId.visualPosition * (fanSpeedId.availableWidth - width)
                                                            y: fanSpeedId.topPadding + fanSpeedId.availableHeight / 2 - height / 2
                                                            implicitWidth: 10* screenScaleFactor
                                                            implicitHeight: 10* screenScaleFactor
                                                            radius: 5* screenScaleFactor
                                                            color: "#00A3FF"

                                                        }
                                                        onPressedChanged: {
                                                            if(!pressed)
                                                                printerListModel.cSourceModel.sendUIControlCmdToDevice(
                                                                            ipAddress,
                                                                            LanPrinterListLocal.PrintControlType.CONTROL_CMD_MODELFANPCT,
                                                                            parseInt(fanSpeedId.value*100,10), deviceType
                                                                            )
                                                        }

                                                    }
                                                }

                                            }
                                        }
                                    }
                                    Rectangle {
                                        visible: isKlipper_4408
                                        width: 207 * screenScaleFactor
                                        height: 109 * screenScaleFactor
                                        color: sourceTheme.fan_block_bgColor
                                        radius: 5 * screenScaleFactor
                                        border.width: 1 * screenScaleFactor
                                        border.color: Constants.currentTheme ? sourceTheme.btn_border_color : "transparent"
                                        ColumnLayout{
                                            y:15* screenScaleFactor
                                            x:15* screenScaleFactor


                                            RowLayout {
                                                visible: isKlipper_4408
                                                spacing: 10 * screenScaleFactor

                                                Text {
                                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                                    Layout.preferredHeight: 14 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    text: qsTr("Auxiliary fan sw")
                                                    color: sourceTheme.text_normal_color
                                                }

                                                Button {
                                                    id: auxiliaryFan
                                                    checkable: true
                                                    background: null
                                                    opacity: enabled ? 1.0 : 0.5
                                                    enabled: root.controlEnabled || root.isWlanPrinting

                                                    property var delayShow: false
                                                    property bool fan_checked: deviceItem ? deviceItem.auxiliaryFan : false

                                                    Layout.preferredWidth: 50 * screenScaleFactor
                                                    Layout.preferredHeight: 32 * screenScaleFactor

                                                    Binding on checked {
                                                        when: !auxiliaryFan.delayShow
                                                        value: auxiliaryFan.fan_checked
                                                    }

                                                    Timer {
                                                        repeat: false
                                                        interval: 10000
                                                        id: auxiliaryFanDelayShow
                                                        onTriggered: auxiliaryFan.delayShow = false
                                                    }

                                                    onClicked: {
                                                        var value = checked ? "1" : "0"
                                                        if(ipAddress !== "")
                                                        {
                                                            auxiliaryFan.delayShow = true
                                                            auxiliaryFanDelayShow.start()
                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_FANAUXILIARY, value, deviceType)
                                                        }
                                                    }

                                                    indicator: Rectangle {
                                                        radius: height / 2
                                                        anchors.fill: parent

                                                        border.width: auxiliaryFan.checked ? 0 : 1
                                                        border.color: sourceTheme.switch_border_color
                                                        color: auxiliaryFan.checked ? sourceTheme.switch_checked_background : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter

                                                            color: auxiliaryFan.checked ? "#00A3FF" : sourceTheme.switch_indicator_color
                                                            x: auxiliaryFan.checked ? (parent.width - width - 3 * screenScaleFactor) : 3 * screenScaleFactor

                                                            Behavior on x {
                                                                NumberAnimation { duration: 100 }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            Item {
                                                Layout.fillHeight: true
                                                Layout.fillWidth: true
                                                implicitHeight: 5* screenScaleFactor
                                            }
                                            ColumnLayout{
                                                RowLayout{
                                                    Image {
                                                        Layout.preferredWidth: 14 * screenScaleFactor
                                                        Layout.preferredHeight: 14 * screenScaleFactor
                                                        source: auxiliaryFan.checked ? sourceTheme.img_fans_open :sourceTheme.img_fans_close
                                                        id: auxiliaryFanImage
                                                        property real rotationAngle: 0
                                                        transform: Rotation {
                                                            origin.x: auxiliaryFanImage.width/2
                                                            origin.y: auxiliaryFanImage.height/2
                                                            angle: auxiliaryFanImage.rotationAngle
                                                        }
                                                        PropertyAnimation {
                                                            target: auxiliaryFanImage
                                                            property: "rotationAngle"
                                                            from: 0
                                                            to: 360
                                                            duration: 1000
                                                            running: auxiliaryFan.checked
                                                            loops: Animation.Infinite
                                                        }
                                                    }
                                                    Text {
                                                        visible: !!deviceItem.auxiliaryFanSpeed && auxiliaryFan.checked
                                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                                        Layout.preferredHeight: 14 * screenScaleFactor
                                                        verticalAlignment: Text.AlignVCenter
                                                        font.family: Constants.mySystemFont.name
                                                        text: qsTr("fan speed") + ":"
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: sourceTheme.text_normal_color
                                                    }
                                                    Text {
                                                        visible: !!deviceItem.auxiliaryFanSpeed && auxiliaryFan.checked
                                                        text:parseInt(auxiliaryFanSpeedId.value*100,10)  + "%"
                                                        font.pointSize: Constants.labelFontPointSize_10
                                                        font.weight: Font.Black
                                                        color: sourceTheme.text_normal_color
                                                        font.family: Constants.labelFontFamilyBold
                                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                                        Layout.preferredHeight: 14 * screenScaleFactor
                                                        verticalAlignment: Text.AlignVCenter
                                                    }
                                                }
                                                Slider{
                                                    id:auxiliaryFanSpeedId
                                                    visible: !!deviceItem.auxiliaryFanSpeed && auxiliaryFan.checked
                                                    value: deviceItem ? deviceItem.auxiliaryFanSpeed/100 : 0
                                                    background: Rectangle {
                                                        x: auxiliaryFanSpeedId.leftPadding
                                                        y: auxiliaryFanSpeedId.topPadding + auxiliaryFanSpeedId.availableHeight / 2 - height / 2
                                                        implicitWidth: 186* screenScaleFactor
                                                        implicitHeight: 2* screenScaleFactor
                                                        width: auxiliaryFanSpeedId.availableWidth
                                                        height: implicitHeight
                                                        radius: 2* screenScaleFactor
                                                        color: "#bdbebf"

                                                        Rectangle {
                                                            width: auxiliaryFanSpeedId.visualPosition * parent.width
                                                            height: parent.height
                                                            color: "#144F71"
                                                            radius: 2* screenScaleFactor
                                                        }
                                                    }

                                                    handle: Rectangle {
                                                        x: auxiliaryFanSpeedId.leftPadding + auxiliaryFanSpeedId.visualPosition * (auxiliaryFanSpeedId.availableWidth - width)
                                                        y: auxiliaryFanSpeedId.topPadding + auxiliaryFanSpeedId.availableHeight / 2 - height / 2
                                                        implicitWidth: 10* screenScaleFactor
                                                        implicitHeight: 10* screenScaleFactor
                                                        radius: 5* screenScaleFactor
                                                        color: "#00A3FF"
                                                    }
                                                    onPressedChanged: {
                                                        if(!pressed)
                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(
                                                                        ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_AUXILIARYFANPCT,
                                                                        parseInt(auxiliaryFanSpeedId.value*100,10), deviceType
                                                                        )
                                                    }
                                                }

                                            }
                                        }
                                    }
                                    Rectangle {
                                        visible: isKlipper_4408
                                        width: 207 * screenScaleFactor
                                        height: 109 * screenScaleFactor
                                        color: sourceTheme.fan_block_bgColor
                                        radius: 5 * screenScaleFactor
                                        border.width: 1 * screenScaleFactor
                                        border.color: Constants.currentTheme ? sourceTheme.btn_border_color : "transparent"
                                        ColumnLayout{
                                            y:15* screenScaleFactor
                                            x:15* screenScaleFactor

                                            RowLayout {
                                                spacing: 10 * screenScaleFactor
                                                Text {
                                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                                    Layout.preferredHeight: 14 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    text: qsTr("Cavity fan sw")
                                                    color: sourceTheme.text_normal_color
                                                }

                                                Button {
                                                    id: cavityFan
                                                    checkable: true
                                                    background: null
                                                    opacity: enabled ? 1.0 : 0.5
                                                    enabled: root.controlEnabled || root.isWlanPrinting

                                                    property var delayShow: false
                                                    property bool fan_checked: deviceItem ? deviceItem.caseFan : false

                                                    Layout.preferredWidth: 50 * screenScaleFactor
                                                    Layout.preferredHeight: 32 * screenScaleFactor

                                                    Binding on checked {
                                                        when: !cavityFan.delayShow
                                                        value: cavityFan.fan_checked
                                                    }

                                                    Timer {
                                                        repeat: false
                                                        interval: 10000
                                                        id: cavityFanDelayShow
                                                        onTriggered: cavityFan.delayShow = false
                                                    }

                                                    onClicked: {
                                                        var value = checked ? "1" : "0"
                                                        if(ipAddress !== "")
                                                        {
                                                            cavityFan.delayShow = true
                                                            cavityFanDelayShow.start()
                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_FANCASE, value, deviceType)
                                                        }
                                                    }

                                                    indicator: Rectangle {
                                                        radius: height / 2
                                                        anchors.fill: parent

                                                        border.width: cavityFan.checked ? 0 : 1
                                                        border.color: sourceTheme.switch_border_color
                                                        color: cavityFan.checked ? sourceTheme.switch_checked_background : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter

                                                            color: cavityFan.checked ? "#00A3FF" : sourceTheme.switch_indicator_color
                                                            x: cavityFan.checked ? (parent.width - width - 3 * screenScaleFactor) : 3 * screenScaleFactor

                                                            Behavior on x {
                                                                NumberAnimation { duration: 100 }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            Item {
                                                Layout.fillHeight: true
                                                Layout.fillWidth: true
                                                implicitHeight: 5* screenScaleFactor
                                            }
                                            Image {
                                                visible: !cavityFan.checked
                                                Layout.preferredWidth: 14 * screenScaleFactor
                                                Layout.preferredHeight: 14 * screenScaleFactor
                                                source: sourceTheme.img_fans_close
                                            }
                                            ColumnLayout{
                                                visible: cavityFan.checked
                                                RowLayout{
                                                    Image {
                                                        Layout.preferredWidth: 14 * screenScaleFactor
                                                        Layout.preferredHeight: 14 * screenScaleFactor
                                                        source: cavityFan.checked ? sourceTheme.img_fans_open :sourceTheme.img_fans_close
                                                        id: cavityFanImage
                                                        property real rotationAngle: 0
                                                        transform: Rotation {
                                                            origin.x: cavityFanImage.width/2
                                                            origin.y: cavityFanImage.height/2
                                                            angle: cavityFanImage.rotationAngle
                                                        }
                                                        PropertyAnimation {
                                                            target: cavityFanImage
                                                            property: "rotationAngle"
                                                            from: 0
                                                            to: 360
                                                            duration: 1000
                                                            running: cavityFan.checked
                                                            loops: Animation.Infinite
                                                        }
                                                    }
                                                    Text {
                                                        visible: !!deviceItem.caseFanSpeed && cavityFan.checked
                                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                                        Layout.preferredHeight: 14 * screenScaleFactor
                                                        verticalAlignment: Text.AlignVCenter
                                                        font.family: Constants.mySystemFont.name
                                                        text: qsTr("fan speed") + ":"
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: sourceTheme.text_normal_color
                                                    }
                                                    Text {
                                                        visible: !!deviceItem.caseFanSpeed && cavityFan.checked
                                                        text:parseInt(cavityFanSpeedId.value*100,10)  + "%"
                                                        font.pointSize: Constants.labelFontPointSize_10
                                                        font.weight: Font.Black
                                                        color: sourceTheme.text_normal_color
                                                        font.family: Constants.mySystemFont.name
                                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                                        Layout.preferredHeight: 14 * screenScaleFactor
                                                        verticalAlignment: Text.AlignVCenter
                                                    }
                                                }
                                                Slider{
                                                    id:cavityFanSpeedId
                                                    visible: !!deviceItem.caseFanSpeed && cavityFan.checked
                                                    value: deviceItem ? deviceItem.caseFanSpeed/100 : 0
                                                    background: Rectangle {
                                                        x: cavityFanSpeedId.leftPadding
                                                        y: cavityFanSpeedId.topPadding + cavityFanSpeedId.availableHeight / 2 - height / 2
                                                        implicitWidth: 186* screenScaleFactor
                                                        implicitHeight: 2* screenScaleFactor
                                                        width: cavityFanSpeedId.availableWidth
                                                        height: implicitHeight
                                                        radius: 2* screenScaleFactor
                                                        color: "#bdbebf"

                                                        Rectangle {
                                                            width: cavityFanSpeedId.visualPosition * parent.width
                                                            height: parent.height
                                                            color: "#144F71"
                                                            radius: 2* screenScaleFactor
                                                        }
                                                    }

                                                    handle: Rectangle {
                                                        x: cavityFanSpeedId.leftPadding + cavityFanSpeedId.visualPosition * (cavityFanSpeedId.availableWidth - width)
                                                        y: cavityFanSpeedId.topPadding + cavityFanSpeedId.availableHeight / 2 - height / 2
                                                        implicitWidth: 10* screenScaleFactor
                                                        implicitHeight: 10* screenScaleFactor
                                                        radius: 5* screenScaleFactor
                                                        color: "#00A3FF"
                                                    }
                                                    onPressedChanged: {
                                                        if(!pressed)
                                                           printerListModel.cSourceModel.sendUIControlCmdToDevice(
                                                               ipAddress,
                                                               LanPrinterListLocal.PrintControlType.CONTROL_CMD_CASEFANPCT,
                                                               parseInt(cavityFanSpeedId.value*100,10),
                                                               deviceType
                                                               )
                                                    }
                                                }
                                            }

                                        }

                                    }

                                    Rectangle{
                                        width: 207 * screenScaleFactor
                                        height: 109 * screenScaleFactor
                                        color: sourceTheme.fan_block_bgColor
                                        radius: 5 * screenScaleFactor
                                        border.width: 1 * screenScaleFactor
                                        border.color: Constants.currentTheme ? sourceTheme.btn_border_color : "transparent"

                                        ColumnLayout{
                                            y:15* screenScaleFactor
                                            x:15* screenScaleFactor

                                            RowLayout {
                                                spacing: 10 * screenScaleFactor

                                                Text {
                                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                                    Layout.preferredHeight: 14 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    text: qsTr("Led switch")
                                                    color: sourceTheme.text_normal_color
                                                }

                                                Button {
                                                    id: ledSwitch
                                                    checkable: true
                                                    background: null
                                                    opacity: enabled ? 1.0 : 0.5
                                                    enabled: root.controlEnabled || root.isWlanPrinting

                                                    property var delayShow: false
                                                    property bool led_checked: deviceItem?deviceItem.ledOpened:false

                                                    Layout.preferredWidth: 50 * screenScaleFactor
                                                    Layout.preferredHeight: 32 * screenScaleFactor

                                                    Binding on checked {
                                                        when: !ledSwitch.delayShow
                                                        value: ledSwitch.led_checked
                                                    }

                                                    Timer {
                                                        repeat: false
                                                        interval: 10000
                                                        id: ledSwitchDelayShow
                                                        onTriggered: ledSwitch.delayShow = false
                                                    }

                                                    onClicked:{
                                                        var value = checked ? "1" : "0"
                                                        if(ipAddress !== "")
                                                        {
                                                            ledSwitch.delayShow = true
                                                            ledSwitchDelayShow.start()
                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_LED, value, deviceType)
                                                        }
                                                    }

                                                    indicator: Rectangle{
                                                        radius: height / 2
                                                        anchors.fill: parent

                                                        border.width: ledSwitch.checked ? 0 : 1
                                                        border.color: sourceTheme.switch_border_color
                                                        color: ledSwitch.checked ? sourceTheme.switch_checked_background : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter

                                                            color: ledSwitch.checked ? "#00A3FF" : sourceTheme.switch_indicator_color
                                                            x: ledSwitch.checked ? (parent.width - width - 3 * screenScaleFactor) : 3 * screenScaleFactor

                                                            Behavior on x {
                                                                NumberAnimation { duration: 100 }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            Item {
                                                Layout.fillHeight: true
                                                Layout.fillWidth: true
                                                implicitHeight: 5* screenScaleFactor
                                            }
                                            Image {
                                                visible: ledSwitch.checked
                                                Layout.preferredWidth: 14 * screenScaleFactor
                                                Layout.preferredHeight: 14 * screenScaleFactor
                                                source: sourceTheme.img_led_open
                                            }
                                            Image {
                                                visible: !ledSwitch.checked
                                                Layout.preferredWidth: 14 * screenScaleFactor
                                                Layout.preferredHeight: 14 * screenScaleFactor
                                                source: sourceTheme.img_led_close
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: 0
                    id: gFileItem
                    property bool showPopup: true

                    CusRoundedBg {
                        leftTop: true
                        rightTop: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.title_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20 * screenScaleFactor
                            anchors.rightMargin: 20 * screenScaleFactor
                            spacing: 10 * screenScaleFactor

                            Image {
                                Layout.preferredWidth: 11 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/gFileIcon.svg"
                            }

                            Button {
                                indicator: null
                                checkable: false
                                property bool isChecked: idSwipeview.currentIndex === LanPrinterDetail.ListViewType.GcodeListView

                                Layout.preferredWidth: 80 * screenScaleFactor
                                Layout.preferredHeight: 32 * screenScaleFactor

                                contentItem: Text {
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.text_normal_color
                                    text: qsTr("File")
                                }

                                background: Rectangle {
                                    border.width: 1
                                    border.color: sourceTheme.btn_border_color
                                    color: (parent.isChecked || parent.hovered) ? sourceTheme.btn_default_color : "transparent"
                                    radius: 5
                                }

                                onClicked: idSwipeview.currentIndex = LanPrinterDetail.ListViewType.GcodeListView
                            }

                            Button {
                                indicator: null
                                checkable: false
                                visible: isKlipper_4408
                                property bool isChecked: idSwipeview.currentIndex === LanPrinterDetail.ListViewType.HistoryListView

                                Layout.preferredWidth: 80 * screenScaleFactor
                                Layout.preferredHeight: 32 * screenScaleFactor

                                contentItem: Text {
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.text_normal_color
                                    text: qsTr("Records")
                                }

                                background: Rectangle {
                                    border.width: 1
                                    border.color: sourceTheme.btn_border_color
                                    color: (parent.isChecked || parent.hovered) ? sourceTheme.btn_default_color : "transparent"
                                    radius: 5
                                }

                                onClicked: idSwipeview.currentIndex = LanPrinterDetail.ListViewType.HistoryListView
                            }

                            Button {
                                indicator: null
                                checkable: false
                                visible: isKlipper_4408
                                property bool isChecked: idSwipeview.currentIndex === LanPrinterDetail.ListViewType.VideoListView

                                Layout.preferredWidth: 80 * screenScaleFactor
                                Layout.preferredHeight: 32 * screenScaleFactor

                                contentItem: Text {
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.text_normal_color
                                    text: qsTr("Timelapse Video")
                                }

                                background: Rectangle {
                                    border.width: 1
                                    border.color: sourceTheme.btn_border_color
                                    color: (parent.isChecked || parent.hovered) ? sourceTheme.btn_default_color : "transparent"
                                    radius: 5
                                }

                                onClicked: idSwipeview.currentIndex = LanPrinterDetail.ListViewType.VideoListView
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            RowLayout {
                                id: importProgress
                                visible: curImportProgress != 0
                                spacing: 10 * screenScaleFactor

                                Text {
                                    id:uploadFileName
                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                    Layout.preferredHeight: 14 * screenScaleFactor
                                    verticalAlignment: Text.AlignVCenter
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    text: deviceItem.sendingFileName //fileDialog.fileName.length >4 ? fileDialog.fileName.slice(0,4)+"..." : fileDialog.fileName // 上传的文件名
                                    color: sourceTheme.text_normal_color
                                    MouseArea {
                                        id: id_uploadFileNameArea
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        acceptedButtons: Qt.NoButton
                                    }
                                    //                                    ToolTip.text: fileDialog.fileName
                                    //                                    ToolTip.visible: id_uploadFileNameArea.containsMouse && id_uploadFileNameArea.truncated//elide set
                                }



                                Rectangle {
                                    radius: height / 2
                                    color: sourceTheme.import_progressbar_color
                                    Layout.preferredWidth: 200 * screenScaleFactor
                                    Layout.preferredHeight: 2 * screenScaleFactor

                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width * curImportProgress / 100.0
                                        height: parent.height
                                        radius: parent.radius
                                        color: "#1E9BE2"
                                    }
                                }

                                Text {
                                    property var showImportText: (curImportProgress < 0) ? qsTr("Upload Failed") : `${curImportProgress}%`
                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                    Layout.preferredHeight: 14 * screenScaleFactor

                                    verticalAlignment: Text.AlignVCenter
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    text: showImportText
                                    color: sourceTheme.text_normal_color
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            Rectangle {
                                opacity: enabled ? 1.0 : 0.5
                                enabled: isDeviceOnline && !fatalErrorCode && curImportProgress <= 0
                                visible: idSwipeview.currentIndex === LanPrinterDetail.ListViewType.GcodeListView

                                border.width: 1
                                border.color: sourceTheme.btn_border_color
                                color: btnImportArea.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                radius: height / 2

                                Layout.preferredWidth: 120 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor

                                Row {
                                    height: parent.height
                                    anchors.centerIn: parent
                                    spacing: 10 * screenScaleFactor

                                    Image {
                                        width: 12 * screenScaleFactor
                                        height: 12 * screenScaleFactor
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: "qrc:/UI/photo/lanPrinterSource/plusSign.svg"
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Import")
                                        color: sourceTheme.text_normal_color
                                    }
                                }

                                MouseArea {
                                    id: btnImportArea
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                    onClicked: {
                                        fileDialog.open()
                                    }
                                }
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor
                                Layout.preferredHeight: 12 * screenScaleFactor

                                Image {
                                    anchors.centerIn: parent
                                    width: 10 * screenScaleFactor
                                    height: 6 * screenScaleFactor
                                    source: gFileItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                    anchors.centerIn: parent

                                    onClicked: {
                                        gFileItem.showPopup = !gFileItem.showPopup
                                        gFilePopup.visible = gFileItem.showPopup
                                    }
                                }
                            }
                        }

                        FileDialog {
                            id: fileDialog
                            title: qsTr("Open File")
                            folder: shortcuts.desktop
                            nameFilters: ["*.gcode"]
                            fileMode : FileDialog.OpenFiles
                            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
                            onAccepted: {
                                var fileUrls = fileDialog.files
                                if(deviceID !== "" && fileUrls.length)
                                {
                                    printerListModel.cSourceModel.importFile(deviceID, fileUrls)
                                }
                            }
                        }
                    }

                    CusRoundedBg {
                        id: gFilePopup
                        visible: true
                        leftBottom: true
                        rightBottom: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.popup_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.minimumHeight: 200 * screenScaleFactor
                        Layout.maximumHeight: 370 * screenScaleFactor

                        ColumnLayout {
                            spacing: 0
                            anchors.fill: parent

                            SwipeView {
                                clip: true
                                id: idSwipeview
                                interactive: false
                                Layout.alignment: Qt.AlignCenter
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: parent.height - 2 * screenScaleFactor

                                ListView {
                                    clip: true
                                    focus: true
                                    id: gcodeListView
                                    contentWidth: 930 * screenScaleFactor
                                    flickableDirection: Flickable.AutoFlickIfNeeded
                                    property var forcedItem: "" // 当前触发了菜单的选项
                                    property bool printerType: gcodeFileListModel.cSourceModel.printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408
                                    ScrollBar.vertical: ScrollBar {
                                        visible: gcodeListView.contentHeight > gcodeListView.height
                                        orientation: ListView.Vertical
                                        contentItem: Rectangle {
                                            radius: width / 2
                                            color: sourceTheme.scrollbar_color
                                            implicitWidth: 6 * screenScaleFactor
                                            implicitHeight: 180 * screenScaleFactor
                                        }
                                        onPositionChanged: {
                                            let currentItem = gcodeListView.forcedItem
                                            let itemGlobalPos = currentItem.mapToGlobal(0, 0)
                                            let viewGlobalPos = gcodeListView.mapToGlobal(0, 0)
                                            /* 判断当前选中的Item是否在显示范围内 */
                                            if (itemGlobalPos.y + currentItem.height <= viewGlobalPos.y ||
                                                itemGlobalPos.y >= viewGlobalPos.y + gcodeListView.height) {
                                                currentItem.closeMenu() // 超出显示范围时关闭菜单
                                            }
                                        }
                                    }
                                    ScrollBar.horizontal: ScrollBar {
                                        visible: gcodeListView.contentWidth > gcodeListView.width
                                        orientation: ListView.Horizontal
                                        contentItem: Rectangle {
                                            radius: height / 2
                                            color: sourceTheme.scrollbar_color
                                            implicitWidth: 180 * screenScaleFactor
                                            implicitHeight: 6 * screenScaleFactor
                                        }
                                    }

                                    model: gcodeFileListModel
                                    header: Rectangle {
                                        width: parent.width
                                        height: 40 * screenScaleFactor
                                        color: sourceTheme.popup_background_color

                                        Row {
                                            Repeater {
                                                model: [qsTr("File name"), qsTr("File size"), qsTr("Layer height"), qsTr("Creation time"), qsTr("Material length")]
                                                delegate: Rectangle {
                                                    width: widthProvider(index)
                                                    height: 40 * screenScaleFactor
                                                    color: sourceTheme.popup_background_color

                                                    Text {
                                                        anchors.centerIn: parent
                                                        verticalAlignment: Text.AlignVCenter
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.weight: Font.Medium
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: sourceTheme.text_normal_color
                                                        text: modelData
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    delegate: Rectangle {
                                        id: gcodeItem
                                        color: "transparent"
                                        width: 930 * screenScaleFactor
                                        height: (40 + 1) * screenScaleFactor

                                        function closeMenu() {
                                            gcodeFileMenu.close()
                                        }

                                        Rectangle{
                                            anchors.left: parent.left
                                            width:parent.width * gCodeExportProgress
                                            height: parent.height
                                            color:"#06B022"
                                        }

                                        MouseArea {
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            enabled: isDeviceOnline && !fatalErrorCode
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: gcodeFileMenu.opened ? gcodeFileMenu.close() : gcodeFileMenu.openMenu(mouseX, mouseY)
                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.item_hovered_color : "transparent"

                                            Popup {
                                                id: gcodeFileMenu
                                                width: (160 + 2) * screenScaleFactor
                                                height: (gcodeMenuContent.height + 2) * screenScaleFactor

                                                background: Rectangle {
                                                    color: sourceTheme.right_submenu_background
                                                    border.color: sourceTheme.right_submenu_border
                                                    border.width: 1 * screenScaleFactor
                                                }

                                                closePolicy: Popup.CloseOnPressOutsideParent

                                                function openMenu(mouseX, mouseY){
                                                    gcodeListView.forcedItem = gcodeItem
                                                    x = mouseX + 10
                                                    y = mouseY - height
                                                    open()
                                                }

                                                Column {
                                                    id: gcodeMenuContent
                                                    anchors.centerIn: parent

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Start printing")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                var printerPath = editGcodeName.text

                                                                if(ipAddress !== "" && printerPath !== "")
                                                                {
                                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_START, printerPath, deviceType)
                                                                }

                                                                gcodeFileMenu.close()
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle
                                                        visible: root.isKlipper_4408
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Print with calibration")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                var printerPath = editGcodeName.text

                                                                if(ipAddress !== "" && printerPath !== "")
                                                                {
                                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_WITH_CALIBRATION, printerPath, deviceType)
                                                                }

                                                                gcodeFileMenu.close()
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isKlipper_4408
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Export")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                saveGcodeDialog.currentFile = "file:///" + gcodeFileName
                                                                saveGcodeDialog.open()
                                                                gcodeFileMenu.close()
                                                            }
                                                        }

                                                        FileDialog{
                                                            id: saveGcodeDialog
                                                            fileMode : FileDialog.SaveFile
                                                            folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
                                                            nameFilters: [ "Gcode files (*.gcode)", "All files (*)" ]
                                                            defaultSuffix : "gcode"
                                                            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
                                                            onAccepted:{
                                                                if(ipAddress !== "")
                                                                {
                                                                    gcodeFileListModel.cSourceModel.downloadGcodeFile(ipAddress, gcodeFileName, saveGcodeDialog.file.toString(), deviceType)
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle || curGcodeName != gcodeFileName
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Delete")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                var printerPath = editGcodeName.text

                                                                if(ipAddress !== "" && printerPath !== "")
                                                                {
                                                                    gcodeFileListModel.cSourceModel.deleteGcodeFile(ipAddress, printerPath, deviceType)
                                                                }

                                                                gcodeFileMenu.close()
                                                            }
                                                        }
                                                    }
                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle || curGcodeName != gcodeFileName
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Rename")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                id_FileName.visible = false
                                                                editGcodeName.visible = true
                                                                editGcodeName.forceActiveFocus()
                                                                gcodeFileMenu.close()
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            Rectangle {
                                                width: parent.width
                                                height: 1 * screenScaleFactor
                                                color: sourceTheme.item_splitline_color
                                            }

                                            Row {
                                                y: 1 * screenScaleFactor

                                                Item {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileName)
                                                    height: 40 * screenScaleFactor

                                                    RowLayout {
                                                        anchors.fill: parent
                                                        spacing: 10 * screenScaleFactor
                                                        Image {
                                                            Layout.preferredWidth: 24 * screenScaleFactor
                                                            Layout.preferredHeight: 24 * screenScaleFactor

                                                            cache: false
                                                            mipmap: true
                                                            smooth: true
                                                            asynchronous: true
                                                            fillMode: Image.PreserveAspectFit
                                                            sourceSize: Qt.size(width, height)
                                                            source: modelItemImagePrefix + gCodeThumbnailImage

                                                            property int tryCount: 0
                                                            onStatusChanged: {
                                                                if (status === Image.Error) {
                                                                    if (!needLoadModelItem)
                                                                        return
                                                                    if (tryCount < maxTryTimes) {
                                                                        /* 重新加载 */
                                                                        let tempSrc = source
                                                                        source = ''
                                                                        source = tempSrc
                                                                        tryCount++
                                                                    } else {
                                                                        /* 加载不成功直接用默认图标 */
                                                                        source = defaultImage
                                                                    }
                                                                }
                                                            }
                                                        }

                                                        ColumnLayout {
                                                            spacing: 6 * screenScaleFactor
                                                            
                                                            Text {
                                                                id: id_FileName
                                                                Layout.preferredWidth: 256 * screenScaleFactor
                                                                Layout.preferredHeight: 14 * screenScaleFactor

                                                                font.weight: Font.Medium
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                horizontalAlignment: Text.AlignLeft
                                                                verticalAlignment: Text.AlignVCenter
                                                                color: sourceTheme.text_normal_color
                                                                elide: Text.ElideMiddle
                                                                text: gcodeFileName

                                                                MouseArea {
                                                                    id: id_FileNameArea
                                                                    hoverEnabled: true
                                                                    anchors.fill: parent
                                                                    acceptedButtons: Qt.NoButton
                                                                    // onDoubleClicked: {
                                                                    //     id_FileName.visible = false
                                                                    //     editGcodeName.visible = true
                                                                    //     editGcodeName.forceActiveFocus()
                                                                    // }
                                                                }
                                                            }

                                                            TextField {
                                                                id: editGcodeName
                                                                visible: false
                                                                property var delayShow: false
                                                                width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldLayerHeight)
                                                                height: 40 * screenScaleFactor

                                                                verticalAlignment: Text.AlignVCenter
                                                                horizontalAlignment: Text.AlignHCenter
                                                                font.weight: Font.Medium
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                color: sourceTheme.text_normal_color
                                                                background: null
                                                                Binding on text {
                                                                    when: !editGcodeName.activeFocus && !editGcodeName.delayShow
                                                                    value: gcodeFileName
                                                                }
                                                                
                                                                Timer {
                                                                    repeat: false
                                                                    interval: 10000
                                                                    id: gcodeNameDelayShow
                                                                    onTriggered: {
                                                                        editGcodeName.delayShow = false
                                                                        id_FileName.visible = true
                                                                        editGcodeName.visible = false
                                                                        root.forceActiveFocus()
                                                                    }
                                                                }

                                                                readOnly: !activeFocus
                                                                Keys.onEnterPressed: gcodeNameDelayShow.triggered()
                                                                Keys.onReturnPressed: gcodeNameDelayShow.triggered()
                                                                onEditingFinished: {
                                                                    editGcodeName.delayShow = true
                                                                    gcodeNameDelayShow.start()
                                                                    if(gcodeFileName != editGcodeName.text){
                                                                        gcodeFileListModel.cSourceModel.renameGcodeFile(ipAddress, gcodeFileName, editGcodeName.text, deviceType)
                                                                    }
                                                                }

                                                                onActiveFocusChanged:{
                                                                    if(!activeFocus)
                                                                        gcodeNameDelayShow.triggered()
                                                                }
                                                            }

                                                            RowLayout {
                                                                spacing: 6 * screenScaleFactor
                                                                visible: (gCodeImportProgress != 0)

                                                                Rectangle {
                                                                    Layout.preferredWidth: 140 * screenScaleFactor
                                                                    Layout.preferredHeight: 2 * screenScaleFactor

                                                                    color: sourceTheme.import_progressbar_color
                                                                    radius: height / 2

                                                                    Rectangle {
                                                                        anchors.left: parent.left
                                                                        anchors.verticalCenter: parent.verticalCenter
                                                                        width: parent.width * gCodeImportProgress / 100.0
                                                                        height: parent.height
                                                                        radius: parent.radius
                                                                        color: "#1E9BE2"
                                                                    }
                                                                }

                                                                Text {
                                                                    Layout.preferredWidth: contentWidth * screenScaleFactor
                                                                    Layout.preferredHeight: 14 * screenScaleFactor

                                                                    font.weight: Font.Medium
                                                                    font.family: Constants.mySystemFont.name
                                                                    font.pointSize: Constants.labelFontPointSize_9
                                                                    verticalAlignment: Text.AlignVCenter
                                                                    color: sourceTheme.text_normal_color
                                                                    text: `${gCodeImportProgress}%`
                                                                }
                                                            }
                                                        }
                                                    }

                                                    ToolTip.text: gcodeFileName
                                                    ToolTip.visible: id_FileNameArea.containsMouse && id_FileName.truncated
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileSize)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: gcodeFileSize + "MB"
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldLayerHeight)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: gcodeListView.printerType ? gcodeLayerHeight + "mm" : "---"
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileTime)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: gcodeFileTime
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldMaterial)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: gcodeListView.printerType ? gcodeMaterialLength + "m" : "---"
                                                }
                                            }
                                        }
                                    }
                                }

                                ListView {
                                    clip: true
                                    id: historyListView
                                    contentWidth: 930 * screenScaleFactor
                                    flickableDirection: Flickable.AutoFlickIfNeeded
                                    property var forcedItem: "" // 当前触发了菜单的选项
                                    ScrollBar.vertical: ScrollBar {
                                        visible: historyListView.contentHeight > historyListView.height
                                        orientation: ListView.Vertical
                                        contentItem: Rectangle {
                                            radius: width / 2
                                            color: sourceTheme.scrollbar_color
                                            implicitWidth: 6 * screenScaleFactor
                                            implicitHeight: 180 * screenScaleFactor
                                        }
                                        onPositionChanged: {
                                            let currentItem = historyListView.forcedItem
                                            let itemGlobalPos = currentItem.mapToGlobal(0, 0)
                                            let viewGlobalPos = historyListView.mapToGlobal(0, 0)
                                            /* 判断当前选中的Item是否在显示范围内 */
                                            if (itemGlobalPos.y + currentItem.height <= viewGlobalPos.y ||
                                                itemGlobalPos.y >= viewGlobalPos.y + historyListView.height) {
                                                currentItem.closeMenu() // 超出显示范围时关闭菜单
                                            }
                                        }
                                    }
                                    ScrollBar.horizontal: ScrollBar {
                                        visible: historyListView.contentWidth > historyListView.width
                                        orientation: ListView.Horizontal
                                        contentItem: Rectangle {
                                            radius: height / 2
                                            color: sourceTheme.scrollbar_color
                                            implicitWidth: 180 * screenScaleFactor
                                            implicitHeight: 6 * screenScaleFactor
                                        }
                                    }

                                    model: historyFileListModel
                                    header: Rectangle {
                                        width: parent.width
                                        height: 40 * screenScaleFactor
                                        color: sourceTheme.popup_background_color

                                        Row {
                                            Repeater {
                                                model: [qsTr("File name"), qsTr("File size"), qsTr("Start time")  , qsTr("Total duration"), qsTr("Material usage")]
                                                delegate: Rectangle {
                                                    width: widthProvider(index)
                                                    height: 40 * screenScaleFactor
                                                    color: sourceTheme.popup_background_color

                                                    Text {
                                                        anchors.centerIn: parent
                                                        verticalAlignment: Text.AlignVCenter
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.weight: Font.Medium
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: sourceTheme.text_normal_color
                                                        text: modelData
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    delegate: Rectangle {
                                        id: historyItem
                                        color: "transparent"
                                        width: 930 * screenScaleFactor
                                        height: (40 + 1) * screenScaleFactor

                                        function closeMenu() {
                                            historyFileMenu.close()
                                        }

                                        Rectangle{
                                            anchors.left: parent.left
                                            width:parent.width * gCodeExportProgress
                                            height: parent.height
                                            color:"#06B022"
                                        }

                                        MouseArea {
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            enabled: isDeviceOnline && !fatalErrorCode
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: historyFileMenu.opened ? historyFileMenu.close() : historyFileMenu.openMenu(mouseX, mouseY)
                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.item_hovered_color : "transparent"

                                            Popup {
                                                id: historyFileMenu
                                                width: (160 + 2) * screenScaleFactor
                                                height: (historyMenuContent.height + 2) * screenScaleFactor

                                                background: Rectangle {
                                                    color: sourceTheme.right_submenu_background
                                                    border.color: sourceTheme.right_submenu_border
                                                    border.width: 1 * screenScaleFactor
                                                }

                                                closePolicy: Popup.CloseOnPressOutsideParent

                                                function openMenu(mouseX, mouseY){
                                                    historyListView.forcedItem = historyItem
                                                    x = mouseX + 10
                                                    y = mouseY - height
                                                    open()
                                                }

                                                Column {
                                                    id: historyMenuContent
                                                    anchors.centerIn: parent
                                                    
                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isKlipper_4408
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Export")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                saveGcodeDialog.currentFile = "file:///" + historyFileName
                                                                saveGcodeDialog.open()
                                                                historyFileMenu.close()
                                                            }
                                                        }

                                                        FileDialog{
                                                            id: saveGcodeDialog
                                                            fileMode : FileDialog.SaveFile
                                                            folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
                                                            nameFilters: [ "Gcode files (*.gcode)", "All files (*)" ]
                                                            defaultSuffix : "gcode"
                                                            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
                                                            onAccepted:{
                                                                if(ipAddress !== "")
                                                                {
                                                                    historyFileListModel.downloadGcodeFile(ipAddress, historyFileName, saveGcodeDialog.file.toString(), deviceType, historyFileNumb)
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            text: qsTr("Restart printing")
                                                            color: sourceTheme.right_submenu_text_color
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                let printerPath = id__FileName.text

                                                                if(deviceID !== "" && printerPath !== "")
                                                                {
                                                                    let exist = gcodeFileListModel.cSourceModel.isGcodeFileExist(deviceID, printerPath)

                                                                    if(exist && ipAddress !== "")
                                                                    {
                                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_START, printerPath, deviceType)
                                                                    }
                                                                    else {
                                                                        idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.NotExist)
                                                                    }
                                                                }

                                                                historyFileMenu.close()
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle
                                                        visible: root.isKlipper_4408
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Reprint with calibration")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                let printerPath = id__FileName.text

                                                                if(deviceID !== "" && printerPath !== "")
                                                                {
                                                                    let exist = gcodeFileListModel.cSourceModel.isGcodeFileExist(deviceID, printerPath)

                                                                    if(exist && ipAddress !== "")
                                                                    {
                                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_WITH_CALIBRATION, printerPath, deviceType)
                                                                    }
                                                                    else {
                                                                        idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.NotExist)
                                                                    }
                                                                }

                                                                historyFileMenu.close()
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor
                                                        color: sourceTheme.right_submenu_background

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            text: qsTr("Delete")
                                                            color: sourceTheme.right_submenu_text_color
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                let printerPath = id__FileName.text

                                                                if(ipAddress !== "" && printerPath !== "")
                                                                {
                                                                    historyFileListModel.deleteHistoryFile(ipAddress, historyFileNumb, deviceType)
                                                                }

                                                                historyFileMenu.close()
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            Rectangle {
                                                width: parent.width
                                                height: 1 * screenScaleFactor
                                                color: sourceTheme.item_splitline_color
                                            }

                                            Row {
                                                y: 1 * screenScaleFactor

                                                Item {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileName)
                                                    height: 40 * screenScaleFactor

                                                    Row {
                                                        height: parent.height
                                                        anchors.centerIn: parent
                                                        spacing: 10 * screenScaleFactor

                                                        Image {
                                                            width: 16 * screenScaleFactor
                                                            height: 16 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            source: isPrintFinished ? "qrc:/UI/photo/lanPrinterSource/history_print_finished.svg" : "qrc:/UI/photo/lanPrinterSource/history_print_unfinished.svg"
                                                        }

                                                        Text {
                                                            id: id__FileName
                                                            height: parent.height
                                                            width: 256 * screenScaleFactor

                                                            verticalAlignment: Text.AlignVCenter
                                                            horizontalAlignment: Text.AlignLeft

                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.text_normal_color
                                                            elide: Text.ElideMiddle
                                                            text: historyFileName

                                                            MouseArea {
                                                                id: id__FileNameArea
                                                                hoverEnabled: true
                                                                anchors.fill: parent
                                                                acceptedButtons: Qt.NoButton
                                                            }
                                                        }
                                                    }

                                                    ToolTip.text: id__FileName.text
                                                    ToolTip.visible: id__FileNameArea.containsMouse && id__FileName.truncated//elide set
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileSize)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: historyFileSize + "MB"
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldInterval)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: historyInterval
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileTime)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: second2String(historyFileTime)
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldMaterial)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: historyMaterialUsage + "m"
                                                }
                                            }
                                        }
                                    }
                                }

                                ListView {
                                    clip: true
                                    focus: true
                                    id: videoListView
                                    contentWidth: 930 * screenScaleFactor
                                    flickableDirection: Flickable.AutoFlickIfNeeded
                                    property bool printerType: videoListModel.printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408
                                    property var forcedItem: "" // 当前触发了菜单的选项
                                    ScrollBar.vertical: ScrollBar {
                                        id: videoListVBar
                                        visible: videoListView.contentHeight > videoListView.height
                                        orientation: ListView.Vertical
                                        contentItem: Rectangle {
                                            radius: width / 2
                                            color: sourceTheme.scrollbar_color
                                            implicitWidth: 6 * screenScaleFactor
                                            implicitHeight: 180 * screenScaleFactor
                                        }
                                        onPositionChanged: {
                                            let currentItem = videoListView.forcedItem
                                            let itemGlobalPos = currentItem.mapToGlobal(0, 0)
                                            let viewGlobalPos = videoListView.mapToGlobal(0, 0)
                                            /* 判断当前选中的Item是否在显示范围内 */
                                            if (itemGlobalPos.y + currentItem.height <= viewGlobalPos.y ||
                                                itemGlobalPos.y >= viewGlobalPos.y + videoListView.height) {
                                                currentItem.closeMenu() // 超出显示范围时关闭菜单
                                            }
                                        }
                                    }
                                    ScrollBar.horizontal: ScrollBar {
                                        visible: videoListView.contentWidth > videoListView.width
                                        orientation: ListView.Horizontal
                                        contentItem: Rectangle {
                                            radius: height / 2
                                            color: sourceTheme.scrollbar_color
                                            implicitWidth: 180 * screenScaleFactor
                                            implicitHeight: 6 * screenScaleFactor
                                        }
                                    }
                                    model: videoListModel
                                    header: Rectangle {
                                        width: parent.width
                                        height: 40 * screenScaleFactor
                                        color: sourceTheme.popup_background_color

                                        Row {
                                            Repeater {
                                                model: [qsTr("Gcode name"), qsTr("Video size"), qsTr("Video name"), qsTr("Start time"), qsTr("Print time")]
                                                delegate: Rectangle {
                                                    width: widthProvider(index)
                                                    height: 40 * screenScaleFactor
                                                    color: sourceTheme.popup_background_color

                                                    Text {
                                                        anchors.centerIn: parent
                                                        verticalAlignment: Text.AlignVCenter
                                                        horizontalAlignment: Text.AlignHCenter
                                                        font.weight: Font.Medium
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: sourceTheme.text_normal_color
                                                        text: modelData
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    delegate: Rectangle {
                                        id: videoItem
                                        color: "transparent"
                                        width: 930 * screenScaleFactor
                                        height: (40 + 1) * screenScaleFactor

                                        Rectangle{
                                            anchors.left: parent.left
                                            width:parent.width * exportProgress
                                            height: parent.height
                                            color:"#06B022"
                                        }

                                        function closeMenu() {
                                            videoFileMenu.close()
                                        }

                                        MouseArea {
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            enabled: isDeviceOnline
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: videoFileMenu.opened ? videoFileMenu.close() : videoFileMenu.openMenu(mouseX, mouseY)
                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.item_hovered_color : "transparent"

                                            Popup {
                                                id: videoFileMenu
                                                width: (160 + 2) * screenScaleFactor
                                                height: (videoMenuContent.height + 2) * screenScaleFactor

                                                background: Rectangle {
                                                    color: sourceTheme.right_submenu_background
                                                    border.color: sourceTheme.right_submenu_border
                                                    border.width: 1 * screenScaleFactor
                                                }

                                                closePolicy: Popup.CloseOnPressOutsideParent

                                                function openMenu(mouseX, mouseY){
                                                    videoListView.forcedItem = videoItem
                                                    x = mouseX + 10
                                                    y = mouseY - height
                                                    open()
                                                }

                                                Column {
                                                    id: videoMenuContent
                                                    anchors.centerIn: parent

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Rename")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onDoubleClicked:
                                                            {
                                                                editVideoName.forceActiveFocus()
                                                                videoFileMenu.close()
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Delete")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                var printerPath = id_FileName.text

                                                                if(ipAddress !== "" && printerPath !== "")
                                                                {
                                                                    videoListModel.deleteVideoFile(ipAddress, videoFileName, deviceType)
                                                                }

                                                                videoFileMenu.close()
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isKlipper_4408
                                                        opacity: enabled ? 1.0 : 0.5

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Export")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                saveDialog.currentFile = "file:///" + videoFileShowName
                                                                saveDialog.open()
                                                                videoFileMenu.close()
                                                            }
                                                        }

                                                        FileDialog{
                                                            id: saveDialog
                                                            fileMode : FileDialog.SaveFile
                                                            folder: StandardPaths.writableLocation(StandardPaths.MoviesLocation)
                                                            nameFilters: [ "Video files (*.mp4)", "All files (*)" ]
                                                            defaultSuffix : "mp4"
                                                            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
                                                            onAccepted:{
                                                                if(ipAddress !== "")
                                                                {
                                                                    videoListModel.downloadVideoFile(ipAddress, videoFileName, saveDialog.file.toString(), deviceType)
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        id: copyLinkAction
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isKlipper_4408
                                                        opacity: enabled ? 1.0 : 0.5

                                                        property string link: ""

                                                        Text {
                                                            elide: Text.ElideRight
                                                            leftPadding: 8 * screenScaleFactor
                                                            verticalAlignment: Text.AlignVCenter
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.right_submenu_text_color
                                                            text: qsTr("Copy Link")
                                                        }

                                                        MouseArea {
                                                            hoverEnabled: true
                                                            anchors.fill: parent
                                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.right_submenu_selection : sourceTheme.right_submenu_background

                                                            onClicked:
                                                            {
                                                                copyLinkAction.link = "http://" + ipAddress + ":80/downloads/video/" + videoFileName
                                                                kernel_ui.copyString2Clipboard(copyLinkAction.link)
                                                                idMessageDlg.show()
                                                                videoFileMenu.close()
                                                            }
                                                        }


                                                        UploadMessageDlg {
                                                            id: idMessageDlg
                                                            visible: false
                                                            cancelBtnVisible: true
                                                            ignoreBtnVisible: false
                                                            okBtnText: qsTr("OK")
                                                            cancelBtnText: qsTr("Open Browser")
                                                            msgText: qsTr("Copy link successfully!")

                                                            onSigOkButtonClicked: idMessageDlg.close()
                                                            onSigCancelButtonClicked: {
                                                                Qt.openUrlExternally(copyLinkAction.link)
                                                                idMessageDlg.close()
                                                            }
                                                        }

                                                    }
                                                }
                                            }

                                            Rectangle {
                                                width: parent.width
                                                height: 1 * screenScaleFactor
                                                color: sourceTheme.item_splitline_color
                                            }

                                            Row {
                                                y: 1 * screenScaleFactor

                                                Item {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileName)
                                                    height: 40 * screenScaleFactor

                                                    RowLayout {
                                                        anchors.fill: parent
                                                        spacing: 10 * screenScaleFactor

                                                        Image {
                                                            Layout.preferredWidth: 24 * screenScaleFactor
                                                            Layout.preferredHeight: 24 * screenScaleFactor

                                                            cache: true
                                                            mipmap: true
                                                            smooth: true
                                                            asynchronous: true
                                                            fillMode: Image.PreserveAspectFit
                                                            sourceSize: Qt.size(width, height)
                                                            source: "http://" + ipAddress + "/downloads/humbnail/" + gcodeFileName + ".png"
                                                        }

                                                        ColumnLayout {
                                                            spacing: 6 * screenScaleFactor

                                                            Text {
                                                                id: id_FileName
                                                                Layout.preferredWidth: 256 * screenScaleFactor
                                                                Layout.preferredHeight: 14 * screenScaleFactor

                                                                font.weight: Font.Medium
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                horizontalAlignment: Text.AlignLeft
                                                                verticalAlignment: Text.AlignVCenter
                                                                color: sourceTheme.text_normal_color
                                                                elide: Text.ElideMiddle
                                                                text: gcodeFileName

                                                                MouseArea {
                                                                    id: id_FileNameArea
                                                                    hoverEnabled: true
                                                                    anchors.fill: parent
                                                                    acceptedButtons: Qt.NoButton
                                                                }
                                                            }

                                                        }
                                                    }

                                                    ToolTip.text: id_FileName.text
                                                    ToolTip.visible: id_FileNameArea.containsMouse && id_FileName.truncated//elide set
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileSize)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: videoFileSize + "MB"
                                                }

                                                TextField {
                                                    id: editVideoName
                                                    property var delayShow: false
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldLayerHeight)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    background: null
                                                    Binding on text {
                                                        when: !editVideoName.activeFocus && !editVideoName.delayShow
                                                        value: videoFileShowName
                                                    }
                                                    
                                                    Timer {
                                                        repeat: false
                                                        interval: 10000
                                                        id: videoNameDelayShow
                                                        onTriggered: editVideoName.delayShow = false
                                                    }

                                                    readOnly: !activeFocus
                                                    Keys.onEnterPressed: root.forceActiveFocus()
                                                    Keys.onReturnPressed: root.forceActiveFocus()
                                                    onEditingFinished: {
                                                        editVideoName.delayShow = true
                                                        videoNameDelayShow.start()
                                                        if(videoFileShowName != editVideoName.text){
                                                            videoListModel.renameVideoFile(ipAddress, videoFileName, editVideoName.text, deviceType)
                                                        }
                                                    }
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldFileTime)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: startTime
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldMaterial)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text: gcodeListView.printerType ? second2String(printTime) : "---"
                                                }
                                            }
                                        }
                                    }
                                }

                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 8 * screenScaleFactor
                Layout.maximumWidth: 16 * screenScaleFactor
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 10 * screenScaleFactor
                Layout.bottomMargin: 10 * screenScaleFactor
                spacing: 16 * screenScaleFactor

                ColumnLayout {
                    spacing: 0
                    id: cameraItem
                    property bool showPopup: false

                    CusRoundedBg {
                        leftTop: true
                        rightTop: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.title_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20 * screenScaleFactor
                            anchors.rightMargin: 20 * screenScaleFactor
                            spacing: 10 * screenScaleFactor

                            Image {
                                Layout.preferredWidth: 11 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/cameraIcon.svg"
                            }

                            Text {
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                text: qsTr("Camera")
                                color: sourceTheme.text_normal_color
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor
                                Layout.preferredHeight: 12 * screenScaleFactor

                                Image {
                                    anchors.centerIn: parent
                                    width: 10 * screenScaleFactor
                                    height: 6 * screenScaleFactor
                                    source: cameraItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: cameraItem.showPopup = !cameraItem.showPopup
                                }
                            }
                        }
                    }

                    CusRoundedBg {
                        id: cameraPopup
                        visible: parent.showPopup
                        leftBottom: true
                        rightBottom: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.popup_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.minimumHeight: 330 * screenScaleFactor
                        Layout.maximumHeight: 680 * screenScaleFactor

                        MouseArea {
                            width: 20 * screenScaleFactor
                            height: 20 * screenScaleFactor
                            visible: root.hasCamera && !idVideoDialog.visible

                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.topMargin: 20 * screenScaleFactor
                            anchors.rightMargin: 20 * screenScaleFactor

                            Image {
                                width: 14 * screenScaleFactor
                                height: 14 * screenScaleFactor
                                source: sourceTheme.img_enlarge
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                            }

                            hoverEnabled: true
                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: idVideoDialog.show()
                        }
                    }
                }

                ColumnLayout {
                    spacing: 0
                    id: temperatureItem
                    property bool showPopup: true

                    CusRoundedBg {
                        leftTop: true
                        rightTop: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.title_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.preferredHeight: 40 * screenScaleFactor

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 20 * screenScaleFactor
                            anchors.rightMargin: 20 * screenScaleFactor
                            spacing: 10 * screenScaleFactor

                            Image {
                                Layout.preferredWidth: 12 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/white_temperatureIcon.svg"
                            }

                            Text {
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                text: qsTr("Temperature")
                                color: sourceTheme.text_normal_color
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor
                                Layout.preferredHeight: 12 * screenScaleFactor

                                Image {
                                    anchors.centerIn: parent
                                    width: 10 * screenScaleFactor
                                    height: 6 * screenScaleFactor
                                    source: temperatureItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        temperatureItem.showPopup = !temperatureItem.showPopup
                                        temperaturePopup.visible = temperatureItem.showPopup
                                    }
                                }
                            }
                        }
                    }

                    CusRoundedBg {
                        id: temperaturePopup
                        visible: true
                        leftBottom: true
                        rightBottom: true
                        clickedable: false
                        borderWidth: 1
                        borderColor: sourceTheme.background_border
                        color: sourceTheme.popup_background_color
                        radius: 5

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        Layout.minimumHeight: 320 * screenScaleFactor
                        Layout.maximumHeight: 500 * screenScaleFactor

                        ColumnLayout {
                            spacing: 0
                            anchors.fill: parent

                            Rectangle {
                                color: "transparent"
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 40 * screenScaleFactor

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10 * screenScaleFactor
                                    anchors.leftMargin: 20 * screenScaleFactor
                                    anchors.rightMargin: 20 * screenScaleFactor

                                    Item {
                                        Layout.preferredWidth: 12
                                        Layout.preferredHeight: 14
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Project")
                                        color: sourceTheme.text_normal_color
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 550 * screenScaleFactor
                                    }

                                    RowLayout {
                                        spacing: 40 * screenScaleFactor

                                        Text {
                                            Layout.preferredWidth: 50 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: qsTr("Current")
                                            color: sourceTheme.text_normal_color
                                        }

                                        Text {
                                            Layout.preferredWidth: 10 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: " "
                                        }

                                        Text {
                                            Layout.preferredWidth: 120 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: qsTr("Target")
                                            color: sourceTheme.text_normal_color
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                color: sourceTheme.item_splitline_color
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 1 * screenScaleFactor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Rectangle {
                                color: "transparent"
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 40 * screenScaleFactor

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10 * screenScaleFactor
                                    anchors.leftMargin: 20 * screenScaleFactor
                                    anchors.rightMargin: 20 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 12 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        source: "qrc:/UI/photo/lanPrinterSource/nozzleDst_temperatureIcon.svg"
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Nozzle") + (isDoubleNozzle ? "L" : "")
                                        color: sourceTheme.text_normal_color
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 550 * screenScaleFactor
                                    }

                                    RowLayout {
                                        spacing: 40 * screenScaleFactor

                                        Text {
                                            Layout.preferredWidth: 50 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: `${deviceItem?deviceItem.pcNozzleTemp.toFixed(2):"undefined"}°C`
                                            color: sourceTheme.text_normal_color
                                        }

                                        Text {
                                            Layout.preferredWidth: 10 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: "/"
                                        }

                                        Rectangle {
                                            id: __nozzleDstTempBox
                                            property int minValue: 0
                                            property int maxValue: 300
                                            property var delayShow: false
                                            property var nozzleDstTemp: deviceItem?deviceItem.pcNozzleDstTemp:"undefined"
                                            property alias boxValue: __editNozzleDstTempBox.text

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
                                                if(ipAddress !== "")
                                                {
                                                    delayShow = true
                                                    __nozzleDstTempDelayShow.start()
                                                    if(isDoubleNozzle){
                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_1STNOZZLETEMP, boxValue, deviceType)
                                                    }
                                                    else{
                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_NOZZLETEMP, boxValue, deviceType)
                                                    }
                                                }
                                            }

                                            Layout.preferredWidth: 120 * screenScaleFactor
                                            Layout.preferredHeight: 36 * screenScaleFactor

                                            radius: 5
                                            opacity: enabled ? 1.0 : 0.5
                                            enabled: root.controlEnabled || root.isWlanPrinting

                                            color: sourceTheme.popup_background_color
                                            border.color: __editNozzleDstTempBox.activeFocus ? "#3699FF" : sourceTheme.item_rectangle_border
                                            border.width: 1

                                            Binding on boxValue {
                                                when: !__editNozzleDstTempBox.activeFocus && !__nozzleDstTempBox.delayShow
                                                value: __nozzleDstTempBox.nozzleDstTemp
                                            }

                                            Timer {
                                                repeat: false
                                                interval: 10000
                                                id: __nozzleDstTempDelayShow
                                                onTriggered: __nozzleDstTempBox.delayShow = false
                                            }

                                            TextField {
                                                id: __editNozzleDstTempBox
                                                width: parent.width
                                                height: parent.height
                                                anchors.left: parent.left
                                                anchors.verticalCenter: parent.verticalCenter

                                                validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1-2][\\d][\\d]|300")}
                                                selectByMouse: true
                                                selectionColor: sourceTheme.textedit_selection_color
                                                selectedTextColor : color
                                                topPadding: 0
                                                bottomPadding: 0
                                                leftPadding: 10 * screenScaleFactor
                                                rightPadding: 4 * screenScaleFactor
                                                verticalAlignment: TextInput.AlignVCenter
                                                horizontalAlignment: TextInput.AlignLeft
                                                color: sourceTheme.text_normal_color
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                background: null

                                                Keys.onUpPressed: __nozzleDstTempBox.increase()
                                                Keys.onDownPressed: __nozzleDstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __nozzleDstTempBox.specifyPcNozzleDstTemp()
                                            }

                                            Text {
                                                anchors.right: parent.right
                                                anchors.rightMargin: 10 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                font: __editBedDstTempBox.font
                                                color: __editBedDstTempBox.color
                                                verticalAlignment: Text.AlignVCenter
                                                text: "°C"
                                            }
                                        }
                                    }
                                }
                            }
                            Rectangle {
                                color: sourceTheme.item_splitline_color
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 1 * screenScaleFactor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Rectangle {
                                color: "transparent"
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 40 * screenScaleFactor
                                visible : isDoubleNozzle

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10 * screenScaleFactor
                                    anchors.leftMargin: 20 * screenScaleFactor
                                    anchors.rightMargin: 20 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 12 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        source: "qrc:/UI/photo/lanPrinterSource/nozzle2Dst_temperatureIcon.svg"
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Nozzle") + "R"
                                        color: sourceTheme.text_normal_color
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 550 * screenScaleFactor
                                    }

                                    RowLayout {
                                        spacing: 40 * screenScaleFactor

                                        Text {
                                            Layout.preferredWidth: 50 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: `${deviceItem?deviceItem.nozzle2Temp.toFixed(2):"undefined"}°C`
                                            color: sourceTheme.text_normal_color
                                        }

                                        Text {
                                            Layout.preferredWidth: 10 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: "/"
                                        }

                                        Rectangle {
                                            id: __nozzle2DstTempBox
                                            property int minValue: 0
                                            property int maxValue: 300
                                            property var delayShow: false
                                            property var nozzleDstTemp: deviceItem?deviceItem.nozzle2DstTemp:"undefined"
                                            property alias boxValue: __editNozzle2DstTempBox.text

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
                                                if(ipAddress !== "")
                                                {
                                                    delayShow = true
                                                    __nozzleDstTempDelayShow.start()
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_2NDNOZZLETEMP, boxValue, deviceType)
                                                }
                                            }

                                            Layout.preferredWidth: 120 * screenScaleFactor
                                            Layout.preferredHeight: 36 * screenScaleFactor

                                            radius: 5
                                            opacity: enabled ? 1.0 : 0.5
                                            enabled: root.controlEnabled || root.isWlanPrinting

                                            color: sourceTheme.popup_background_color
                                            border.color: __editNozzle2DstTempBox.activeFocus ? "#3699FF" : sourceTheme.item_rectangle_border
                                            border.width: 1

                                            Binding on boxValue {
                                                when: !__editNozzle2DstTempBox.activeFocus && !__nozzle2DstTempBox.delayShow
                                                value: __nozzle2DstTempBox.nozzleDstTemp
                                            }

                                            Timer {
                                                repeat: false
                                                interval: 10000
                                                id: __nozzle2DstTempDelayShow
                                                onTriggered: __nozzle2DstTempBox.delayShow = false
                                            }

                                            TextField {
                                                id: __editNozzle2DstTempBox
                                                width: parent.width
                                                height: parent.height
                                                anchors.left: parent.left
                                                anchors.verticalCenter: parent.verticalCenter

                                                validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1-2][\\d][\\d]|300")}
                                                selectByMouse: true
                                                selectionColor: sourceTheme.textedit_selection_color
                                                selectedTextColor : color
                                                topPadding: 0
                                                bottomPadding: 0
                                                leftPadding: 10 * screenScaleFactor
                                                rightPadding: 4 * screenScaleFactor
                                                verticalAlignment: TextInput.AlignVCenter
                                                horizontalAlignment: TextInput.AlignLeft
                                                color: sourceTheme.text_normal_color
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                background: null

                                                Keys.onUpPressed: __nozzle2DstTempBox.increase()
                                                Keys.onDownPressed: __nozzle2DstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __nozzle2DstTempBox.specifyPcNozzleDstTemp()
                                            }

                                            Text {
                                                anchors.right: parent.right
                                                anchors.rightMargin: 10 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                font: __editBedDstTempBox.font
                                                color: __editBedDstTempBox.color
                                                verticalAlignment: Text.AlignVCenter
                                                text: "°C"
                                            }
                                        }
                                    }
                                }
                            }
                            
                            Rectangle {
                                color: sourceTheme.item_splitline_color
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 1 * screenScaleFactor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Rectangle {
                                color: "transparent"
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 40 * screenScaleFactor

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10 * screenScaleFactor
                                    anchors.leftMargin: 20 * screenScaleFactor
                                    anchors.rightMargin: 20 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 12 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        source: "qrc:/UI/photo/lanPrinterSource/bedDst_temperatureIcon.svg"
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Hot Bed")
                                        color: sourceTheme.text_normal_color
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 550 * screenScaleFactor
                                    }

                                    RowLayout {
                                        spacing: 40 * screenScaleFactor

                                        Text {
                                            Layout.preferredWidth: 50 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: `${deviceItem?deviceItem.pcBedTemp.toFixed(2):"undefined"}°C`
                                            color: sourceTheme.text_normal_color
                                        }

                                        Text {
                                            Layout.preferredWidth: 10 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: "/"
                                        }

                                        Rectangle {
                                            id: __bedDstTempBox
                                            property int minValue: 0
                                            property int maxValue: 200
                                            property var delayShow: false
                                            property var bedDstTemp: deviceItem?deviceItem.pcBedDstTemp:"undefined"
                                            property alias boxValue: __editBedDstTempBox.text

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
                                                if(ipAddress !== "")
                                                {
                                                    delayShow = true
                                                    __bedDstTempDelayShow.start()
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_BEDTEMP, boxValue, deviceType)
                                                }
                                            }

                                            Layout.preferredWidth: 120 * screenScaleFactor
                                            Layout.preferredHeight: 36 * screenScaleFactor

                                            radius: 5
                                            opacity: enabled ? 1.0 : 0.5
                                            enabled: root.controlEnabled || root.isWlanPrinting

                                            border.width: 1
                                            color: sourceTheme.popup_background_color
                                            border.color: __editBedDstTempBox.activeFocus ? "#3699FF" : sourceTheme.item_rectangle_border

                                            Binding on boxValue {
                                                when: !__editBedDstTempBox.activeFocus && !__bedDstTempBox.delayShow
                                                value: __bedDstTempBox.bedDstTemp
                                            }

                                            Timer {
                                                repeat: false
                                                interval: 10000
                                                id: __bedDstTempDelayShow
                                                onTriggered: __bedDstTempBox.delayShow = false
                                            }

                                            TextField {
                                                id: __editBedDstTempBox
                                                width: parent.width
                                                height: parent.height
                                                anchors.left: parent.left
                                                anchors.verticalCenter: parent.verticalCenter

                                                validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1][\\d][\\d]|200")}
                                                selectByMouse: true
                                                selectionColor: sourceTheme.textedit_selection_color
                                                selectedTextColor : color
                                                topPadding: 0
                                                bottomPadding: 0
                                                leftPadding: 10 * screenScaleFactor
                                                rightPadding: 4 * screenScaleFactor
                                                verticalAlignment: TextInput.AlignVCenter
                                                horizontalAlignment: TextInput.AlignLeft
                                                color: sourceTheme.text_normal_color
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                background: null

                                                Keys.onUpPressed: __bedDstTempBox.increase()
                                                Keys.onDownPressed: __bedDstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __bedDstTempBox.specifyPcBedDstTemp()
                                            }

                                            Text {
                                                anchors.right: parent.right
                                                anchors.rightMargin: 10 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                font: __editBedDstTempBox.font
                                                color: __editBedDstTempBox.color
                                                verticalAlignment: Text.AlignVCenter
                                                text: "°C"
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                color: sourceTheme.item_splitline_color
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 1 * screenScaleFactor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Rectangle {
                                color: "transparent"
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 40 * screenScaleFactor
                                visible : isKlipper_4408 || isDoubleNozzle

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 10 * screenScaleFactor
                                    anchors.leftMargin: 20 * screenScaleFactor
                                    anchors.rightMargin: 20 * screenScaleFactor

                                    Image {
                                        Layout.preferredWidth: 12 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        source: "qrc:/UI/photo/lanPrinterSource/chamberDst_temperatureIcon.svg"
                                    }

                                    Text {
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor

                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: qsTr("Chamber")
                                        color: sourceTheme.text_normal_color
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        Layout.minimumWidth: 100 * screenScaleFactor
                                        Layout.maximumWidth: 550 * screenScaleFactor
                                    }

                                    RowLayout {
                                        spacing: 40 * screenScaleFactor

                                        Text {
                                            Layout.preferredWidth: 50 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: `${deviceItem?deviceItem.chamberTemp.toFixed(2):"undefined"}°C`
                                            color: sourceTheme.text_normal_color
                                        }

                                        Text {
                                            Layout.preferredWidth: 10 * screenScaleFactor
                                            Layout.preferredHeight: 14 * screenScaleFactor

                                            horizontalAlignment: Text.horizontalAlignment
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: "/"
                                        }

                                        Rectangle {
                                            id: __chamberDstTempBox
                                            property int minValue: 0
                                            property int maxValue: 300
                                            property var delayShow: false
                                            property var nozzleDstTemp: deviceItem?deviceItem.chamberDstTemp:"undefined"
                                            property alias boxValue: __editChamberDstTempBox.text

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
                                                if(ipAddress !== "")
                                                {
                                                    delayShow = true
                                                    __nozzleDstTempDelayShow.start()
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CHAMBERTEMP, boxValue, deviceType)
                                                }
                                            }

                                            Layout.preferredWidth: 120 * screenScaleFactor
                                            Layout.preferredHeight: 36 * screenScaleFactor

                                            radius: 5
                                            opacity: enabled ? 1.0 : 0.5
                                            enabled: root.controlEnabled || root.isWlanPrinting

                                            color: sourceTheme.popup_background_color
                                            border.color: __editChamberDstTempBox.activeFocus ? "#3699FF" : sourceTheme.item_rectangle_border
                                            border.width: 1

                                            Binding on boxValue {
                                                when: !__editChamberDstTempBox.activeFocus && !__chamberDstTempBox.delayShow
                                                value: __chamberDstTempBox.nozzleDstTemp
                                            }

                                            Timer {
                                                repeat: false
                                                interval: 10000
                                                id: __chamberDstTempDelayShow
                                                onTriggered: __chamberDstTempBox.delayShow = false
                                            }

                                            TextField {
                                                id: __editChamberDstTempBox
                                                width: parent.width
                                                height: parent.height
                                                anchors.left: parent.left
                                                anchors.verticalCenter: parent.verticalCenter

                                                validator: RegExpValidator{regExp: new RegExp("[\\d]|[1-9][\\d]|[1-2][\\d][\\d]|300")}
                                                selectByMouse: true
                                                selectionColor: sourceTheme.textedit_selection_color
                                                selectedTextColor : color
                                                topPadding: 0
                                                bottomPadding: 0
                                                leftPadding: 10 * screenScaleFactor
                                                rightPadding: 4 * screenScaleFactor
                                                verticalAlignment: TextInput.AlignVCenter
                                                horizontalAlignment: TextInput.AlignLeft
                                                color: sourceTheme.text_normal_color
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                background: null

                                                Keys.onUpPressed: __chamberDstTempBox.increase()
                                                Keys.onDownPressed: __chamberDstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __chamberDstTempBox.specifyPcNozzleDstTemp()
                                                enabled: !isKlipper_4408
                                            }

                                            Text {
                                                anchors.right: parent.right
                                                anchors.rightMargin: 10 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter

                                                font: __editBedDstTempBox.font
                                                color: __editBedDstTempBox.color
                                                verticalAlignment: Text.AlignVCenter
                                                text: "°C"
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                color: sourceTheme.item_splitline_color
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: 1 * screenScaleFactor
                                Layout.alignment: Qt.AlignHCenter
                            }

                            QCxChart {
                                id: chartView
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.alignment: Qt.AlignHCenter
                                Layout.minimumWidth: 600 * screenScaleFactor
                                Layout.maximumWidth: 900 * screenScaleFactor
                                Layout.minimumHeight: 200 * screenScaleFactor
                                Layout.maximumHeight: 300 * screenScaleFactor

                                graphCount: isDoubleNozzle ? 4 : isKlipper_4408 ? 3 : 2
                                penColors: isDoubleNozzle ? ["#289ADA", "#A55757", "#CDB548", "#8841FF"] : isKlipper_4408? ["#289ADA", "#A55757", "#8841FF"] : ["#289ADA", "#A55757"]
                                fillColors: isDoubleNozzle ? [Qt.rgba(40/255, 154/255, 218/255, 0.08), Qt.rgba(165/255, 87/255, 87/255, 0.08), Qt.rgba(165/255, 87/255, 87/255, 0.08), Qt.rgba(165/255, 87/255, 87/255, 0.08)] : isKlipper_4408 ? [Qt.rgba(40/255, 154/255, 218/255, 0.08), Qt.rgba(165/255, 87/255, 87/255, 0.08), Qt.rgba(165/255, 87/255, 87/255, 0.08)] : [Qt.rgba(40/255, 154/255, 218/255, 0.08), Qt.rgba(165/255, 87/255, 87/255, 0.08)]
                                backgroundColor: sourceTheme.popup_background_color
                                gridLineColor: sourceTheme.item_splitline_color
                                labelsColor: sourceTheme.text_normal_color

                                labelsFont.weight: Font.Medium
                                labelsFont.family: Constants.mySystemFont.name
                                labelsFont.pointSize: Constants.labelFontPointSize_9

                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 10 * screenScaleFactor
                Layout.maximumWidth: 20 * screenScaleFactor
            }
        }
    }
}
