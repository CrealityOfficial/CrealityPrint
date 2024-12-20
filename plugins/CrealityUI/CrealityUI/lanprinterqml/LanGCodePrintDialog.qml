import "../components"
import "../qml"
import "../secondqml"
import "qrc:/CrealityUI/cxcloud"
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtQuick.Window 2.15

BasicDialogV4{
    id: root

    //0:Creality Cloud 1: LAN
    property int uploadDlgType: 1
    property real borderWidth: 1 * screenScaleFactor
    property real shadowWidth: 5 * screenScaleFactor
    property real titleHeight: 30 * screenScaleFactor
    property real borderRadius: 5 * screenScaleFactor
    property string titleText:  qsTranslate("CusControlTableView","Print")
    property string textColor: Constants.currentTheme ? "#333333" : "#CBCBCC"
    property string shadowColor: Constants.currentTheme ? "#BBBBBB" : "#333333"
    property string borderColor: Constants.currentTheme ? "#D6D6DC" : "#262626"
    property string titleFtColor: Constants.currentTheme ? "#333333" : "#FFFFFF"
    property string titleBgColor: Constants.currentTheme ? "#E9E9EF" : "#6E6E73"
    property string backgroundColor: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
    //Slice parameter
    property string printTime: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.printing_time() : ""
    property double materialWeight: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain.material_weight() : ""
    property string ipAddress: currentObject ? currentObject.pcIpAddr : ""
    property string modelItemImagePrefix: "http://" + ipAddress
    visible: true
    property var currentObject
    property string gcodePath: ""
    property int type: 0
    property var fileListModel
    property string gcodeFileName: ""
    property string gcodeImageSource:  ""
    property real gcodeVolumn: 0
    property bool currentPrinterNotMap:false
    signal dlgClose()
    modal : true
    Component.onCompleted: {
        
    }
    function devicePrintOps(){
        currentObject.enableSelfTest = printerCalibration.checked
        let materialBoxModel = currentObject.materialBoxList
        let arr = []
        if(materialBoxModel&&enableCfs.checked){
            for(let i=0;i<materialMap.count; i++){
                let obj = {}
                let item = materialMap.itemAt(i)
                if(item){
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
        printerListModel.cSourceModel.printDeviceGCode(macAddress,gcodePath,arr)

        dlgClose()
        root.visible = false
    }
    width: 793* screenScaleFactor
    height: 700* screenScaleFactor
    function initShow()
    {
        console.log("initShow1"+type)
        materialMap.model = null
        materialMap.model = Qt.binding(function(){return kernel_gcodecolor_manager.dataList;});
        if(type!=0)
        {
            currentObject.materialBoxList.GenCloseGCodeColors2(fileListModel[0].materialColors, fileListModel[0].material);
            kernel_gcodecolor_manager.setDefaultGcodeInfo(fileListModel[0].material, fileListModel[0].materialColors, fileListModel[0].materialIds);
        }
        if(type==3)
        {
            multiPrintId.firstSlice = 0;
            gcodePath = fileListModel[0].gCodePrefixPath+fileListModel[0].gCodeFileName;
            
            //currentObject.materialBoxList.getColorMap(ipAddress, gcodePath);
        }else{
            imgPreview.source = "";
            imgPreview.source = gcodeImageSource //"file:///" + kernel_slice_flow.gcodeThumbnail();
        }

        idGcodeFileName.text = gcodeFileName //kernel_slice_flow.getExportName();
        idGcodeFileName.forceActiveFocus();

        if(ipAddress !== "" && gcodePath != "")
        {
            //kernel_gcodecolor_manager->setCurGcodeInfo(AAA);
            //currentObject.materialBoxList.getColorMap(ipAddress, gcodePath);
        }
        if(currentObject.materialBoxList)
        {
            enableCfs.visible = true;
        }
        
        //判断是否只有外挂料架
        if(currentObject.materialBoxList&&currentObject.materialBoxList.isOnlyRackMaterial(currentObject.pcPrinterID))
        {
            enableCfs.checked = false;
            enableCfs.visible = false;
            root.currentPrinterNotMap = false
        }
        
       
    }
    function materialMapError(){
        console.log("materialMapError")
        let model = currentObject.materialBoxList
        root.currentPrinterNotMap = false
        if(!enableCfs.checked)
        {
            return;
        }
        console.log("materialMapError count =",materialMap.count)
        for(let i=0;i<materialMap.count; i++){
            let item = materialMap.itemAt(i)
            if(item&&!item.visible) continue;
            console.log("materialMap:",item.materialId)
            //let idx =  model.getCloseColorFromPrinter(item.gcodeColor, item.gcodeType)
            if(item.materialId === ""){
                root.currentPrinterNotMap = true
                break
            }
        }
        console.log("currentPrinterNotMap =",currentPrinterNotMap)
    }
    Connections{
        target:kernel_gcodecolor_manager
        function onColorDataChanged(){
            console.log("kernel_gcodecolor_manager onCountChanged22222222222")
            materialMapError()
            if(kernel_gcodecolor_manager.count==0)
            {
                //enableCfs.checked = false;
                //enableCfs.enabled = false;
            }else{
                //enableCfs.checked = true;
                //enableCfs.enabled = true;
            }
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
                enabled: true
                onPressed: clickPos = Qt.point(mouse.x + 5, mouse.y+ 5)
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
                onClicked:
                {
                    dlgClose()
                    root.visible = false

                }

                Image {
                    anchors.centerIn: parent
                    width: 10 * screenScaleFactor
                    height: 10 * screenScaleFactor
                    source: parent.isHovered ? "qrc:/UI/photo/closeBtn_d.svg" : "qrc:/UI/photo/closeBtn.svg"
                }

            }

        }

        Item {
            id: __contentItem
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
                    visible:type !== 3
                    Column {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.bottomMargin: 10 * screenScaleFactor
                        anchors.leftMargin: 10 * screenScaleFactor
                        spacing: 10 * screenScaleFactor


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
                                text: gcodeVolumn + "mm"
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
                ScrollView{
                        id:multiPrintId
                        Layout.preferredWidth: 674 * screenScaleFactor
                        Layout.preferredHeight: 340 * screenScaleFactor
                        ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                        clip:true
                        visible:type === 3
                        property int firstSlice;
      
                        Flow{
                            id:flowId
                            spacing: 10* screenScaleFactor
                            width: 674 * screenScaleFactor
                            Repeater{
                                id: printer_repeater
                                model: fileListModel
                                delegate: Rectangle{
                                    readonly property var modelItem: model
                                    property bool isHover:false
                                    width:  206* screenScaleFactor
                                    height:  152* screenScaleFactor
                                    radius: 5* screenScaleFactor
                                    color:  Constants.currentTheme ? "#FFFFFF" : "#424244"
                                    border.width:multiPrintId.firstSlice == model.index?1:0
                                    border.color: isHover?"#FFFFFF":(multiPrintId.firstSlice == model.index)? Constants.themeGreenColor: "transparent"
                                    MouseArea{
                                        hoverEnabled: true
                                        anchors.fill:parent
                                        onEntered:parent.isHover = true
                                        onExited:parent.isHover = false
                                        onClicked: {
                                            multiPrintId.firstSlice = model.index
                                            gcodePath = modelData.gCodePrefixPath+modelData.gCodeFileName;
                                            currentObject.materialBoxList.GenCloseGCodeColors2(fileListModel[index].materialColors, fileListModel[index].material);
                                            kernel_gcodecolor_manager.setDefaultGcodeInfo(fileListModel[index].material, fileListModel[index].materialColors, fileListModel[index].materialIds);
                                            //currentObject.materialBoxList.getColorMap(ipAddress, gcodePath);
                                        }
                                    }

                                    Image {
                                        anchors.centerIn:parent
                                        width:90* screenScaleFactor
                                        height:80* screenScaleFactor
                                        fillMode: model.isEmpty ? Image.Pad : Image.PreserveAspectFit
                                        source: modelItemImagePrefix+modelData.gCodeThumbnailImage
                                    }

                                    Text {
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
                                        //visible: model.isSliced 
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
                                                text: modelData.gCodeFileTime
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
                                                text: modelData.gCodeMaterialLength + "mm"
                                            }

                                        }
                                    }
                                }
                            }
                            
                        }
                    }

                ScrollView {
                    id:materialList
                    visible:currentObject&&currentObject.materialBoxList&& +currentObject.cfsConnect === 1
                    Layout.preferredWidth:674 * screenScaleFactor
                    clip:true
                    Layout.maximumHeight: 110 * screenScaleFactor
                    Layout.minimumHeight: 60* screenScaleFactor
                    ScrollBar.vertical.policy: contentHeight > height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    Layout.topMargin:  20 * screenScaleFactor
//                    anchors.top: thumbMsg.bottom
//                    anchors.topMargin: 10 * screenScaleFactor
                    Grid {
                        id: materialColorFlow
                        Layout.topMargin: 10 * screenScaleFactor
                        Layout.bottomMargin: 10 * screenScaleFactor
                        width:  parent.width
                        columns: 4
                        spacing: 13* screenScaleFactor

                        Repeater {
                            id:materialMap
                            //model:kernel_gcodecolor_manager.dataList

                            delegate: RowLayout {
                                id:materialMapItem
                                property var colorMsg:  {}
                                property string gcodeType: ""
                                property string gcodeColor: ""
                                property string boxId: ""
                                property string colorId: curMapDataObj.colorId
                                property string materialId: ""
                                property var curMapDataObj :  modelData
                                property string tmpColor:   curMapDataObj.gcodeColor
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
                                onTmpColorChanged:
                                {
                                    console.log("kernel_gcodecolor_manager onCountChanged")
                                    console.log("curMaterialColor.defaultBtnBgColor =",curMaterialColor.defaultBtnBgColor)
                                    console.log("modelData.defaultBtnBgColor =",modelData.gcodeColor)
                                    console.log("curMaterialColor.text =",curMaterialColor.text)
                                    console.log("model =",root.currentObject)
                                    if(root.currentObject)
                                    {
                                        let model = currentObject.materialBoxList
                                        materialComboBoxId.curIndex =  model.getCloseColorByIndex(cindex)
                                        //materialComboBoxId.curIndex =  model.getCloseColorFromPrinter(curMaterialColor.defaultBtnBgColor,curMaterialColor.text)
                                        boxId = currentObject.materialBoxList.getMaterialBoxId(currentObject.pcPrinterID,materialComboBoxId.curIndex)
                                        materialId = currentObject.materialBoxList.getMaterialId(currentObject.pcPrinterID,materialComboBoxId.curIndex)
                                    }

                                }
                                BasicDialogButton {
                                    id: curMaterialColor
                                    Layout.preferredWidth: 70 * screenScaleFactor
                                    Layout.preferredHeight: 24 * screenScaleFactor
                                    text: curMapDataObj.gcodeType
                                    btnRadius: height / 2
                                    defaultBtnBgColor: curMapDataObj.gcodeColor
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
                                    visible:enableCfs.checked
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
                                    pcPrinterID: currentObject.pcPrinterID
                                    mapGcodeType:curMaterialColor.text
                                    materialmodel:currentObject.materialBoxList
                                    curIndex: currentObject.materialBoxList.getCloseColorByIndex(cindex)
                                    visible:enableCfs.checked
                                    onCurIndexChanged:{
                                        if(curIndex===-1 || !materialmodel) return
                                        parent.boxId = materialmodel.getMaterialBoxId(pcPrinterID,curIndex)
                                        parent.materialId = materialmodel.getMaterialId(pcPrinterID,curIndex)
                                        materialMapError()
                                    }
                                    onVisibleChanged:{
                                        if(visible){
                                            if(curIndex===-1 || !materialmodel) return
                                            parent.boxId = materialmodel.getMaterialBoxId(pcPrinterID,curIndex)
                                            parent.materialId = materialmodel.getMaterialId(pcPrinterID,curIndex)
                                            materialMapError()
                                        }
                                    }
                                }
                            }

                        }

                    }
                }
                
                ColumnLayout{
                    Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 45 * screenScaleFactor
                        color: "#FD265A"
                        text:qsTranslate("UploadGcodeDlg","The current printer is offline, please connect it before printing")
                        visible: currentObject.pcPrinterStatus!==1
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
                        text:qsTranslate("UploadGcodeDlg","The current printer is printing and only supports sending Gcode")
                        visible:  currentObject.pcPrinterStatus===1 && currentObject.pcPrinterState === 1
                    }
                    RowLayout{
                        id: __printRow
                        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                        Layout.leftMargin: 50 * screenScaleFactor
                        property var progress: currentObject.gCodeTransProgress
                        property bool statusDisable:  currentObject.pcPrinterStatus !== 1   //在线状态，0：离线； 1: 在线
                        property bool stateDisable: currentObject.pcPrinterState === 1 || currentObject.pcPrinterState === 5  ////打印状态 0：空闲 1：打印中 2：打印完成 3：打印失败 4：停止打印 5：打印暂停
                        property int errorCode: +currentObject.errorCode
                        property var btnDisable: (__printRow.progress < 100 && __printRow.progress > 0)  || statusDisable || stateDisable
                        StyleCheckBox{
                            id:printerCalibration
                            Layout.preferredWidth:120 * screenScaleFactor
                            text: qsTranslate("UploadGcodeDlg","Print calibration")
                            checked:currentObject.enableSelfTest
                        }
                        RowLayout{
                            visible: materialList.visible
                            spacing:10* screenScaleFactor
                            StyleCheckBox{
                                id:enableCfs
                                enabled: onePrint.enabled
                                Layout.preferredWidth:80 * screenScaleFactor
                                text: qsTranslate("UploadGcodeDlg","Enable CFS")
                                checked: true
                                onCheckedChanged: {                              
                                        materialMapError()           
                                }
                            }
                            Image{
                                visible:enableCfs.visible
                                Layout.preferredWidth: 18 * screenScaleFactor
                                Layout.preferredHeight: 18 * screenScaleFactor
                                source: cfsNote.visible? "qrc:/UI/photo/upload3mf/materialMapTip_hover.svg" : "qrc:/UI/photo/upload3mf/materialMapTip_default.svg"
                                MouseArea{
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: cfsNote.visible = true
                                    onExited: cfsNote.visible = false
                                }
                            }
                        }

                        Item {
                            Layout.fillWidth: true

                        }
                        BasicDialogButton {
                            id: onePrint

                            Layout.rightMargin: 20  * screenScaleFactor
                            Layout.preferredHeight: 30 * screenScaleFactor
                            Layout.preferredWidth: 150 * screenScaleFactor
                            text: qsTranslate("CusControlTableView","Print") //qsTr("One-click Printing")
                            btnRadius: 15 * screenScaleFactor
                            btnBorderW: 1 * screenScaleFactor
                            defaultBtnBgColor: Constants.leftToolBtnColor_normal
                            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                            enabled:  __printRow.errorCode === 0 && !__printRow.btnDisable &&  !root.currentPrinterNotMap
                            onSigButtonClicked: {
                                root.devicePrintOps()
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
                        y: 0 - height* screenScaleFactor
                        x: 10* screenScaleFactor
                        closePolicy: Popup.CloseOnPressOutsideParent
                        RowLayout{
                            spacing: 20 * screenScaleFactor

                            ColumnLayout{
                                Layout.preferredWidth:140* screenScaleFactor
                                Layout.leftMargin: 20* screenScaleFactor
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color:Constants.textColor
                                    text: qsTranslate("UploadGcodeDlg","Enable CFS")
                                }
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_8
                                    color:textColor
                                    text: qsTranslate("UploadGcodeDlg","Printing with consumables from CFS")
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
                            ColumnLayout{
                                Layout.preferredWidth:180* screenScaleFactor
                                Layout.rightMargin: 20* screenScaleFactor
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_8
                                    color:Constants.textColor
                                    text: qsTranslate("UploadGcodeDlg","Disable CFS")
                                }
                                Text {
                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color:textColor
                                    text: qsTranslate("UploadGcodeDlg","Printing using consumables plugged into the back of the machine")
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
                    Layout.fillHeight: true
                }
            }

            Image{
                visible:materialList.visible&&enableCfs.checked
                width: 18 * screenScaleFactor
                height: 18 * screenScaleFactor
                source: materialMapNote.visible? "qrc:/UI/photo/upload3mf/materialMapTip_hover.svg" : "qrc:/UI/photo/upload3mf/materialMapTip_default.svg"
                anchors.left:__contentItem.left
                anchors.leftMargin:10 * screenScaleFactor
                anchors.top:__contentItem.top
                anchors.topMargin:  432 * screenScaleFactor
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: materialMapNote.visible = true
                    onExited: materialMapNote.visible = false
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
                y:40* screenScaleFactor
                Text {
                    font.weight: Font.Medium
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    color:textColor
                    text: qsTranslate("UploadGcodeDlg","Supplies used in this print job")
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
                    text: qsTranslate("UploadGcodeDlg","CFS slot for this consumable")
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
                    text: qsTranslate("UploadGcodeDlg","Need to click manually to specify the CFS slot")
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
                    text: qsTranslate("UploadGcodeDlg","The slot has consumables but is not edited.")
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
                    text: qsTranslate("UploadGcodeDlg","Empty slot")
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
                        text: qsTranslate("UploadGcodeDlg","Material Selection Instructions")
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

        }
    }

}
