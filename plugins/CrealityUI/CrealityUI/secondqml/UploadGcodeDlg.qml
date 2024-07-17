import "../components"
import "../qml"
import "../secondqml"
import "qrc:/CrealityUI/cxcloud"
import QtGraphicalEffects 1.12
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtQuick.Window 2.13

BasicDialogV4{
    id: root

    enum WindowState {
        State_Normal,
        State_Progress,
        State_Message
    }

    enum PrintControlType {
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
        CONTROL_CMD_AUXILIARYFANPCT,
        CONTROL_CMD_EXCLUDEOBJECTS,
        CONTROL_CMD_HOSTNAME
    }


    //0:Creality Cloud 1: LAN
    property int uploadDlgType: 0
    property int uploadProgress: 0
    property int curWindowState: -1
    property real borderWidth: 1 * screenScaleFactor
    property real shadowWidth: 5 * screenScaleFactor
    property real titleHeight: 30 * screenScaleFactor
    property real borderRadius: 5 * screenScaleFactor
    property real windowMinWidth: 500 * screenScaleFactor
    property real windowMaxWidth: 756 * screenScaleFactor
    property real windowMinHeight: 152 * screenScaleFactor
    property real windowMaxHeight: 684 * screenScaleFactor
    property alias curGcodeFileName: idGcodeFileName.text
    property var gcodeFileName: ""
    property var gcodeFilePath: ""
    //Slice parameter
    property bool isFromFile: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.isFromFile() : false
    property string printTime: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.printing_time() : ""
    property double materialWeight: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_weight() : ""
    property string titleText: uploadDlgType ? qsTr("Send G-code") : qsTr("Upload G-code")
    property string textColor: Constants.currentTheme ? "#333333" : "#CBCBCC"
    property string shadowColor: Constants.currentTheme ? "#BBBBBB" : "#333333"
    property string borderColor: Constants.currentTheme ? "#D6D6DC" : "#262626"
    property string titleFtColor: Constants.currentTheme ? "#333333" : "#FFFFFF"
    property string titleBgColor: Constants.currentTheme ? "#E9E9EF" : "#6E6E73"
    property string backgroundColor: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
    property var curTransProgress: 0
    property var cloudContext

    property var colorMatchList:[]

    signal sigCancelBtnClicked()
    signal sigConfirmBtnClicked()

    function showUploadDialog(type) {
        uploadDlgType = type;
        root.show();
    }

    function uploadGcodeProgress(value) {
        uploadProgress = value;
    }

    function uploadGcodeSuccess() {
        idMessageText.text = qsTr("Uploaded Successfully");
        idMessageImage.source = "qrc:/UI/photo/upload_success_image.png";
        curWindowState = UploadGcodeDlg.WindowState.State_Message;
    }

    function uploadGcodeFailed() {
        idMessageText.text = qsTr("Failed to upload gcode!");
        idMessageImage.source = "qrc:/UI/photo/upload_msg.png";
        curWindowState = UploadGcodeDlg.WindowState.State_Message;
    }

    // color: "transparent"
    width: 793* screenScaleFactor
    height: 700* screenScaleFactor// + materialColorFlow.height
    // modality: Qt.ApplicationModal
    // flags: Qt.FramelessWindowHint | Qt.Dialog
    onVisibleChanged: {
        if (visible) {
            curWindowState = UploadGcodeDlg.WindowState.State_Normal;
            imgPreview.source = "";
            imgPreview.source = "file:///" + kernel_slice_flow.gcodeThumbnail();
            idGcodeFileName.text = kernel_slice_flow.getExportName();
            idGcodeFileName.forceActiveFocus();
        }
    }
    onCurWindowStateChanged: {
        width = curWindowState ? windowMinWidth : windowMaxWidth;
        height = (curWindowState ? windowMinHeight : windowMaxHeight) //+ materialColorFlow.height;
    }

    Connections {
        target: cxkernel_cxcloud.sliceService
        onUploadSliceSuccessed: function() {
            root.uploadGcodeSuccess();
        }
        onUploadSliceFailed: function() {
            root.uploadGcodeFailed();
        }
        onUploadSliceProgressUpdated: function(progress) {
            console.log("progressprogress",progress)
            root.uploadGcodeProgress(progress);
        }
    }

    contentItem:Rectangle {
        id: rect

        anchors.centerIn: parent
        anchors.margins: shadowWidth
        border.width: borderWidth
        border.color: borderColor
        color: backgroundColor
        radius: borderRadius
        layer.enabled: true

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
                property point clickPos: "0,0"

                anchors.fill: parent
                enabled: curWindowState === UploadGcodeDlg.WindowState.State_Normal
                onPressed: clickPos = Qt.point(mouse.x, mouse.y)
                onPositionChanged: {
                    var cursorPos = WizardUI.cursorGlobalPos();
                    root.x = cursorPos.x - clickPos.x;
                    root.y = cursorPos.y - clickPos.y;
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
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    color: titleFtColor
                    text: titleText
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
                onClicked: root.visible = false

                Image {
                    anchors.centerIn: parent
                    width: 10 * screenScaleFactor
                    height: 10 * screenScaleFactor
                    source: parent.isHovered ? "qrc:/UI/photo/closeBtn_d.svg" : "qrc:/UI/photo/closeBtn.svg"
                }

            }

        }

        Item {
            width: parent.width
            height: parent.height - cusTitle.height
            anchors.top: cusTitle.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: forceActiveFocus()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 20 * screenScaleFactor
                anchors.leftMargin: 40 * screenScaleFactor
                anchors.rightMargin: 40 * screenScaleFactor
                anchors.bottomMargin: 20 * screenScaleFactor
                //                spacing: 20 * screenScaleFactor
                visible: curWindowState === UploadGcodeDlg.WindowState.State_Normal

                RowLayout {
                    spacing: 8 * screenScaleFactor
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillHeight: true

                    Image {
                        Layout.preferredWidth: (uploadDlgType ? 22 : 28) * screenScaleFactor
                        Layout.preferredHeight: (uploadDlgType ? 23 : 26) * screenScaleFactor
                        source: uploadDlgType ? `qrc:/UI/photo/wifiPrint_upload_${Constants.currentTheme ? "light" : "dark"}.svg` : "qrc:/UI/photo/CloudLogo.png"
                    }

                    Text {
                        Layout.preferredWidth: contentWidth * screenScaleFactor
                        Layout.preferredHeight: 26 * screenScaleFactor
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_12
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        color: textColor
                        text: uploadDlgType ? qsTr("Local Area Network") : qsTr("Creality Cloud")
                    }

                }

                RowLayout {
                    Layout.preferredHeight: 20 * screenScaleFactor
                    Layout.preferredWidth: idGcodeFileName.width + 20 * screenScaleFactor

                    //                    Layout.maximumWidth: parent.width
                    TextField {
                        id: idGcodeFileName

                        property var editGcodeReadOnly: true

                        readOnly: editGcodeReadOnly
                        background: null
                        Layout.preferredWidth: contentWidth + 30 * screenScaleFactor
                        Layout.preferredHeight: 30 * screenScaleFactor
                        text: kernel_slice_flow.getExportName()
                        maximumLength: 100
                        selectByMouse: true
                        selectionColor: Constants.currentTheme ? "#98DAFF" : "#1E9BE2"
                        selectedTextColor: color
                        leftPadding: 10 * screenScaleFactor
                        rightPadding: 10 * screenScaleFactor
                        verticalAlignment: TextInput.AlignVCenter
                        horizontalAlignment: TextInput.AlignLeft
                        color: textColor
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        Keys.onEnterPressed: editGcodeReadOnly = true
                        Keys.onReturnPressed: editGcodeReadOnly = true
                        activeFocusOnPress: {
                            if (!activeFocus)
                                editGcodeReadOnly = true;

                        }

                        validator: RegExpValidator {
                            regExp: /^.{100}$/
                        }

                    }

                    Rectangle {
                        id: editGroupRect

                        color: "transparent"
                        width: 14 * screenScaleFactor
                        height: 14 * screenScaleFactor
                        Layout.alignment: Qt.AlignVCenter

                        Image {
                            anchors.centerIn: parent
                            source: "qrc:/UI/photo/editDevice.png"
                        }

                        MouseArea {
                            id: editGroupArea

                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                if (idGcodeFileName.editGcodeReadOnly) {
                                    idGcodeFileName.editGcodeReadOnly = false;
                                    idGcodeFileName.forceActiveFocus();
                                }
                            }
                        }

                    }

                }

                    Rectangle {
                        id:thumbMsg
                        Layout.preferredWidth: 674 * screenScaleFactor
                        Layout.preferredHeight: 340 * screenScaleFactor
                        color: Constants.currentTheme ? "#FFFFFF" : "#424244"
                        border.color: Constants.currentTheme ? "#DDDDE1" : "transparent"
                        border.width: Constants.currentTheme ? 1 : 0
                        radius: 5

                        Column {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.bottomMargin: 10 * screenScaleFactor
                            anchors.leftMargin: 10 * screenScaleFactor
                            spacing: 10 * screenScaleFactor

                            Row {
                                spacing: 8 * screenScaleFactor

                                Image {
                                    width: 18 * screenScaleFactor
                                    height: 16 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: Constants.currentTheme ? "qrc:/UI/photo/print_time_light.svg" : "qrc:/UI/photo/print_time_dark.svg"
                                }

                                Text {
                                    width: 16 * screenScaleFactor
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: textColor
                                    text: printTime
                                }

                            }

                            Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 2 * screenScaleFactor
                                spacing: 10 * screenScaleFactor

                                Image {
                                    width: 14 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: Constants.currentTheme ? "qrc:/UI/photo/material_weight_light.svg" : "qrc:/UI/photo/material_weight_dark.svg"
                                }

                                Text {
                                    width: 16 * screenScaleFactor
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: textColor
                                    text: materialWeight + "g"
                                }

                            }

                        }

                        Image {
                            id: imgPreview

                            property string imgDefault: "qrc:/UI/photo/imgPreview_default.png"

                            cache: false
                            anchors.centerIn: parent
                            sourceSize: Qt.size(300, 300)
                            onStatusChanged: {
                                if (status == Image.Error)
                                    source = imgDefault;

                            }
                        }

                    }

                        ScrollView {
                            id:materialList
                            visible:uploadDlgType&& printerListModel.getDeviceObject(control.currentIndex).materialBoxList //printerListModel.cSourceModel.getPinterName(control.currentText).toLowerCase() === "k1 max" //"k2 plus"
                            width:674 * screenScaleFactor // 
                         //   Layout.preferredWidth: 674 * screenScaleFactor
                            clip:true
                            Layout.maximumHeight: 110 * screenScaleFactor
                            Layout.minimumHeight: 60* screenScaleFactor 
                            ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            anchors.top: thumbMsg.bottom
                            anchors.topMargin: 10 * screenScaleFactor
                            Flow {
                                id: materialColorFlow
                                Layout.topMargin: 10 * screenScaleFactor
                                Layout.bottomMargin: 10 * screenScaleFactor
                                width:  parent.width
                                // Layout.preferredHeight:
                                spacing: 13* screenScaleFactor
                                onVisibleChanged:{
                                    if(visible)materialMap.model=kernel_slice_model.extruderTableModel   
                                }

                                Repeater {
                                    id:materialMap
                                    model:kernel_slice_model.extruderTableModel   
                                   
                                    delegate: RowLayout {
                                        id:materialMapItem
                                        property var colorMsg:{}
                                        property string gcodeType: ""
                                        property string gcodeColor: ""
                                        property string boxId: ""
                                        property string materialId: ""
                                        function setTextColor(color) {
                                            let result = "";
                                            let bgColor = [color.r * 255, color.g * 255, color.b * 255];
                                            let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
                                            let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
                                            if (whiteContrast > blackContrast)
                                                result = "#ffffff";
                                            else
                                                result = "#000000";
                                            return result;
                                        }

                                        BasicDialogButton {
                                            id: curMaterialColor
                                            Layout.preferredWidth: 70 * screenScaleFactor
                                            Layout.preferredHeight: 24 * screenScaleFactor
                                            text: kernel_parameter_manager.currentMachineObject.extrudersModel.get(model.ctModel.getMaterialIndex()).curMaterialObj.materialType
                                            btnRadius: height / 2
                                            defaultBtnBgColor: model.ctModel.getMaterialColor()
                                            hoveredBtnBgColor: defaultBtnBgColor
                                            btnBorderW: 0
                                            btnTextColor: parent.setTextColor(defaultBtnBgColor)
                                            Component.onCompleted:{
                                              parent.gcodeType = text
                                              parent.gcodeColor = defaultBtnBgColor
                                            }
                                        }

                                        Canvas {
                                            width: 7 * screenScaleFactor
                                            height: 10 * screenScaleFactor
                                            contextType: "2d"
                                            onPaint: {
                                                const context = getContext("2d")
                                                context.clearRect(0, 0, width, height);
                                                context.beginPath();
                                                context.moveTo(0, 0);
                                                context.lineTo(0,height)
                                                context.lineTo(width,height/2)
                                                context.closePath();
                                                context.fillStyle = "#9F9FA2"
                                                context.fill();
                                            }
                                        }

                                        MaterialComboBox{
                                            id:materialComboBoxId
                                            width:70 * screenScaleFactor
                                            height:43* screenScaleFactor
                                            pcPrinterID: printerListModel.getDeviceObject(control.currentIndex).pcPrinterID
                                            materialmodel:printerListModel.getDeviceObject(control.currentIndex).materialBoxList
                                            curIndex: 0
                                            onCurIndexChanged:{
                                                parent.boxId = materialmodel.getMaterialBoxId(pcPrinterID,curIndex)
                                                parent.materialId = materialmodel.getMaterialId(pcPrinterID,curIndex)
                                            }
                                            onVisibleChanged:{
                                                if(visible){
                                                     parent.boxId = materialmodel.getMaterialBoxId(pcPrinterID,curIndex)
                                                    parent.materialId = materialmodel.getMaterialId(pcPrinterID,curIndex)
                                                }
                                            }
                                            Connections{
                                                target:control
                                                function onCurrentIndexChanged(){
                                                    let model = printerListModel.getDeviceObject(control.currentIndex).materialBoxList
                                                    materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor)
                                                }
                                            }
                                        }
                                    }

                                }

                            }
                        }

                        RowLayout {
                            id: machineIp
                            visible:uploadDlgType
                            spacing: 10 * screenScaleFactor
                            Text {
                                Layout.preferredWidth: contentWidth * screenScaleFactor
                                Layout.preferredHeight: 28 * screenScaleFactor
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                color: textColor
                                text: qsTr("Printer")
                            }

                            CXComboBox {
                                // if (root.visible)
                                //     swtich_engine_dialog.show();

                                id: control

                                property var process

                                model: printerListModel
                                Layout.preferredWidth:  430 * screenScaleFactor // errorLayout.fatalErrorCode?: 624 * screenScaleFactor
                                Layout.preferredHeight: 30 * screenScaleFactor
                                textRole: "ipAddr"
                            
                                function setPrinterStatus(printerStatus,printerState) {
                                    if (printerStatus !== 1)
                                        return qsTr("Offline");

                                    switch (printerState) {
                                    case 5:
                                        return qsTr("Pause");
                                    case 1:
                                        return qsTr("Printing");
                                    case 0:                       
                                    case 2:
                                    case 3:
                                    case 4:
                                       return qsTr("Idle");
                                   
                                    }
                                }

                                onCurrentIndexChanged:{
                                     let id = printerListModel.getDeviceObject(control.currentIndex).pcPrinterID
                                     let type = printerListModel.getDeviceObject(control.currentIndex).printerType
                                     printerListModel.getDeviceObject(control.currentIndex).materialBoxList.switchDataSource(id,type)
                                }
                                contentItem:RowLayout{
                                    anchors.fill:parent
                                    property var printState:  printerListModel.getDeviceObject(control.currentIndex).pcPrinterState
                                    property var printStatus:  printerListModel.getDeviceObject(control.currentIndex).pcPrinterStatus
                                    property var deviceName:  printerListModel.getDeviceObject(control.currentIndex).deviceName
                                    property var pcPrinterModel:  printerListModel.getDeviceObject(control.currentIndex).pcPrinterModel
                                    property string printerName: deviceName?deviceName: kernel_add_printer_model.getModelNameByCodeName(pcPrinterModel)
                                    Text {
                                        leftPadding: 10 * screenScaleFactor
                                        verticalAlignment:  Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                        elide: Text.ElideRight
                                        color: Constants.right_panel_text_default_color
                                        font: Constants.font
                                        text:  (parent.printerName? (parent.printerName + "/") : "" )+ control.currentText
                                    }
                                    Text {
                                        anchors.right:parent.right
                                        anchors.rightMargin:50 * screenScaleFactor
                                        verticalAlignment:  Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignRight
                                        elide: Text.ElideRight
                                        visible:!!control.currentText
                                        color: Constants.right_panel_text_default_color
                                        font: Constants.font
                                        text: control.setPrinterStatus(parent.printStatus, parent.printState)
                                    }

                                }
                                delegate: ItemDelegate {
                                    property bool currentItem: control.highlightedIndex === index

                                    width: control.width
                                    height: control.popHeight
                                    hoverEnabled: control.hoverEnabled

                                    contentItem: Rectangle {

                                        anchors.fill: parent
                                        color: control.highlightedIndex === index ? control.itemHighlightColor : control.itemNormalColor

                                        Text {
                                            id: myText
                                            property string printerName: mItem.deviceName ? mItem.deviceName : kernel_add_printer_model.getModelNameByCodeName(mItem.pcPrinterModel)
                                            x: 5 * screenScaleFactor
                                            height: control.popHeight
                                            width: parent.width - 5 * screenScaleFactor
                                            text: (printerName? (printerName + "/") : "" )+ ipAddr
                                            color: control.textColor
                                            font: control.font
                                            elide: Text.ElideMiddle
                                            verticalAlignment: Text.AlignVCenter
                                        }

                                        Text {
                                            id: name

                                            text: control.setPrinterStatus(printerStatus,printerState)
                                            color: control.textColor
                                            height: control.popHeight
                                            font: control.font
                                            verticalAlignment: Text.AlignVCenter
                                            anchors.right: parent.right
                                            anchors.rightMargin: 50 * screenScaleFactor
                                        }

                                    }

                                }

                            }

                            RowLayout {
                                id: errorLayout
                                property var ipAddr:  printerListModel.getDeviceObject(control.currentIndex).pcIpAddr
                                property int errorCode: +printerListModel.cSourceModel.getPinterErrorCode(control.currentText)
                                property string errorMessage: printerListModel.cSourceModel.getPinterErrorMsg(control.currentText)
                                property string errorKey: printerListModel.cSourceModel.getPinterErrorKey(control.currentText)
                                property string printerType: printerListModel.cSourceModel.getPrinterType(control.currentText)
                                property bool fatalErrorCode: (errorCode >= 1 && errorCode <= 100) || errorCode == 2000 || errorCode == 2001
                                property bool isRepoPlrStatus: errorCode == 2000
                                property bool isMaterialStatus: errorCode == 2001
                                property color displayTextColor: fatalErrorCode ? "#FD265A" : (Constants.currentTheme ? "#333333" : "#FCE100")

                                visible: errorCode != 0
                                spacing: 10 * screenScaleFactor
                                Layout.alignment: Qt.AlignRight
                                onVisibleChanged: {
                                    if (!visible)
                                        errorTooltip.close();

                                }

                                Item {
                                    Layout.preferredWidth: 20 * screenScaleFactor
                                    Layout.preferredHeight: 20 * screenScaleFactor

                                    MouseArea {
                                        id: imageArea

                                        hoverEnabled: true
                                        anchors.fill: parent
                                        onContainsMouseChanged: {
                                            if (containsMouse && !errorTooltip.opened)
                                                errorTooltip.open();

                                        }

                                        Image {
                                            Layout.preferredWidth: 20 * screenScaleFactor
                                            Layout.preferredHeight: 20 * screenScaleFactor
                                            anchors.centerIn: parent
                                            source: errorLayout.fatalErrorCode ? "qrc:/UI/photo/lanPrinterSource/printer_error_icon.svg" : "qrc:/UI/photo/lanPrinterSource/printer_warning_icon.svg"
                                        }

                                    }

                                    ToolTip {
                                        id: errorTooltip

                                        x: -10 * screenScaleFactor
                                        y: -implicitHeight - 10 * screenScaleFactor
                                        padding: 0
                                        margins: 0
                                        timeout: (imageArea.containsMouse || tooltipArea.containsMouse) ? -1 : 1000

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
                                                        text: qsTr("Error code:") + `E${String(errorLayout.errorCode).padStart(4, '0')}`
                                                        visible: !(errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus)
                                                    }

                                                    Text {
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        verticalAlignment: Text.AlignVCenter
                                                        color: errorLayout.displayTextColor
                                                        text: (errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus) ? errorLayout.errorMessage : (qsTr("Description:") + "Key" + errorLayout.errorKey + errorLayout.errorMessage)
                                                    }

                                                }

                                                Button {
                                                    width: 60 * screenScaleFactor
                                                    height: 24 * screenScaleFactor
                                                    visible: !errorLayout.fatalErrorCode
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    onClicked: {
                                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_CLEANERR, " ", errorLayout.printerType);
                                                    }

                                                    background: Rectangle {
                                                        border.width: 1
                                                        border.color: Constants.currentTheme ? "#D9D9DE" : "#606064"
                                                        color: parent.hovered ? (Constants.currentTheme ? "#FFFFFF" : "#38383C") : (Constants.currentTheme ? "#FFFFFF" : "#28282B")
                                                        radius: height / 2
                                                    }

                                                    contentItem: Text {
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_9
                                                        horizontalAlignment: Text.AlignHCenter
                                                        verticalAlignment: Text.AlignVCenter
                                                        color: Constants.textColor
                                                        text: qsTr("Ignore")
                                                    }

                                                }

                                            }

                                        }

                                        background: Rectangle {
                                            color: Constants.currentTheme ? "#FFFFFF" : "#414143"
                                            border.color: Constants.currentTheme ? "#D6D6DC" : "#626266"
                                            border.width: 1
                                            radius: 5
                                        }

                                    }

                                }

                                Button {
                                    Layout.preferredWidth: 73 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor
                                    visible: (errorLayout.errorCode >= 101 && errorLayout.errorCode <= 200) || errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus
                                    onClicked: {
                                        if (errorLayout.isRepoPlrStatus) {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "1", errorLayout.printerType);
                                        } else {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.PRINT_CONTINUE, " ", errorLayout.printerType);
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_CLEANERR, " ", errorLayout.printerType);
                                        }
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                                        color: parent.hovered ? (Constants.currentTheme ? "#E8E8ED" : "#6E6E73") : (Constants.currentTheme ? "#FFFFFF" : "#59595D")
                                        radius: height / 2
                                    }

                                    contentItem: Text {
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                                        text: qsTr("Continue")
                                    }

                                }

                                Button {
                                    Layout.preferredWidth: 73 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor
                                    visible: (errorLayout.errorCode >= 101 && errorLayout.errorCode <= 200) || errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus
                                    onClicked: {
                                        if (errorLayout.isRepoPlrStatus) {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "0", errorLayout.printerType);
                                        } else {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.PRINT_STOP, " ", errorLayout.printerType);
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_CLEANERR, " ", errorLayout.printerType);
                                        }
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                                        color: parent.hovered ? (Constants.currentTheme ? "#E8E8ED" : "#6E6E73") : (Constants.currentTheme ? "#FFFFFF" : "#59595D")
                                        radius: height / 2
                                    }

                                    contentItem: Text {
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                                        text: qsTr("Cancel")
                                    }

                                }

                                Button {
                                    Layout.preferredWidth: 73 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor
                                    visible: (errorLayout.errorCode >= 1 && errorLayout.errorCode <= 100)
                                    onClicked: {
                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_RESTART_KLIPPER, " ", errorLayout.printerType);
                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, UploadGcodeDlg.PrintControlType.CONTROL_CMD_RESTART_FIRMWARE, " ", errorLayout.printerType);
                                    }

                                    background: Rectangle {
                                        border.width: 1
                                        border.color: Constants.currentTheme ? "#D6D6DC" : "transparent"
                                        color: parent.hovered ? (Constants.currentTheme ? "#E8E8ED" : "#6E6E73") : (Constants.currentTheme ? "#FFFFFF" : "#59595D")
                                        radius: height / 2
                                    }

                                    contentItem: Text {
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                                        text: qsTr("Reboot")
                                    }

                                }

                            }

                        }

                        Item {
                            visible:uploadDlgType
                            Layout.fillHeight: true
                        }

                        RowLayout {
                            id: btn
                            visible:uploadDlgType
                            // anchors.horizontalCenter: parent.horizontalCenter
                            property var progress: printerListModel.data(printerListModel.index(+control.currentIndex, 0),37).gCodeTransProgress
                            property bool statusDisable:  printerListModel.getDeviceObject(control.currentIndex).pcPrinterStatus !== 1
                            property bool stateDisable: printerListModel.getDeviceObject(control.currentIndex).pcPrinterState === 1
                            property var errorCode: +printerListModel.cSourceModel.getPinterErrorCode(control.currentText)
                            property var btnDisable: (btn.progress < 100 && btn.progress > 0) || !control.currentText || statusDisable

                            Layout.preferredHeight: 30 * screenScaleFactor
                            Layout.preferredWidth: 340 * screenScaleFactor
                            Layout.alignment: Qt.AlignHCenter

                            ProgressBar {
                                id: sendId

                                property bool clicked: false

                                Layout.preferredHeight: 30 * screenScaleFactor
                                Layout.preferredWidth: 100 * screenScaleFactor
                                from: 0
                                to: 100
                                value: btn.progress

                                background: BasicDialogButton {
                                    Layout.preferredHeight: 30 * screenScaleFactor
                                    Layout.preferredWidth: 100 * screenScaleFactor
                                    text: qsTr("Send G-code")
                                    btnRadius: 15 * screenScaleFactor
                                    btnBorderW: 1 * screenScaleFactor
                                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                                    enabled: !btn.btnDisable
                                    onSigButtonClicked: {
                                        printerListModel.cSourceModel.setOneSelected(control.currentText);
                                        let filePrefixPath = printerListModel.getDeviceObject(control.currentIndex).filePrefixPath
                                        printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".gcode", kernel_slice_flow.currentGCodeImageFileName(), true, filePrefixPath);
                                        sendId.clicked = true;
                                    }
                                }

                                contentItem: Rectangle {
                                    width: parent.visualPosition * parent.width
                                    height: parent.height
                                    color: Constants.themeGreenColor // Constants.currentTheme ? "#00A3FF" : "#1E9BE2"
                                    radius: height / 2
                                    visible: sendId.clicked && btn.progress > 0 && btn.progress < 100
                                    onVisibleChanged: {
                                        if (!visible)
                                            sendId.clicked = false;

                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: btn.progress
                                        color: Constants.textColor
                                    }

                                }

                            }

                            ProgressBar {
                                id: onePrintId

                                property bool clicked: false

                                Layout.preferredHeight: 30 * screenScaleFactor
                                Layout.preferredWidth: 100 * screenScaleFactor
                                from: 0
                                to: 100
                                value: btn.progress

                                background: BasicDialogButton {
                                    id: onePrint

                                    Layout.preferredHeight: 30 * screenScaleFactor
                                    Layout.preferredWidth: 100 * screenScaleFactor
                                    text: qsTr("One-click Printing")
                                    btnRadius: 15 * screenScaleFactor
                                    btnBorderW: 1 * screenScaleFactor
                                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                                    enabled: !btn.btnDisable && !btn.stateDisable && btn.errorCode === 0
                                    onSigButtonClicked: {
                                        printerListModel.cSourceModel.setOneSelected(control.currentText);
                                        printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".gcode", kernel_slice_flow.currentGCodeImageFileName(), false);
                                        onePrintId.clicked = true;
                                    }
                                }

                                contentItem: Rectangle {
                                    width: parent.visualPosition * parent.width
                                    height: parent.height
                                    color:Constants.themeGreenColor // Constants.currentTheme ? "#00A3FF" : "#1E9BE2"
                                    radius: height / 2
                                    visible: onePrintId.clicked && btn.progress > 0 && btn.progress < 100
                                    onVisibleChanged: {
                                        if (!visible)
                                            onePrintId.clicked = false;

                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        text: btn.progress
                                        color: Constants.textColor
                                    }

                                }
                                // onValueChanged:{
                                //     if(value === 100){
                                //            let arr = []
                                //     for(let i=0;i<materialMap.count; i++){
                                //         let obj = {}
                                //         let item = materialMap.itemAt(i)
                                //         if(item){
                                //             obj["type"] = item.gcodeType
                                //             obj["color"] = item.gcodeColor
                                //             obj["boxId"] = item.boxId
                                //             obj["materialId"] = item.materialId
                                //             arr.push(obj)
                                //             obj = null
                                //         }
                                //     }
                                //     let materialBoxModel = printerListModel.getDeviceObject(control.currentIndex).materialBoxList
                                //     let filePrefixPath = printerListModel.getDeviceObject(control.currentIndex).filePrefixPath
                                //     let path = filePrefixPath + "/" + idGcodeFileName.text + ".gcode"
                              
                                //     materialBoxModel.materialColorMap(control.currentText, path,arr)
                                //     }
                                // }

                            }

                            BasicDialogButton {
                                Layout.preferredHeight: 30 * screenScaleFactor
                                Layout.preferredWidth: 100 * screenScaleFactor
                                text: qsTr("Multi-machine control")
                                btnRadius: 15 * screenScaleFactor
                                btnBorderW: 1 * screenScaleFactor
                                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                                onSigButtonClicked: {
                                    gcodeFileName = idGcodeFileName.text;
                                    gcodeFilePath = kernel_slice_flow.currentGCodeImageFileName();
                                    sigConfirmBtnClicked();
                                    // let arr = []
                                    // for(let i=0;i<materialMap.count; i++){
                                    //     let obj = {}
                                    //     let item = materialMap.itemAt(i)
                                    //     if(item){
                                    //         obj["type"] = item.gcodeType
                                    //         obj["color"] = item.gcodeColor
                                    //         obj["boxId"] = item.boxId
                                    //         obj["materialId"] = item.materialId
                                    //         arr.push(obj)
                                    //         obj = null
                                    //     }
                                    // }
                                    // let materialBoxModel = printerListModel.getDeviceObject(control.currentIndex).materialBoxList
                                    // let filePrefixPath = printerListModel.getDeviceObject(control.currentIndex).filePrefixPath
                                    // let path = filePrefixPath + "/" + idGcodeFileName.text + ".gcode"
                              
                                    // materialBoxModel.materialColorMap(control.currentText, path,arr)
                                }
                            }

                        }


                        RowLayout {
                            spacing: 10 * screenScaleFactor
                            Layout.alignment: Qt.AlignHCenter
                            visible:!uploadDlgType
                            Repeater {
                                model: ListModel
                                {
                                    ListElement { modelData: QT_TR_NOOP("Confirm") }
                                    ListElement { modelData: QT_TR_NOOP("Cancel")  }
                                }
                                delegate: ItemDelegate
                                {
                                    implicitWidth: 120 * screenScaleFactor
                                    implicitHeight: 28 * screenScaleFactor

                                    opacity: enabled ? 1.0 : 0.7
                                    enabled: index || idGcodeFileName.text.length

                                    background: Rectangle {
                                        color: parent.hovered ? (Constants.currentTheme ? "#E8E8ED" : "#59595D") : (Constants.currentTheme ? "#FFFFFF" : "#6E6E73")
                                        border.color: Constants.currentTheme ? "#DDDDE1" : "transparent"
                                        border.width: Constants.currentTheme ? 1 : 0
                                        radius: parent.height / 2
                                    }

                                    contentItem: Text
                                    {
                                        font.weight: Font.Medium
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        color: textColor
                                        text: qsTr(modelData)
                                    }

                                    onClicked: {
                                        if(!index) gcodeFileName = idGcodeFileName.text
                                        index ? sigCancelBtnClicked() : sigConfirmBtnClicked()
                                    }
                                }
                            }
                        }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 8 * screenScaleFactor
                visible: curWindowState === UploadGcodeDlg.WindowState.State_Progress

                Text {
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    Layout.alignment: Qt.AlignHCenter
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    color: textColor
                    text: uploadProgress + "%"
                }

                ProgressBar {
                    Layout.preferredWidth: 380 * screenScaleFactor
                    Layout.preferredHeight: 2 * screenScaleFactor
                    from: 0
                    to: 100
                    value: uploadProgress

                    background: Rectangle {
                        color: Constants.currentTheme ? "#D6D6DC" : "#38383B"
                        radius: 1
                    }

                    contentItem: Rectangle {
                        width: parent.visualPosition * parent.width
                        height: parent.height
                        color: Constants.currentTheme ? "#00A3FF" : "#1E9BE2"
                    }

                }

            }
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20 * screenScaleFactor
                visible: curWindowState === UploadGcodeDlg.WindowState.State_Message

                RowLayout {
                    spacing: 10 * screenScaleFactor
                    Layout.alignment: Qt.AlignHCenter

                    Image {
                        id: idMessageImage
                        sourceSize: Qt.size(24, 24)
                        source: root.msgImageSource
                    }

                    Text {
                        id: idMessageText
                        font.weight: Font.Normal
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        verticalAlignment: Text.AlignVCenter
                        color: textColor
                        text: root.errorTxt
                    }
                }

                RowLayout {
                    spacing: 10 * screenScaleFactor

                    Repeater {
                        model: ListModel
                        {
                            ListElement { modelData: QT_TR_NOOP("View My Uploads") }
                            ListElement { modelData: QT_TR_NOOP("Cloud Printing")  }
                        }
                        delegate: ItemDelegate
                        {
                            implicitWidth: 120 * screenScaleFactor
                            implicitHeight: 28 * screenScaleFactor

                            background: Rectangle {
                                color: parent.hovered ? (Constants.currentTheme ? "#E8E8ED" : "#59595D") : (Constants.currentTheme ? "#FFFFFF" : "#6E6E73")
                                border.color: Constants.currentTheme ? "#DDDDE1" : "transparent"
                                border.width: Constants.currentTheme ? 1 : 0
                                radius: parent.height / 2
                            }

                            contentItem: Text
                            {
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                color: textColor
                                text: qsTr(modelData)
                            }

                            onClicked:
                            {
                                if(index)
                                {
                                    root.visible = false
                                    cxkernel_cxcloud.printerService.openCloudPrintWebpage()
                                }
                                else {
                                   
                                    cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_SLICE);
                                     root.visible = false
                                }
                            }
                        }
                    }
                }
            }

        }

        layer.effect: DropShadow {
            radius: 8
            spread: 0.2
            samples: 17
            color: shadowColor
        }

    }

}
