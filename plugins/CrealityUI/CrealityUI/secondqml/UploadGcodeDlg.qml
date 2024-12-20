import "../components"
import "../lanprinterqml/errorData.js" as ErrorData
import "../qml"
import "../secondqml"
import "qrc:/CrealityUI/cxcloud"
import "../lanprinterqml"
import QtGraphicalEffects 1.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15

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
        CONTROL_CMD_HOSTNAME,
        CONTROL_CMD_GETCOLORMATCH = 33
    }


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
    property real windowMaxHeight: 720 * screenScaleFactor
    readonly property real uploadCloudHeight: 540* screenScaleFactor
    property alias curGcodeFileName: idGcodeFileName.text
    property var gcodeFileName: ""
    property var gcodeFilePath: ""
    //Slice parameter
    property bool isFromFile: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.isFromFile() : false
    property string printTime: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.printing_time() : ""
    property double materialWeight: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_weight() : ""
    property string textColor: Constants.currentTheme ? "#333333" : "#CBCBCC"
    property string shadowColor: Constants.currentTheme ? "#BBBBBB" : "#333333"
    property string borderColor: Constants.currentTheme ? "#D6D6DC" : "#262626"
    property string titleFtColor: Constants.currentTheme ? "#333333" : "#FFFFFF"
    property string titleBgColor: Constants.currentTheme ? "#E9E9EF" : "#6E6E73"
    property string backgroundColor: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
    property var curTransProgress: 0
    property var cloudContext
    property var currentObject: printerListModel.cSourceModel.getDeviceObject(control.currentIndex)
    property bool currentPrinterNotMap:false
    property bool connectCfs:false
    property bool currentMachineSupportMutilPlate: currentObject.pcPrinterModel.includes("F008") || uploadDlgType == 0


    property string edit:  "qrc:/UI/photo/upload3mf/edit_name_dark.svg"
    property string slice:  "qrc:/UI/photo/upload3mf/slice_dark.svg"
    property string empty:  "qrc:/UI/photo/upload3mf/empty_dark.svg"
    property string plate:  "qrc:/UI/photo/upload3mf/plate_dark.svg"
    property string printer:  "qrc:/UI/photo/upload3mf/printer_dark.svg"
    property string profile:  "qrc:/UI/photo/upload3mf/profile_dark.svg"
    property string slice_msg:  "qrc:/UI/photo/upload3mf/slice_msg_dark.svg"
    property string material:  "qrc:/UI/photo/upload3mf/material_dark.svg"
    property string logo:  "qrc:/UI/photo/upload3mf/logo.svg"
    property string no_slice:  "qrc:/UI/photo/upload3mf/no_slice.svg"
    property string pre_arrow:  "qrc:/UI/photo/upload3mf/pre_arrow.svg"
    property string next_arrow:  "qrc:/UI/photo/upload3mf/next_arrow.svg"
    property bool upload_cloud:uploadDlgType === 4
    property string titleText: upload_cloud? qsTr("Upload") : qsTr("Send")
    property var modelInsideUseExtruderIndex:[]
    signal sigCancelBtnClicked()
    signal sigConfirmBtnClicked()
    function initColorMap(materialBoxList,usedIndexs)
    {
        var colors = kernel_parameter_manager.currentMachineObject.extrudersModel.colorList();
        var types = kernel_parameter_manager.currentMachineObject.extrudersModel.typeList();
        //var usedIndexs = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(kernel_printer_manager.selectedPrinterIndex);
        materialBoxList.GenCloseGCodeColors(colors,types,usedIndexs)

        console.log("usedIndex:"+usedIndexs)
    }
    function showUploadDialog(type) {

        uploadDlgType = type;

        curWindowState = UploadGcodeDlg.WindowState.State_Normal
        // if(type === 0)
        // {
        //     height = uploadCloudHeight

        // }
        // else if(type === 1)
        // {
        //     height = (curWindowState ? windowMinHeight : windowMaxHeight)
        //     // currentObject = printerListModel.cSourceModel.getDeviceObject(control.currentIndex)
        //     // if(currentObject==null&&control.currentIndex!=0)
        //     // {
        //     //     currentObject = printerListModel.cSourceModel.getDeviceObject(0)
        //     // }
        // }
        if(uploadDlgType === 1)
        {
            idGcodeFileName.text = kernel_slice_flow.get3MfExportName();
        }else{
            idGcodeFileName.text = kernel_slice_flow.getExportName();
        }

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

    function devicePrintOps(){
        currentObject.enableSelfTest = printerCalibration.checked
        printerListModel.cSourceModel.setOneSelected(control.currentText);
        let materialBoxModel = currentObject.materialBoxList
        let arr = []
        if(materialBoxModel&&enableCfs.checked){
            for(let i=0;i<materialMap.count; i++){
                let obj = {}
                let item = materialMap.itemAt(i)

                if(item&&item.visible){
                    obj["id"] = item.colorId
                    obj["type"] = item.gcodeType
                    obj["color"] = item.gcodeColor
                    obj["boxId"] = item.boxId
                    obj["materialId"] = item.materialId
                    arr.push(obj)
                    obj = null
                }
            }
        }
        if(uploadDlgType === 1)
        {
            printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".3mf", project_kernel.saveTempGCode3mf(), multiPrintId.firstSlice+1, arr);
        }else{
            if(kernel_slice_flow.isPreviewGCodeFile){
                printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".gcode", kernel_slice_flow.currentGCodeImageFileName(), false, arr);
            }else{
                var curGcodeFile = kernel_slice_flow.slicePreviewListModel.getGcodeFileName(singlePlate.currentIndex)
                printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".gcode", curGcodeFile, false, arr);
            }
        }

        onePrintId.clicked = true;
        if(printerMsgTip.visible){
            printerMsgTimer.stop()
            printerMsgTimer.start()
        }else { printerMsgTip.visible = true }
        printerMsgTipContent.text = qsTr("The printer is printing...")
    }
    function findNumberForMultipleOfThree(n) {
        let sum = 0;
        let temp = n;
        while (temp !== 0) {
            sum += temp % 10;
            temp = Math.floor(temp / 10);
        }
        let m = 3 - (sum % 3);
        if (m === 3) {
            m = 0;
        }
        return m;
    }
    function getEmptyModel(){
        let count=root.rowCount;
        if(count===1) return 0
        if(count<=6&&count>1){
            return 6 - count
        }else{
            return root.findNumberForMultipleOfThree(count)
        }
    }

    // color: "transparent"
    width: 793* screenScaleFactor
    //    height: 700* screenScaleFactor //+ (materialMap.count > 4 ? 100 : 0)
    // modality: Qt.ApplicationModal
    // flags: Qt.FramelessWindowHint | Qt.Dialog
    property int rowCount

    title: titleText
    maxBtnVis: false
    onVisibleChanged: {
        if (visible) {
            var usedIndexs = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(kernel_printer_manager.selectedPrinterIndex);
            if(currentObject&&currentObject.materialBoxList)
                initColorMap(currentObject.materialBoxList,usedIndexs);
            curWindowState = UploadGcodeDlg.WindowState.State_Normal;
            rowCount =  kernel_slice_flow.slicePreviewListModel.rowCount()
            imgPreview.source = "";
            imgPreview.source = "file:///" + kernel_slice_flow.gcodeThumbnail();

            idGcodeFileName.forceActiveFocus();
            enableCfs.checked = true
            materialMap.model=null
            if(kernel_slice_flow.isPreviewGCodeFile)
                materialMap.model= kernel_slice_flow.modelInsideColors
            else
                materialMap.model= kernel_parameter_manager.currentMachineObject.extrudersModel
            control.currentIndex = -1
            control.currentIndex = printerListModel.cSourceModel.getSyncPrinter()
            console.log("materialMapError"+control.currentIndex)
            empty_repeater.model = getEmptyModel()
            printer_repeater.model = null
            printer_repeater.model = Qt.binding(function(){return kernel_slice_flow.slicePreviewListModel;})

            root.modelInsideUseExtruderIndex = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(kernel_printer_manager.selectedPrinterIndex)
            if(currentObject&&currentObject.materialBoxList)
                initColorMap(currentObject.materialBoxList,root.modelInsideUseExtruderIndex);
            if(!kernel_slice_flow.isPreviewGCodeFile)
            {
                if(kernel_printer_manager.selectedPrinter.isSliced){
                    multiPrintId.firstSlice = kernel_printer_manager.selectedPrinterIndex
                }else{
                    let slicedPrinters = kernel_printer_manager.slicedPrinters
                    multiPrintId.firstSlice = slicedPrinters[0]
                }
            }

            singlePlate.currentIndex = kernel_printer_manager.selectedPrinterIndex



            root.materialMapError()
        }
    }
    function materialMapError(){
        if(!currentObject)
        {
            return;
        }
        let model = currentObject.materialBoxList
        if(!model)
            return;
        root.currentPrinterNotMap = false
        if(!enableCfs.checked)
            return;
        for(let i=0;i<materialMap.count; i++){
            let item = materialMap.itemAt(i)
            if(!item.visible) continue;
            console.log("materialMapErrorX"+item.cindex)
            if(item.materialId === ""){
                console.log("materialMapErrorY"+item.materialId)
                root.currentPrinterNotMap = true
                break
            }
        }
        console.log("materialMapError"+root.currentPrinterNotMap)
    }
    onCurWindowStateChanged: {
        width = curWindowState ? windowMinWidth : windowMaxWidth;
        height = (curWindowState ? windowMinHeight : windowMaxHeight)
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
            root.uploadGcodeProgress(progress);
        }
    }

    Item {
        id: rect
        x: 0
        y: titleHeight
        width: parent.width
        height: parent.height - titleHeight

        Item {
            width: parent.width
            height: parent.height - titleHeight
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: forceActiveFocus()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 20 * screenScaleFactor
                anchors.leftMargin: 20 * screenScaleFactor
                anchors.rightMargin: 20 * screenScaleFactor
                anchors.bottomMargin: 20 * screenScaleFactor
                //                spacing: 20 * screenScaleFactor
                visible: curWindowState === UploadGcodeDlg.WindowState.State_Normal

                RowLayout{
                    Layout.fillWidth: true
                    RowLayout {
                        Layout.preferredHeight: 20 * screenScaleFactor
                        Layout.preferredWidth: idGcodeFileName.width + 20 * screenScaleFactor

                        TextField {
                            id: idGcodeFileName

                            property var editGcodeReadOnly: true

                            readOnly: editGcodeReadOnly
                            background: null
                            Layout.preferredWidth: contentWidth + 30 * screenScaleFactor
                            Layout.preferredHeight: 30 * screenScaleFactor
                            //text: kernel_slice_flow.getExportName()
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

                            onTextEdited: {
                                if(root.visible)
                                {
                                    kernel_printer_manager.selectedPrinter.slicedPlateName = idGcodeFileName.text
                                    kernel_slice_flow.setExportName(idGcodeFileName.text,uploadDlgType)
                                }
                            }

                            onEditingFinished:{
                                editGcodeReadOnly = true
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
                    Item{
                        Layout.fillWidth:true
                    }

                    RowLayout {
                        Layout.rightMargin:10 * screenScaleFactor
                        Layout.alignment:Qt.AlignRight
                        spacing: 8 * screenScaleFactor

                        Image {
                            Layout.preferredWidth: 14 * screenScaleFactor
                            Layout.preferredHeight: 14 * screenScaleFactor
                            Layout.alignment: Qt.AlignVCenter
                            source: Constants.currentTheme ? "qrc:/UI/photo/printer_name_light.svg" : "qrc:/UI/photo/printer_name_dark.svg"
                        }

                        Text {
                            Layout.maximumWidth: 150 * screenScaleFactor
                            elide: Text.ElideMiddle
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            color: textColor
                            text: kernel_parameter_manager.currentMachineObject.name()
                        }

                    }

                }

                Rectangle {
                    id:thumbMsg
                    Layout.preferredWidth: 694 * screenScaleFactor
                    Layout.preferredHeight: 340 * screenScaleFactor
                    color: Constants.currentTheme ? "#FFFFFF" : uploadDlgType ===1?backgroundColor :"#424244"
                    border.color: Constants.currentTheme ? "#DDDDE1" : "transparent"
                    border.width: Constants.currentTheme ? 1 : 0
                    radius: 5

                    ColumnLayout {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.bottomMargin: 10 * screenScaleFactor
                        anchors.leftMargin: 10 * screenScaleFactor
                        spacing: 10 * screenScaleFactor

                        RowLayout {
                            spacing: 8 * screenScaleFactor

                            Image {
                                width: 14 * screenScaleFactor
                                height: 14 * screenScaleFactor
                                Layout.alignment: Qt.AlignVCenter
                                source: Constants.currentTheme ? "qrc:/UI/photo/printer_name_light.svg" : "qrc:/UI/photo/printer_name_dark.svg"
                            }

                            Text {
                                Layout.preferredWidth: 160 * screenScaleFactor
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                color: textColor
                                elide: Text.ElideRight
                                text: kernel_parameter_manager.currentMachineObject.name()
                            }

                        }
                        visible:(uploadDlgType === 0&&rowCount === 1) || uploadDlgType === 4 || (uploadDlgType === 0&&kernel_slice_flow.isPreviewGCodeFile)

                        Row {
                            spacing: 8 * screenScaleFactor

                            Image {
                                width: 14 * screenScaleFactor
                                height: 14 * screenScaleFactor
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
                    SwipeView{
                        id:singlePlate
                        currentIndex:0
                        anchors.fill:parent
                        anchors.centerIn: parent
                        clip: true
                        visible:rowCount>1 && uploadDlgType === 0 && !kernel_slice_flow.isPreviewGCodeFile
                        onCurrentIndexChanged:{
                            if(!root.visible)
                            {
                                return;
                            }
                            kernel_printer_manager.selectPrinter(currentIndex)
                            // kernel_slice_flow.sliceIndex = currentIndex
                            //idGcodeFileName.text = kernel_slice_flow.getExportName();
                            var usedIndexs = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(currentIndex);
                            if(currentObject.materialBoxList)
                                initColorMap(currentObject.materialBoxList,usedIndexs);
                            root.modelInsideUseExtruderIndex = usedIndexs


                        }
                        property bool curIsSliced:true
                        Repeater{
                            model:kernel_slice_flow.slicePreviewListModel
                            delegate:Item{
                                width:singlePlate.width
                                height:singlePlate.height
                                Image {
                                    property string imgDefault: no_slice
                                    cache: false
                                    source:isSliced?model.imageSource:imgDefault
                                    anchors.centerIn: parent
                                    width:300* screenScaleFactor
                                    height:300* screenScaleFactor
                                }
                                Column {
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left
                                    anchors.bottomMargin: 10 * screenScaleFactor
                                    anchors.leftMargin: 10 * screenScaleFactor
                                    spacing: 10 * screenScaleFactor

                                    Row {
                                        visible:isSliced
                                        spacing: 8 * screenScaleFactor
                                        Image {
                                            width: 14 * screenScaleFactor
                                            height: 14 * screenScaleFactor
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
                                        visible:isSliced
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
                            }
                        }
                    }
                    Rectangle{
                        property bool isHover:false
                        width:60* screenScaleFactor
                        height:width
                        radius:height/2
                        color:isHover? Qt.rgba(0,0,0,0.1):"transparent"
                        anchors.verticalCenter:parent.verticalCenter
                        anchors.left:parent.left
                        anchors.leftMargin:40* screenScaleFactor
                        visible:singlePlate.visible
                        Image{
                            anchors.centerIn:parent
                            width:12 * screenScaleFactor
                            height:24 * screenScaleFactor
                            source:pre_arrow
                        }
                        MouseArea{
                            hoverEnabled: true
                            onEntered:parent.isHover = true
                            onExited:parent.isHover = false
                            anchors.fill:parent
                            onClicked:{
                                let slicedPrinters = kernel_printer_manager.slicedPrinters
                                let curIndex = slicedPrinters.indexOf(singlePlate.currentIndex)
                                if(curIndex === 0){
                                    singlePlate.currentIndex = slicedPrinters[slicedPrinters.length -1]
                                }else{
                                    singlePlate.currentIndex = slicedPrinters[curIndex-1]
                                }
                            }
                        }
                    }
                    Rectangle{
                        property bool isHover:false
                        width:60 * screenScaleFactor
                        height:width
                        radius:height/2
                        color:isHover? Qt.rgba(0,0,0,0.1):"transparent"
                        anchors.verticalCenter:parent.verticalCenter
                        anchors.right:parent.right
                        anchors.rightMargin:40 * screenScaleFactor
                        visible:singlePlate.visible
                        Image{
                            anchors.centerIn:parent
                            width:12 * screenScaleFactor
                            height:24 * screenScaleFactor
                            source:next_arrow
                        }
                        MouseArea{
                            hoverEnabled: true
                            anchors.fill:parent
                            onEntered:parent.isHover = true
                            onExited:parent.isHover = false
                            onClicked:{
                                let slicedPrinters = kernel_printer_manager.slicedPrinters
                                let curIndex = slicedPrinters.indexOf(singlePlate.currentIndex)
                                if(curIndex === slicedPrinters.length -1){
                                    singlePlate.currentIndex = slicedPrinters[0]
                                }else{
                                    singlePlate.currentIndex = slicedPrinters[curIndex+1]
                                }
                            }
                        }
                    }


                    Text {
                        text:  singlePlate.currentIndex+1 +"/"+root.rowCount
                        color: textColor
                        font.weight: Font.ExtraBold
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_10
                        anchors.right: parent.right
                        anchors.rightMargin: 40* screenScaleFactor
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 10* screenScaleFactor
                        visible:singlePlate.visible
                        width:30* screenScaleFactor
                    }



                    ScrollView{
                        id:multiPrintId
                        width: parent.width
                        height: 324* screenScaleFactor
                        ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        clip:true
                        visible:uploadDlgType === 1
                        property int firstSlice
                        Flow{
                            id:flowId
                            spacing: 10* screenScaleFactor
                            width: 674* screenScaleFactor
                            Repeater{
                                id: printer_repeater
                                model: kernel_slice_flow.slicePreviewListModel
                                delegate: Rectangle{
                                    id: idSliceModelItem
                                    readonly property var modelItem: model
                                    property bool isHover:false
                                    width:  218* screenScaleFactor
                                    height:  152* screenScaleFactor
                                    radius: 5* screenScaleFactor
                                    color:  Constants.currentTheme ? "#FFFFFF" : "#424244"
                                    border.width:model.isSliced?1:0
                                    border.color: isHover&&(multiPrintId.firstSlice != model.index)?"#FFFFFF":(multiPrintId.firstSlice == model.index)? Constants.themeGreenColor: "transparent"
                                    MouseArea{
                                        hoverEnabled: true
                                        anchors.fill:parent
                                        onEntered:parent.isHover = true
                                        onExited:parent.isHover = false
                                        enabled: model.isSliced
                                        onClicked: {

                                            var usedIndexs = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(model.index)
                                            if(currentObject.materialBoxList)
                                                initColorMap(currentObject.materialBoxList,usedIndexs);
                                            root.modelInsideUseExtruderIndex= usedIndexs;
                                            multiPrintId.firstSlice = model.index
                                            console.log("modelInsideUseExtruderIndex:",root.modelInsideUseExtruderIndex)
                                        }
                                    }

                                    Image {
                                        anchors.centerIn:parent
                                        cache: false
                                        width:90* screenScaleFactor
                                        height:80* screenScaleFactor
                                        fillMode: model.isEmpty ? Image.Pad : Image.PreserveAspectFit
                                        source: model.isSliced ? model.imageSource : logo
                                    }

                                    Text {
                                        visible: model.isSliced
                                        text: model.index + 1
                                        color: textColor
                                        font.weight: Font.ExtraBold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_10
                                        anchors.left: parent.left
                                        anchors.leftMargin: 10* screenScaleFactor
                                        anchors.top: parent.top
                                        anchors.topMargin: 10* screenScaleFactor
                                    }
                                    Column {
                                        visible: model.isSliced
                                        anchors.bottom: parent.bottom
                                        anchors.left: parent.left
                                        anchors.bottomMargin: 10 * screenScaleFactor
                                        anchors.leftMargin: 10 * screenScaleFactor
                                        //  spacing: 5 * screenScaleFactor

                                        Row {
                                            spacing: 5 * screenScaleFactor

                                            Image {
                                                width: 14 * screenScaleFactor
                                                height: 14 * screenScaleFactor
                                                anchors.verticalCenter: parent.verticalCenter
                                                source: Constants.currentTheme ? "qrc:/UI/photo/print_time_light.svg" : "qrc:/UI/photo/print_time_dark.svg"
                                            }

                                            Text {
                                                width: 16 * screenScaleFactor
                                                font.weight: Font.Medium
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_8
                                                horizontalAlignment: Text.AlignLeft
                                                verticalAlignment: Text.AlignVCenter
                                                color: textColor
                                                text: model.printTime
                                            }

                                        }

                                        Row {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 2 * screenScaleFactor
                                            spacing: 5 * screenScaleFactor

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
                                                font.pointSize: Constants.labelFontPointSize_8
                                                horizontalAlignment: Text.AlignLeft
                                                verticalAlignment: Text.AlignVCenter
                                                color: textColor
                                                text: model.materialWeight + "g"
                                            }

                                        }
                                    }
                                }
                            }
                            Repeater{
                                id: empty_repeater
                                model:{}
                                delegate: Rectangle{
                                    width: 218* screenScaleFactor
                                    height: 152* screenScaleFactor
                                    radius: 5* screenScaleFactor
                                    color:  Constants.currentTheme ? "#FFFFFF" : "#424244"
                                    Image {
                                        anchors.centerIn:parent
                                        width:90* screenScaleFactor
                                        height:80* screenScaleFactor
                                        fillMode: Image.PreserveAspectFit
                                        source: logo
                                    }
                                }
                            }

                        }
                    }


                    Image {
                        id: imgPreview
                        visible:(rowCount==1&&uploadDlgType===0) || uploadDlgType === 4 || (uploadDlgType===0&&kernel_slice_flow.isPreviewGCodeFile)
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

                Popup{
                    id:materialMapNote
                    width: 424 * screenScaleFactor
                    height: 381 * screenScaleFactor
                    visible: false
                    background: Rectangle {
                        color: root.backgroundColor
                        border.color: root.borderColor
                        border.width: 1 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                    }
                    closePolicy: Popup.CloseOnPressOutsideParent
                    x:-200 * screenScaleFactor
                    y:0* screenScaleFactor
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color:textColor
                        text: qsTr("Supplies used in this print job")
                        anchors.left:parent.left
                        anchors.leftMargin: 200 * screenScaleFactor
                        anchors.top: parent.top
                        anchors.topMargin: 15 * screenScaleFactor
                    }
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color:textColor
                        text: qsTr("CFS slot for this consumable")
                        anchors.left:parent.left
                        anchors.leftMargin: 200 * screenScaleFactor
                        anchors.top: parent.top
                        anchors.topMargin: 45 * screenScaleFactor
                    }
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color:textColor
                        text: qsTr("Need to click manually to specify the CFS slot")
                        anchors.left:parent.left
                        anchors.leftMargin: 120 * screenScaleFactor
                        anchors.top: parent.top
                        anchors.topMargin: 95 * screenScaleFactor
                    }
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color:textColor
                        text: qsTr("The slot has consumables but is not edited.")
                        anchors.left:parent.left
                        anchors.leftMargin: 120 * screenScaleFactor
                        anchors.top: parent.top
                        anchors.topMargin: 240 * screenScaleFactor
                    }
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        color:textColor
                        text: qsTr("Empty slot")
                        anchors.left:parent.left
                        anchors.leftMargin: 120 * screenScaleFactor
                        anchors.top: parent.top
                        anchors.topMargin: 290 * screenScaleFactor
                    }
                    ColumnLayout{
                        Layout.leftMargin: 20 * screenScaleFactor
                        Text {
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            color:Constants.textColor
                            text: qsTr("Material Selection Instructions")
                        }
                        Image{
                            visible:materialList.visible
                            width: 389 * screenScaleFactor
                            height: 301 * screenScaleFactor
                            source: "qrc:/UI/photo/upload3mf/materialMapNote.png"
                            sourceSize:Qt.size(389,301)
                        }
                    }
                }

                Row{
                    Layout.alignment: Qt.AlignLeft
                    Layout.topMargin: 10 * screenScaleFactor
                    spacing: 10 * screenScaleFactor
                    Image{
                        visible:materialList.visible&&enableCfs.checked
                        width: 18
                        height: 18
                        anchors.verticalCenter: parent.verticalCenter
                        source: materialMapNote.visible? "qrc:/UI/photo/upload3mf/materialMapTip_hover.svg" : "qrc:/UI/photo/upload3mf/materialMapTip_default.svg"
                        MouseArea{
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: materialMapNote.visible = true
                            onExited: materialMapNote.visible = false
                        }
                    }

                    ScrollView {
                        id:materialList
                        clip:true
                        visible:!upload_cloud && currentObject&&currentObject.materialBoxList && +currentObject.cfsConnect === 1
                        width:674 * screenScaleFactor
                        height: {
                            let visibleNum = 0;
                            for(let index = 0; index < materialMap.count; ++index){
                                let isVisible = root.modelInsideUseExtruderIndex.includes(model.index.toString())
                                if(isVisible)
                                    ++visibleNum;
                            }
                            return visibleNum <= 3 ? 50 * screenScaleFactor : 110 * screenScaleFactor
                        }
                        ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff


                        Grid {
                            id: materialColorFlow
                            width:  parent.width
                            columns: 4
                            spacing: 13* screenScaleFactor
                            onVisibleChanged:{
                                if(visible)
                                {
                                    if(kernel_slice_flow.isPreviewGCodeFile)
                                    {
                                        materialMap.model= kernel_slice_flow.modelInsideColors
                                    }else{
                                        materialMap.model= kernel_parameter_manager.currentMachineObject.extrudersModel
                                    }

                                }else{
                                    materialMap.model=null
                                }

                            }

                            Repeater {
                                id:materialMap
                                property var isAvailableMachine : currentObject.pcPrinterStatus!==1
                                model: kernel_parameter_manager.currentMachineObject.extrudersModel//kernel_slice_model.extruderTableModel
                                onIsAvailableMachineChanged:{
                                    if(isAvailableMachine)
                                    {
                                        var cIndex = control.currentIndex
                                        control.currentIndex = -1
                                        control.currentIndex = cIndex
                                    }
                                }
                                delegate: RowLayout {
                                    visible:root.modelInsideUseExtruderIndex.includes(model.index.toString())
                                    id:materialMapItem
                                    property var colorMsg:{}
                                    property string colorId : "T" + Math.floor(index/1.0/4 +1) + String.fromCharCode(65+index%4)
                                    property string gcodeType: ""
                                    property string gcodeColor: ""
                                    property string boxId: ""
                                    property string materialId: ""
                                    property var cindex: index
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
                                        text: kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getTypeByColorIndex(index):model.curMaterialObj.materialType // kernel_parameter_manager.currentMachineObject.extrudersModel.get(model.ctModel.getMaterialIndex()).curMaterialObj.materialType
                                        btnRadius: height / 2
                                        defaultBtnBgColor: kernel_slice_flow.isPreviewGCodeFile?modelData:model.extruderColor// model.ctModel.getMaterialColor()
                                        hoveredBtnBgColor: defaultBtnBgColor
                                        btnBorderW: 0
                                        btnTextColor: parent.setTextColor(defaultBtnBgColor)
                                        Component.onCompleted:{
                                            parent.gcodeType = text
                                            parent.gcodeColor = defaultBtnBgColor
                                        }
                                    }

                                    Canvas {
                                        visible:enableCfs.checked && currentObject
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
                                        visible:enableCfs.checked && currentObject
                                        id:materialComboBoxId
                                        width:70 * screenScaleFactor
                                        height:43* screenScaleFactor
                                        pcPrinterID: currentObject.pcPrinterID
                                        mapGcodeType:curMaterialColor.text
                                        materialmodel:currentObject.materialBoxList
                                        curIndex: -1
                                        enabled: !materialMap.isAvailableMachine
                                        onCurIndexChanged:{
                                            if(curIndex===-1) return
                                            parent.boxId = materialmodel.getMaterialBoxId(pcPrinterID,curIndex)
                                            parent.materialId = materialmodel.getMaterialId(pcPrinterID,curIndex)
                                            console.log("materialComboBoxId.curIndex:",curIndex)
                                            root.materialMapError()
                                        }
                                        onVisibleChanged:{
                                            if(visible){
                                                if(curIndex===-1) return
                                                parent.boxId = materialmodel.getMaterialBoxId(pcPrinterID,curIndex)
                                                parent.materialId = materialmodel.getMaterialId(pcPrinterID,curIndex)
                                                let model = currentObject.materialBoxList
                                                if(model)
                                                    materialComboBoxId.curIndex = model.getCloseColorByIndex(materialMapItem.cindex)
                                                //materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor,curMaterialColor.text)
                                                console.log("materialComboBoxId.onVisibleChanged:",visible)
                                            }
                                        }
                                        Connections{
                                            target:materialMap
                                            onIsAvailableMachineChanged:{
                                                if(materialMap.isAvailableMachine)
                                                {
                                                    materialComboBoxId.curIndex = -1
                                                }else{
                                                    let model = currentObject.materialBoxList
                                                    materialComboBoxId.curIndex = model.getCloseColorByIndex(materialMapItem.cindex)
                                                    //materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor,curMaterialColor.text)
                                                }
                                            }
                                        }
                                        Connections{
                                            target:multiPrintId
                                            function onFirstSliceChanged(){
                                                materialComboBoxId.curIndex = -1
                                                let model = currentObject.materialBoxList
                                                if(model)
                                                    materialComboBoxId.curIndex = model.getCloseColorByIndex(materialMapItem.cindex)
                                                //materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor,curMaterialColor.text)
                                                if(materialMap.isAvailableMachine)
                                                {
                                                    materialComboBoxId.curIndex = -1
                                                }
                                                if(materialComboBoxId.curIndex==-1)
                                                {
                                                    materialComboBoxId.parent.materialId = ""
                                                }
                                                console.log("singlePlate:",materialComboBoxId.curIndex)
                                                root.materialMapError()
                                            }
                                        }
                                        Connections{
                                            target:singlePlate
                                            function onCurrentIndexChanged(){
                                                materialComboBoxId.curIndex = -1
                                                let model = currentObject.materialBoxList
                                                if(model)
                                                    materialComboBoxId.curIndex = model.getCloseColorByIndex(materialMapItem.cindex)
                                                //materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor,curMaterialColor.text)
                                                if(materialMap.isAvailableMachine)
                                                {
                                                    materialComboBoxId.curIndex = -1
                                                }
                                                if(materialComboBoxId.curIndex==-1)
                                                {
                                                    materialComboBoxId.parent.materialId = ""
                                                }
                                                console.log("singlePlate:",materialComboBoxId.curIndex)
                                                root.materialMapError()
                                            }
                                        }
                                        Connections{
                                            target:control
                                            function onCurrentIndexChanged(){
                                                materialComboBoxId.curIndex = -1
                                                if(currentObject&&currentObject.materialBoxList)
                                                    materialComboBoxId.curIndex = currentObject.materialBoxList.getCloseColorByIndex(materialMapItem.cindex)
                                                //materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor,curMaterialColor.text)
                                                if(materialMap.isAvailableMachine)
                                                {
                                                    materialComboBoxId.curIndex = -1
                                                }
                                                if(materialComboBoxId.curIndex==-1)
                                                {
                                                    materialComboBoxId.parent.materialId = ""
                                                }
                                                console.log("materialComboBoxId.curIndex:",materialMapItem.cindex)
                                                console.log("materialComboBoxId.materialId:",materialComboBoxId.parent.materialId)
                                                root.materialMapError()
                                            }
                                        }
                                    }
                                }

                            }

                        }
                    }
                }
                ColumnLayout{
                    visible:!upload_cloud
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 45 * screenScaleFactor
                        color: "#FD265A"
                        text:qsTr("The current printer is offline, please connect it before printing")
                        visible: !!control.currentText && currentObject.pcPrinterStatus!==1
                    }

                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 45 * screenScaleFactor
                        color: "#FD265A"
                        text:qsTr("The current printer is printing and only supports sending Gcode")
                        visible: !!control.currentText && currentObject.pcPrinterStatus===1 && currentObject.pcPrinterState === 1
                    }

                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 45 * screenScaleFactor
                        color: "#FD265A"
                        text:qsTr("The currently selected printer does not support multi-plate printing")
                        visible: !currentMachineSupportMutilPlate
                    }

                    RowLayout {
                        id: machineIp
                        visible:!upload_cloud
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

                        ComboBox {

                            id: control

                            property var popHeight:  28 * screenScaleFactor

                            model: printerListModel.cSourceModel
                            enabled: currentObject
                            Layout.preferredWidth:  430 * screenScaleFactor // errorLayout.fatalErrorCode?: 624 * screenScaleFactor
                            Layout.preferredHeight: 30 * screenScaleFactor
                            textRole: "ipAddr"

                            function setPrinterStatus(printerStatus,printerState) {
                                if (printerStatus !== 1)
                                {
                                    console.log("printerStatus:",printerStatus)
                                    return qsTr("Offline");
                                }


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
                                case 6:
                                    return qsTr("Pausing");
                                case 7:
                                    return qsTr("Stopping");
                                case 8:
                                    return qsTr("Restoring");
                                }
                            }
                            onCurrentIndexChanged:{
                                var curObject = printerListModel.cSourceModel.getDeviceObject(currentIndex)
                                console.log("onCurrentIndexChanged:",currentIndex)
                                let id = curObject.pcPrinterID
                                let type = curObject.printerType
                                if(curObject.materialBoxList)
                                {
                                    curObject.materialBoxList.switchDataSource(id,type)
                                    enableCfs.checked = true
                                }else{
                                    enableCfs.checked = false
                                }
                                var usedIndexs;
                                if(uploadDlgType === 0)
                                {
                                    usedIndexs = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(kernel_printer_manager.selectedPrinterIndex);
                                }else{
                                    usedIndexs = kernel_slice_flow.isPreviewGCodeFile?kernel_slice_flow.getModelInsideUseColors():kernel_slice_flow.slicePreviewListModel.getModelInsideUseColors(multiPrintId.firstSlice);
                                }
                                if(curObject.materialBoxList)
                                    initColorMap(curObject.materialBoxList,usedIndexs);
                                console.log("currentObject:",currentIndex)

                            }
                            background: CusRoundedBg {
                                borderColor: (control.popup.visible || control.hovered) ?  Constants.themeGreenColor: Constants.right_panel_border_default_color
                                borderWidth: 1
                                allRadius: true
                                color: "transparent"
                                radius: 5
                            }
                            popup: Popup {
                                id: popCom
                                y: control.height
                                width: control.width
                                horizontalPadding: 1
                                verticalPadding: 2* screenScaleFactor
                                implicitHeight:  __col.height + 5* screenScaleFactor
                                contentItem:Item{
                                    Column {
                                        id : __col
                                        width: parent.width
                                        spacing: 3* screenScaleFactor
                                        ListView {
                                            id: listview
                                            clip: true
                                            width: parent.width
                                            implicitHeight: contentHeight > 7*control.popHeight ? 7*control.popHeight : contentHeight
                                            model: control.popup.visible ? control.delegateModel : null
                                            currentIndex: control.highlightedIndex


                                            ScrollBar.vertical: ScrollBar {
                                                id :box_bar
                                                implicitWidth: 10 * screenScaleFactor
                                                visible: control.delegateModel ? (control.delegateModel.count > 7) : false
                                                contentItem: Rectangle {
                                                    implicitWidth:10 * screenScaleFactor
                                                    color: box_bar.pressed
                                                           ? Qt.rgba(0.6,0.6,0.6)
                                                           : Qt.rgba(0.6,0.6,0.6,0.5)
                                                }
                                            }
                                        }
                                    }
                                }
                                background: Rectangle {
                                    border.width: 1
                                    border.color: control.popup.visible? Constants.themeGreenColor: Constants.right_panel_border_default_color
                                    color: Constants.right_panel_item_default_color
                                    radius: 5* screenScaleFactor
                                }

                            }
                            contentItem:RowLayout{
                                anchors.fill:parent

                                property string printerName: currentObject.deviceName?currentObject.deviceName: kernel_add_printer_model.getModelNameByCodeName(currentObject.pcPrinterModel)
                                Text {
                                    leftPadding: 10 * screenScaleFactor
                                    verticalAlignment:  Qt.AlignVCenter
                                    horizontalAlignment: Qt.AlignLeft
                                    elide: Text.ElideRight
                                    color: Constants.right_panel_text_default_color
                                    font: Constants.font
                                    text:  currentObject?(parent.printerName? (parent.printerName + "/"+currentObject.pcIpAddr) : currentObject.pcIpAddr):""
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
                                    text: currentObject?control.setPrinterStatus(currentObject.pcPrinterStatus, currentObject.pcPrinterState):""
                                }

                            }
                            indicator:Item
                            {
                                anchors.right: control.right
                                anchors.rightMargin: 5
                                implicitHeight: 16 * screenScaleFactor
                                implicitWidth: 16 * screenScaleFactor
                                anchors.verticalCenter: control.verticalCenter
                                Image {
                                    width: sourceSize.width* screenScaleFactor
                                    height: sourceSize.height* screenScaleFactor
                                    anchors.centerIn: parent
                                    source:  control.down || control.hovered ? Constants.downBtnImgSource_d :  Constants.downBtnImgSource
                                    opacity: enabled ? 1 : 0.3
                                    fillMode: Image.Pad
                                    visible: control.width > 20
                                }
                            }
                            delegate: ItemDelegate {
                                property bool currentItem: control.highlightedIndex === index

                                width: control.width
                                height: control.popHeight
                                hoverEnabled: control.hoverEnabled

                                contentItem: Rectangle {

                                    anchors.fill: parent
                                    color: control.highlightedIndex === index ?  Constants.themeGreenColor : Constants.right_panel_item_default_color

                                    Text {
                                        id: myText
                                        property string printerName: mItem.deviceName ? mItem.deviceName : kernel_add_printer_model.getModelNameByCodeName(mItem.pcPrinterModel)
                                        x: 5 * screenScaleFactor
                                        height: control.popHeight
                                        width: parent.width - 100 * screenScaleFactor
                                        text: (printerName? (printerName + "/") : "" )+ ipAddr
                                        color:  Constants.right_panel_text_default_color
                                        font.family: Constants.labelFontFamily
                                        elide: Text.ElideMiddle
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Text {
                                        id: name

                                        text: control.setPrinterStatus(printerStatus,printerState)
                                        color:  Constants.right_panel_text_default_color
                                        height: control.popHeight
                                        font.family: Constants.labelFontFamily
                                        verticalAlignment: Text.AlignVCenter
                                        anchors.right: parent.right
                                        anchors.rightMargin: 50 * screenScaleFactor
                                    }

                                }

                            }

                        }

                        RowLayout {
                            id: errorLayout
                            property var deviceItem: printerListModel.cSourceModel.getDeviceItemByIp(control.currentText)
                            property int errorCode:deviceItem.errorCode
                            property var ipAddr:  currentObject.pcIpAddr
                            property string errorMessage: printerListModel.cSourceModel.getPinterErrorMsg(control.currentText)
                            property string errorKey: deviceItem.errorKey//Qt.binding(function (){ return printerListModel.cSourceModel.getPinterErrorKey(control.currentText); })
                            property string printerType: printerListModel.cSourceModel.getPrinterType(control.currentText)
                            property bool fatalErrorCode: (errorCode >= 1 && errorCode <= 100) || errorCode == 20000 || errorCode == 20010
                            property bool isRepoPlrStatus: errorCode == 20000
                            property bool isMaterialStatus: errorCode == 20010
                            property color displayTextColor: fatalErrorCode ? "#FD265A" : (Constants.currentTheme ? "#333333" : "#FCE100")

                            visible: errorCode != 0 && currentObject.pcPrinterStatus==1
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
                                        if (containsMouse && !errorTooltip.opened){
                                            errorTooltip.open();
                                        }else{
                                            errorTooltip.close()
                                        }

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

                                                    text:"["+ (!!ErrorData.errorJson[errorLayout.errorKey]?ErrorData.errorJson[errorLayout.errorKey].code:errorLayout.errorKey)+"]"
                                                    visible: !errorLayout.errorKey
                                                }

                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    verticalAlignment: Text.AlignVCenter
                                                    color: errorLayout.displayTextColor
                                                    text:ErrorData.errorJson[errorLayout.errorKey]?ErrorData.errorJson[errorLayout.errorKey].content: qsTr("Unknown exception")
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



                            ErrorMsgBtnState{
                                id: errorBtnState
                                errorCode: errorLayout.errorKey
                                errorLevel: ErrorData.errorJson[errorLayout.errorKey].level
                                machineState: currentObject.pcPrinterState
                                supportVersion: ErrorData.errorJson[errorLayout.errorKey] ? ErrorData.errorJson[errorLayout.errorKey].supportVersion : -1
                                Component.onCompleted: {
                                    items[0] = continueBtn
                                    items[1] = cancelBtn
                                    items[2] = idMachineRetryButton
                                    items[3] = okBtn
                                }
                            }

                            Button {
                                id: continueBtn
                                visible: false
                                Layout.preferredWidth: 73 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                //                                visible: ((errorLayout.errorCode >= 101 && errorLayout.errorCode <= 200) || errorLayout.isRepoPlrStatus ||
                                //                                          errorLayout.isMaterialStatus) && errorLayout.errorCode == 115
                                onClicked: {
                                    if (errorLayout.isRepoPlrStatus) {
                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_REPOPLRSTATUS, "1", errorLayout.printerType);
                                    } else {
                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, LanPrinterListLocal.PrintControlType.PRINT_CONTINUE, " ", errorLayout.printerType);
                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", errorLayout.printerType);
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
                                    text: qsTranslate("LanPrinterDetail", "Continue")
                                }

                            }

                            Button {
                                id:cancelBtn
                                visible: false
                                Layout.preferredWidth: 73 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                //                                visible: ((errorLayout.errorCode >= 101 && errorLayout.errorCode <= 200) || errorLayout.isRepoPlrStatus ||
                                //                                          errorLayout.isMaterialStatus) && errorLayout.errorCode == 115
                                onClicked: {
                                    __LANload.item.showMessage(LanPrinterDetail.MessageType.CancelBrokenMaterial, errorLayout.ipAddr, errorLayout.printerType)

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
                                id: idMachineRetryButton
                                visible: false
                                Layout.preferredWidth: 73 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor

                                onClicked: {
                                    if(errorLayout.ipAddr !== "")
                                    {
                                        printerListModel.cSourceModel.sendUIControlCmdToDevice(errorLayout.ipAddr, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RETRY_ERROR, "0", errorLayout.printerType)

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
                                    text: qsTranslate("LanPrinterListLocal", "Retry")
                                }

                            }
                            Button {
                                id: okBtn
                                visible: true
                                Layout.preferredWidth: 73 * screenScaleFactor
                                Layout.preferredHeight: 24 * screenScaleFactor
                                //                                visible: ((errorLayout.errorCode >= 1 && !(errorLayout.isRepoPlrStatus || errorLayout.isMaterialStatus))
                                //                                          || errorLayout.errorCode == 116) && errorLayout.errorCode != 115
                                onClicked: {
                                    if(errorLayout.ipAddr !== "")
                                    {
                                        if(idMachineRetryButton.visible)
                                        {
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_RETRY_ERROR, "1", errorLayout.printerType)
                                        }else{
                                            printerListModel.cSourceModel.sendUIControlCmdToDevice(ipAddress, LanPrinterListLocal.PrintControlType.CONTROL_CMD_CLEANERR, " ", errorLayout.printerType)
                                        }

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
                                    text: qsTranslate("LanPrinterDetail", "OK")
                                }

                            }

                        }

                    }
                    RowLayout{
                        id:checkPrint
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 50 * screenScaleFactor
                        property var disableCheck:currentObject.pcPrinterState === 1 || currentObject.pcPrinterState === 5
                        StyleCheckBox{
                            id:printerCalibration
                            Layout.preferredWidth:120 * screenScaleFactor
                            text: qsTr("Print calibration")
                            visible:!upload_cloud && currentObject.printerType == 3
                            checked:currentObject.enableSelfTest
                            enabled:!parent.disableCheck && currentObject
                        }
                        RowLayout{
                            visible: materialList.visible
                            spacing:10* screenScaleFactor
                            StyleCheckBox{
                                id:enableCfs
                                enabled:!checkPrint.disableCheck && currentObject
                                Layout.preferredWidth:80 * screenScaleFactor
                                text: qsTr("Enable CFS")
                                checked: true
                                onCheckedChanged: {
                                    root.materialMapError()
                                }
                            }
                            Image{
                                width: 18 * screenScaleFactor
                                height: 18 * screenScaleFactor
                                source: cfsNote.visible? "qrc:/UI/photo/upload3mf/materialMapTip_hover.svg" : "qrc:/UI/photo/upload3mf/materialMapTip_default.svg"
                                MouseArea{
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: cfsNote.visible = true
                                    onExited: cfsNote.visible = false
                                }
                            }
                        }

                    }
                    Popup{
                        id:cfsNote
                        width: 424 * screenScaleFactor
                        height: 256 * screenScaleFactor
                        visible: false
                        background: Rectangle {
                            color: root.backgroundColor
                            border.color: root.borderColor
                            border.width: 1 * screenScaleFactor
                            radius: 5* screenScaleFactor
                        }
                        y:35-height* screenScaleFactor
                        x:10* screenScaleFactor
                        closePolicy: Popup.CloseOnPressOutsideParent
                        RowLayout{

                            ColumnLayout{
                                Layout.preferredWidth:140* screenScaleFactor
                                Layout.leftMargin: 20* screenScaleFactor
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color:Constants.textColor
                                    text: qsTr("Enable CFS")
                                }
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_8
                                    color:textColor
                                    text: qsTr("Printing with consumables from CFS")
                                    Layout.preferredWidth:120* screenScaleFactor
                                }
                                Image{
                                    Layout.topMargin: 35* screenScaleFactor
                                    width: 130 * screenScaleFactor
                                    height: 125 * screenScaleFactor
                                    sourceSize: Qt.size(width,height)
                                    source: "qrc:/UI/photo/upload3mf/cfs.png"
                                }
                            }
                            Rectangle{
                                width: 1* screenScaleFactor
                                height: 160* screenScaleFactor
                                color:"#626266"
                                Layout.alignment: Qt.AlignHCenter
                                Layout.leftMargin: 40* screenScaleFactor
                                Layout.rightMargin: 20* screenScaleFactor
                                Layout.topMargin: 48* screenScaleFactor
                            }
                            ColumnLayout{
                                Layout.preferredWidth:180* screenScaleFactor
                                Layout.rightMargin: 20* screenScaleFactor
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_8
                                    color:Constants.textColor
                                    text: qsTr("Disable CFS")
                                }
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color:textColor
                                    text: qsTr("Printing using consumables plugged into the back of the machine")
                                    Layout.preferredWidth:180* screenScaleFactor
                                    wrapMode: Text.WordWrap

                                }
                                Image{
                                    Layout.topMargin: 35* screenScaleFactor
                                    width: 111 * screenScaleFactor
                                    height: 128 * screenScaleFactor
                                    sourceSize: Qt.size(width,height)
                                    source: "qrc:/UI/photo/upload3mf/rackMaterial.png"
                                }
                            }

                        }
                    }



                }
                Item {
                    visible:!upload_cloud
                    Layout.fillHeight: true
                }

                RowLayout {
                    id: btn
                    visible:!upload_cloud
                    // anchors.horizontalCenter: parent.horizontalCenter
                    property var progress: printerListModel.cSourceModel.data(printerListModel.cSourceModel.index(+control.currentIndex, 0),37).gCodeTransProgress
                    property bool statusDisable:  currentObject.pcPrinterStatus !== 1
                    property bool stateDisable: currentObject.pcPrinterState === 1 ||currentObject.pcPrinterState === 5
                    property var errorCode: +printerListModel.cSourceModel.getPinterErrorCode(control.currentText)
                    property var btnDisable: (btn.progress < 100 && btn.progress > 0) || statusDisable

                    Layout.preferredHeight: 30 * screenScaleFactor
                    Layout.preferredWidth: 340 * screenScaleFactor
                    Layout.alignment: Qt.AlignHCenter

                    ProgressBar {
                        id: sendId

                        property bool clicked: false
                        enabled: currentMachineSupportMutilPlate
                        Layout.preferredHeight: 30 * screenScaleFactor
                        Layout.preferredWidth: 100 * screenScaleFactor
                        from: 0
                        to: 100
                        value: btn.progress

                        background: BasicDialogButton {
                            Layout.preferredHeight: 30 * screenScaleFactor
                            Layout.preferredWidth: 100 * screenScaleFactor
                            text: qsTr("Send")
                            btnRadius: 15 * screenScaleFactor
                            btnBorderW: 1 * screenScaleFactor
                            defaultBtnBgColor: Constants.leftToolBtnColor_normal
                            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                            enabled: (uploadDlgType == 0? (kernel_slice_flow.slicePreviewListModel.isSliced(singlePlate.currentIndex)||kernel_slice_flow.isPreviewGCodeFile)&&!btn.btnDisable:!btn.btnDisable) && currentMachineSupportMutilPlate
                            onSigButtonClicked: {
                                printerListModel.cSourceModel.setOneSelected(control.currentText);
                                let filePrefixPath = currentObject.filePrefixPath
                                if(uploadDlgType === 1)
                                {
                                    var filename = project_kernel.saveTempGCode3mf()
                                    printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".3mf", filename, true);
                                }else{
                                    var curGcodeFile = kernel_slice_flow.slicePreviewListModel.getGcodeFileName(singlePlate.currentIndex)
                                    if(kernel_slice_flow.isPreviewGCodeFile)
                                    {
                                        curGcodeFile = kernel_slice_flow.currentGCodeImageFileName()
                                    }
                                    printerListModel.cSourceModel.batch_command(idGcodeFileName.text + ".gcode", curGcodeFile, true);
                                }

                                sendId.clicked = true;
                                if(printerMsgTip.visible){
                                    printerMsgTimer.stop()
                                    printerMsgTimer.start()
                                }else { printerMsgTip.visible = true }
                                printerMsgTipContent.text = qsTr("The printer is sending the file...")

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
                                text: btn.progress + "%"
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
                            text: qsTr("Print")
                            btnRadius: 15 * screenScaleFactor
                            btnBorderW: 1 * screenScaleFactor
                            defaultBtnBgColor: Constants.leftToolBtnColor_normal
                            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                            enabled: (uploadDlgType == 0?(!btn.btnDisable && !btn.stateDisable && btn.errorCode === 0 && ((!root.currentPrinterNotMap  && kernel_slice_flow.slicePreviewListModel.isSliced(singlePlate.currentIndex))||kernel_slice_flow.isPreviewGCodeFile)):!btn.btnDisable && !btn.stateDisable && btn.errorCode === 0 &&  !root.currentPrinterNotMap) && currentMachineSupportMutilPlate
                            onSigButtonClicked: {
                                let printerModelName = currentObject.pcPrinterModel
                                if(printerModelName!== kernel_parameter_manager.currentMachineObject.inheritBaseName){
                                    deviceMapId.visible = true
                                    return
                                }
                                root.devicePrintOps()
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
                                text: btn.progress + "%"
                                color: Constants.textColor
                            }

                        }
                    }

                    BasicDialogButton {
                        Layout.preferredHeight: 30 * screenScaleFactor
                        Layout.preferredWidth: 100 * screenScaleFactor
                        text: qsTr("Multi-machine control")
                        enabled: currentMachineSupportMutilPlate
                        btnRadius: 15 * screenScaleFactor
                        btnBorderW: 1 * screenScaleFactor
                        defaultBtnBgColor: Constants.leftToolBtnColor_normal
                        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                        onSigButtonClicked: {
                            gcodeFileName = idGcodeFileName.text;
                            gcodeFilePath = kernel_slice_flow.currentGCodeImageFileName();
                            sigConfirmBtnClicked();
                        }
                    }

                }


                RowLayout {
                    spacing: 10 * screenScaleFactor
                    Layout.alignment: Qt.AlignHCenter
                    visible:upload_cloud
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

                                    userInfoId.visible = true
                                    cloudContext.userInfoDialog.setPage(UserInfoDialog.Page.UPLOADED_SLICE);
                                    root.visible = false
                                }
                            }
                        }
                    }
                }
            }

        }

        UserInfoDialog{
            visible:false
            id:userInfoId
        }

        Timer{
            repeat:false
            interval:5000
            id:printerMsgTimer
            onTriggered: printerMsgTip.visible = false
        }


        Window {
            id:printerMsgTip
            width: 430 * screenScaleFactor
            height:67* screenScaleFactor
            x:10* screenScaleFactor
            y: Screen.height - height - 60* screenScaleFactor
            //  visible: pcPrinterStatus === 1 && pcPrinterState === 1
            flags: Qt.FramelessWindowHint
            color: "transparent"
            onVisibleChanged:{
                if(visible)printerMsgTimer.start()
            }
            Rectangle {
                color: Qt.rgba(0, 0, 0, 0.4)
                radius: 5* screenScaleFactor
                anchors.fill: parent
                RowLayout{
                    anchors.fill: parent
                    Layout.alignment:Qt.AlignVCenter
                    ColumnLayout{
                        Layout.leftMargin:15 * screenScaleFactor
                        RowLayout{
                            Image{
                                width:20 * screenScaleFactor
                                height:20 * screenScaleFactor
                                sourceSize:Qt.size(width,height)
                                source:"qrc:/UI/photo/tip.svg"
                                Layout.alignment:Qt.AlignVCenter | Qt.AlignRight
                            }
                            Text{
                                text:qsTr("Tip")+":"
                                color: Constants.themeGreenColor
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                            }
                            Text{
                                text: currentObject.deviceName+"/"+currentObject.pcIpAddr
                                color: "#DCDCDC"
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                            }
                        }
                        Text{
                            id: printerMsgTipContent
                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.leftMargin: 60 * screenScaleFactor
                            color: "#DCDCDC"
                        }
                    }
                    Image{
                        width:10 * screenScaleFactor
                        height:10 * screenScaleFactor
                        sourceSize:Qt.size(width,height)
                        source:"qrc:/UI/photo/closeBtn.svg"
                        Layout.rightMargin:10 * screenScaleFactor
                        Layout.alignment:Qt.AlignVCenter | Qt.AlignRight
                        MouseArea{
                            anchors.fill:parent
                            onClicked: printerMsgTip.visible = false
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }

        }

        UploadMessageDlg2 {
            id: deviceMapId
            property var receiver
            visible: false
            messageType:0
            cancelBtnVisible: true
            msgText: qsTr("The slicing machine model is inconsistent with the printer model. Inconsistency will cause deviations in the printing effect. Do you want to continue printing?")
            onSigOkButtonClicked:{
                root.devicePrintOps()
                deviceMapId.visible = false
            }
        }

        // layer.effect: DropShadow {
        //     radius: 8
        //     spread: 0.2
        //     samples: 17
        //     color: shadowColor
        // }

    }

}
