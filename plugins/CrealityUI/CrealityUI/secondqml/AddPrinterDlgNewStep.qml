import QtQuick 2.10
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls 1.4 as Control14
import QtQml 2.13

import "../qml"
import "../secondqml"
import "../components"
DockItem {
    id: root
    title: qsTr("New model")
    modality: Qt.ApplicationModal
    width: 710 * screenScaleFactor
    height: 600 * screenScaleFactor
//    panelColor: sourceTheme.background_color

    property var stepIndex: 1
    property var materialCheck: []
    property var processCheck: []
    property var printMaterialsModel
    property var selPrinter
    property var templateMachine

    ListModel{ id:materialModel }
    ListModel{ id:processModel }
    function setNextButtonState(){
        basicComButton.enabled = (stepIndex===2 && processCheck.length > 0) ||  (stepIndex===1 && idPrinterName.text)
    }
    function setFinishedButtonState(){
        finishButton.enabled = stepIndex===3 && materialCheck.length > 0
    }

    onStepIndexChanged: {
        setNextButtonState()
        setFinishedButtonState()
    }
      
    onVisibleChanged: {
        if(!visible){
            return
        }
        idPrinterName.text = ""
        let materialArr =  kernel_material_center.types()
        let processArr =  kernel_parameter_manager.processTemplates()
        for(let i =0;i<materialArr.count;i++){
            materialModel.setProperty(i,"name",materialArr.get(i).value)
            materialModel.setProperty(i,"check",false)
        }
        for(let j =0;j<processArr.count;j++){
            processModel.setProperty(j,"name",processArr.get(i).value)
            processModel.setProperty(j,"check",false)
        }
        stepIndex = 1
    }

    ColumnLayout{
        anchors.fill: parent
        anchors.topMargin: root.currentTitleHeight
        Grid {
            id:step
            width : 320*screenScaleFactor
            height: 40*screenScaleFactor
            columns: 5
            spacing: 10
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 30*screenScaleFactor
            Layout.bottomMargin: 30*screenScaleFactor
            BasicDialogButton {
                width: 80*screenScaleFactor
                height: parent.height
                text: qsTr("Step 1")
                btnRadius: height/2
                btnBorderW:stepIndex >=1?0:1
                defaultBtnBgColor: stepIndex >=1? Constants.leftBtnBgColor_selected : Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: stepIndex >=1? Constants.leftBtnBgColor_selected : Constants.leftToolBtnColor_normal
                onSigButtonClicked:
                {


                }
            }
            StyledLabel {
                height: parent.height
                text: qsTr("＞")
                Layout.alignment: Qt.AlignVCenter
            }

            BasicDialogButton {
                width: 80*screenScaleFactor
                height:  parent.height
                text: qsTr("Step 2")
                btnRadius: height/2
                btnBorderW:stepIndex >=2?0:1
                defaultBtnBgColor: stepIndex >=2? Constants.leftBtnBgColor_selected : Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: stepIndex >=2? Constants.leftBtnBgColor_selected : Constants.leftToolBtnColor_normal
                onSigButtonClicked: {

                }
            }
            StyledLabel {
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("＞")
                height: parent.height
            }
            BasicDialogButton {
                width: 80*screenScaleFactor
                height:  parent.height
                text: qsTr("Step 3")
                btnRadius: height/2
                btnBorderW:stepIndex >=3?0:1
                defaultBtnBgColor:stepIndex >=3? Constants.leftBtnBgColor_selected : Constants.leftToolBtnColor_normal
                hoveredBtnBgColor:stepIndex >=3? Constants.leftBtnBgColor_selected : Constants.leftToolBtnColor_normal
                onSigButtonClicked: {

                }
            }
        }

        Grid{
            id:first_step
            visible: stepIndex === 1
            Layout.leftMargin: 30* screenScaleFactor
            Layout.rightMargin: 30* screenScaleFactor
            width: parent.width
            height: 200
            columns: 2
            rows: 10
            rowSpacing: 10
            horizontalItemAlignment:Grid.AlignRight
            property var machineModel: kernel_parameter_manager.userMachineModel()
            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 30* screenScaleFactor
                text:qsTr("Printer name")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
                Layout.alignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                id:idPrinterName
                width: 230 * screenScaleFactor
                height: 28 * screenScaleFactor
                baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                font.pointSize: Constants.labelFontPointSize_9
                text: first_step.machineModel.name
                Layout.alignment: Qt.AlignRight
                onTextChanged:{
                    first_step.machineModel.name = text
                    setNextButtonState()
                }
                
            }
            // StyledLabel{
            //     activeFocusOnTab: false
            //     width: 280* screenScaleFactor
            //     height: 28* screenScaleFactor
            //     text:qsTr("Printer platform shape")
            //     font.pointSize:Constants.labelFontPointSize_10
            //     color: Constants.themeTextColor
            //     horizontalAlignment: Qt.AlignLeft
            // }

            // CXComboBox {
            //     width: 230 * screenScaleFactor
            //     height: 28 * screenScaleFactor
            //     model:first_step.machineModel.shapeOptions
            //     //displayText: first_step.machineModel.shape
            //     onCurrentContentChanged: first_step.machineModel.shape = ctext
            //     Component.onCompleted: {
            //         for(let i =0;i<model.count;i++){
            //             if(model.get(i).value === first_step.machineModel.shape){
            //                 currentIndex = i
            //                 break
            //             }
            //         }

            //     }
            // }

            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 28* screenScaleFactor
                text:qsTr("Printer platform size")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
            }
            Row{
                spacing: 10
                Row{
                    spacing: 3* screenScaleFactor
                    StyledLabel{
                        activeFocusOnTab: false
                        width: 10
                        height: 28* screenScaleFactor
                        text:"X"
                        font.pointSize:Constants.labelFontPointSize_10
                        color: Constants.themeTextColor
                    }
                    BasicDialogTextField {
                        width: 100 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                        font.pointSize: Constants.labelFontPointSize_9
                        text: first_step.machineModel.width
                        unitChar: "mm"
                        onTextChanged: first_step.machineModel.width = +text
                    }

                }
                Row{
                    spacing: 3* screenScaleFactor
                    StyledLabel{
                        activeFocusOnTab: false
                        width: 10
                        height: 28* screenScaleFactor
                        text:"Y"
                        font.pointSize:Constants.labelFontPointSize_10
                        color: Constants.themeTextColor
                    }
                    BasicDialogTextField {
                        width: 100 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                        font.pointSize: Constants.labelFontPointSize_9
                        text: first_step.machineModel.depth
                        unitChar: "mm"
                        onTextChanged: first_step.machineModel.depth = +text
                    }

                }
                Row{
                    spacing: 3* screenScaleFactor
                    StyledLabel{
                        activeFocusOnTab: false
                        width: 10
                        height: 28* screenScaleFactor
                        text:"Z"
                        font.pointSize:Constants.labelFontPointSize_10
                        color: Constants.themeTextColor
                    }
                    BasicDialogTextField {
                        width: 100 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                        font.pointSize: Constants.labelFontPointSize_9
                        text: first_step.machineModel.height
                        unitChar: "mm"
                        onTextChanged: first_step.machineModel.height = +text
                    }

                }

            }
            // StyledLabel{
            //     activeFocusOnTab: false
            //     width: 280* screenScaleFactor
            //     height: 28* screenScaleFactor
            //     text:qsTr("Extruder type")
            //     font.pointSize:Constants.labelFontPointSize_10
            //     color: Constants.themeTextColor
            // }
            // CXComboBox {
            //     width: 230 * screenScaleFactor
            //     height: 28 * screenScaleFactor
            //     model:first_step.machineModel.extruderTypeOptions
            //     displayText: qsTr(currentText)
            //     onCurrentContentChanged: first_step.machineModel.extruderType = ctext
            //     //displayText: first_step.machineModel.extruderType

            // }
            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 28* screenScaleFactor
                text:qsTr("Nozzle diameter")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
            }
            CXComboBox {
                width: 230 * screenScaleFactor
                height: 28 * screenScaleFactor
                model:["0.2","0.4","0.6","0.8","1.0","1.2"]
                displayText: qsTr(currentText)
                currentIndex: 1
                onCurrentContentChanged: first_step.machineModel.nozzleSize = ctext

            }
            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 28* screenScaleFactor
                text:qsTr("Compatible filament diameters")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
            }
            CXComboBox {
                width: 230 * screenScaleFactor
                height: 28 * screenScaleFactor
                model:["1.75","2.85"]
                displayText: qsTr(currentText)
                onCurrentContentChanged: first_step.machineModel.materialDiameter = ctext
                //  displayText: first_step.machineModel.materialDiameter

            }
            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 28* screenScaleFactor
                text:qsTr("Nozzle offset")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
            }
            Row{
                spacing: 10
                Row{
                    spacing: 5
                    StyledLabel{
                        activeFocusOnTab: false
                        width: 10
                        height: 28* screenScaleFactor
                        text:"X"
                        font.pointSize:Constants.labelFontPointSize_10
                        color: Constants.themeTextColor
                    }
                    BasicDialogTextField {
                        width: 100 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                        font.pointSize: Constants.labelFontPointSize_9
                        text: first_step.machineModel.nozzleOffsetX
                        unitChar: "mm"
                        onTextChanged: first_step.machineModel.nozzleOffsetX = +text
                    }

                }
                Row{
                    spacing: 5
                    StyledLabel{
                        activeFocusOnTab: false
                        width: 10
                        height: 28* screenScaleFactor
                        text:"Y"
                        font.pointSize:Constants.labelFontPointSize_10
                        color: Constants.themeTextColor
                    }
                    BasicDialogTextField {
                        width: 100 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        baseValidator: RegExpValidator { regExp: /^\S{100}$/ }
                        font.pointSize: Constants.labelFontPointSize_9
                        text: first_step.machineModel.nozzleOffsetY
                        unitChar: "mm"
                        onTextChanged: first_step.machineModel.nozzleOffsetY = +text
                    }

                }
            }
            // StyledLabel{
            //     activeFocusOnTab: false
            //     width: 280* screenScaleFactor
            //     height: 28* screenScaleFactor
            //     text:qsTr("G-code style")
            //     font.pointSize:Constants.labelFontPointSize_10
            //     color: Constants.themeTextColor
            // }
            // CXComboBox {
            //     width: 230 * screenScaleFactor
            //     height: 28 * screenScaleFactor
            //     model:first_step.machineModel.gcodeFlavorOptions
            //     displayText: qsTr(currentText)
            //     onCurrentContentChanged: first_step.machineModel.gcodeFlavor = ctext
            //     //   displayText: first_step.machineModel.gcodeFlavor

            // }
            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 28* screenScaleFactor
                text:qsTr("Starts G-code")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
            }
            BasicDialogTextArea{
                width: 230 * screenScaleFactor
                height: 50*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
                text: first_step.machineModel.startGcode
                onTextChanged: first_step.machineModel.startGcode = text
            }
            StyledLabel{
                activeFocusOnTab: false
                width: 280* screenScaleFactor
                height: 28* screenScaleFactor
                text:qsTr("End G-code")
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
            }
            BasicDialogTextArea{
                width: 230 * screenScaleFactor
                height: 50*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
                text: first_step.machineModel.endGcode
                onTextChanged: first_step.machineModel.endGcode = text
            }
        }


        Column {
            id:second_step
            visible: stepIndex === 2
            Layout.leftMargin: 30* screenScaleFactor
            Layout.rightMargin: 30* screenScaleFactor
            spacing: 30*screenScaleFactor
            anchors.top:separateLine.top
            width : parent.width
            property string curText
            Row{
                spacing: 5 * screenScaleFactor
                StyledLabel {text: qsTr("printer")}
                StyledLabel {
                    text: idPrinterName.text
                    font.weight: Font.ExtraBold
                }
            }
            Row{
                width : parent.width
                spacing: 5 * screenScaleFactor
                StyledLabel {
                    text: qsTr("Printer model")
                    height: 28 * screenScaleFactor
                }
                CXComboBox {
                    width: 200 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    anchors.verticalCenter:parent.verticalCenter
                    model: kernel_parameter_manager.defaultMachineNameList
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        processCheck = []
                        setNextButtonState()
                    }
                    onActivated: {
                        second_step.curText = currentText
                        templateMachine = kernel_parameter_manager.machineObject(currentText)
                        profileId.model = templateMachine.profilesModel
                    }
                    onVisibleChanged: {
                        if(!visible){
                            return
                        }
                        second_step.curText = currentText
                        templateMachine = kernel_parameter_manager.machineObject(currentText)
                        profileId.model = templateMachine.profilesModel
                    }
                }
            }
            Row{
                spacing: 5 * screenScaleFactor
                width : parent.width
                StyledLabel {text: qsTr("Profile")}
                Item {
                    width : parent.width
                    height:profileFlow.height                  
                    Flow{
                        height: 250 * screenScaleFactor
                        width : parent.width
                        spacing: 10* screenScaleFactor
                        id:profileFlow
                        Repeater {
                            id:profileId

                            delegate: CusCheckBox {
                                text: name
                                checked: false
                                textColor: Constants.themeTextColor
                                font.pointSize: Constants.labelFontPointSize_9
                                font.weight: Font.ExtraBold
                                onCheckedChanged: {
                                    
                                    if(checked){
                                        if(!processCheck.includes(name)){
                                            processCheck.push(name)
                                        }
                                    }else {
                                        processCheck = processCheck.filter(f=>f!==name)
                                    }
                                     setNextButtonState()
                                }
                            }
                        }

                    }
                }

            }
        }

        Column{
            id:third_step
            visible: stepIndex === 3
            Layout.leftMargin: 30* screenScaleFactor
            Layout.rightMargin: 30* screenScaleFactor
            spacing: 30*screenScaleFactor
            Layout.fillWidth: true
            onVisibleChanged: {
                if(!visible){
                    return
                }
                printMaterialsModel = kernel_parameter_manager.machineObject(second_step.curText).materialsModelProxy()
                material_type.model = Qt.binding(function(){ return  kernel_material_center.types() })
                // material_name.model = Qt.binding(function(){ return  kernel_parameter_manager.currentMachineObject.materialsModelProxy(); })
                printMaterialsModel.refreshState()
                materialCheck = []
            }
            Row{
                spacing: 5 * screenScaleFactor
                StyledLabel {text: qsTr("printer")}
                StyledLabel {
                    text: idPrinterName.text
                    font.weight: Font.ExtraBold
                }
            }
            Row{
                spacing: 5 * screenScaleFactor
                StyledLabel {text: qsTr("material type")}
                width : parent.width- 60* screenScaleFactor
                height:50* screenScaleFactor
                 Item {
                    width : parent.width
                    height:materialTypeFlow.height
                    Flow{
                        width : parent.width
                        spacing: 10 * screenScaleFactor
                        id:materialTypeFlow

                        Repeater {
                            id:material_type
                            model: kernel_material_center.types()
                            delegate: CusCheckBox {
                                checked: printMaterialsModel.isVisible(modelData)
                                text: modelData
                                textColor: Constants.themeTextColor
                                font.pointSize: Constants.labelFontPointSize_9
                                font.weight: Font.ExtraBold
                                onClicked:
                                {
                                    printMaterialsModel.filterMaterialType(modelData,checked)
                                  
                                }
                            }
                        }

                    }
                 }
            }

            Rectangle{
                id: separateLine
                width: parent.width
                height: 1*screenScaleFactor
                color: Constants.dialogItemRectBgBorderColor
            }


            ScrollView{
                width : parent.width
                height: 250 * screenScaleFactor
                clip: true
                ScrollBar.vertical.policy: (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                Grid{
                    columns: 4
                    rows: 10
                    columnSpacing: 40
                    rowSpacing: 25
                    anchors.fill: parent
                    Repeater {
                        id: material_name
                        model: printMaterialsModel
                        delegate: CusCheckBox {
                            text: meName
                            textColor: Constants.themeTextColor
                            font.pointSize: Constants.labelFontPointSize_9
                            font.weight: Font.ExtraBold
                            onCheckedChanged: {                              
                 
                                if(checked){
                                    if(!materialCheck.includes(meName))materialCheck.push(meName)
                                } else {
                                    materialCheck = materialCheck.filter(f=>f!==meName)
                                }
                                // let materialCheck =[]
                                console.log("materialCheck = ", materialCheck)
                                setFinishedButtonState()
                            }
                        }
                    }
                }
            }


        }


        Grid {
            columns: 3
            spacing: 10
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 10*screenScaleFactor
            BasicDialogButton {
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                btnRadius:15*screenScaleFactor
                text: qsTr("Previous")
                btnBorderW:1
                visible: stepIndex!==1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: stepIndex--
            }
            BasicDialogButton {
                id: basicComButton
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                visible: stepIndex !== 3
                text: qsTr("Next")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                enabled: false
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: stepIndex++
            }

            BasicDialogButton {
                id: finishButton
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                visible: stepIndex === 3
                text: qsTr("Finish")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                // enabled:stepIndex === 3 && processCheck.length>0
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:{
                    console.log("materialCheck111 = ", materialCheck)
                    kernel_parameter_manager.addUserMachine(templateMachine, materialCheck, processCheck)
                    root.close()
                }
            }

            BasicDialogButton {
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Cancel")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: root.close()
            }

        }
    }
}
