import QtQuick 2.15
import QtCharts 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
import com.cxsw.SceneEditor3D 1.0
import Qt.labs.platform 1.1
import QtMultimedia 5.15
import "qrc:/CrealityUI/secondqml/frameless"
import "errorData.js" as ErrorData
import "../qml"
import "../secondqml"
import "qrc:/CrealityUI/components"


Rectangle {
    id: root
    visible: true
    width: parent.width
    height: parent.height
    color: sourceTheme.background_color

    // property int themeType: -1
    // property var sourceTheme: ""

    property var deviceID: ""
    property var deviceType: ""
    property var deviceItem: ""

    property string object_current: "qrc:/UI/photo/lanPrinterSource/object_current.svg"
    property string material: "qrc:/UI/photo/lanPrinterSource/material.svg"
    property string materialBoxUpdate: "qrc:/UI/photo/lanPrinterSource/materialBoxUpdate.svg"
    property string materialBoxUpdate_disable: "qrc:/UI/photo/lanPrinterSource/materialBoxUpdate_disable.svg"
    property string humidity: "qrc:/UI/photo/lanPrinterSource/humidity.svg"
    property string humidity_normal: "qrc:/UI/photo/lanPrinterSource/humidity_normal.svg"
    property string humidity_height: "qrc:/UI/photo/lanPrinterSource/humidity_height.svg"
    property string humidity_heightest: "qrc:/UI/photo/lanPrinterSource/humidity_heightest.svg"
    property string materialBoxSetting: "qrc:/UI/photo/lanPrinterSource/materialBoxSetting.svg"
    property string dry: "qrc:/UI/photo/lanPrinterSource/dry.svg"
    property string showBoxMsg: "qrc:/UI/photo/lanPrinterSource/showBoxMsg.svg"
    property string showBoxMsg_light: "qrc:/UI/photo/lanPrinterSource/showBoxMsg_light.svg"
    property string editBoxMsg: "qrc:/UI/photo/lanPrinterSource/editBoxMsg.svg"
    property string editBoxMsg_disable: "qrc:/UI/photo/lanPrinterSource/editBoxMsg_disable.svg"
    property string editBoxMsg_light: "qrc:/UI/photo/lanPrinterSource/editBoxMsg_light.svg"
    property string noBoxPrinter: "qrc:/UI/photo/lanPrinterSource/noBoxPrinter.png"
    property string rackRfid: "qrc:/UI/photo/lanPrinterSource/RFID.svg"
    property string colorCheck: "qrc:/UI/photo/lanPrinterSource/colorCheck.svg"
    property string object_default: "qrc:/UI/photo/lanPrinterSource/object_default.svg"
    property string object_delete: "qrc:/UI/photo/lanPrinterSource/object_delete.svg"
    property string object_hover: "qrc:/UI/photo/lanPrinterSource/object_hover.svg"

    property string file_sort_default: "qrc:/UI/photo/lanPrinterSource/file_sort_default.svg"
    property string file_sort_hover: "qrc:/UI/photo/lanPrinterSource/file_sort_hover.svg"

    property string blanking: "qrc:/UI/photo/lanPrinterSource/blanking.svg"

    property int errorKey: deviceItem ? deviceItem.errorKey : 0
    property int errorCode: deviceItem ? deviceItem.errorCode : 0
    property int printerState: deviceItem ? deviceItem.pcPrinterState : 0
    property int printerStatus: deviceItem ? deviceItem.pcPrinterStatus : 0
    property int printerMethod: deviceItem ? deviceItem.pcPrinterMethod : 0
    property int curImportProgress: deviceItem ? deviceItem.gCodeImportProgress : 0
    property int maxTryTimes : 2    // 加载图片失败时，自动重新加载的最大次数
    property bool isPrinting:+deviceItem.pcPrinterState === 1 &&  +deviceItem.pcPrinterStatus === 1

    property bool hasCamera: deviceItem ? deviceItem.pcHasCamera : false
    property bool zAutohome: deviceItem ? (isKlipper_4408 ? deviceItem.zAutohome : true) : false
    property bool xyAutohome: deviceItem ? (isKlipper_4408 ? deviceItem.xyAutohome : true) : false
    property bool isDeviceIdle: (printerState != 1 && printerState <= 5)
    property bool isDeviceOnline: (printerStatus == 1) //在线状态
    property bool isWlanPrinting: (printerMethod === 1 || isKlipper_4408) && isDeviceOnline && !fatalErrorCode
    property bool controlEnabled: (isDeviceIdle && isDeviceOnline && !fatalErrorCode)
    property bool fatalErrorCode: errorCode == 20000 || errorCode == 20010
    property bool isKlipper_4408: deviceType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408
    property bool isDoubleNozzle : (deviceItem && deviceItem.nozzleCount > 1)
    property bool needLoadModelItem : false // 当前打印的预览图加载完了再加载ModelItem(列表),qml的Image不能多个同时加载，同时加载存在图片混淆的问题，用这个标志位做加载顺序的处理 */

    property string ipAddress: deviceItem ? deviceItem.pcIpAddr : ""
    property string macAddress: deviceItem ? deviceItem.pcPrinterID : ""
    property string curGcodeName: deviceItem ? deviceItem.pcGCodeName : ""
    property string errorMessage: deviceItem ? deviceItem.errorMessage : ""
    property string webrtcSupport: deviceItem ? deviceItem.webrtcSupport : ""
    //    property string previewImage: (isKlipper_4408 && !isDeviceIdle) ? `http://${ipAddress}:80/downloads/original/current_print_image.png` : `image://preview/icons/${deviceID}`

    property string previewImage: root.isDeviceIdle ? "" : `http://${ipAddress}:80/downloads/original/current_print_image.png`
    property string defaultImage: Constants.currentTheme ? "qrc:/UI/images/default_preview_light.svg" : "qrc:/UI/images/default_preview_dark.svg"
    property string modelItemImagePrefix: "http://" + ipAddress

    property string printObjectName: ""

    property string fileDefaultPrefixPath: "/usr/data/printer_data/gcodes"

    onDeviceIDChanged: {
        imagePreview_min.init()
    }

    onPrinterStateChanged: {
        imagePreview_min.load()
    }
    Connections {
        target:kernel_ui
        function onCurrentLanguageChanged(){
            ErrorData.updateErrorJsonTranslation();
        }
    }

    function second2String(sec) {
        var hours = Math.floor(sec / 3600)
        var minutes = Math.floor(sec % 3600 / 60)
        var seconds = Math.floor(sec % 3600 % 60)
        return `${hours}h ${minutes}m ${seconds}s`
    }

    function setTextColor(color) {
        let result = "";
        let bgColor = [color.r * 255, color.g * 255, color.b * 255];
        let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
        let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
        if (whiteContrast > blackContrast)
            result = "#ebebec";
        else
            result = "#28282b";
        return result;
    }

    function startPlayVideo() {
        if(ipAddress !== "" && hasCamera)
        {
            var urlStr = "rtsp://" + ipAddress + "/ch0_0"
            cameraItem.showPopup = true
            if(deviceType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_LAN)
                urlStr = "rtsp://" + ipAddress + "/ch0_0"
            else if(webrtcSupport==1)
            {
                urlStr = "http://" + ipAddress + ":8000/call/webrtc_local"
            }else{
                urlStr = "http://" + ipAddress + ":8080/?action=stream"
            }
            idVideoPlayer.setQmlEngine(Qt.qmlEngine);
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


        if(!isKlipper_4408)
            idSwipeview.currentIndex = LanPrinterDetail.ListViewType.GcodeListView
        object_delete_canvas.clean()
        deviceItem = printerListModel.cSourceModel.getDeviceItem(deviceID)  //获取设备
        historyFileListModel.setCurrentMacAddr(deviceID);
        historyFileListModel.switchDataSource(deviceID, deviceType)

        deviceItem.gcodeFileListModel.cSourceModel.setCurrentMacAddr(deviceID);
        deviceItem.gcodeFileListModel.cSourceModel.switchDataSource(deviceID, deviceType)

        videoListModel.setCurrentMacAddr(deviceID);
        videoListModel.switchDataSource(deviceID, deviceType)
        needLoadModelItem = isDeviceIdle
        initChartView(initData)
        imagePreview_min.load()
    }

    function widthProvider(value) {
        var w = 0
        switch(value) {
        case LanPrinterDetail.ListViewFieldType.FieldFileName:
            w = 320 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldFileSize:
            w = 100 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldLayerHeight:
            w = 100 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldInterval:
            w = 150 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldFileTime:
            w = 180 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldMaterial:
            w = 130 * screenScaleFactor;
            break;
        case LanPrinterDetail.ListViewFieldType.FieldMultiColor:
            w = 80 * screenScaleFactor;
            break;

        }
        return w
    }

    function getPrinterMethodText(method,printerState) {
        var methodText = ""
        switch(method)
        {
        case 0:
            methodText = cxTr("TF Card Printing")
            break;
        case 1:
            if(printerState===1)
                methodText = cxTr("WLAN Printing")
            if(printerState===6)
                methodText = cxTr("WLAN Pausing")
            if(printerState===7)
                methodText = cxTr("WLAN Stopping")
            if(printerState===8)
                methodText = cxTr("WLAN Restoring")

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

    property variant feedState: {
        0:qsTr('Select a slot and click the "Feed"/"Retract" button'),
        1: qsTr("Temperature will heat up to 240°C before feed"),
        2: qsTr("Extruding"),
        3: qsTr("Finish"),
        11: qsTr("Temperature will heat up to 240°C before retreat"),
        12: qsTr("Retract Prepare"),
        13: qsTr("Retracting"),
        14: qsTr("Retract Finish"),
        101: qsTr("Feed nozzle heating (220℃) 220℃ is an example, the actual operating temperature is based on the maximum temperature of the consumables"),
        102: qsTr("Feeding inspection line position"),
        103: qsTr("Feeding and cutting line"),
        104: qsTr("Feed back to the current material line"),
        105: qsTr("Feeding and delivering new material line"),
        106: qsTr("Feeding and biting line"),
        107: qsTr("Feed flushing old material line"),
        111: qsTr("Check the feed line position"),
        112: qsTr("Cutting line"),
        113: qsTr("Retract the current material line"),
        114: qsTr("Flushing old material line")
    }

    enum MessageType
    {
        Pause,
        Continue,
        Stop,
        CancelBrokenMaterial,
        NotExist,
        ObjectDelete,
        ObjectNotDelete
    }

    enum ListViewType
    {
        GcodeListView,
        HistoryListView,
        VideoListView
    }

    enum ListViewControlType
    {
        ControlListView,
        MaterialListView
    }

    enum ListViewFieldType
    {
        FieldFileName,
        FieldFileSize,
        FieldLayerHeight,
        FieldInterval,
        FieldFileTime,
        FieldMaterial,
        FieldMultiColor
    }

    enum FileOrderType
    {
        E_GCodeFileName,
        E_GCodeFileSize,
        E_GCodeFileTime,
        E_GCodeLayerHeight,
        E_GCodeMaterialLength,
        E_GCodeThumbnailImage,
        E_GCodeImportProgress,
        E_GCodeExportProgress
    }
    Loader
    {
        id: __prePrintDlgLoader

        active:  true
        //        anchors.fill: parent
        z : 1000
        property string gcodeFile: ""
        property string gcodeFileName: ""
        property string gcodeImageSource: ""
        property real gcodeVolumn: 0
        property var type: 0
        property var fileListModel
        parent: standaloneWindow
        onLoaded:
        {
            console.log("__prePrintDlgLoader loader...")
            item.currentObject = deviceItem
            item.type = __prePrintDlgLoader.type
            item.fileListModel = __prePrintDlgLoader.fileListModel
            item.gcodePath = __prePrintDlgLoader.gcodeFile
            item.gcodeFileName = __prePrintDlgLoader.gcodeFileName
            item.gcodeImageSource = __prePrintDlgLoader.gcodeImageSource
            item.gcodeVolumn = __prePrintDlgLoader.gcodeVolumn
            item.initShow()
            item.dlgClose.connect(()=>{
                                      console.log("__prePrintDlgLoader close...")
                                      __prePrintDlgLoader.sourceComponent = undefined
                                  })
        }
        onStatusChanged:
        {
            if (__prePrintDlgLoader.status == Loader.Ready) console.log('Loaded')
            console.log("__prePrintDlgLoader onStatusChanged = ",__prePrintDlgLoader.status)
        }
    }

    Component
    {
        id: __prePrintDlgCom
        LanGCodePrintDialog
        {

        }
    }

    // Binding on themeType {
    //     value: Constants.currentTheme
    // }

    // onThemeTypeChanged: sourceTheme = themeModel.get(themeType)//切换主题

    LanPrinterDialog {
        visible: false
        id: idVideoDialog
        title: qsTr("Camera")
        width: frameLessView.isMax ? 1330 * screenScaleFactor : 971 * screenScaleFactor
        height:frameLessView.isMax ? 810 * screenScaleFactor:607 * screenScaleFactor
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
            radius: 5 * screenScaleFactor

            Rectangle {
                anchors.fill: parent
                anchors.margins: 10 * screenScaleFactor

                color: sourceTheme.imgpreview_color
                radius: 5 * screenScaleFactor

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
    UploadMessageDlg {
        id: idMessageDlg
        property var link
        visible: false
        cancelBtnVisible: true
        ignoreBtnVisible: false
        okBtnText: qsTr("OK")
        cancelBtnText: qsTr("Open Browser")
        msgText: qsTr("Copy link successfully!")

        onSigOkButtonClicked: idMessageDlg.close()
        onSigCancelButtonClicked: {
            Qt.openUrlExternally(link)
            idMessageDlg.close()
        }
    }
    QMLPlayer {
        visible: true
        id: idVideoPlayer
        VideoOutput {
            id: display
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            source: frameProvider
            width: parent.width
            height: parent.height

        }
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
                    width:frameLessView.isMax ? 1280 * screenScaleFactor : 896 * screenScaleFactor
                    height:frameLessView.isMax ?  720 * screenScaleFactor : 504 * screenScaleFactor
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
                    property bool isAbort: printerState == 4
                    property bool isSuspended: printerState == 5
                    property bool isSuspending: printerState == 6
                    property bool isAborting: printerState == 7
                    property bool isRestoreing: printerState == 8


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
                                text: qsTr("Printing information")
                                color: sourceTheme.text_normal_color
                            }

                            Rectangle{
                                visible:errorCode != 0&&!printerPopup.visible
                                width:6* screenScaleFactor
                                height:6* screenScaleFactor
                                radius:height/2
                                anchors.top:parent.top
                                anchors.topMargin:10* screenScaleFactor
                                anchors.left:printMsg.right
                                anchors.leftMargin:2* screenScaleFactor
                                color:"#FD265A"
                            }

                            Item {
                                Layout.preferredWidth: 20 * screenScaleFactor - 2 * parent.spacing
                                Layout.fillHeight: true
                            }

                            Rectangle {
                                Layout.preferredWidth: 69 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                visible: +errorCode===20010

                                border.width: 1
                                border.color: sourceTheme.label_border_color
                                color: sourceTheme.label_background_color
                                radius: 5

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 5 * screenScaleFactor

                                    Image {
                                        width: 18 * screenScaleFactor
                                        height: 10 * screenScaleFactor
                                        source: blanking
                                        anchors.verticalCenter: parent.verticalCenter
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: "#FCE100"
                                        text: qsTr("Blanking")
                                    }
                                }
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
                                        text: qsTr("Idle")
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
                                        color: Constants.themeGreenColor
                                        text: getPrinterMethodText(printerMethod,printerState)
                                    }
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                            }

                            Rectangle {
                                radius: height / 2
                                opacity: enabled ? 1.0 : 0.5
                                enabled: root.isWlanPrinting
                                visible: errorCode == 0 && (printerItem.isPrinting || printerItem.isSuspended)

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
                                        source: `qrc:/UI/photo/lanPrinterSource/${printerItem.isSuspended?"continue":"pause"}_detail.svg`
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: printerItem.isSuspended?qsTr("Continue"):qsTr("Pause")
                                        color: sourceTheme.text_normal_color
                                    }
                                }

                                MouseArea {
                                    id: btnStateArea
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        var msgType = printerItem.isSuspended ? LanPrinterDetail.MessageType.Continue : LanPrinterDetail.MessageType.Pause
                                        idMessageDialog.showMessageDialog(msgType)
                                        idMessageDialog.ipAddress = ipAddress
                                        idMessageDialog.deviceType = deviceType
                                    }
                                }
                            }

                            Rectangle {
                                radius: height / 2
                                opacity: enabled ? 1.0 : 0.5
                                enabled: root.isWlanPrinting
                                visible: errorCode == 0 && (printerItem.isPrinting || printerItem.isSuspended)

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

                                    onClicked: {
                                        idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.Stop)
                                        idMessageDialog.ipAddress = ipAddress
                                        idMessageDialog.deviceType = deviceType
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
                                        console.log("delayLoadModelItem refresh")
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
                                    Connections{
                                        target:root
                                        function onPrinterStateChanged(){
                                            if(+root.printerState===1){
                                                delayLoadModelItem.start()
                                            }
                                        }
                                    }
                                    Timer {
                                        id: delayLoadModelItem
                                        interval: 3000
                                        repeat: false
                                        onTriggered: imagePreview_min.refresh()
                                    }
                                    // onStatusChanged: {
                                    //     if (source == "")
                                    //         return

                                    //     if (status === Image.Error) {
                                    //         if (reloadTimes) {
                                    //             reloadTimes--
                                    //             delayLoadModelItem.start()
                                    //         } else {
                                    //             /* 加载不成功时使用默认图标 */
                                    //             source = defaultImage
                                    //             refreshBtn.visible = true && !root.isDeviceIdle
                                    //             maximizeBtn.visible = false
                                    //         }
                                    //     } else if (status === Image.Ready){
                                    //         if (source == defaultImage) {
                                    //             refreshBtn.visible = !root.isDeviceIdle
                                    //             maximizeBtn.visible = false
                                    //             needLoadModelItem = true
                                    //         } else {
                                    //             if (reloadTimes) {
                                    //                 reloadTimes--
                                    //                 delayLoadModelItem.start()
                                    //             } else {
                                    //                 refreshBtn.visible = false
                                    //                 maximizeBtn.visible = true && !root.isDeviceIdle
                                    //                 needLoadModelItem = true
                                    //             }
                                    //         }
                                    //     }
                                    // }

                                    // Timer {
                                    //     /* 当前打印的预览图加载完了再加载ModelItem（列表）
                                    //         qml的Image不能多个同时加载，同时加载存在图片混淆的问题，因此这里需要做加载顺序的处理 */
                                    //     id: delayLoadModelItem
                                    //     interval: 600
                                    //     repeat: false
                                    //     onTriggered: imagePreview_min.refresh()
                                    // }

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

                                            color: Constants.themeGreenColor
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
                        Rectangle{
                            id: errorLayout
                            visible:  errorCode != 0 && (!ErrorData.errorJson[errorKey] || !ErrorData.errorJson[errorKey].isCfs)
                            Layout.alignment: Qt.AlignRight
                            property bool isRepoPlrStatus: errorCode == 20000
                            property bool isMaterialStatus: errorCode == 20010
                            property color displayTextColor: fatalErrorCode ? "#FD265A" : (themeType ? "#333333" : "#FCE100")
                            width: parent.width
                            height:30* screenScaleFactor
                            anchors.top:printerPopup.top
                            color:"#78714F"
                            RowLayout{
                                anchors.fill:parent
                                RowLayout{
                                    anchors.left: parent.left
                                    anchors.leftMargin:10* screenScaleFactor
                                    Layout.alignment: Qt.AlignLeft
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        visible:errorCode !== 20010
                                        color:ErrorData.errorJson[errorKey]? ["","#fd265a","#17cc5f","#fce100"][ErrorData.errorJson[errorKey].level]:"#fd265a"
                                        text:"["+ (!!ErrorData.errorJson[errorKey]?ErrorData.errorJson[errorKey].code:errorKey)+"]"
                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: tooltip.visible = !!ErrorData.errorJson[errorKey].wiki
                                            onExited: tooltip.visible = false
                                            onClicked:Qt.openUrlExternally(ErrorData.errorJson[errorKey].wiki)
                                            cursorShape:Qt.PointingHandCursor
                                            BasicTooltip {
                                                id:tooltip
                                                text: qsTr("Click to jump to Wiki")
                                                textColor:sourceTheme.text_weight_color
                                                font.pointSize: Constants.labelFontPointSize_9
                                            }
                                        }
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_normal_color
                                        text: ErrorData.errorJson[errorKey]?ErrorData.errorJson[errorKey].content: qsTr("Unknown exception")
                                    }
                                }


                                RowLayout{
                                    anchors.right: parent.right
                                    anchors.rightMargin:10* screenScaleFactor
                                    Layout.alignment: Qt.AlignRight
                                    //visible:root.isPrinting

                                    ErrorMsgBtnState{
                                        errorCode: root.errorKey
                                        errorLevel: ErrorData.errorJson[errorKey].level
                                        supportVersion: ErrorData.errorJson[errorKey].supportVersion
                                        machineState: root.printerState
                                        state: "printingNo_errorSerious"
                                        Component.onCompleted: {
                                            items[0] = continueBtnD
                                            items[1] = cancelBtnD
                                            items[2] = idRetryButtonD
                                            items[3] = okBtnD

                                            //                                            errorCode = Qt.binding(function(){ return root.errorCode; })
                                            //                                            errorLevel = Qt.binding(function() { return ErrorData.errorJson[errorKey].level; })
                                            //                                            machineState = Qt.binding(function() { return root.printerState; })
                                        }
                                    }

                                    Button {
                                        id: continueBtnD
                                         visible: false
                                        Layout.preferredWidth: 73 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                        id: cancelBtnD
                                         visible: false
                                        Layout.preferredWidth: 100 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                            text: qsTr("Cancel print")
                                        }

                                        onClicked:{
                                            idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.CancelBrokenMaterial)
                                            idMessageDialog.ipAddress = ipAddress
                                            idMessageDialog.deviceType = deviceType
                                        }

                                    }

                                    Button {
                                        id:idRetryButtonD
                                         visible: false
                                        Layout.preferredWidth: 73 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                            text: qsTranslate("LanPrinterListLocal", "Retry")
                                        }

                                        onClicked:
                                        {
                                            if(ipAddress !== "")
                                            {
                                                printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RETRY_ERROR, "0", deviceType)

                                            }
                                        }
                                    }
                                    Button {
                                        id: okBtnD
                                         visible: true
                                        Layout.preferredWidth: 73 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                            text: qsTr("OK")
                                        }

                                        onClicked:
                                        {
                                            if(ipAddress !== "")
                                            {
                                                if(idRetryButtonD.visible)
                                                {
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RETRY_ERROR, "1", deviceType)
                                                }else{
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", deviceType)
                                                }

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
                                Layout.preferredWidth: 11 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/controlIcon.svg"
                            }

                            Text {
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: sourceTheme.text_normal_color
                                text: qsTr("Control")
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
                        implicitHeight: 220* screenScaleFactor + flowItem.height //colLayoutItem.height + 30 * screenScaleFactor


                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: parent.forceActiveFocus()
                        }


                        ColumnLayout {
                            anchors.topMargin: 10 * screenScaleFactor
                            anchors.bottomMargin: 10 * screenScaleFactor
                            width: parent.width
                            id: controlListView
                            clip:true
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
                                        color: y_plusArea.containsMouse ? Constants.themeGreenColor : sourceTheme.btn_default_color

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
                                                source: sourceTheme.axis_plusIcon
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
                                                source: sourceTheme.axis_plusIcon
                                            }

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "Z"
                                            }
                                        }

                                        MouseArea {
                                            id: z_plusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var targetValue
                                                var targetCmd
                                                var targetValueLimit
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                if(deviceItem && deviceItem.machinePlatformMotionEnable)
                                                {
                                                    targetValue  = parseFloat(idZPostion.text) - curUnit
                                                    targetCmd = `Z-${curUnit} F600`
                                                    targetValueLimit = targetValue < 0

                                                }else {
                                                    targetValue  =parseFloat(idZPostion.text) + curUnit
                                                    targetCmd = `Z${curUnit} F600`
                                                    targetValueLimit = targetValue > deviceItem.machineDepth

                                                }

                                                if(!isKlipper_4408 && targetValueLimit){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? targetCmd : targetValue
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
                                        color: x_minusArea.containsMouse ? Constants.themeGreenColor : sourceTheme.btn_default_color

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
                                                source:Constants.currentTheme?"qrc:/UI/photo/lanPrinterSource/x_minusIcon_light.svg": "qrc:/UI/photo/lanPrinterSource/x_minusIcon.svg"
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
                                        color: xy_homeArea.containsMouse ? Constants.themeGreenColor : sourceTheme.btn_default_color

                                        Layout.preferredWidth: 40 * screenScaleFactor
                                        Layout.preferredHeight: 40 * screenScaleFactor

                                        Image {
                                            anchors.centerIn: parent
                                            width: 14 * screenScaleFactor
                                            height: 14 * screenScaleFactor
                                            source: Constants.currentTheme? "qrc:/UI/photo/lanPrinterSource/axis_homeIcon_light.svg" : "qrc:/UI/photo/lanPrinterSource/axis_homeIcon.svg"
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
                                        color: x_plusArea.containsMouse ? Constants.themeGreenColor : sourceTheme.btn_default_color

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
                                                source: Constants.currentTheme ? "qrc:/UI/photo/lanPrinterSource/x_plusIcon_light.svg"  : "qrc:/UI/photo/lanPrinterSource/x_plusIcon.svg"
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
                                            source: Constants.currentTheme? "qrc:/UI/photo/lanPrinterSource/axis_homeIcon_light.svg" : "qrc:/UI/photo/lanPrinterSource/axis_homeIcon.svg"
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
                                        color: y_minusArea.containsMouse ? Constants.themeGreenColor : sourceTheme.btn_default_color

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
                                                source: Constants.currentTheme ? "qrc:/UI/photo/lanPrinterSource/axis_minusIcon_light.svg":"qrc:/UI/photo/lanPrinterSource/axis_minusIcon.svg"
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
                                                source: Constants.currentTheme?"qrc:/UI/photo/lanPrinterSource/axis_minusIcon_light.svg": "qrc:/UI/photo/lanPrinterSource/axis_minusIcon.svg"
                                            }

                                            Text {
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                verticalAlignment: Text.AlignVCenter
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: "Z"
                                            }
                                        }

                                        MouseArea {
                                            id: z_minusArea
                                            hoverEnabled: true
                                            anchors.fill: parent
                                            cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                            onClicked: {
                                                var targetValue
                                                var targetCmd
                                                var targetValueLimit
                                                var curUnit = parseFloat(controlItem.currentUnit)
                                                if(deviceItem && deviceItem.machinePlatformMotionEnable)
                                                {
                                                    targetValue  =parseFloat(idZPostion.text) + curUnit
                                                    targetCmd = `Z${curUnit} F600`
                                                    targetValueLimit = targetValue > deviceItem.machineDepth

                                                }else {

                                                    targetValue  = parseFloat(idZPostion.text) - curUnit
                                                    targetCmd = `Z-${curUnit} F600`
                                                    targetValueLimit = targetValue < 0
                                                }

                                                if(!isKlipper_4408 && targetValueLimit){
                                                    return
                                                }
                                                var postion = isKlipper_4408 ? targetCmd : targetValue
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
                                                    color: parent.checked?Constants.themeGreenColor:(parent.hovered ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color)
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
                                                    text: qsTr("Moder Fan")
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


                                                        property var checkColor:Constants.currentTheme?Constants.themeGreenColor: "#047832"
                                                        color: fanSwitch.checked ? checkColor : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            property var checkColor:Constants.currentTheme?"#FFFFFF": Constants.themeGreenColor
                                                            color: fanSwitch.checked ? checkColor : sourceTheme.switch_indicator_color
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
                                                                color: Constants.themeGreenColor
                                                                radius: 2* screenScaleFactor
                                                            }
                                                        }

                                                        handle: Rectangle {
                                                            x: fanSpeedId.leftPadding + fanSpeedId.visualPosition * (fanSpeedId.availableWidth - width)
                                                            y: fanSpeedId.topPadding + fanSpeedId.availableHeight / 2 - height / 2
                                                            implicitWidth: 10* screenScaleFactor
                                                            implicitHeight: 10* screenScaleFactor
                                                            radius: 5* screenScaleFactor
                                                            color: Constants.themeGreenColor

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
                                        visible: deviceItem && deviceItem.machineCdsFanExist
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
                                                    text: qsTr("Side fan")
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
                                                        property var checkColor:Constants.currentTheme?Constants.themeGreenColor: "#047832"
                                                        color: auxiliaryFan.checked ? checkColor : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            property var checkColor:Constants.currentTheme?"#FFFFFF": Constants.themeGreenColor
                                                            color: auxiliaryFan.checked ? checkColor : sourceTheme.switch_indicator_color
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
                                                            color: Constants.themeGreenColor
                                                            radius: 2* screenScaleFactor
                                                        }
                                                    }

                                                    handle: Rectangle {
                                                        x: auxiliaryFanSpeedId.leftPadding + auxiliaryFanSpeedId.visualPosition * (auxiliaryFanSpeedId.availableWidth - width)
                                                        y: auxiliaryFanSpeedId.topPadding + auxiliaryFanSpeedId.availableHeight / 2 - height / 2
                                                        implicitWidth: 10* screenScaleFactor
                                                        implicitHeight: 10* screenScaleFactor
                                                        radius: 5* screenScaleFactor
                                                        color: Constants.themeGreenColor
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
                                        visible: deviceItem && deviceItem.machineChamberFanExist
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
                                                    text: qsTr("Back Fan")
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
                                                        property var checkColor:Constants.currentTheme?Constants.themeGreenColor: "#047832"
                                                        color: cavityFan.checked ? checkColor : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            property var checkColor:Constants.currentTheme?"#FFFFFF": Constants.themeGreenColor
                                                            color: cavityFan.checked ? checkColor : sourceTheme.switch_indicator_color
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
                                                            color: Constants.themeGreenColor
                                                            radius: 2* screenScaleFactor
                                                        }
                                                    }

                                                    handle: Rectangle {
                                                        x: cavityFanSpeedId.leftPadding + cavityFanSpeedId.visualPosition * (cavityFanSpeedId.availableWidth - width)
                                                        y: cavityFanSpeedId.topPadding + cavityFanSpeedId.availableHeight / 2 - height / 2
                                                        implicitWidth: 10* screenScaleFactor
                                                        implicitHeight: 10* screenScaleFactor
                                                        radius: 5* screenScaleFactor
                                                        color: Constants.themeGreenColor
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
                                        visible: deviceItem && deviceItem.machineLEDLightExist
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
                                                        property var checkColor:Constants.currentTheme?Constants.themeGreenColor: "#047832"
                                                        color: ledSwitch.checked ? checkColor : sourceTheme.switch_unchecked_background

                                                        Rectangle {
                                                            radius: height / 2
                                                            width: 26 * screenScaleFactor
                                                            height: 26 * screenScaleFactor
                                                            anchors.verticalCenter: parent.verticalCenter
                                                            property var checkColor:Constants.currentTheme?"#FFFFFF": Constants.themeGreenColor
                                                            color: ledSwitch.checked ? checkColor : sourceTheme.switch_indicator_color
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
                            title: qsTr("Open File1")
                            folder: shortcuts.desktop
                            nameFilters: ["*.gcode","*.3mf"]
                            fileMode : FileDialog.OpenFiles
                            options: if(Qt.platform.os == "linux") return FileDialog.DontUseNativeDialog
                            property string fileName: ""
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

                            RowLayout{
                                Layout.preferredHeight:40 * screenScaleFactor
                                Layout.leftMargin:10* screenScaleFactor
                                Layout.rightMargin:10* screenScaleFactor
                                Layout.alignment:Qt.AlignVCenter
                                Text{
                                    color: "#FFFFFF"
                                    text: {
                                        let idx = idSwipeview.currentIndex
                                        switch(idx){
                                        case LanPrinterDetail.ListViewType.GcodeListView:
                                            return qsTr("Selected files")+":"+ deviceItem.gcodeFileListModel.cSourceModel.gcodeFileSelectedCount+ "/" + deviceItem.gcodeFileListModel.cSourceModel.rowCount()
                                        case LanPrinterDetail.ListViewType.HistoryListView:
                                            return qsTr("Selected files")+":"+ historyFileListModel.historyFileSelectedCount+"/" +historyFileListModel.rowCount()
                                        case LanPrinterDetail.ListViewType.VideoListView:
                                            return qsTr("Selected files")+":"+ videoListModel.videoFileSelectedCount+"/" +videoListModel.rowCount()
                                        }
                                    }
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
                                        // 限制显示长度
                                        function limitFilename(name){
                                            let path = deviceItem.sendingFileName
                                            let idx = path.lastIndexOf("/") + 1
                                            let fileName = path.substring(idx)
                                            let result = fileName.length > 10?(fileName.substring(0,7)+"..."):fileName
                                            return result
                                        }
                                        id:uploadFileName
                                        Layout.preferredWidth: contentWidth * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        text: limitFilename(deviceItem.sendingFileName)  //deviceItem.sendingFileName
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
                                            color: Constants.themeGreenColor
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



                                Rectangle {
                                    opacity: enabled ? 1.0 : 0.5
                                    enabled: isDeviceOnline && curImportProgress <= 0//&& !fatalErrorCode
                                    visible: idSwipeview.currentIndex === LanPrinterDetail.ListViewType.GcodeListView

                                    border.width: 1
                                    border.color: sourceTheme.btn_border_color
                                    color: btnImportArea0.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                    radius: height / 2

                                    Layout.preferredWidth: 62 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor

                                    Row {
                                        height: parent.height
                                        anchors.centerIn: parent
                                        spacing: 6 * screenScaleFactor

                                        Image {
                                            width: 12 * screenScaleFactor
                                            height: 12 * screenScaleFactor
                                            anchors.verticalCenter: parent.verticalCenter
                                            source: "qrc:/UI/photo/lanPrinterSource/import.svg"
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
                                        id: btnImportArea0
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                        onClicked: {
                                            fileDialog.open()
                                        }
                                    }
                                }


                                Rectangle {
                                    opacity: enabled ? 1.0 : 0.5

                                    border.width: 1
                                    border.color: sourceTheme.btn_border_color
                                    color: btnImportArea1.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                    radius: height / 2
                                    enabled:{
                                        let idx = idSwipeview.currentIndex
                                        switch(idx){
                                        case LanPrinterDetail.ListViewType.GcodeListView:
                                            return deviceItem.gcodeFileListModel.cSourceModel.gcodeFileSelectedCount>0
                                        case LanPrinterDetail.ListViewType.HistoryListView:
                                            return historyFileListModel.historyFileSelectedCount>0
                                        case LanPrinterDetail.ListViewType.VideoListView:
                                            return videoListModel.videoFileSelectedCount>0
                                        }
                                    }
                                    Layout.preferredWidth: 62 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor

                                    Row {
                                        height: parent.height
                                        anchors.centerIn: parent
                                        spacing: 6 * screenScaleFactor

                                        Image {
                                            width: 12 * screenScaleFactor
                                            height: 12 * screenScaleFactor
                                            anchors.verticalCenter: parent.verticalCenter
                                            source: "qrc:/UI/photo/lanPrinterSource/export.svg"
                                        }

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: qsTr("Export")
                                            color: sourceTheme.text_normal_color
                                        }
                                    }

                                    MouseArea {
                                        id: btnImportArea1
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                        onClicked: {
                                            //检查所选文件列表是否有已经删除的
                                            let fileList = historyFileListModel.historyFileSelectedList;
                                            let noExistFiles = [];
                                            for(let index = 0; index < fileList.length; ++index){
                                                let fileName = fileList[index];
                                                let exist = deviceItem.gcodeFileListModel.cSourceModel.isGcodeFileExist(deviceID, fileName)
                                                if(!exist && noExistFiles.indexOf(fileName) == -1){
                                                    noExistFiles.push(fileName);
                                                }
                                            }

                                            console.log("noExistFiles === ", noExistFiles)
                                            if(noExistFiles.length > 0){
                                                let res = noExistFiles.length === fileList.length
                                                noExistFilesDialog.fileList = noExistFiles
                                                noExistFilesDialog.isAllDelete = res
                                                noExistFilesDialog.open()
                                            }
                                        }
                                    }
                                }

                                FolderDialog {
                                    id: save_dir_select_dialog
                                    options: Qt.platform.os !== "linux" ? FolderDialog.ShowDirsOnly
                                                                        : FolderDialog.ShowDirsOnly |
                                                                          FolderDialog.DontUseNativeDialog
                                    folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
                                    onAccepted:{
                                        switch(idSwipeview.currentIndex){
                                        case LanPrinterDetail.ListViewType.GcodeListView:
                                            deviceItem.gcodeFileListModel.cSourceModel.downloadSelectedGcodeFile(macAddress, ipAddress, currentFolder)
                                            break;
                                        case LanPrinterDetail.ListViewType.HistoryListView:
                                            historyFileListModel.downloadSelectedHistoryFile(macAddress, ipAddress, currentFolder)
                                            break;
                                        case LanPrinterDetail.ListViewType.VideoListView:
                                            videoListModel.downloadSelectedVideoFile(macAddress, ipAddress, currentFolder)
                                            break;
                                        }
                                    }


                                }




                                Rectangle {
                                    opacity: enabled ? 1.0 : 0.5
                                    enabled:{
                                        if(root.controlEnabled){
                                            let idx = idSwipeview.currentIndex
                                            switch(idx){
                                            case LanPrinterDetail.ListViewType.GcodeListView:
                                                return deviceItem.gcodeFileListModel.cSourceModel.gcodeFileSelectedCount>0
                                            case LanPrinterDetail.ListViewType.HistoryListView:
                                                return historyFileListModel.historyFileSelectedCount>0
                                            case LanPrinterDetail.ListViewType.VideoListView:
                                                return videoListModel.videoFileSelectedCount>0
                                            }
                                        }
                                        return false;
                                    }

                                    border.width: 1
                                    border.color: sourceTheme.btn_border_color
                                    color: btnImportArea2.containsMouse ? sourceTheme.btn_hovered_color : sourceTheme.btn_default_color
                                    radius: height / 2

                                    Layout.preferredWidth: 62 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor

                                    Row {
                                        height: parent.height
                                        anchors.centerIn: parent
                                        spacing: 6 * screenScaleFactor

                                        Image {
                                            width: 12 * screenScaleFactor
                                            height: 12 * screenScaleFactor
                                            anchors.verticalCenter: parent.verticalCenter
                                            source: "qrc:/UI/photo/lanPrinterSource/delete.svg"
                                        }

                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            verticalAlignment: Text.AlignVCenter
                                            font.weight: Font.Medium
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: qsTr("Delete")
                                            color: sourceTheme.text_normal_color
                                        }
                                    }

                                    MouseArea {
                                        id: btnImportArea2
                                        hoverEnabled: true
                                        anchors.fill: parent
                                        cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor
                                        onClicked: {
                                            let idx = idSwipeview.currentIndex
                                            switch(idx){
                                            case LanPrinterDetail.ListViewType.GcodeListView:
                                                deviceItem.gcodeFileListModel.cSourceModel.deleteSelectedGcodeFile(ipAddress)
                                                break;
                                            case LanPrinterDetail.ListViewType.HistoryListView:
                                                historyFileListModel.deleteSelectedHistoryFile(ipAddress)
                                                break;
                                            case LanPrinterDetail.ListViewType.VideoListView:
                                                videoListModel.deleteSelectedVideoFile(ipAddress)
                                                break;
                                            }
                                        }
                                    }
                                }



                            }
                            Rectangle{
                                Layout.fillWidth:true
                                Layout.preferredHeight:1
                                color:sourceTheme.item_splitline_color
                            }

                            SwipeView {
                                clip: true
                                id: idSwipeview
                                interactive: false
                                Layout.alignment: Qt.AlignCenter
                                Layout.preferredWidth: parent.width - 2 * screenScaleFactor
                                Layout.preferredHeight: parent.height - 42 * screenScaleFactor
                                ListView {
                                    clip: true
                                    focus: true
                                    id: gcodeListView
                                    reuseItems:true
                                    contentWidth: 930 * screenScaleFactor
                                    flickableDirection: Flickable.AutoFlickIfNeeded
                                    property var forcedItem: "" // 当前触发了菜单的选项
                                    property bool printerType: deviceItem.gcodeFileListModel.cSourceModel.printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408
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
                                            if(!currentItem) return
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

                                    model: deviceItem.gcodeFileListModel
                                    cacheBuffer: (40 + 1) * screenScaleFactor *2
                                    header: Rectangle {
                                        width: parent.width
                                        height: 40 * screenScaleFactor
                                        color: sourceTheme.popup_background_color

                                        RowLayout {
                                            ListModel {
                                                id:file_table_header
                                                ListElement{ title:qsTr("File name"); type:0;sort:true;}
                                                ListElement{ title:qsTr("File size"); type:1;sort:true }
                                                ListElement{ title:qsTr("Multicolor"); type:6 }
                                                ListElement{ title:qsTr("Layer height"); type:2 }
                                                ListElement{ title:qsTr("Creation time"); type:3;sort:true }
                                                ListElement{ title:qsTr("Material length"); type:4}

                                            }

                                            CXCheckBox{
                                                anchors.left:parent.left
                                                anchors.leftMargin:10* screenScaleFactor
                                                Layout.preferredWidth:60 * screenScaleFactor
                                                Layout.preferredHeight:40 * screenScaleFactor
                                                text: qsTr("All")
                                                onClicked:deviceItem.gcodeFileListModel.cSourceModel.setGcodeFileSelected(checked)
                                            }

                                            Repeater {
                                                model: file_table_header
                                                delegate: Rectangle {
                                                    id: gcodeRootItem
                                                    width: model.index===0? widthProvider(type) - 60* screenScaleFactor : widthProvider(type)
                                                    height: 40 * screenScaleFactor
                                                    color:  sourceTheme.popup_background_color
                                                    // visible: type === 6? !!deviceItem.materialBoxList:true
                                                    // border.width:2
                                                    // border.color: "#ffffff"
                                                    Row {
                                                        spacing: 6* screenScaleFactor
                                                        anchors.centerIn: parent
                                                        Text {
                                                            width: parent.width - image.width - spacing
                                                            height:  gcodeRootItem.height
                                                            verticalAlignment: Text.AlignVCenter
                                                            horizontalAlignment: Text.AlignLeft
                                                            font.weight: Font.Medium
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            color: sourceTheme.text_normal_color
                                                            text: title
                                                        }

                                                        Image {
                                                            property bool isHover: false
                                                            source: isHover ? file_sort_hover : file_sort_default
                                                            width: 12 * screenScaleFactor
                                                            height:  gcodeRootItem.height //12 * screenScaleFactor
                                                            visible: !!sort
                                                            verticalAlignment: Image.AlignVCenter
                                                            horizontalAlignment: Image.AlignRight
                                                            fillMode: Image.PreserveAspectFit

                                                            MouseArea {
                                                                anchors.fill: parent
                                                                hoverEnabled: true
                                                                onEntered: parent.isHover = true
                                                                onExited: parent.isHover = false
                                                                cursorShape: Qt.PointingHandCursor
                                                                onClicked: {
                                                                    deviceItem.gcodeFileListModel.setSort(0, model.type)

                                                                }
                                                            }
                                                        }
                                                    }
                                                    //                                                    MouseArea {
                                                    //                                                    anchors.fill: parent
                                                    //                                                    acceptedButtons: Qt.LeftButton
                                                    //                                                    onClicked: {
                                                    //                                                        gcodeRootItem.forceActiveFocus()
                                                    //                                                    }
                                                    //                                                    }
                                                }
                                            }
                                        }
                                    }
                                    delegate: Rectangle {
                                        id: gcodeItem
                                        color: "transparent"
                                        width: 930 * screenScaleFactor
                                        height: (40 + 1) * screenScaleFactor
                                        property bool isItemPrinting: false
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
                                            enabled: isDeviceOnline
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: {
                                                gcodeItem.forceActiveFocus()
                                                gcodeFileMenu.opened ? gcodeFileMenu.close() : gcodeFileMenu.openMenu(mouseX, mouseY)
                                                //isItemPrinting = gcodeFileNames.contains(curGcodeName)
                                                if(gcodeFileNames)
                                                {
                                                    for(var i = 0; i < gcodeFileNames.length; i++)
                                                    {
                                                        if(gcodeFileNames[i] === curGcodeName)
                                                        {
                                                            isItemPrinting = true
                                                            break
                                                        }
                                                    }
                                                }
                                                isItemPrinting = isItemPrinting && root.isPrinting
                                                console.log("gcodeItem.onClick"+isItemPrinting)
                                            }
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
                                                        enabled: root.isDeviceIdle&&errorKey===0 && !isItemPrinting
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
                                                                let prefixPath = gCodePrefixPath?gCodePrefixPath:fileDefaultPrefixPath
                                                                var printerPath = prefixPath+ "/"+ id_FileName.text
                                                                gcodeFileMenu.close()
                                                                if(!deviceItem.materialBoxList)
                                                                {
                                                                    console.log("single-color print,",deviceItem.materialBoxList)
                                                                    if(ipAddress !== "" && printerPath !== "")
                                                                    {
                                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_START, printerPath, deviceType)
                                                                    }

                                                                    return
                                                                }

                                                                var gcodePrewImage = modelItemImagePrefix + gCodeThumbnailImage
                                                                console.log("printerPath =",printerPath)
                                                                console.log("modelItemImagePrefix + gCodeThumbnailImage =",gcodePrewImage)
                                                                __prePrintDlgLoader.active = true
                                                                if(type!=0)
                                                                {
                                                                    __prePrintDlgLoader.fileListModel = fileListModel
                                                                    __prePrintDlgLoader.type = type
                                                                }
                                                                __prePrintDlgLoader.gcodeFile = printerPath
                                                                __prePrintDlgLoader.gcodeFileName = id_FileName.text
                                                                __prePrintDlgLoader.gcodeImageSource = gcodePrewImage
                                                                __prePrintDlgLoader.gcodeVolumn = gcodeLayerHeight
                                                                __prePrintDlgLoader.sourceComponent = __prePrintDlgCom

                                                            }
                                                        }
                                                    }
                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle&&errorKey===0
                                                        visible: root.isKlipper_4408 && !deviceItem.materialBoxList
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
                                                                var printerPath = id_FileName.text // editGcodeName.text

                                                                if(ipAddress !== "" && printerPath !== "")
                                                                {
                                                                    let prefixPath = gCodePrefixPath?gCodePrefixPath:fileDefaultPrefixPath
                                                                    var path = prefixPath+ "/"+ id_FileName.text
                                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_WITH_CALIBRATION, path, deviceType)
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
                                                                    deviceItem.gcodeFileListModel.cSourceModel.downloadGcodeFile(macAddress,ipAddress, gcodeFileName, saveGcodeDialog.file.toString(), deviceType)
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: !isItemPrinting
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
                                                                    let prefixPath = gCodePrefixPath?gCodePrefixPath:fileDefaultPrefixPath
                                                                    var path = prefixPath+ "/"+ id_FileName.text
                                                                    deviceItem.gcodeFileListModel.cSourceModel.deleteGcodeFile(ipAddress, path, deviceType)
                                                                }

                                                                gcodeFileMenu.close()
                                                            }
                                                        }
                                                    }
                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle || !isItemPrinting
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

                                                        CXCheckBox{
                                                            Layout.preferredWidth: 10 * screenScaleFactor
                                                            Layout.preferredHeight: 10 * screenScaleFactor
                                                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                                            Layout.leftMargin: 10 * screenScaleFactor
                                                            checked:mItem.gCodeSelected
                                                            onClicked: {
                                                                mItem.gCodeSelected = !mItem.gCodeSelected
                                                                checked = Qt.binding(function(){ return mItem.gCodeSelected; })
                                                            }
                                                        }
                                                        Loader {
                                                            id: imgLoader
                                                            Layout.preferredWidth: 24 * screenScaleFactor
                                                            Layout.preferredHeight: 24 * screenScaleFactor

                                                            sourceComponent: lazyImage
                                                        }
                                                        Component {
                                                            id: lazyImage
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

                                                                id:gCodeThumbnailImageId
                                                                property bool isHover:false
                                                                MouseArea{
                                                                    anchors.fill:parent
                                                                    hoverEnabled:true
                                                                    cursorShape:Qt.PointingHandCursor
                                                                    onEntered:parent.isHover = true
                                                                    onExited:parent.isHover = false
                                                                }

                                                                Popup {
                                                                    width: 110 * screenScaleFactor
                                                                    height: 110 * screenScaleFactor
                                                                    visible:gCodeThumbnailImageId.isHover
                                                                    background: Rectangle {
                                                                        color: "#000000"
                                                                        border.color: sourceTheme.right_submenu_border
                                                                        border.width: 1 * screenScaleFactor
                                                                        radius:5
                                                                    }
                                                                    onVisibleChanged:{

                                                                        if (visible) {
                                                                            gCodeThumbnailSource.source=modelItemImagePrefix + gCodeThumbnailImage
                                                                            console.log(gCodeThumbnailSource.source)
                                                                            console.log(lazyImage.status)
                                                                            if(lazyImage.status == Image.Error)
                                                                            {
                                                                                lazyImage.source="";
                                                                                lazyImage.source= modelItemImagePrefix + gCodeThumbnailImage
                                                                            }
                                                                        }else{
                                                                            gCodeThumbnailSource.source="";
                                                                        }
                                                                    }
                                                                    x: gCodeThumbnailImageId.width
                                                                    y: -height

                                                                    Image {
                                                                        id:gCodeThumbnailSource
                                                                        anchors.fill:parent
                                                                        anchors.centerIn:parent
                                                                        cache: false
                                                                        mipmap: true
                                                                        smooth: true
                                                                        asynchronous: true
                                                                        fillMode: Image.PreserveAspectFit
                                                                        sourceSize: Qt.size(width, height)

                                                                        property int tryCount: 0

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
                                                                property var ext: gcodeFileName.substring(gcodeFileName.lastIndexOf("."))
                                                                property var name: gcodeFileName.substring(0,gcodeFileName.lastIndexOf("."))
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
                                                                    value: editGcodeName
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
                                                                    if(name !== editGcodeName.text){
                                                                        let prefixPath = gCodePrefixPath?gCodePrefixPath:fileDefaultPrefixPath
                                                                        let path = prefixPath+ "/"+gcodeFileName
                                                                        var targetName = prefixPath+ "/"+ editGcodeName.text+ext
                                                                        deviceItem.gcodeFileListModel.cSourceModel.renameGcodeFile(ipAddress, path, targetName, deviceType)
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
                                                                        color: Constants.themeGreenColor
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
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldMultiColor)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text:[qsTr("No"),qsTr("Yes"),"--"][+gCodeIsMultiColor]
                                                    // visible:!!deviceItem.materialBoxList
                                                }

                                                Text {
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldLayerHeight)+10* screenScaleFactor
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
                                    reuseItems:true
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

                                        RowLayout {
                                            ListModel {
                                                id:history_table_header
                                                ListElement{ title:qsTr("File name"); type:0;}
                                                ListElement{ title:qsTr("File size"); type:1; }
                                                ListElement{ title:qsTr("Multicolor"); type:6 }
                                                ListElement{ title:qsTr("Start time"); type:3 }
                                                ListElement{ title:qsTr("Total duration"); type:5;}
                                                ListElement{ title:qsTr("Material usage"); type:4}
                                            }
                                            CXCheckBox{
                                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                                Layout.leftMargin: 10 * screenScaleFactor
                                                Layout.preferredWidth:60 * screenScaleFactor
                                                Layout.preferredHeight:40 * screenScaleFactor
                                                text: qsTr("All")
                                                onClicked:historyFileListModel.setHistoryFileSelected(checked)
                                            }
                                            Repeater {
                                                model: history_table_header
                                                delegate: Rectangle {
                                                    width:  model.index===0? widthProvider(type) - 60* screenScaleFactor : widthProvider(type)
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
                                                        text: title
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
                                            enabled: isDeviceOnline
                                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            onClicked: historyFileMenu.opened ? historyFileMenu.close() : historyFileMenu.openMenu(mouseX, mouseY)
                                            onContainsMouseChanged: parent.color = containsMouse ? sourceTheme.item_hovered_color : "transparent"

                                            Popup {
                                                property bool isExportEnabled
                                                id: historyFileMenu
                                                width: (160 + 2) * screenScaleFactor
                                                height: (historyMenuContent.height + 2) * screenScaleFactor

                                                background: Rectangle {
                                                    color: sourceTheme.right_submenu_background
                                                    border.color: sourceTheme.right_submenu_border
                                                    border.width: 1 * screenScaleFactor
                                                }
                                                onVisibleChanged: {
                                                    if(visible){
                                                        let exist = deviceItem.gcodeFileListModel.cSourceModel.isGcodeFileExist(deviceID, model.historyFileName)
                                                        historyFileMenu.isExportEnabled = exist && root.isKlipper_4408
                                                    }
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
                                                        enabled: historyFileMenu.isExportEnabled
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
                                                                    historyFileListModel.downloadGcodeFile(macAddress,ipAddress, historyFileName, saveGcodeDialog.file.toString(), deviceType, historyFileNumb)
                                                                }
                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle&&errorKey===0
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
                                                                historyFileMenu.close()
                                                                if(deviceID !== "" && printerPath !== "")
                                                                {
                                                                    let exist = deviceItem.gcodeFileListModel.cSourceModel.isGcodeFileExist(deviceID, printerPath)

                                                                    if(exist && ipAddress !== "")
                                                                    {
                                                                        let prefixPath = gCodePrefixPath?gCodePrefixPath:fileDefaultPrefixPath
                                                                        var path = prefixPath+ "/"+ printerPath
                                                                        if(!deviceItem.materialBoxList)
                                                                        {
                                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_START, path, deviceType)
                                                                            return;

                                                                        }
                                                                        var gcodePrewImage = modelItemImagePrefix + historyThumbnailImage

                                                                        __prePrintDlgLoader.active = true
                                                                        if(type!=0)
                                                                        {
                                                                            gcodePrewImage = modelItemImagePrefix + deviceItem.gcodeFileListModel.cSourceModel.getThumbByFileName(path)
                                                                            __prePrintDlgLoader.fileListModel = deviceItem.gcodeFileListModel.cSourceModel.getItemFileByFileName(path);
                                                                            __prePrintDlgLoader.type = type
                                                                        }
                                                                        __prePrintDlgLoader.gcodeFile = path
                                                                        __prePrintDlgLoader.gcodeFileName = historyFileName
                                                                        __prePrintDlgLoader.gcodeImageSource = gcodePrewImage
                                                                        __prePrintDlgLoader.gcodeVolumn = historyMaterialUsage
                                                                        __prePrintDlgLoader.sourceComponent = __prePrintDlgCom
                                                                    }
                                                                    else {
                                                                        idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.NotExist)
                                                                        idMessageDialog.ipAddress = ipAddress
                                                                        idMessageDialog.deviceType = deviceType
                                                                    }
                                                                }


                                                            }
                                                        }
                                                    }

                                                    Rectangle {
                                                        width: 160 * screenScaleFactor
                                                        height: 28 * screenScaleFactor

                                                        color: sourceTheme.right_submenu_background
                                                        enabled: root.isDeviceIdle&&errorKey===0
                                                        visible: root.isKlipper_4408 && !deviceItem.materialBoxList
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
                                                                    let exist = deviceItem.gcodeFileListModel.cSourceModel.isGcodeFileExist(deviceID, printerPath)

                                                                    if(exist && ipAddress !== "")
                                                                    {
                                                                        let prefixPath = gCodePrefixPath?gCodePrefixPath:fileDefaultPrefixPath
                                                                        var path = prefixPath+ "/"+ printerPath
                                                                        if(!deviceItem.materialBoxList)
                                                                        {
                                                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.PRINT_WITH_CALIBRATION, path, deviceType)
                                                                        }
                                                                    }
                                                                    else {
                                                                        idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.NotExist)
                                                                        idMessageDialog.ipAddress = ipAddress
                                                                        idMessageDialog.deviceType = deviceType
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
                                                        CXCheckBox{
                                                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                                            Layout.leftMargin: 10 * screenScaleFactor
                                                            anchors.top:parent.top
                                                            anchors.topMargin:6* screenScaleFactor
                                                            checked:mItem.historySelected
                                                            onClicked: {

                                                                mItem.historySelected = !mItem.historySelected
                                                                checked = Qt.binding(function(){ return mItem.historySelected; })
                                                            }
                                                        }

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
                                                    width: widthProvider(LanPrinterDetail.ListViewFieldType.FieldMultiColor)
                                                    height: 40 * screenScaleFactor

                                                    verticalAlignment: Text.AlignVCenter
                                                    horizontalAlignment: Text.AlignHCenter
                                                    font.weight: Font.Medium
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    elide: Text.ElideMiddle
                                                    text:[qsTr("No"),qsTr("Yes"),"--"][+historyIsMultiColor]
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
                                    reuseItems:true
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

                                        RowLayout {
                                            CXCheckBox{
                                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                                Layout.leftMargin: 10 * screenScaleFactor
                                                Layout.preferredWidth:60 * screenScaleFactor
                                                Layout.preferredHeight:40 * screenScaleFactor
                                                text: qsTr("All")
                                                onClicked:videoListModel.setVideoFileSelected(checked)
                                            }
                                            Repeater {
                                                model: [qsTr("Gcode name"), qsTr("Video size"), qsTr("Video name"), qsTr("Start time"), qsTr("Print time")]
                                                delegate: Rectangle {
                                                    id: videoRootItem
                                                    width: index===0? widthProvider(index)-70* screenScaleFactor : widthProvider(index)
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

                                                    MouseArea {
                                                        anchors.fill: parent
                                                        acceptedButtons: Qt.LeftButton
                                                        onClicked: {
                                                            videoRootItem.forceActiveFocus()
                                                        }
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
                                            onClicked: {
                                                videoItem.forceActiveFocus()
                                                videoFileMenu.opened ? videoFileMenu.close() : videoFileMenu.openMenu(mouseX, mouseY)
                                            }
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

                                                            onClicked:
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
                                                                    videoListModel.deleteVideoFile(ipAddress, videoFilePath, deviceType)
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
                                                                    videoListModel.downloadVideoFile(macAddress, ipAddress, videoFileName, saveDialog.file.toString(), deviceType)
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
                                                                idMessageDlg.link = copyLinkAction.link
                                                                idMessageDlg.show()
                                                                videoFileMenu.close()
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
                                                        CXCheckBox{
                                                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                                            Layout.leftMargin: 10 * screenScaleFactor

                                                            checked:mItem.videoSelected
                                                            onClicked: {

                                                                mItem.videoSelected = !mItem.videoSelected
                                                                checked = Qt.binding(function(){ return mItem.videoSelected; })
                                                            }
                                                        }

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
                                                    property var ext: videoFileShowName.substring(videoFileShowName.lastIndexOf("."))
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
                                                        let textName = editVideoName.text
                                                        if(videoFileName !== textName){
                                                            let resultName = textName.endsWith(ext)?textName:textName+ext
                                                            videoListModel.renameVideoFile(ipAddress, videoFilePath, resultName, deviceType)
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

                ColumnLayout{
                    id: objectDeleteItem
                    spacing: 0
                    visible: deviceItem&&deviceItem.printObjects !== undefined
                    property bool showPopup: true //deviceItem&&!!deviceItem.printObjects
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
                                source: "qrc:/UI/photo/lanPrinterSource/object_title.svg"
                            }

                            Text {
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                text: qsTr("Object deletion")
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
                                    source: objectDeleteItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        objectDeleteItem.showPopup = !objectDeleteItem.showPopup
                                        objectDeletePopup.visible = objectDeleteItem.showPopup
                                    }
                                }
                            }
                        }
                    }
                    CusRoundedBg {
                        id: objectDeletePopup
                        visible: true //deviceItem&&!!deviceItem.printObjects
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
                        Layout.minimumHeight: 626 * screenScaleFactor
                        Layout.maximumHeight: 932 * screenScaleFactor
                        Layout.preferredHeight: width

                        Item {
                            anchors.fill: parent
                            Item{
                                id: content_grid
                                anchors.fill: parent
                                Canvas{
                                    id:object_delete_canvas
                                    width: parent.width
                                    height: parent.height
                                    anchors.centerIn: parent
                                    antialiasing: true
                                    property int wgrid: 30
                                    onVisibleChanged:{
                                        console.log("machineWidthmachineWidth:",+deviceItem.machineWidth)
                                    }
                                    property var printObjects:deviceItem?deviceItem.printObjects: "[]"
                                    property var excludedObjects: deviceItem?deviceItem.excludedObjects: "[]"
                                    property var currentObject:  deviceItem?deviceItem.currentObject: ""

                                    property var shapes: []
                                    property var images: []
                                    property var pointUnit: height/ (wgrid*12)
                                    function repaintCanvas(){
                                        if(!object_delete_canvas.printObjects) return
                                        let canvas = object_delete_canvas
                                        for (let image of canvas.images) {
                                            image.destroy();
                                            delete image;
                                        }
                                        canvas.images = []
                                        canvas.shapes = []
                                        let printArr = JSON.parse(canvas.printObjects)
                                        let excludedArr = canvas.excludedObjects || []
                                        let currentObject = canvas.currentObject

                                        for(let item of printArr){
                                            let type = 2
                                            if(excludedArr.includes(item.name)) type = 1
                                            if(item.name === currentObject)type = 0
                                            let arr = []
                                            for(let i of item.polygon){
                                                arr.push([Math.floor(i[0]*canvas.pointUnit),canvas.height-Math.floor(i[1]*canvas.pointUnit)])
                                            }
                                            item.type = type
                                            item.polygon = arr
                                            item.center=[Math.floor(item.center[0]*canvas.pointUnit),canvas.height-Math.floor(item.center[1]*canvas.pointUnit)]
                                            item.rectLong = canvas.calculateShortestDistance(item.center,item.polygon)
                                            canvas.shapes.push(item)
                                        }
                                        canvas.clean();
                                        canvas.requestPaint();
                                    }
                                    Connections {
                                        target: deviceItem
                                        onPrintObjectsChanged: {
                                            console.log("printArr connect changed:",deviceItem.printObjects)
                                            object_delete_canvas.printObjects = deviceItem.printObjects
                                            object_delete_canvas.repaintCanvas()
                                        }
                                    }
                                    //onExcludedObjectsChanged: repaintCanvas()
                                    onCurrentObjectChanged: repaintCanvas()
                                    function clean()
                                    {
                                        if(visible){
                                            getContext("2d").reset();
                                            getContext("2d").clearRect(0, 0, object_delete_canvas.width, object_delete_canvas.height);
                                        }
                                    }
                                    // 检查点是否在多边形内部
                                    function isPointInPolygon(x, y, polygon) {
                                        var isInside = false;
                                        var j = polygon.length - 1;
                                        for (var i = 0; i < polygon.length; i++) {
                                            let checkPoint = (polygon[i][1] > y) != (polygon[j][1] > y) &&
                                                x < (polygon[j][0] - polygon[i][0]) * (y - polygon[i][1]) /
                                                (polygon[j][1] - polygon[i][1]) + polygon[i][0]
                                            if (checkPoint) isInside = !isInside;
                                            j = i;
                                        }
                                        return isInside;
                                    }
                                    function isMouseInSquare(mouseX, mouseY, center) {
                                        var squareSize = 50; // 正方形的边长
                                        var squareLeft = center[0] - (squareSize / 2);
                                        var squareTop = center[1] - (squareSize / 2);
                                        return mouseX >= squareLeft && mouseX <= squareLeft + squareSize &&
                                                mouseY >= squareTop && mouseY <= squareTop + squareSize
                                    }
                                    function calculateShortestDistance(center, polygon) {
                                        var shortestDistance = Number.MAX_VALUE;
                                        for (var i = 0; i < polygon.length; i++) {
                                            var currentVertex = polygon[i];
                                            var nextVertex = polygon[(i + 1) % polygon.length];
                                            var edgeVector = [nextVertex[0] - currentVertex[0], nextVertex[1] - currentVertex[1]];
                                            var centerToEdgeVector = [center[0] - currentVertex[0], center[1] - currentVertex[1]];
                                            var edgeLengthSquared = Math.pow(edgeVector[0], 2) + Math.pow(edgeVector[1], 2);
                                            if (edgeLengthSquared === 0)  continue;

                                            var projectionLength = (centerToEdgeVector[0] * edgeVector[0] + centerToEdgeVector[1] * edgeVector[1]) / edgeLengthSquared;
                                            if (projectionLength < 0 || projectionLength > 1) continue;
                                            var projectionPoint = [
                                                        currentVertex[0] + projectionLength * edgeVector[0],
                                                        currentVertex[1] + projectionLength * edgeVector[1]
                                                    ];
                                            var distance = Math.sqrt(Math.pow(center[0] - projectionPoint[0], 2) + Math.pow(center[1] - projectionPoint[1], 2));
                                            shortestDistance = Math.min(shortestDistance, distance);
                                        }

                                        return shortestDistance;
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            let  canvas = object_delete_canvas
                                            let printArr = JSON.parse(canvas.printObjects)
                                            let excludedArr = JSON.parse(canvas.excludedObjects) || []
                                            if(printArr.length - excludedArr.length ===1){
                                                idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.ObjectNotDelete)
                                                idMessageDialog.ipAddress = ipAddress
                                                idMessageDialog.deviceType = deviceType
                                            }else {
                                                for (var s = 0; s < canvas.shapes.length; s++) {
                                                    var shape = canvas.shapes[s];
                                                    if (canvas.isPointInPolygon( mouse.x,  mouse.y, shape.polygon)&& shape.type !== 1) {
                                                        printObjectName = shape.name
                                                        idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.ObjectDelete, "",printObjectName)
                                                        idMessageDialog.ipAddress = ipAddress
                                                        idMessageDialog.deviceType = deviceType
                                                        break;
                                                    }
                                                }
                                            }

                                        }
                                    }

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height);
                                        ctx.save()
                                        var zero = Qt.point(0,height)
                                        ctx.lineWidth = 0.2756410256410256
                                        ctx.strokeStyle = '#5a5a5a'
                                        ctx.lineDashOffset=0
                                        ctx.beginPath()
                                        var nrows = width/wgrid;
                                        ctx.moveTo(zero);

                                        for(var i=1; i<nrows+1; i++)
                                        {
                                            var p1 = Qt.point(zero.x+wgrid*i,zero.y)
                                            var p2 = Qt.point(p1.x,p1.y-height)
                                            ctx.moveTo(p1.x,p1.y);
                                            ctx.lineTo(p2.x,p2.y);
                                        }
                                        var ncols = height/wgrid
                                        for(var j=1; j < ncols+1; j++){
                                            p1 = Qt.point(zero.x,zero.y-wgrid*j)
                                            p2 = Qt.point(p1.x+width,p1.y)
                                            ctx.moveTo(p1.x,p1.y);
                                            ctx.lineTo(p2.x,p2.y);
                                        }

                                        ctx.stroke();

                                        for (var c = 0; c < object_delete_canvas.shapes.length; c++) {
                                            ctx.beginPath();
                                            var icon = object_delete_canvas.shapes[c];
                                            var image = Qt.createQmlObject('import QtQuick 2.0; Image {}', object_delete_canvas);
                                            var type = "white"
                                            image.source = ""

                                            if(icon.type === 1)
                                            {
                                                image.source  = root.object_delete;
                                                type = "gray"
                                            }
                                            if(icon.type === 0){
                                                image.source  = root.object_current;
                                                type = "white"
                                            }
                                            if(icon.type === 2) {
                                                image.source  = root.object_default;
                                                type = "blue"
                                            }

                                            if (icon.polygon.length > 0) {
                                                var startPoint = icon.polygon[0];
                                                ctx.moveTo(startPoint[0], startPoint[1]);
                                                for (var p = 1; p < icon.polygon.length; p++) {
                                                    var point = icon.polygon[p];
                                                    //console.log("polygonpoint", point)
                                                    ctx.lineTo(point[0], point[1]);
                                                }
                                            }
                                            ctx.closePath()
                                            ctx.lineWidth = 2;  // 设置边框宽度
                                            ctx.strokeStyle = "gray";  // 设置边框颜色

                                            console.log("rectLong",image.width,image.height)
                                            image.width = icon.rectLong*2; // 设置图片宽度
                                            image.height = icon.rectLong*2; // 设置图片高度
                                            image.x = icon.center[0] - image.width / 2; // 计算图片的左上角x坐标
                                            image.y =icon.center[1] - image.height / 2; // 计算图片的左上角y坐标
                                            image.sourceSize.width = image.width; // 设置源图片宽度
                                            image.sourceSize.height = image.height; // 设置源图片高度
                                            image.fillMode = Image.PreserveAspectFit; // 设置图片填充模式
                                            object_delete_canvas.images.push(image);
                                            ctx.stroke();
                                        }

                                        ctx.restore();
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
                        // Layout.fillHeight: true
                        Layout.minimumWidth: 626 * screenScaleFactor
                        Layout.maximumWidth: 932 * screenScaleFactor
                        // Layout.minimumHeight: 330 * screenScaleFactor
                        // Layout.maximumHeight: 680 * screenScaleFactor
                        implicitHeight: 500 * screenScaleFactor

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
                    id: mateialItem
                    visible:!!deviceItem.materialBoxList
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
                                Layout.preferredWidth: 11 * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor
                                source: "qrc:/UI/photo/lanPrinterSource/material.svg"
                            }

                            Text {
                                id:materialTitle
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 14 * screenScaleFactor

                                verticalAlignment: Text.AlignVCenter
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                text: qsTr("Filament")
                                color: sourceTheme.text_normal_color
                            }
                            Rectangle{
                                visible:errorCode != 0&&!materialPopup.visible&&ErrorData.errorJson[errorKey].isCfs
                                width:6* screenScaleFactor
                                height:6* screenScaleFactor
                                radius:height/2
                                anchors.top:parent.top
                                anchors.topMargin:10* screenScaleFactor
                                anchors.left:materialTitle.right
                                anchors.leftMargin:2* screenScaleFactor
                                color:"#FD265A"
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
                                    source: mateialItem.showPopup ? sourceTheme.img_upArrow : sourceTheme.img_downArrow
                                }

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                                    onClicked: {
                                        mateialItem.showPopup = !mateialItem.showPopup
                                        materialPopup.visible = mateialItem.showPopup
                                    }
                                }
                            }
                        }
                    }


                    CusRoundedBg {
                        id: materialPopup
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
                        implicitHeight: 330* screenScaleFactor //colLayoutItem.height + 30 * screenScaleFactor


                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            onClicked: parent.forceActiveFocus()
                        }
                        Rectangle
                        {
                            color: "transparent"
                            anchors.fill:parent

                            ColumnLayout{
                                anchors.centerIn: parent
                                RowLayout{
                                    id:boxList
                                    spacing:10 * screenScaleFactor
                                    Rectangle{
                                        color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                        radius:5 * screenScaleFactor
                                        border.width:1
                                        border.color: Constants.currentTheme? "#D6D6DC": "#515157"
                                        width:89* screenScaleFactor
                                        height:209* screenScaleFactor
                                        ColumnLayout{
                                            anchors.fill:parent
                                            Text {
                                                id:boxName
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                horizontalAlignment: Text.AlignHCenter
                                                anchors.top:parent.top
                                                anchors.topMargin:40* screenScaleFactor
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("Spool Holder")
                                                anchors.horizontalCenter:parent.horizontalCenter
                                            }
                                            Rectangle{
                                                id:materialBox
                                                width:57* screenScaleFactor
                                                height:57* screenScaleFactor
                                                radius:height/2
                                                color: "#505156"
                                                border.width:2
                                                border.color: "#686870"
                                                anchors.top:boxName.bottom
                                                anchors.topMargin:8* screenScaleFactor
                                                anchors.horizontalCenter:parent.horizontalCenter
                                                Rectangle{
                                                    id: idRack
                                                    visible:deviceItem.materialBoxList.rack
                                                    width:53* screenScaleFactor
                                                    height:53* screenScaleFactor
                                                    color:deviceItem.materialBoxList.color
                                                    anchors.centerIn:parent
                                                    radius:height/2
                                                    Rectangle{
                                                        anchors.centerIn:parent
                                                        width:13* screenScaleFactor
                                                        height:13* screenScaleFactor
                                                        radius:height/2
                                                        color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                                    }
                                                }
                                                property var lColor: (+model.state===0 || +model.state===1) ? "#FFFFFF": root.setTextColor(materialColor)
                                                Rectangle{
                                                    width:8* screenScaleFactor
                                                    height:1
                                                    color:parent.lColor
                                                    anchors.left:parent.left
                                                    anchors.leftMargin:4* screenScaleFactor
                                                    anchors.top:parent.top
                                                    anchors.topMargin:parent.height/2
                                                }
                                                Rectangle{
                                                    width:6* screenScaleFactor
                                                    height:2
                                                    color:parent.lColor
                                                    anchors.right:parent.right
                                                    anchors.rightMargin:4* screenScaleFactor
                                                    anchors.top:parent.top
                                                    anchors.topMargin:parent.height/2
                                                }
                                                Rectangle{
                                                    width:1
                                                    height:8* screenScaleFactor
                                                    color:parent.lColor
                                                    anchors.left:parent.left
                                                    anchors.leftMargin:parent.width/2
                                                    anchors.top:parent.top
                                                    anchors.topMargin:4* screenScaleFactor
                                                }
                                                Rectangle{
                                                    width:1
                                                    height:8* screenScaleFactor
                                                    color:parent.lColor
                                                    anchors.left:parent.left
                                                    anchors.leftMargin:parent.width/2
                                                    anchors.bottom:parent.bottom
                                                    anchors.bottomMargin:4* screenScaleFactor
                                                }
                                                Text {
                                                    visible:!deviceItem.materialBoxList.rack
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_12
                                                    color: sourceTheme.text_normal_color
                                                    text: qsTr("?")
                                                    anchors.centerIn:parent
                                                }
                                            }

                                            Canvas {
                                                anchors.top: materialBox.bottom
                                                anchors.topMargin: 4 * screenScaleFactor
                                                anchors.horizontalCenter:parent.horizontalCenter
                                                visible: deviceItem.materialBoxList.used
                                                width: 14 * screenScaleFactor
                                                height: 8 * screenScaleFactor

                                                contextType: "2d"

                                                onPaint: {
                                                    const context = getContext("2d")
                                                    context.clearRect(0, 0, width, height);
                                                    context.beginPath();
                                                    if (!root.down) {
                                                        context.moveTo(0, 0);
                                                        context.lineTo(width, 0);
                                                        context.lineTo(width / 2, height);
                                                    } else {
                                                        context.moveTo(0, height);
                                                        context.lineTo(width, height);
                                                        context.lineTo(width / 2, 0);
                                                    }
                                                    context.closePath();
                                                    context.fillStyle = deviceItem.materialBoxList.color
                                                    context.fill();
                                                }
                                            }

                                            Text {
                                                id:rackTypeId
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                horizontalAlignment: Text.AlignHCenter
                                                anchors.top:materialBox.bottom
                                                anchors.topMargin:18* screenScaleFactor
                                                color: sourceTheme.text_normal_color
                                                text: !!deviceItem.materialBoxList.type?deviceItem.materialBoxList.type:"--"
                                                anchors.horizontalCenter:parent.horizontalCenter
                                            }

                                            Image{
                                                //visible:deviceItem.materialBoxList.rack
                                                anchors.top:rackTypeId.bottom
                                                anchors.topMargin:14* screenScaleFactor
                                                width:21* screenScaleFactor
                                                height:16* screenScaleFactor
                                                source: Constants.currentTheme? editBoxMsg_light: root.editBoxMsg
                                                sourceSize:Qt.size(width,height)
                                                anchors.horizontalCenter:parent.horizontalCenter
                                                MouseArea{
                                                    anchors.fill:parent
                                                    cursorShape:Qt.PointingHandCursor
                                                    onClicked: {
                                                        boxMap.materialItem={
                                                            materialType:deviceItem.materialBoxList.type,
                                                            materialColor:deviceItem.materialBoxList.color,
                                                            name:deviceItem.materialBoxList.name,
                                                            vendor:deviceItem.materialBoxList.vendor,
                                                            minTemp:deviceItem.materialBoxList.minTemp,
                                                            maxTemp:deviceItem.materialBoxList.maxTemp,
                                                            pressure:deviceItem.materialBoxList.pressure||0.04,
                                                            state:1

                                                        }
                                                        materialMsg.visible = true
                                                    }
                                                }
                                            }

                                        }
                                    }
                                    Rectangle{
                                        id:boxMap
                                        color: Constants.currentTheme?"#FFFFFF" :"#37373B"
                                        radius:5 * screenScaleFactor
                                        border.width:1
                                        border.color: Constants.currentTheme? "#D6D6DC": "#515157"
                                        height:209*screenScaleFactor
                                        width:353*screenScaleFactor
                                        clip:true
                                        property int curBoxId;
                                        property int curGroup:0;
                                        property int curMaterialId;
                                        property var materialItem:null
                                        ColumnLayout{
                                            visible: +deviceItem.cfsConnect===1
                                            anchors.fill:parent
                                            RowLayout{
                                                Layout.fillWidth:true
                                                Layout.preferredHeight:parent.height-30* screenScaleFactor
                                                Repeater{
                                                    model:deviceItem.materialBoxList
                                                    delegate: Rectangle{
                                                        color: "transparent"
                                                        width:80* screenScaleFactor
                                                        height:195* screenScaleFactor
                                                        Layout.topMargin:1
                                                        Layout.leftMargin:1
                                                        visible:model.index+1>boxMap.curGroup*4 && (model.index+1)<=(boxMap.curGroup+1)*4
                                                        MouseArea{
                                                            anchors.fill:parent
                                                            onClicked: {
                                                                deviceItem.materialBoxList.getMaterialSelected(materialBoxId,materialId)
                                                                boxMap.curBoxId = materialBoxId
                                                                boxMap.curMaterialId = materialId


                                                            }
                                                        }
                                                        ColumnLayout{
                                                            anchors.fill:parent
                                                            property var selectedColor:Constants.currentTheme?Constants.themeGreenColor:"#FFFFFF"
                                                            property var defaultColor:Constants.currentTheme?"#333333":"#999999"
                                                            Image{
                                                                id:boxName
                                                                anchors.top:parent.top
                                                                anchors.topMargin:27* screenScaleFactor
                                                                width:28* screenScaleFactor
                                                                height:28* screenScaleFactor
                                                                source:selected? root.materialBoxUpdate:root.materialBoxUpdate_disable
                                                                sourceSize:Qt.size(width,height)
                                                                anchors.horizontalCenter:parent.horizontalCenter
                                                                enabled: !root.isPrinting
                                                                Behavior on rotation {
                                                                    PropertyAnimation {
                                                                        duration: 500
                                                                        from: 0
                                                                        to: 360
                                                                    }
                                                                }
                                                                MouseArea{
                                                                    anchors.fill:parent
                                                                    cursorShape:Qt.PointingHandCursor
                                                                    onClicked:{
                                                                        if(+model.state===0) return
                                                                        if(+deviceItem.printerState==1){
                                                                            busyMsg.visible = true
                                                                            busyTimer.start()
                                                                            return
                                                                        }


                                                                        boxName.rotation = 0;
                                                                        boxName.rotation = 360;
                                                                        deviceItem.materialBoxList.refreshBox(deviceItem.pcIpAddr,materialBoxId,materialId)
                                                                    }
                                                                }

                                                            }
                                                            Text {
                                                                font.weight: Font.Bold
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_8
                                                                color: selected ? parent.selectedColor: parent.defaultColor
                                                                text:  materialBoxId+["A","B","C", "D"][materialId]
                                                                anchors.left:boxName.left
                                                                anchors.leftMargin:boxName.width/4
                                                                anchors.top:boxName.top
                                                                anchors.topMargin:boxName.height/4

                                                            }
                                                            Rectangle{
                                                                id:materialBox
                                                                width:57* screenScaleFactor
                                                                height:57* screenScaleFactor
                                                                radius:height/2
                                                                color: "#505156"
                                                                border.width:2
                                                                border.color:  selected ? parent.selectedColor: parent.defaultColor
                                                                anchors.top:boxName.bottom
                                                                anchors.topMargin:8* screenScaleFactor
                                                                anchors.horizontalCenter:parent.horizontalCenter
                                                                Rectangle{
                                                                    visible:+model.state!==0&&model.materialType
                                                                    width: ((+percent/100)*40* screenScaleFactor)+13* screenScaleFactor
                                                                    height:width
                                                                    color: model.materialColor
                                                                    anchors.centerIn:parent
                                                                    radius:height/2
                                                                    Rectangle{
                                                                        anchors.centerIn:parent
                                                                        width:13* screenScaleFactor
                                                                        height:13* screenScaleFactor
                                                                        radius:height/2
                                                                        color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                                                    }
                                                                }
                                                                property var mColor:  root.setTextColor(materialColor)
                                                                Rectangle{
                                                                    width:8* screenScaleFactor
                                                                    height:1
                                                                    color:parent.mColor
                                                                    anchors.left:parent.left
                                                                    anchors.leftMargin:4* screenScaleFactor
                                                                    anchors.top:parent.top
                                                                    anchors.topMargin:parent.height/2
                                                                }
                                                                Rectangle{
                                                                    width:8* screenScaleFactor
                                                                    height:1
                                                                    color:parent.mColor
                                                                    anchors.right:parent.right
                                                                    anchors.rightMargin:4* screenScaleFactor
                                                                    anchors.top:parent.top
                                                                    anchors.topMargin:parent.height/2
                                                                }
                                                                Rectangle{
                                                                    width:1
                                                                    height:8* screenScaleFactor
                                                                    color:parent.mColor
                                                                    anchors.left:parent.left
                                                                    anchors.leftMargin:parent.width/2
                                                                    anchors.top:parent.top
                                                                    anchors.topMargin:4* screenScaleFactor
                                                                }
                                                                Rectangle{
                                                                    width:1
                                                                    height:8* screenScaleFactor
                                                                    color:parent.mColor
                                                                    anchors.left:parent.left
                                                                    anchors.leftMargin:parent.width/2
                                                                    anchors.bottom:parent.bottom
                                                                    anchors.bottomMargin:4* screenScaleFactor
                                                                }

                                                                Text {
                                                                    visible:+model.state===0
                                                                    font.weight: Font.Bold
                                                                    font.family: Constants.mySystemFont.name
                                                                    font.pointSize: Constants.labelFontPointSize_12
                                                                    color: sourceTheme.text_normal_color
                                                                    text: qsTr("/")
                                                                    anchors.centerIn:parent
                                                                }

                                                                Text {
                                                                    visible:+model.state===1&&!model.materialType
                                                                    font.weight: Font.Bold
                                                                    font.family: Constants.mySystemFont.name
                                                                    font.pointSize: Constants.labelFontPointSize_12
                                                                    color: sourceTheme.text_normal_color
                                                                    text: qsTr("?")
                                                                    anchors.centerIn:parent
                                                                }

                                                            }
                                                            Canvas {
                                                                anchors.top: materialBox.bottom
                                                                anchors.topMargin: 4 * screenScaleFactor
                                                                anchors.horizontalCenter:parent.horizontalCenter
                                                                visible: model.used
                                                                width: 14 * screenScaleFactor
                                                                height: 8 * screenScaleFactor

                                                                contextType: "2d"

                                                                onPaint: {
                                                                    const context = getContext("2d")
                                                                    context.clearRect(0, 0, width, height);
                                                                    context.beginPath();
                                                                    if (!root.down) {
                                                                        context.moveTo(0, 0);
                                                                        context.lineTo(width, 0);
                                                                        context.lineTo(width / 2, height);
                                                                    } else {
                                                                        context.moveTo(0, height);
                                                                        context.lineTo(width, height);
                                                                        context.lineTo(width / 2, 0);
                                                                    }
                                                                    context.closePath();
                                                                    context.fillStyle = materialColor
                                                                    context.fill();
                                                                }
                                                            }
                                                            Text {
                                                                id:materialName
                                                                font.weight: Font.Bold
                                                                font.family: Constants.mySystemFont.name
                                                                font.pointSize: Constants.labelFontPointSize_9
                                                                horizontalAlignment: Text.AlignHCenter
                                                                anchors.top:materialBox.bottom
                                                                anchors.topMargin:18* screenScaleFactor
                                                                color:   selected ? parent.selectedColor: parent.defaultColor
                                                                text: materialType?materialType:"--"
                                                                anchors.horizontalCenter:parent.horizontalCenter
                                                            }
                                                            Image{
                                                                visible:+model.state == 2
                                                                anchors.top:materialName.bottom
                                                                anchors.topMargin:10* screenScaleFactor
                                                                width:21* screenScaleFactor
                                                                height:16* screenScaleFactor
                                                                source: Constants.currentTheme?showBoxMsg_light: root.showBoxMsg
                                                                sourceSize:Qt.size(width,height)
                                                                anchors.horizontalCenter:parent.horizontalCenter
                                                                MouseArea{
                                                                    anchors.fill:parent
                                                                    cursorShape:Qt.PointingHandCursor
                                                                    onClicked: {
                                                                        boxMap.materialItem={
                                                                            materialBoxId,
                                                                            materialId,
                                                                            materialType,
                                                                            materialColor,
                                                                            name,
                                                                            vendor,
                                                                            minTemp,
                                                                            maxTemp,
                                                                            pressure,
                                                                            rfid,
                                                                            state:model.state
                                                                        }
                                                                        materialMsg.visible = true
                                                                    }
                                                                }
                                                            }

                                                            Image{
                                                                visible:+model.state!== 2
                                                                enabled:+model.state !== 0
                                                                anchors.top:materialName.bottom
                                                                anchors.topMargin:5* screenScaleFactor
                                                                width:21* screenScaleFactor
                                                                height:16* screenScaleFactor
                                                                source:enabled? Constants.currentTheme? editBoxMsg_light: root.editBoxMsg :editBoxMsg_disable
                                                                sourceSize:Qt.size(width,height)
                                                                anchors.horizontalCenter:parent.horizontalCenter
                                                                MouseArea{
                                                                    anchors.fill:parent
                                                                    cursorShape:Qt.PointingHandCursor
                                                                    onClicked: {
                                                                        boxMap.materialItem={
                                                                            materialBoxId,
                                                                            materialId,
                                                                            materialType,
                                                                            materialColor,
                                                                            name,
                                                                            vendor,
                                                                            minTemp,
                                                                            maxTemp,
                                                                            pressure,
                                                                            rfid,
                                                                            state:model.state
                                                                        }
                                                                        materialMsg.visible = true
                                                                    }
                                                                }
                                                            }

                                                        }
                                                    }
                                                }
                                            }

                                            RowLayout{
                                                id:humidityId
                                                spacing:10 * screenScaleFactor
                                                Layout.alignment:Qt.AlignRight
                                                property var humidity:+deviceItem.materialBoxList.humidity
                                                property var humidityImg
                                                property var humidityTextColor
                                                onVisibleChanged:{
                                                    if(humidity>=0&&humidity<39) {
                                                        humidityImg = humidity_normal
                                                        humidityTextColor = Constants.themeGreenColor
                                                    }
                                                    if(humidity>=40&&humidity<59) {
                                                        humidityImg = humidity_height
                                                        humidityTextColor = "#FF8D23"
                                                    }
                                                    if(humidity>=59&&humidity<=100) {
                                                        humidityImg = humidity_heightest
                                                        humidityTextColor = "#FE5A51"

                                                    }
                                                }
                                                Rectangle{
                                                    Layout.preferredHeight:22 * screenScaleFactor
                                                    Layout.preferredWidth: 44 * screenScaleFactor
                                                    radius:5
                                                    color: Constants.currentTheme? "#D6D6DC": "#515157"
                                                    visible: !!+deviceItem.materialBoxList.autoRefill
                                                    Text{
                                                        anchors.centerIn:parent
                                                        text:"AUTO"
                                                        color: Constants.themeGreenColor
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_10
                                                    }
                                                    MouseArea{
                                                        anchors.fill:parent
                                                        onClicked:material_auto_refill_panel.visible=true
                                                    }
                                                }
                                                CusRoundedBg{
                                                    Layout.preferredHeight:22 * screenScaleFactor
                                                    Layout.preferredWidth: 44 * screenScaleFactor
                                                    rightBottom:true
                                                    color:Constants.currentTheme? "#D6D6DC": "#515157"
                                                    radius: 5* screenScaleFactor
                                                    Layout.alignment:Qt.AlignRight
                                                    RowLayout{
                                                        Layout.fillWidth:true
                                                        Layout.fillHeight:true

                                                        Image{
                                                            width:11* screenScaleFactor
                                                            height:14* screenScaleFactor
                                                            source:humidityId.humidityImg
                                                            Layout.leftMargin:8* screenScaleFactor
                                                            Layout.topMargin: 3* screenScaleFactor
                                                        }
                                                        Text{
                                                            text: humidityId.humidity
                                                            color: humidityId.humidityTextColor
                                                            font.weight: Font.Bold
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_8
                                                            Layout.topMargin: 3* screenScaleFactor
                                                        }
                                                    }
                                                    MouseArea{
                                                        anchors.fill:parent
                                                        onClicked:humidityMsg.visible=true
                                                    }
                                                }
                                            }


                                        }
                                        Rectangle{
                                            visible: +deviceItem.cfsConnect===0
                                            anchors.fill:parent
                                            color: "transparent"
                                            Image{
                                                id: printerImg
                                                width:159*screenScaleFactor
                                                height:147*screenScaleFactor
                                                source:noBoxPrinter
                                                anchors.left:parent.left
                                                anchors.leftMargin:27*screenScaleFactor
                                                anchors.top:parent.top
                                                anchors.topMargin:28*screenScaleFactor

                                            }
                                            Text {
                                                anchors.left:printerImg.right
                                                anchors.leftMargin:-10 * screenScaleFactor
                                                anchors.top:parent.top
                                                anchors.topMargin:64*screenScaleFactor
                                                height:66*screenScaleFactor
                                                width:163*screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: "#999999"
                                                text: qsTr("The Creative Consumables System (CFS) allows you to easily manage consumables and perform functions such as automatic refilling, multi-color printing, and support printing.")
                                                wrapMode: Text.Wrap
                                            }
                                        }

                                    }
                                    ColumnLayout{
                                        anchors.left:boxMap.right
                                        anchors.leftMargin:10 * screenScaleFactor
                                        anchors.top:parent.top
                                        anchors.topMargin:10* screenScaleFactor
                                        spacing:20 * screenScaleFactor
                                        visible:deviceItem.materialBoxList.rowCount()>4
                                        Repeater{
                                            id:boxGroup
                                            model:deviceItem.materialBoxList.rowCount()/4
                                            delegate:ColumnLayout{
                                                readonly property var modelItem: model
                                                spacing:4 * screenScaleFactor

                                                Rectangle{
                                                    width:62 * screenScaleFactor
                                                    height:24 * screenScaleFactor
                                                    color: Constants.currentTheme?"#FFFFFF" :"#37373B"
                                                    radius:5 * screenScaleFactor
                                                    border.width: 1 * screenScaleFactor
                                                    property var hoverColor: Constants.currentTheme?"#999999":"#ffffff"
                                                    property var defaultColor: Constants.currentTheme? "#D6D6DC": "#515157"
                                                    border.color: boxMap.curGroup===modelItem.index? Constants.themeGreenColor: isHover? hoverColor :defaultColor
                                                    property bool isHover:false
                                                    MouseArea{
                                                        anchors.fill:parent
                                                        hoverEnabled:true
                                                        onEntered:parent.isHover = true
                                                        onExited:parent.isHover = false
                                                        onClicked:boxMap.curGroup = modelItem.index
                                                    }
                                                    RowLayout{
                                                        anchors.fill:parent
                                                        spacing: 3 * screenScaleFactor
                                                        anchors.left:parent.left
                                                        anchors.leftMargin:3* screenScaleFactor
                                                        anchors.right:parnet.right
                                                        anchors.rightMargin:3* screenScaleFactor
                                                        Repeater{
                                                            model:deviceItem.materialBoxList
                                                            delegate:Rectangle{
                                                                width:10 * screenScaleFactor
                                                                height:width
                                                                radius:width/2
                                                                color:model.materialColor
                                                                visible: model.index+1>(modelItem.index)*4 && (model.index+1)<=(modelItem.index+1)*4
                                                                Text{
                                                                    text:{
                                                                        if(+model.state === 0) return "/"
                                                                        if(+model.state === 1){
                                                                            if(!model.materialType) return "?"
                                                                        }
                                                                        return ""
                                                                    }
                                                                    visible:+model.state!==2
                                                                    color:sourceTheme.text_normal_color
                                                                    font.weight: Font.Bold
                                                                    anchors.centerIn:parent
                                                                }
                                                                Canvas {
                                                                    anchors.bottom: parent.top
                                                                    anchors.bottomMargin: 10 * screenScaleFactor
                                                                    visible:model.used
                                                                    width: 11 * screenScaleFactor
                                                                    height: 6 * screenScaleFactor
                                                                    contextType: "2d"
                                                                    onPaint: {
                                                                        const context = getContext("2d")
                                                                        context.clearRect(0, 0, width, height);
                                                                        context.beginPath();
                                                                        if (!root.down) {
                                                                            context.moveTo(0, 0);
                                                                            context.lineTo(width, 0);
                                                                            context.lineTo(width / 2, height);
                                                                        } else {
                                                                            context.moveTo(0, height);
                                                                            context.lineTo(width, height);
                                                                            context.lineTo(width / 2, 0);
                                                                        }
                                                                        context.closePath();
                                                                        context.fillStyle = parent.color
                                                                        context.fill();
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
                                RowLayout{

                                    anchors.top:boxList.bottom
                                    anchors.topMargin:10* screenScaleFactor
                                    RowLayout{
                                        spacing:5 * screenScaleFactor
                                        Layout.alignment:Qt.AlignTop
                                        Rectangle{
                                            width:10 * screenScaleFactor
                                            height:width
                                            color: Constants.themeGreenColor
                                            radius:height/2
                                        }
                                        Text {
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: feedState[deviceItem.feedState]
                                        }
                                    }
                                    Item{
                                        Layout.fillWidth:true
                                    }
                                }
                                RowLayout{
                                    spacing:10 * screenScaleFactor
                                    Item{
                                        Layout.fillWidth: true
                                    }

                                    CusImglButton{
                                        Layout.preferredWidth: 48 * screenScaleFactor
                                        Layout.preferredHeight: 32 * screenScaleFactor
                                        imgWidth: 26 * screenScaleFactor
                                        imgHeight: 24 * screenScaleFactor
                                        defaultBtnBgColor: "transparent"
                                        hoveredBtnBgColor: "transparent"
                                        selectedBtnBgColor: "transparent"
                                        opacity: 1
                                        borderWidth: 0
                                        state: "imgOnly"
                                        enabledIconSource: root.materialBoxSetting
                                        pressedIconSource:root.materialBoxSetting
                                        highLightIconSource: root.materialBoxSetting
                                        onClicked:boxMsg.visible = true
                                        visible:+deviceItem.cfsConnect===1

                                    }
                                    BasicDialogButton {
                                        Layout.preferredWidth: 80*screenScaleFactor
                                        Layout.preferredHeight: 32*screenScaleFactor
                                        text: qsTr("Guide")
                                        btnRadius:5*screenScaleFactor
                                        btnBorderW:0
                                        defaultBtnBgColor:Constants.currentTheme?"#ffffff":"#515157"
                                        hoveredBtnBgColor: Qt.lighter(defaultBtnBgColor,1.2)
                                        onSigButtonClicked:deviceItem.materialBoxList.toMultiColorGuide()
                                        visible:+deviceItem.cfsConnect===1
                                    }
                                    BasicDialogButton {
                                        id:feedId
                                        Layout.preferredWidth: 80*screenScaleFactor
                                        Layout.preferredHeight: 32*screenScaleFactor
                                        text: qsTr("Feed")
                                        btnRadius:5*screenScaleFactor
                                        btnBorderW:0
                                        enabled:deviceItem.feedState==0&&!root.isPrinting && deviceItem.materialBoxList.materialState !== 0
                                        defaultBtnBgColor: Constants.themeGreenColor
                                        hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                        onSigButtonClicked:{
                                            deviceItem.materialBoxList.feedInOrOutMaterial(deviceItem.pcIpAddr,boxMap.curBoxId,boxMap.curMaterialId,"1")
                                        }

                                    }
                                    BasicDialogButton {
                                        id:retractId
                                        Layout.preferredWidth: 80*screenScaleFactor
                                        Layout.preferredHeight: 32*screenScaleFactor
                                        text: qsTr("Retract")
                                        enabled:deviceItem.feedState==0&&!root.isPrinting && deviceItem.materialBoxList.materialState !== 0
                                        btnRadius:5*screenScaleFactor
                                        btnBorderW:0
                                        defaultBtnBgColor: Constants.themeGreenColor
                                        hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                        onSigButtonClicked:{
                                            deviceItem.materialBoxList.feedInOrOutMaterial(deviceItem.pcIpAddr,boxMap.curBoxId,boxMap.curMaterialId,"0")
                                        }

                                    }
                                }

                            }

                            Rectangle{
                                id:materialMsg
                                anchors.fill:parent
                                color:sourceTheme.popup_background_color
                                visible:false
                                property var materialItem:null
                                property var materialItemOriginalValue:null//Save the original modified values to verify if the remote has been modified during the save process
                                property var materialChangeData:{}
                                property int resetBrandComboxIndex
                                property int resetTypeComboxIndex
                                property int resetNameComboxIndex
                                property bool editEnable:!root.isPrinting&&materialMsg.materialItem&&+materialMsg.materialItem.state === 1
                                onVisibleChanged:{
                                    if(visible){
                                        materialMsg.materialItem = boxMap.materialItem
                                        materialMsg.materialChangeData = JSON.parse(JSON.stringify(boxMap.materialItem))
                                        var colorString = boxMap.materialItem.materialColor;
                                        materialColorId.color = colorString
                                        materialMsg.materialChangeData.materialColor = colorString

                                        materialMsg.materialItemOriginalValue = JSON.parse(JSON.stringify(boxMap.materialItem))
                                        materialMsg.materialItemOriginalValue.materialColor = colorString

                                        let brandIndex=""
                                        let typeIndex=""
                                        let nameIndex=""

                                        if(materialMsg.editEnable){
                                            let vendor = deviceItem.materialLinkageModel? deviceItem.materialLinkageModel.getBrands():kernel_parameter_manager.materialLinkageModel.getBrands()
                                            brandCombox.model = vendor
                                            if(materialMsg.materialItem.vendor){
                                                brandIndex = vendor.indexOf(materialMsg.materialItem.vendor)
                                            }else{
                                                brandIndex= 0
                                            }

                                            if(boxMap.materialItem.state == 1 && !boxMap.materialItem.materialType){
                                                brandIndex = -1
                                            }
                                            brandCombox.currentIndex = brandIndex
                                            materialMsg.resetBrandComboxIndex = brandIndex
                                            let type = deviceItem.materialLinkageModel? deviceItem.materialLinkageModel.getTypes(brandCombox.model[brandCombox.currentIndex]):kernel_parameter_manager.materialLinkageModel.getTypes(brandCombox.model[brandCombox.currentIndex])
                                            typeCombox.model = type
                                            if(materialMsg.materialItem.materialType){
                                                typeIndex= type.indexOf(materialMsg.materialItem.materialType)
                                            }else {
                                                typeIndex= 0
                                            }
                                            if(boxMap.materialItem.state == 1 && !boxMap.materialItem.materialType){
                                                typeIndex = -1
                                            }
                                            typeCombox.currentIndex  = typeIndex
                                            materialMsg.resetTypeComboxIndex = typeIndex

                                            nameCombox.currentIndex = deviceItem.materialLinkageModel.materialIndex(materialMsg.materialItem.name)
                                            if(boxMap.materialItem.state == 1 && !boxMap.materialItem.materialType){
                                                nameCombox.currentIndex = -1
                                                materialColorId.color = "transparent";
                                            }
                                            materialMsg.resetNameComboxIndex = nameCombox.currentIndex

                                        }
                                        maxTempId.text = materialMsg.materialItem.maxTemp
                                        minTempId.text = materialMsg.materialItem.minTemp
                                        pressureId.text = materialMsg.materialItem.pressure

                                    }
                                }
                                MouseArea{
                                    anchors.fill:parent
                                    onClicked:{ return; }
                                }
                                Rectangle{
                                    width:524* screenScaleFactor
                                    height:255* screenScaleFactor
                                    color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                    radius:5* screenScaleFactor
                                    border.width:1
                                    border.color:Constants.currentTheme? "#D6D6DC": "#515157"
                                    anchors.left:parent.left
                                    anchors.leftMargin:60 * screenScaleFactor
                                    anchors.top:parent.top
                                    anchors.topMargin:35 * screenScaleFactor
                                    ColumnLayout{
                                        spacing:5 * screenScaleFactor
                                        anchors.fill:parent
                                        RowLayout{
                                            Layout.fillWidth:true
                                            Layout.topMargin:10 * screenScaleFactor
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("brand")

                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            Text {
                                                Layout.rightMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text:  materialMsg.materialItem.vendor || "--"
                                                visible:!materialMsg.editEnable
                                            }

                                            CustomComboBox {
                                                id:brandCombox
                                                Layout.rightMargin:20 * screenScaleFactor

                                                visible:materialMsg.editEnable
                                                Layout.preferredWidth: 200 * screenScaleFactor
                                                Layout.preferredHeight: 24 * screenScaleFactor
                                                displayText: {
                                                    currentIndex==-1?"--":translater ? translater(currentText) : currentText
                                                }
                                                model: deviceItem.materialLinkageModel? deviceItem.materialLinkageModel.getBrands():kernel_parameter_manager.materialLinkageModel.getBrands()
                                                currentIndex: 0
                                                onCurrentTextChanged:{
                                                    materialMsg.materialChangeData.vendor = currentText
                                                    typeCombox.model = deviceItem.materialLinkageModel?deviceItem.materialLinkageModel.getTypes(currentText):kernel_parameter_manager.materialLinkageModel.getTypes(currentText)

                                                    typeCombox.currentIndex = 0
                                                    if(deviceItem.materialLinkageModel)
                                                    {
                                                        deviceItem.materialLinkageModel.setMaterials(currentText,typeCombox.model[0])
                                                    }else{
                                                        nameCombox.model = kernel_parameter_manager.materialLinkageModel.getMaterials(currentText,typeCombox.model[0])
                                                    }

                                                    nameCombox.currentIndex = 0
                                                }
                                            }
                                        }
                                        RowLayout{
                                            Layout.fillWidth:true
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("type")
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            Text {
                                                Layout.rightMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text:  materialMsg.materialItem.materialType || "--"
                                                visible:!materialMsg.editEnable
                                            }
                                            CustomComboBox {
                                                Layout.rightMargin:20 * screenScaleFactor
                                                id:typeCombox
                                                displayText: {
                                                    currentIndex==-1?"--":translater ? translater(currentText) : currentText
                                                }
                                                visible:materialMsg.editEnable
                                                Layout.preferredWidth: 200 * screenScaleFactor
                                                Layout.preferredHeight: 24 * screenScaleFactor
                                                onCurrentTextChanged: {
                                                    materialMsg.materialChangeData.vendor = brandCombox.currentText
                                                    materialMsg.materialChangeData.materialType = currentText

                                                    if(deviceItem.materialLinkageModel)
                                                    {
                                                        deviceItem.materialLinkageModel.setMaterials(brandCombox.currentText,currentText)
                                                    }else{
                                                        nameCombox.model = kernel_parameter_manager.materialLinkageModel.getMaterials(brandCombox.currentText,currentText)
                                                    }
                                                    nameCombox.currentIndex = 0


                                                }
                                            }
                                        }
                                        RowLayout{
                                            Layout.fillWidth:true
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("name")
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            Text {
                                                Layout.rightMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text:  materialMsg.materialItem.name || "--"
                                                visible:!materialMsg.editEnable
                                            }
                                            CustomComboBox {
                                                id:nameCombox
                                                Layout.rightMargin:20 * screenScaleFactor
                                                textRole: "name"
                                                displayText: {
                                                    currentIndex==-1?"--":translater ? translater(currentText) : currentText
                                                }
                                                model: deviceItem.materialLinkageModel
                                                visible:materialMsg.editEnable
                                                Layout.preferredWidth: 200 * screenScaleFactor
                                                Layout.preferredHeight: 24 * screenScaleFactor
                                                onCurrentTextChanged:{
                                                    if(!visible)
                                                    {
                                                        return
                                                    }
                                                    materialMsg.materialChangeData.vendor = brandCombox.currentText
                                                    materialMsg.materialChangeData.typeCombox = currentText
                                                    materialMsg.materialChangeData.name = currentText
                                                    maxTempId.text = deviceItem.materialLinkageModel.getMaxTemp(currentIndex)
                                                    minTempId.text = deviceItem.materialLinkageModel.getMinTemp(currentIndex)
                                                    pressureId.text = deviceItem.materialLinkageModel.getPressAdv(currentIndex)
                                                    idErrorMsg.text = ""
                                                }
                                            }
                                        }
                                        RowLayout{
                                            Layout.fillWidth:true
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("color")
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            RowLayout{
                                                Layout.rightMargin:20 * screenScaleFactor
                                                Layout.preferredWidth: 60* screenScaleFactor
                                                MouseArea{
                                                    anchors.fill:parent
                                                    onClicked:{
                                                        if(!materialMsg.editEnable)return
                                                        colorMsg.curSelected = (""+ materialColorId.color).toUpperCase()
                                                        colorMsg.visible = true
                                                    }
                                                }
                                                Rectangle{
                                                    id:materialColorId
                                                    width:40* screenScaleFactor
                                                    height:20* screenScaleFactor
                                                    radius:height/2
                                                    color:materialMsg.materialItem.materialColor

                                                }
                                                Image{
                                                    visible:materialMsg.editEnable
                                                    source:Constants.currentTheme? editBoxMsg_light: root.editBoxMsg
                                                    width:21* screenScaleFactor
                                                    height:16* screenScaleFactor
                                                }

                                            }

                                        }

                                        RowLayout{
                                            Layout.fillWidth:true
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("Nozzle temperature")
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            Text {

                                                Layout.rightMargin:20 * screenScaleFactor
                                                visible:!materialMsg.editEnable
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: materialMsg.materialItem.minTemp+"~"+materialMsg.materialItem.maxTemp
                                            }
                                            RowLayout{
                                                id: idMaxNozzleTemp
                                                Layout.rightMargin:20 * screenScaleFactor
                                                visible:materialMsg.editEnable
                                                BasicDialogTextField{
                                                    id:minTempId
                                                    Layout.preferredWidth: 80* screenScaleFactor
                                                    Layout.preferredHeight: 24* screenScaleFactor
                                                    radius: 5* screenScaleFactor
                                                    text:materialMsg.materialItem.minTemp
                                                    unitChar: qsTr("°C")
                                                    onTextChanged:{
                                                        if(+text> +deviceItem.maxNozzleTemp)
                                                            text = deviceItem.maxNozzleTemp
                                                        materialMsg.materialChangeData.minTemp = text
                                                    }

                                                }
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    text: "~"
                                                }
                                                BasicDialogTextField{
                                                    id:maxTempId
                                                    Layout.preferredWidth: 80* screenScaleFactor
                                                    Layout.preferredHeight: 24* screenScaleFactor
                                                    radius: 5* screenScaleFactor
                                                    text:materialMsg.materialItem.maxTemp
                                                    unitChar:  qsTr("°C")
                                                    onTextChanged: {
                                                        if(+text> +deviceItem.maxNozzleTemp)
                                                            text = deviceItem.maxNozzleTemp
                                                        materialMsg.materialChangeData.maxTemp = text
                                                    }
                                                }
                                            }
                                        }

                                        RowLayout{
                                            Layout.fillWidth:true
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("PA distance")
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            Text {
                                                visible:!materialMsg.editEnable
                                                Layout.rightMargin:20 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text:  materialMsg.materialItem.pressure || "--"
                                            }
                                            BasicDialogTextField{
                                                id:pressureId
                                                Layout.rightMargin:20 * screenScaleFactor
                                                visible:materialMsg.editEnable
                                                Layout.preferredWidth: 50* screenScaleFactor
                                                Layout.preferredHeight: 20* screenScaleFactor
                                                radius: 5* screenScaleFactor
                                                text:materialMsg.materialItem.pressure
                                                validator: RegExpValidator {
                                                    regExp: /^(\d+(\.\d{0,3})?)?$/
                                                }
                                                onTextChanged: {
                                                    if(+text> 2) text = 2
                                                    materialMsg.materialChangeData.pressure = text
                                                }
                                            }
                                        }

                                        RowLayout{
                                            Layout.fillWidth:true
                                            Text {
                                                Layout.leftMargin:20 * screenScaleFactor
                                                visible:!materialMsg.editEnable
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: "#CBCBCC"
                                                text: +materialMsg.materialItem.state === 2?qsTr("RFID consumables information does not support modification"): qsTr("It does not support setting consumables information during printing.")
                                            }
                                            Text {
                                                id: idErrorMsg
                                                Layout.leftMargin:20 * screenScaleFactor
                                                visible:materialMsg.editEnable
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: "#FF0000"

                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            RowLayout{
                                                // Layout.preferredWidth:150*screenScaleFactor
                                                id : __row
                                                Layout.rightMargin:20 * screenScaleFactor
                                                BasicDialogButton {
                                                    Layout.preferredWidth: 50*screenScaleFactor
                                                    Layout.preferredHeight: 28*screenScaleFactor
                                                    text: qsTr("Back")
                                                    btnRadius:5*screenScaleFactor
                                                    btnBorderW:0
                                                    defaultBtnBgColor: Constants.themeGreenColor
                                                    hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                                    onSigButtonClicked: materialMsg.visible = false

                                                }
                                                BasicDialogButton {
                                                    Layout.preferredWidth: 50*screenScaleFactor
                                                    Layout.preferredHeight: 28*screenScaleFactor
                                                    text: qsTr("Reset")
                                                    visible:materialMsg.editEnable
                                                    btnRadius:5*screenScaleFactor
                                                    btnBorderW:0
                                                    defaultBtnBgColor: Constants.themeGreenColor
                                                    hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                                    onSigButtonClicked: {
                                                        //brandCombox.currentIndex = materialMsg.resetBrandComboxIndex
                                                        //typeCombox.currentIndex = materialMsg.resetTypeComboxIndex
                                                        //nameCombox.currentIndex = materialMsg.resetNameComboxIndex
                                                        brandCombox.currentIndex = -1
                                                        typeCombox.currentIndex = -1
                                                        nameCombox.currentIndex = -1
                                                        minTempId.text = boxMap.materialItem.minTemp
                                                        maxTempId.text = boxMap.materialItem.maxTemp
                                                        pressureId.text= boxMap.materialItem.pressure
                                                        materialColorId.color = "transparent";//boxMap.materialItem.materialColor
                                                    }

                                                }

                                                BasicMessageDialog
                                                {
                                                    id: confirmation_dialog
                                                    width: 550 * screenScaleFactor
                                                    height: 190 * screenScaleFactor

                                                    title: qsTr("Warning")
                                                    messageText: qsTr("The printer parameters have been modified. If you continue, it will overwrite the current parameters of the printer")
                                                    yesBtnText: qsTr("Continue")
                                                    noBtnText: qsTr("Cancel")

                                                    onAccept: {
                                                        __row.modifyMaterial()
                                                    }

                                                    onCancel: {

                                                    }
                                                }

                                                function modifyMaterial()
                                                {
                                                    if(materialMsg.editEnable){
                                                        let obj = {}

                                                        obj["boxId"] = +materialMsg.materialChangeData.materialBoxId
                                                        obj["id"] =  +materialMsg.materialChangeData.materialId
                                                        obj["vendor"] =  materialMsg.materialChangeData.vendor
                                                        obj["type"] =  materialMsg.materialChangeData.materialType
                                                        obj["name"] =  materialMsg.materialChangeData.name
                                                        obj["color"] =  materialMsg.materialChangeData.materialColor
                                                        obj["minTemp"] =  +materialMsg.materialChangeData.minTemp
                                                        obj["maxTemp"] = + materialMsg.materialChangeData.maxTemp
                                                        obj["pressure"] = +materialMsg.materialChangeData.pressure
                                                        obj["rfid"] =   materialMsg.materialChangeData.rfid

                                                        deviceItem.materialBoxList.modifyMaterial(deviceItem.pcIpAddr,obj)
                                                    }

                                                    idErrorMsg.text = "";
                                                    materialMsg.visible = false
                                                }


                                                BasicDialogButton {
                                                    Layout.preferredWidth: 50*screenScaleFactor
                                                    Layout.preferredHeight: 28*screenScaleFactor
                                                    text: qsTr("Confirm")
                                                    visible:materialMsg.editEnable
                                                    btnRadius:5*screenScaleFactor
                                                    btnBorderW:0
                                                    defaultBtnBgColor: Constants.themeGreenColor
                                                    hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                                    onSigButtonClicked: {
                                                        if(brandCombox.currentIndex == -1)
                                                        {
                                                            idErrorMsg.text = qsTr("Please choose a brand")
                                                            return;
                                                        }
                                                        if(typeCombox.currentText == -1)
                                                        {
                                                            idErrorMsg.text = qsTr("Please choose a type")
                                                            return;
                                                        }
                                                        if(nameCombox.currentText == -1)
                                                        {
                                                            idErrorMsg.text = qsTr("Please choose a name")
                                                            return;
                                                        }
                                                        if(materialColorId.color.toString()=="#00000000")
                                                        {
                                                            idErrorMsg.text = qsTr("Please choose a color")
                                                            return;
                                                        }
                                                        if(+materialMsg.materialChangeData.maxTemp<=+materialMsg.materialChangeData.minTemp)
                                                        {
                                                            idErrorMsg.text = qsTr("temperature range invalide,please reset")
                                                            return;
                                                        }

                                                        if(materialMsg.editEnable){
                                                            let obj_ori = {}//
                                                            obj_ori["boxId"] = +materialMsg.materialItemOriginalValue.materialBoxId
                                                            obj_ori["id"] =  +materialMsg.materialItemOriginalValue.materialId
                                                            obj_ori["vendor"] =  materialMsg.materialItemOriginalValue.vendor
                                                            obj_ori["type"] =  materialMsg.materialItemOriginalValue.materialType
                                                            obj_ori["name"] =  materialMsg.materialItemOriginalValue.name
                                                            obj_ori["color"] =  materialMsg.materialItemOriginalValue.materialColor
                                                            obj_ori["minTemp"] =  +materialMsg.materialItemOriginalValue.minTemp
                                                            obj_ori["maxTemp"] = + materialMsg.materialItemOriginalValue.maxTemp
                                                            obj_ori["pressure"] = +materialMsg.materialItemOriginalValue.pressure
                                                            obj_ori["rfid"] =   materialMsg.materialItemOriginalValue.rfid

                                                            if(deviceItem.materialBoxList.isRemoteModifyMaterial(deviceItem.pcIpAddr, obj_ori))
                                                            {
                                                                confirmation_dialog.showDoubleBtn()
                                                                confirmation_dialog.visible = true
                                                            }
                                                            else
                                                            {
                                                                __row.modifyMaterial()
                                                            }
                                                        }
                                                    }

                                                }

                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle{
                                id:boxMsg
                                anchors.fill:parent
                                color:sourceTheme.popup_background_color
                                visible:false
                                property var autoRefill:false
                                property var cAutoFeed:false
                                property var cSelfTest:false
                                property var cAutoUpdateFilament:false
                                Rectangle{
                                    width:524* screenScaleFactor
                                    height:255* screenScaleFactor
                                    color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                    radius:5* screenScaleFactor
                                    border.width:1
                                    border.color: Constants.currentTheme? "#D6D6DC": "#515157"
                                    anchors.left:parent.left
                                    anchors.leftMargin:60 * screenScaleFactor
                                    anchors.top:parent.top
                                    anchors.topMargin:35 * screenScaleFactor
                                    ColumnLayout{
                                        spacing:8 * screenScaleFactor
                                        anchors.fill:parent
                                        ColumnLayout{

                                            Layout.leftMargin:20 * screenScaleFactor
                                            Layout.rightMargin:20 * screenScaleFactor
                                            Layout.topMargin:10 * screenScaleFactor
                                            Text {
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("Material box setting")
                                            }
                                            Rectangle{
                                                Layout.fillWidth:true
                                                Layout.preferredHeight:1
                                                color:Constants.currentTheme? "#D6D6DC": "#515157"
                                            }

                                        }
                                        RowLayout{
                                            Layout.fillWidth:true
                                            ColumnLayout{
                                                Layout.leftMargin:20 * screenScaleFactor
                                                Layout.rightMargin:20 * screenScaleFactor
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    text: qsTr("Detection on insertion")
                                                }

                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    Layout.preferredWidth:420 * screenScaleFactor
                                                    wrapMode:Text.WordWrap
                                                    color: "#999999"
                                                    text: qsTr("When a new material line is inserted, the system automatically reads the material line information, which takes about 20 seconds.")
                                                }
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }

                                            CusSwitchBtn{
                                                id: idAutoFeed
                                                Layout.alignment:Qt.AlignRight
                                                Layout.rightMargin:20*screenScaleFactor
                                                Layout.preferredWidth: 50*screenScaleFactor
                                                Layout.preferredHeight: 28*screenScaleFactor
                                                btnHeight: 22*screenScaleFactor
                                                Layout.bottomMargin: 8*screenScaleFactor
                                                onClicked:
                                                {
                                                    boxMsg.cAutoFeed = checked
                                                    deviceItem.materialBoxList.boxConfig(deviceItem.pcIpAddr,+boxMsg.autoRefill,+boxMsg.cAutoFeed,+boxMsg.cSelfTest)
                                                }
                                                onVisibleChanged: {
                                                    if(visible) checked = !!+deviceItem.materialBoxList.cAutoFeed
                                                    boxMsg.cAutoFeed = checked
                                                    console.log(deviceItem.materialBoxList.cAutoFeed,"cAutoFeed")
                                                }
                                                Connections{
                                                    target:deviceItem.materialBoxList
                                                    onCAutoFeedChanged:{
                                                        idAutoFeed.checked = !!+deviceItem.materialBoxList.cAutoFeed
                                                        boxMsg.cAutoFeed = idAutoFeed.checked
                                                    }
                                                }
                                            }

                                        }
                                        RowLayout{
                                            Layout.fillWidth:true
                                            ColumnLayout{
                                                Layout.leftMargin:20 * screenScaleFactor
                                                Layout.rightMargin:20 * screenScaleFactor
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    text: qsTr("Detection at startup")
                                                }

                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color:"#999999"
                                                    Layout.preferredWidth:420 * screenScaleFactor
                                                    wrapMode:Text.WordWrap
                                                    text: qsTr("Each time the machine is turned on, the system automatically reads the material line information, which takes about 1 minute.")
                                                }
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }

                                            CusSwitchBtn{
                                                id:idSelfTest
                                                Layout.alignment:Qt.AlignRight
                                                Layout.rightMargin:20*screenScaleFactor
                                                Layout.preferredWidth: 50*screenScaleFactor
                                                Layout.preferredHeight: 28*screenScaleFactor
                                                btnHeight: 22*screenScaleFactor
                                                Layout.bottomMargin: 8*screenScaleFactor
                                                onClicked:{
                                                    boxMsg.cSelfTest = checked
                                                    deviceItem.materialBoxList.boxConfig(deviceItem.pcIpAddr,+boxMsg.autoRefill,+boxMsg.cAutoFeed,+boxMsg.cSelfTest)
                                                }
                                                onVisibleChanged: {
                                                    if(visible) checked = !!+deviceItem.materialBoxList.cSelfTest
                                                    boxMsg.cSelfTest = checked
                                                }
                                                Connections{
                                                    target:deviceItem.materialBoxList
                                                    onCSelfTestChanged:{
                                                        idSelfTest.checked = !!+deviceItem.materialBoxList.cSelfTest
                                                        boxMsg.cSelfTest = idSelfTest.checked
                                                    }
                                                }
                                            }

                                        }
                                        RowLayout{
                                            Layout.fillWidth:true
                                            ColumnLayout{
                                                Layout.leftMargin:20 * screenScaleFactor
                                                Layout.rightMargin:20 * screenScaleFactor
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    text: qsTr("Automatic feeding system")
                                                }

                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    Layout.preferredWidth:420 * screenScaleFactor
                                                    wrapMode:Text.WordWrap
                                                    color:"#999999"
                                                    text: qsTr("When the material line is exhausted, it will automatically switch to a consumable with exactly the same properties.")
                                                }
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }

                                            CusSwitchBtn{
                                                id: idAutoRefill
                                                Layout.alignment:Qt.AlignRight
                                                Layout.rightMargin:20*screenScaleFactor
                                                Layout.preferredWidth: 50*screenScaleFactor
                                                Layout.preferredHeight: 28*screenScaleFactor
                                                btnHeight: 22*screenScaleFactor
                                                Layout.bottomMargin: 8*screenScaleFactor
                                                onClicked: {
                                                    boxMsg.autoRefill = checked
                                                    deviceItem.materialBoxList.boxConfig(deviceItem.pcIpAddr,+boxMsg.autoRefill,+boxMsg.cAutoFeed,+boxMsg.cSelfTest)
                                                    }
                                                 onVisibleChanged: {
                                                    if(visible) checked = !!+deviceItem.materialBoxList.autoRefill
                                                    boxMsg.autoRefill = checked
                                                }
                                                Connections{
                                                    target:deviceItem.materialBoxList
                                                    onAutoRefillChanged:{
                                                        idAutoRefill.checked = !!+deviceItem.materialBoxList.autoRefill
                                                        boxMsg.autoRefill = idAutoRefill.checked
                                                    }
                                                }
                                            }

                                        }
                                        BasicDialogButton {
                                            Layout.alignment:Qt.AlignRight
                                            Layout.rightMargin:20*screenScaleFactor
                                            Layout.bottomMargin:10*screenScaleFactor
                                            Layout.preferredWidth: 80*screenScaleFactor
                                            Layout.preferredHeight: 32*screenScaleFactor
                                            text: qsTr("Back")
                                            btnRadius:5*screenScaleFactor
                                            btnBorderW:0
                                            defaultBtnBgColor: Constants.themeGreenColor
                                            hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                            onSigButtonClicked:{
                                                console.log("boxMsg.cAutoFeed"+boxMsg.cAutoFeed)
                                                //deviceItem.materialBoxList.boxConfig(deviceItem.pcIpAddr,+boxMsg.autoRefill,+boxMsg.cAutoFeed,+boxMsg.cSelfTest)
                                                boxMsg.visible = false
                                            }

                                        }
                                    }


                                }

                            }

                            Rectangle{
                                id:humidityMsg
                                anchors.fill:parent
                                color:sourceTheme.popup_background_color
                                visible:false
                                property var autoRefill:false
                                property var cAutoFeed:false
                                property var cSelfTest:false
                                property var cAutoUpdateFilament:false
                                Rectangle{
                                    width:524* screenScaleFactor
                                    height:255* screenScaleFactor
                                    color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                    radius:5* screenScaleFactor
                                    border.width:1
                                    border.color: Constants.currentTheme? "#D6D6DC": "#515157"
                                    anchors.left:parent.left
                                    anchors.leftMargin:60 * screenScaleFactor
                                    anchors.top:parent.top
                                    anchors.topMargin:35 * screenScaleFactor
                                    ColumnLayout{
                                        anchors.fill:parent

                                        Image{
                                            Layout.preferredHeight:10 * screenScaleFactor
                                            Layout.preferredWidth:10 * screenScaleFactor
                                            source:sourceTheme.img_closeBtn

                                            anchors.right:parent.right
                                            anchors.rightMargin:10 * screenScaleFactor
                                            anchors.top:parent.top
                                            anchors.topMargin:10 * screenScaleFactor
                                            MouseArea{
                                                anchors.fill:parent
                                                onClicked:humidityMsg.visible = false
                                            }
                                        }
                                        RowLayout{
                                            Layout.leftMargin:20 * screenScaleFactor
                                            Layout.rightMargin:20 * screenScaleFactor
                                            ColumnLayout{
                                                Layout.preferredHeight:30 * screenScaleFactor
                                                spacing:2
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: sourceTheme.text_normal_color
                                                    text: qsTr("CFS Humidity")
                                                }
                                                RowLayout{
                                                    Text {
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: "#999999"
                                                        text: qsTr("Scope")+"（"+qsTr("unit")+":"
                                                    }
                                                    ColumnLayout{
                                                        spacing:0
                                                        Text {
                                                            color: "#999999"
                                                            font.weight: Font.Bold
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            text: "%"
                                                        }
                                                        Text {
                                                            color: "#999999"
                                                            font.weight: Font.Bold
                                                            font.family: Constants.mySystemFont.name
                                                            font.pointSize: Constants.labelFontPointSize_9
                                                            text: "RH"
                                                        }
                                                    }
                                                    Text {
                                                        color: "#999999"
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        text: ")"
                                                    }
                                                }
                                            }
                                            Item{
                                                Layout.fillWidth:true
                                            }
                                            RowLayout{
                                                Layout.rightMargin:45* screenScaleFactor
                                                property var humidity:+deviceItem.materialBoxList.humidity
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    color: humidityId.humidityTextColor
                                                    text: qsTr("current")
                                                }
                                                Image{
                                                    width:11* screenScaleFactor
                                                    height:14* screenScaleFactor
                                                    source:humidityId.humidityImg
                                                    Layout.leftMargin:8* screenScaleFactor
                                                    Layout.topMargin: 3* screenScaleFactor
                                                }
                                                Text{
                                                    text: humidityId.humidity
                                                    color: humidityId.humidityTextColor
                                                    font.weight: Font.ExtraBold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_11
                                                    Layout.topMargin: 3* screenScaleFactor
                                                }
                                                Column{
                                                    Text {
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: humidityId.humidityTextColor
                                                        text: "%"
                                                    }
                                                    Text {
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        color: humidityId.humidityTextColor
                                                        text: "RH"
                                                    }
                                                }
                                            }
                                        }
                                        RowLayout{
                                            Text {
                                                Layout.leftMargin:64 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("normal")
                                            }
                                            Text {
                                                Layout.leftMargin:154 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("high")
                                            }
                                            Text {
                                                Layout.leftMargin:154 * screenScaleFactor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                color: sourceTheme.text_normal_color
                                                text: qsTr("Too high")
                                            }
                                        }
                                        Image{
                                            Layout.leftMargin:40 * screenScaleFactor
                                            Layout.bottomMargin:50 * screenScaleFactor
                                            Layout.preferredHeight:80 * screenScaleFactor
                                            Layout.preferredWidth:439 * screenScaleFactor
                                            source:humidity
                                        }
                                    }
                                }

                            }

                            Rectangle{
                                id:colorMsg
                                anchors.fill:parent
                                color:sourceTheme.popup_background_color
                                visible:false
                                property var selected:"#ffffff"
                                property var curSelected:"#ffffff"

                                Rectangle{
                                    width:524* screenScaleFactor
                                    height:255* screenScaleFactor
                                    color:  Constants.currentTheme?"#FFFFFF" :"#37373B"
                                    radius:5* screenScaleFactor
                                    border.width:1
                                    border.color:Constants.currentTheme? "#D6D6DC": "#515157"
                                    anchors.left:parent.left
                                    anchors.leftMargin:60 * screenScaleFactor
                                    anchors.top:parent.top
                                    anchors.topMargin:35 * screenScaleFactor
                                    ColumnLayout{
                                        anchors.fill:parent
                                        Image{
                                            Layout.preferredHeight:10 * screenScaleFactor
                                            Layout.preferredWidth:10 * screenScaleFactor
                                            source:sourceTheme.img_closeBtn

                                            anchors.right:parent.right
                                            anchors.rightMargin:10 * screenScaleFactor
                                            anchors.top:parent.top
                                            anchors.topMargin:10 * screenScaleFactor
                                            MouseArea{
                                                anchors.fill:parent
                                                onClicked:colorMsg.visible = false
                                            }
                                        }
                                        Text {
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: qsTr("color")
                                            anchors.left:parent.left
                                            anchors.leftMargin:10 * screenScaleFactor
                                            anchors.top:parent.top
                                            anchors.topMargin:10 * screenScaleFactor
                                        }
                                        Grid{
                                            columns:6
                                            spacing:10 * screenScaleFactor
                                            Layout.preferredWidth:300 * screenScaleFactor
                                            Layout.leftMargin:135 * screenScaleFactor
                                            anchors.top:parent.top
                                            anchors.topMargin:10
                                            // Layout.preferredHeight:200 * screenScaleFactor
                                            Repeater{
                                                model:[
                                                    "#F4E076","#FF614B","#6C84FF","#9CFF4F","#BA552A","#F8441D",
                                                    "#FFFFFF","#9EA7AE","#CE58F8","#26EEEE","#66FBA2","#FC9DA9",
                                                    "#FF8B1F","#1B04AE","#000000","#7A92AC","#00A3FF","#72A530",
                                                    "#3DDF57","#FF37AF","#EBFD1B","#B2A1E1","#8A43FF","#FFA800"]
                                                delegate:Rectangle{
                                                    width:40 * screenScaleFactor
                                                    height:40 * screenScaleFactor
                                                    radius:height/2
                                                    border.width:2
                                                    border.color: colorMsg.curSelected === modelData ? Constants.themeGreenColor : "transparent"
                                                    color:"transparent"
                                                    Rectangle{
                                                        anchors.centerIn:parent
                                                        width:30 * screenScaleFactor
                                                        height:30 * screenScaleFactor
                                                        radius:height/2
                                                        color:modelData
                                                    }
                                                    Image{
                                                        anchors.centerIn:parent
                                                        width: 14* screenScaleFactor
                                                        height:10* screenScaleFactor
                                                        source:colorCheck
                                                        visible:colorMsg.curSelected === modelData
                                                    }

                                                    MouseArea{
                                                        anchors.fill:parent
                                                        onClicked:colorMsg.curSelected = modelData
                                                    }
                                                }
                                            }
                                        }
                                        BasicDialogButton {
                                            anchors.right:parent.right
                                            anchors.rightMargin:10*screenScaleFactor
                                            anchors.bottom:parent.bottom
                                            anchors.bottomMargin:10*screenScaleFactor
                                            Layout.preferredWidth: 80*screenScaleFactor
                                            Layout.preferredHeight: 32*screenScaleFactor
                                            text: qsTr("Confirm")
                                            btnRadius:5*screenScaleFactor
                                            btnBorderW:0
                                            defaultBtnBgColor: Constants.themeGreenColor
                                            hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                            onSigButtonClicked: {
                                                materialColorId.color = colorMsg.curSelected
                                                materialMsg.materialChangeData.materialColor = materialColorId.color
                                                colorMsg.visible = false
                                            }

                                        }
                                    }
                                }

                            }

                            MaterialAutoRefillPanel {
                                id: material_auto_refill_panel

                                visible: false

                                anchors.centerIn: parent
                                width: 540 * screenScaleFactor
                                height: 280 * screenScaleFactor

                                radius: 5 * screenScaleFactor
                                color: Constants.currentTheme ? "#FFFFFF" :"#37373B"
                                border.width: 1 * screenScaleFactor
                                border.color: Constants.currentTheme? "#D6D6DC": "#515157"

                                model: deviceItem.materialBoxList.autoRefillListModel.generlatePageFilterListModel(3)
                            }

                            Timer{
                                id:busyTimer
                                repeat:false
                                interval:1500
                                onTriggered: busyMsg.visible = false
                            }

                            Rectangle{
                                id:busyMsg
                                anchors.fill:parent
                                color:sourceTheme.popup_background_color
                                visible:false
                                property var selected:"#ffffff"
                                property var curSelected:"#ffffff"

                                Rectangle{
                                    anchors.fill:parent
                                    color: Qt.rgba(0,0,0,0.6)
                                    Rectangle{
                                        anchors.centerIn:parent
                                        width:300 * screenScaleFactor
                                        height:60 * screenScaleFactor
                                        radius:5 * screenScaleFactor
                                        color:Constants.currentTheme? "#D6D6DC": "#515157"
                                        Text{
                                            anchors.centerIn:parent
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: sourceTheme.text_normal_color
                                            text: qsTr("CFS is busy, please try again later.")
                                        }
                                    }
                                }

                            }
                        }
                        Rectangle{
                            id: errorLayout1
                            visible: errorCode != 0 && ErrorData.errorJson[errorKey].isCfs
                            Layout.alignment: Qt.AlignRight
                            property bool isRepoPlrStatus: errorCode == 20000
                            property bool isMaterialStatus: errorCode == 20010
                            property color displayTextColor: fatalErrorCode ? "#FD265A" : (themeType ? "#333333" : "#FCE100")
                            width: parent.width
                            height:30* screenScaleFactor
                            anchors.top:materialPopup.top
                            color:"#78714F"
                            RowLayout{
                                anchors.fill:parent
                                RowLayout{
                                    anchors.left: parent.left
                                    anchors.leftMargin:10* screenScaleFactor
                                    Layout.alignment: Qt.AlignLeft
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color:ErrorData.errorJson[errorKey]? ["","#fd265a","#17cc5f","#fce100"][ErrorData.errorJson[errorKey].level]:"#fd265a"
                                        text:"["+ (!!ErrorData.errorJson[errorKey]?ErrorData.errorJson[errorKey].code:errorKey)+"]"
                                        MouseArea {
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onEntered: tooltip1.visible = !!ErrorData.errorJson[errorKey].wiki
                                            onExited: tooltip1.visible = false
                                            onClicked:Qt.openUrlExternally(ErrorData.errorJson[errorKey].wiki)
                                            cursorShape:Qt.PointingHandCursor
                                            BasicTooltip {
                                                id:tooltip1
                                                text: qsTr("Click to jump to Wiki")
                                                textColor:sourceTheme.text_weight_color
                                                font.pointSize: Constants.labelFontPointSize_9
                                            }
                                        }
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        verticalAlignment: Text.AlignVCenter
                                        color: sourceTheme.text_normal_color
                                        text: ErrorData.errorJson[errorKey]?ErrorData.errorJson[errorKey].content: qsTr("Unknown exception")
                                    }
                                }

                                Item{
                                    Layout.fillWidth: true
                                }

                                RowLayout{
                                    Layout.fillWidth: true
                                    Layout.rightMargin: 10

                                    ErrorMsgBtnState{
                                        errorCode: root.errorKey
                                        errorLevel: ErrorData.errorJson[root.errorKey].level
                                        supportVersion: ErrorData.errorJson[root.errorKey].supportVersion
                                        machineState: root.printerState
                                        Component.onCompleted: {
                                            items[0] = continueBtn
                                            items[1] = cancelBtn
                                            items[2] = idRetryButton
                                            items[3] = okBtn

//                                            supportVersion = Qt .binding(function(){return ErrorData.errorJson[errorKey] ? ErrorData.errorJson[errorKey].supportVersion : -1} )
                                        }
                                    }

                                    Button {
                                        id: continueBtn
                                        visible: false
                                        Layout.preferredWidth: 73 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                        id: cancelBtn
                                         visible: false
                                        Layout.preferredWidth: 100 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                            text: qsTr("Cancel print")
                                        }

                                        onClicked:{
                                            idMessageDialog.showMessageDialog(LanPrinterDetail.MessageType.CancelBrokenMaterial)
                                            idMessageDialog.ipAddress = ipAddress
                                            idMessageDialog.deviceType = deviceType
                                        }

                                    }

                                    Button {
                                        id: idRetryButton
                                         visible: false
                                        Layout.preferredWidth: 73 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                            text: qsTranslate("LanPrinterListLocal", "Retry")
                                        }

                                        onClicked:
                                        {
                                            if(ipAddress !== "")
                                            {
                                                printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RETRY_ERROR, "0", deviceType)

                                            }
                                        }
                                    }
                                    Button {
                                        id: okBtn
                                         visible: true
                                        Layout.preferredWidth: 73 * screenScaleFactor
                                        Layout.preferredHeight: 24 * screenScaleFactor

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
                                            text: qsTr("OK")
                                        }

                                        onClicked:
                                        {
                                            if(ipAddress !== "")
                                            {
                                                if(idRetryButton.visible)
                                                {
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RETRY_ERROR, "1", deviceType)
                                                }else{
                                                    printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", deviceType)
                                                }

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
                                            property int maxValue: deviceItem?deviceItem.maxNozzleTemp? deviceItem.maxNozzleTemp:300: 300
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

                                                validator: RegExpValidator{regExp: new RegExp(/^[0-9]*$/)}
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
                                                placeholderText:qsTr("Maximum value")+ __nozzleDstTempBox.maxValue
                                                Keys.onUpPressed: __nozzleDstTempBox.increase()
                                                Keys.onDownPressed: __nozzleDstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __nozzleDstTempBox.specifyPcNozzleDstTemp()
                                                onTextChanged: {
                                                    if(+text > __nozzleDstTempBox.maxValue) text = __nozzleDstTempBox.maxValue
                                                }
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
                                            property int maxValue: deviceItem?deviceItem.maxNozzleTemp? deviceItem.maxNozzleTemp:300: 300
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
                                                    if(boxValue == "") boxValue = minValue
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

                                                validator: RegExpValidator{regExp: new RegExp(/^[0-9]*$/)}
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
                                                placeholderText:qsTr("Maximum value")+ __nozzle2DstTempBox.maxValue
                                                Keys.onUpPressed: __nozzle2DstTempBox.increase()
                                                Keys.onDownPressed: __nozzle2DstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __nozzle2DstTempBox.specifyPcNozzleDstTemp()
                                                onTextChanged: {
                                                    if(+text > __nozzle2DstTempBox.maxValue) text = __nozzle2DstTempBox.maxValue
                                                }
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
                                            property int maxValue: deviceItem?deviceItem.maxBedTemp? deviceItem.maxBedTemp:200: 200
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
                                                    if(boxValue == "") boxValue = minValue
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

                                                validator: RegExpValidator{regExp: new RegExp(/^[0-9]*$/)}
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
                                                placeholderText:qsTr("Maximum value")+ __bedDstTempBox.maxValue
                                                Keys.onUpPressed: __bedDstTempBox.increase()
                                                Keys.onDownPressed: __bedDstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __bedDstTempBox.specifyPcBedDstTemp()
                                                onTextChanged: {
                                                    if(+text > __bedDstTempBox.maxValue) text = __bedDstTempBox.maxValue
                                                }
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
                                                    if(boxValue == "") boxValue = minValue
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
                                                placeholderText:qsTr("Maximum value")+ 60
                                                background: null

                                                Keys.onUpPressed: __chamberDstTempBox.increase()
                                                Keys.onDownPressed: __chamberDstTempBox.decrease()
                                                Keys.onEnterPressed: parent.forceActiveFocus()
                                                Keys.onReturnPressed: parent.forceActiveFocus()
                                                onEditingFinished: __chamberDstTempBox.specifyPcNozzleDstTemp()
                                                // enabled: !isKlipper_4408
                                                onTextChanged: {
                                                    if(+text >60) text = 60
                                                    if(+text<0) text = 0
                                                }

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

    BasicDialogV4{
        property var fileList
        property var isAllDelete
        id: noExistFilesDialog
        width: 600 * screenScaleFactor
        height: 400 * screenScaleFactor
        title: cxTr("Tips")
        maxBtnVis: false
        bdContentItem: Item{
            Text{
                id: title1Txt
                width: parent.width - 40
                wrapMode: Text.WordWrap
                text: cxTr("The exported file contains deleted files. This operation can only export non-deleted files. Do you want to continue?")
                font.weight: Font.Normal
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_12
                color: sourceTheme.text_normal_color
                anchors.top: parent.top
                anchors.topMargin: 20 * screenScaleFactor
                anchors.left: parent.left
                anchors.leftMargin: 20 * screenScaleFactor
            }

            Text{
                id: title2Txt
                width: parent.width - 40
                wrapMode: Text.WordWrap
                text: cxTr("Deleted files list")+":"
                font.weight: Font.Normal
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_10
                color: sourceTheme.text_normal_color
                anchors.top: title1Txt.bottom
                anchors.topMargin: 5 * screenScaleFactor
                anchors.left: parent.left
                anchors.leftMargin: 20 * screenScaleFactor
            }

            Rectangle{
                anchors.top: title2Txt.bottom
                anchors.topMargin: 10 * screenScaleFactor

                anchors.bottom: btnsRow.top
                anchors.bottomMargin: 10 * screenScaleFactor

                anchors.left: parent.left
                anchors.leftMargin: 20 * screenScaleFactor
                anchors.right: parent.right
                anchors.rightMargin: 20 * screenScaleFactor

                border.width: 1 * screenScaleFactor
                border.color: Constants.dialogTitleColor
                radius: 5 * screenScaleFactor
                color:"transparent"
                ListView{
                    clip: true
                    anchors.fill: parent
                    anchors.margins: 20 * screenScaleFactor
                    model: noExistFilesDialog.fileList
                    delegate: Text{
                        text: modelData
                        font.weight: Font.Normal
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color: sourceTheme.text_normal_color
                    }
                }
            }

            Row{
                id: btnsRow
                spacing: 20 * screenScaleFactor
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20 * screenScaleFactor
                CusButton{
                    width: 100 * screenScaleFactor
                    height: 30 * screenScaleFactor
                    txtText: cxTr("OK")
                    enabled: noExistFilesDialog.isAllDelete
                    //                    txtColor: sourceTheme.text_normal_color
                    radius: 15 * screenScaleFactor
                    onClicked: {
                        noExistFilesDialog.close()
                        save_dir_select_dialog.open()
                    }
                }

                CusButton{
                    width: 100 * screenScaleFactor
                    height: 30 * screenScaleFactor
                    txtText: cxTr("Cancel")
                    //                    txtColor: sourceTheme.text_normal_color
                    radius: 15 * screenScaleFactor
                    onClicked: {
                        noExistFilesDialog.close()
                    }
                }
            }
        }
    }
}
