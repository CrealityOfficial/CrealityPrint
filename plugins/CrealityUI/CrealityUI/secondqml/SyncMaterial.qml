import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import "qrc:/CrealityUI"
import ".."
import "../components"
import "../qml"
DockItem {
    id: idDialog
    width: 730 * screenScaleFactor
    height: (Constants.languageType == 0 ? 510 : 560) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Synchronize material information from printer")
    property string version:""
    property string website: ""
    property bool resyncEnable:false
    property bool syncEnable:false
    property string materialDisableBg: "qrc:/UI/photo/upload3mf/materialDisableBg.png"
    onVisibleChanged: {
        printerListView.model = null
        printerListView.model = printerListModel.cSourceModel
        idDialog.resyncEnable = false
        idDialog.syncEnable = false

    }
    Column
    {
        y:60* screenScaleFactor
        width: parent.width
        height: parent.height-20* screenScaleFactor

        ScrollView{
            width:  690* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            height: 380* screenScaleFactor
            clip: true
            ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
            Column{
                id: parentCol
                spacing: 10*screenScaleFactor
                anchors.fill: parent
                ButtonGroup { id: idMaterialGroup }
                Repeater{
                    id:printerListView
                    delegate: Rectangle{
                        id:printerItem
                        property var hoverBorderColor: Constants.currentTheme ? Constants.themeGreenColor :"#6e6e73"
                        property var defaultBorderColor: Constants.currentTheme ? "#CBCBCC" :"#545458"
                        width: 690* screenScaleFactor
                        height: materialList?(materialList.rowCount()>8? 60 * screenScaleFactor*2 :60* screenScaleFactor): 60* screenScaleFactor//flowLayout.height>0?  flowLayout.height + 20*screenScaleFactor :60*screenScaleFactor
                        border.width: radioBtn.checked?2:isHover ? 2: 1
                        border.color:radioBtn.checked? Constants.themeGreenColor: isHover? hoverBorderColor: defaultBorderColor
                        color: Constants.currentTheme? "#FFFFFF" :"#545458"
                        radius: 5* screenScaleFactor
                        visible: printerStatus == 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        property var isHover: false
                        MouseArea{
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.isHover = true
                            onExited: parent.isHover = false
                            onClicked: {
                                radioBtn.checked = true
                                //判断是否有可用耗材
                                let hasType = false
                                for(let i=0;i<curMaterialList.count;i++){
                                    let item = curMaterialList.itemAt(i)
                                    if(item.curState!==0&&item.curMaterial){
                                        hasType = true
                                        break
                                    }
                                }
                                if(materialList&&materialList.rack)
                                {
                                    hasType = true;
                                }
                                idDialog.resyncEnable =materialList&&(materialList.rowCount()>0||materialList.rack) && hasType
                                idDialog.syncEnable = sync&&hasType
                            }

                        }

                        RowLayout{
                            anchors.fill: parent
                            RowLayout{
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignLeft
                                BasicRadioButton {
                                    property var printerModel:model
                                    property var printerId: printerListModel.cSourceModel.getDeviceObject(model.index).pcPrinterID
                                    property int materialModelIndex:model.index
                                    id:radioBtn
                                    checked: idMaterialGroup.checkedButton === this
                                    ButtonGroup.group: idMaterialGroup
                                    padding: 0
                                    Layout.leftMargin: 6*screenScaleFactor
                                    onClicked:{
                                        radioBtn.checked = true
                                        let hasType = false

                                        for(let i=0;i<curMaterialList.count;i++){
                                            let item = curMaterialList.itemAt(i)
                                            if(item.curState!==0&&item.curMaterial){
                                                hasType = true
                                                break
                                            }
                                        }

                                        if(materialList&&materialList.rack)
                                        {
                                            hasType = true;
                                        }
                                        
                                        idDialog.resyncEnable =materialList&&(materialList.rowCount()>0||materialList.rack) && hasType
                                        idDialog.syncEnable = sync&&hasType
                                    }

                                }
                                CusText{
                                    fontText: kernel_add_printer_model.getModelNameByCodeName(mItem.pcPrinterModel).trim()+ "/"+ ipAddr
                                    Layout.alignment: Qt.AlignVCenter|Qt.AlignLeft
                                    fontColor: Constants.themeTextColor
                                    fontPointSize:Constants.labelFontPointSize_9
                                    font.weight: Font.ExtraBold
                                    hAlign:0
                                    isDefault: true

                                }
                            }

                            Item{
                                Layout.fillWidth: true
                            }
                            Item {
                                Layout.preferredWidth : flowLayout.width
                                Layout.preferredHeight: flowLayout.height
                                Layout.alignment: Qt.AlignVCenter
                                Flow{
                                    id:flowLayout
                                    width: Math.min(materialList&&materialList.rowCount()*45+50, 420)* screenScaleFactor
                                    spacing: 5*screenScaleFactor
                                    onVisibleChanged:{
                                        if(visible)
                                            printerItem.height = height+20*screenScaleFactor
                                    }
                                    Rectangle{
                                        width: 40*screenScaleFactor
                                        height:  40*screenScaleFactor
                                        radius: 3*screenScaleFactor
                                        color:materialList ? materialList.color : "#000000"
                                        visible: materialList
                                        function setTextColor(){
                                                let bgColor = [color.r * 255, color.g * 255, color.b * 255]
                                                let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
                                                let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
                                              
                                                if(whiteContrast > blackContrast)
                                                    colorText.fontColor = "#ffffff"
                                                else
                                                    colorText.fontColor =  "#000000"
                                            }
                                         Image{
                                                anchors.fill:parent
                                                clip:true
                                                // fillMode: Image.PreserveAspectCrop
                                                source:idDialog.materialDisableBg
                                                visible:!(materialList&&materialList.rack)
                                                

                                            }
                                        Component.onCompleted: setTextColor() 
                                        Column{
                                                spacing: 2
                                                anchors.centerIn: parent
                                                width: colorText.width + materialTypeText.width
                                                height: colorText.height + materialTypeText.height +spacing
                                                CusText{
                                                    id:colorText
                                                    fontText:"EXT"
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    fontPointSize:Constants.labelFontPointSize_9
                                                }
                                                Rectangle{
                                                    width:36*screenScaleFactor
                                                    height:1
                                                    color:colorText.fontColor
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                }
                                                CusText{
                                                    id:materialTypeText
                                                    fontColor:colorText.fontColor
                                                    property var extState:  materialList ? materialList.type : undefined
                                                    fontText:{
                                                        if(!(materialList ? materialList.rack : undefined)||extState==""){
                                                            return "?"
                                                        }else {
                                                            return extState
                                                        }
                                                    }
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    fontPointSize:Constants.labelFontPointSize_8
                                                }

                                            }  

                                    }
                                    Repeater{
                                        id:curMaterialList
                                        model: materialList
                                        delegate:Rectangle{
                                            property var curState:+model.state
                                            property var curMaterial:kernel_parameter_manager.currentMachineObject.findMaterialByRFID(name)==""?"":materialType
                                            function setTextColor(){
                                                let bgColor = [color.r * 255, color.g * 255, color.b * 255]
                                                let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
                                                let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
                                                if((+model.state === 2&&!materialType)||+model.state === 0 || (+model.state === 1&& !materialType) || curMaterial=="")
                                                {
                                                     colorText.fontColor =  "#000000"
                                                     return
                                                }
                                                if(whiteContrast > blackContrast)
                                                    colorText.fontColor = "#ffffff"
                                                else
                                                    colorText.fontColor =  "#000000"
                                            }
                                            clip:true
                                            width: 40*screenScaleFactor
                                            height:  40*screenScaleFactor
                                            radius: 3*screenScaleFactor
                                            color:+model.state === 0 ? "#707070" :materialColor
                                            Component.onCompleted: setTextColor()   
                                            BasicTooltip {
                                                id: noteTooptip
                                                visible:isHover&&text
                                                position: BasicTooltip.Position.BOTTOM
                                                text: {
                                                        if(+model.state === 2&&!materialType){
                                                            return qsTr("Synchronize material information from printer")
                                                        }else {
                                                            return +model.state === 0? qsTr("Empty slot"): (+model.state === 1&& !materialType)? qsTr("Synchronize material information from printer"):""
                                                        }
                                                    }
                                            }
                                            property bool isHover:false
                                            MouseArea{
                                                anchors.fill:parent
                                                hoverEnabled:true
                                                onEntered:parent.isHover=true
                                                onExited:parent.isHover=false


                                            }
                                            Image{
                                                anchors.fill:parent
                                                clip:true
                                                // fillMode: Image.PreserveAspectCrop
                                                source:idDialog.materialDisableBg
                                                visible:{
                                                    if((+model.state === 2&&!materialType)|| curMaterial==""){
                                                        return true
                                                    }else {
                                                        return +model.state === 0|| (+model.state === 1&& !materialType)
                                                    }
                                                }

                                            }
                                            Column{
                                                spacing: 2
                                                anchors.centerIn: parent
                                                width: colorText.width + materialTypeText.width
                                                height: colorText.height + materialTypeText.height +spacing
                                                CusText{
                                                    id:colorText
                                                    fontText:materialBoxId+["A","B","C","D"][materialId]
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    fontPointSize:Constants.labelFontPointSize_9
                                                }
                                                Rectangle{
                                                    width:36*screenScaleFactor
                                                    height:1
                                                    color:colorText.fontColor
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                }
                                                CusText{
                                                    id:materialTypeText
                                                    fontColor:colorText.fontColor
                                                    fontText:{
                                                        if((+model.state === 2&&!materialType)|| (+model.state !== 0 &&curMaterial=="")){
                                                            return "?"
                                                        }else {
                                                            return +model.state === 0? "/": (+model.state === 1&& !materialType)?"?":materialType
                                                        }
                                                    }
                                                    anchors.horizontalCenter: parent.horizontalCenter
                                                    fontPointSize:Constants.labelFontPointSize_8
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
        }

        Rectangle{
            height: 20*screenScaleFactor
            width: parent.width
            color: "transparent"
        }
        RowLayout{
            Layout.preferredHeight: 30*screenScaleFactor
            Layout.preferredWidth: 230*screenScaleFactor
            Layout.alignment: Qt.AlignHCenter
            anchors.horizontalCenter: parent.horizontalCenter
            BasicDialogButton {
                id:synchronizeBtn
                Layout.preferredHeight: 30*screenScaleFactor
                Layout.preferredWidth: 100*screenScaleFactor
                enabled:idDialog.syncEnable
                text: qsTr("Synchronize")
                btnRadius:15*screenScaleFactor
                btnBorderW:1* screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {
                     let curIndex = idMaterialGroup.checkedButton.materialModelIndex
                    let printerModel = printerListModel.cSourceModel.getDeviceObject(curIndex)
                    let materialColorModel = printerModel.materialBoxList

                    var idList = []
                    var colorList = []
                     var arr=[]
                     //没有盒子的情况
                    if(!materialColorModel)
                        return;
                    //没有外挂且盒子没有耗材的情况的情况
                    if(materialColorModel.rowCount()==0 && !materialColorModel.rack && !materialColorModel.type!="")
                        return;
                    for(let i =0;i<materialColorModel.rowCount();i++){
                         let name = materialColorModel.getMaterialName(printerModel.pcPrinterID,i)
                        let rfid = materialColorModel.getRfid(printerModel.pcPrinterID,i)
                        let materialId =  materialColorModel.getMaterialId(printerModel.pcPrinterID,i)
                        let boxId =  materialColorModel.getMaterialBoxId(printerModel.pcPrinterID,i)
                        let materialColor = materialColorModel.getMaterialColor(printerModel.pcPrinterID,i)
                        let materialState = materialColorModel.getMaterialState(printerModel.pcPrinterID,i)
                        idList.push(rfid)
                        colorList.push(materialColor)
                          let obj = {}
                            obj["syncId"] = boxId+""+materialId
                            obj["name"] = name
                            obj["color"] = materialColor
                            arr.push(obj)
                    }

                    //如果有外挂，添加外挂耗材
                    if(materialColorModel.rack&&materialColorModel.type!="")
                    {
                        idList.push(materialColorModel.rfid)
                        colorList.push(materialColorModel.color)
                        let obj = {"syncId":"00","name":materialColorModel.name,"color":materialColorModel.color}
                        arr.push(obj)
                    }
                    kernel_parameter_manager.currentMachineObject.syncExtruders(idList, colorList,arr)
                    close()

                }
            }
            BasicDialogButton {
                Layout.preferredHeight: 30*screenScaleFactor
                Layout.preferredWidth: 100*screenScaleFactor
                text: qsTr("Resync")
                btnRadius:15*screenScaleFactor
                btnBorderW:1* screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                enabled: idDialog.resyncEnable
                onSigButtonClicked:
                {
                    let curIndex = idMaterialGroup.checkedButton.materialModelIndex
                    let printerModel = printerListModel.cSourceModel.getDeviceObject(curIndex)
                    let materialColorModel = printerModel.materialBoxList

                    var idList = []
                    var colorList = []
                    var syncIdList = []
                    var arr=[]
                    var stateList =[]
                     //没有盒子的情况
                    if(!materialColorModel)
                        return;
                    //没有外挂且盒子没有耗材的情况的情况
                    if(materialColorModel.rowCount()==0 && !materialColorModel.rack&&materialColorModel.type!="")
                        return;
                    for(let i =0;i<materialColorModel.rowCount();i++){
                        let rfid = materialColorModel.getRfid(printerModel.pcPrinterID,i)
                        let name = materialColorModel.getMaterialName(printerModel.pcPrinterID,i)
                        let materialId =  materialColorModel.getMaterialId(printerModel.pcPrinterID,i)
                        let boxId =  materialColorModel.getMaterialBoxId(printerModel.pcPrinterID,i)
                        let materialColor = materialColorModel.getMaterialColor(printerModel.pcPrinterID,i)
                        let materialState = materialColorModel.getMaterialState(printerModel.pcPrinterID,i)
                        let state= +materialState ===1? (!+materialState||!!name)?2:1:+materialState //0:"/" 1:"?" 2:有耗材
                        if(+materialState ===2 && !name){
                            state = 1
                        }
                        stateList.push(String(state))
                        idList.push(name)
                        colorList.push(materialColor)
                        syncIdList.push(boxId+""+materialId)
                        let obj = {}
                        obj["syncId"] =  boxId+""+materialId
                        obj["name"] = name
                        obj["color"] = materialColor
                        obj["state"] = state
                        
                        arr.push(obj)
                    }
                    console.log("stateList",stateList)
                    printerListModel.cSourceModel.setSyncPrinter(printerModel.pcIpAddr)
                    //在数组末尾添加EXT
                    //如果有外挂，添加外挂耗材
                    if(materialColorModel.rack&&materialColorModel.type!="")
                    {
                        stateList.push(String(2)) //外挂永远是有耗材可编辑状态
                        idList.push(materialColorModel.name)
                        colorList.push(materialColorModel.color)
                        syncIdList.push("00")
                        let obj = {"syncId":"00","name":materialColorModel.name,"color":materialColorModel.color}
                        arr.push(obj)
                    }

                    kernel_parameter_manager.currentMachineObject.resyncExtruders(idList, colorList,syncIdList,stateList)
                    kernel_parameter_manager.currentMachineObject.savePrinterSlot(arr)
                    close()

                }
            }
            BasicDialogButton {
                Layout.preferredHeight: 30*screenScaleFactor
                Layout.preferredWidth: 100*screenScaleFactor
                text: qsTr("Cancel")
                btnRadius:15*screenScaleFactor
                btnBorderW:1* screenScaleFactor
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:
                {

                    close()

                }
            }


        }


    }
}
