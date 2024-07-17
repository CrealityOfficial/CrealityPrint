import QtQuick 2.10
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import "qrc:/CrealityUI"
import ".."
import "../components"
import "../qml"
DockItem {
    id: idDialog
    width: 1000 * screenScaleFactor
    height: (Constants.languageType == 0 ? 652 : 602) *  screenScaleFactor //mac中文字显示差异，需多留空白
    titleHeight : 30 * screenScaleFactor
    property int spacing: 5
    title: qsTr("Flush volume when filament replacement")
    ListModel{ id:tableData }
    property var materialColor: []
    property var matrixs: []
    property var colorArr: []
    property var nozzleVolum
    property var tipColor:Constants.textColor


    function setTextColor(color){
            let calcColor = ""
            let bgColor = [color.r * 255, color.g * 255, color.b * 255]
            let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
            let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
            if(whiteContrast > blackContrast)
                calcColor = "#ffffff"
            else
                calcColor =  "#000000"

            return calcColor
        }


    function calcTableData(isInit = 0){
        tableData.clear()
        tableViewContainer.model = null
        tableViewContainer.model = tableData
        let extrudersModel = kernel_parameter_manager.currentMachineObject.extrudersModel
        let rowCount = extrudersModel.rowCount()
        materialColor=[{ key: qsTr("from/to"),color: Constants.profileBtnColor}]
         colorArr = []
        for(var i =0; i<rowCount;i++){
            let color =extrudersModel.data(extrudersModel.index(i,0),258)
            colorArr.push(color)
        }
        if(isInit ===0){
            matrixs =kernel_material_center.getFlushingVolumesDefault(colorArr.length)
        }else if(isInit === 1) {
            matrixs =kernel_material_center.editFlushingValue(multiplier.realValue)
        } else{
            matrixs =kernel_material_center.getFlushingVolumes(colorArr,multiplier.realValue)
        }

        for(var j =0; j<rowCount;j++){
            let data = matrixs.slice(j*rowCount,j*rowCount+rowCount)
            let textColor = setTextColor(colorArr[j])
            console.log(colorArr[j],textColor, "textColor" )
            materialColor.push({ key: j+1+"",color:colorArr[j]+"", textColor, data})
        }
        for(let item of materialColor){
            const { key,color, data,textColor } = item
            let obj = {title:key,color,textColor}
            if(data){
                for(let i =0;i<data.length;i++){
                    let v = `num${i+1}`
                    obj[v]=data[i]
                }
                tableData.append(obj)
                delete obj;
            }
        }
    }
    function setFlushingParams(){
        let arr = []
        for(let i=0;i<tableData.count;i++){
            for(let j=1;j<tableViewContainer.columnCount;j++){
                let value = tableData.get(i)[`num${j}`]
                arr.push(value)
            }
        }
        return arr
    }

    function setTipWarning(){
        for(let i=0;i<tableData.count;i++){
            for(let j=1;j<tableViewContainer.columnCount;j++){
                let value = tableData.get(i)[`num${j}`]
                if(j-1 === i) continue
                console.log(value,"setTipWarning")
                if(value>=800 || value<nozzleVolum){
                    tipColor = "red"
                    return;
                }else {
                    tipColor = Constants.textColor
                }
            }
        }
    }

    function createColumn(){
        while (tableViewContainer &&tableViewContainer.columnCount > 0) {
            tableViewContainer.removeColumn(0);
        }
        let w = parseInt((idDialog.width-50-2)/materialColor.length)
        for(let cc of materialColor){
            const { key } = cc
            let label = `num${key}`
            let colStr =`import QtQuick.Controls 1.4; TableViewColumn { role: "${label}"; title: "${key}";width:${w} }`
            var newColumn = Qt.createQmlObject(colStr,tableViewContainer);
            tableViewContainer.addColumn(newColumn);
            delete newColumn
        }
    }
    onVisibleChanged: {
        if(!visible){
            let arr = setFlushingParams()
            kernel_material_center.setFlushingParams(arr,multiplier.realValue)
        }else{
            nozzleVolum = kernel_material_center.getNozzleVolume()
            multiplier.realValue = kernel_material_center.getFlushingMultiplier()
            calcTableData(0)
            createColumn()
            setTipWarning()
        }
    }



    Column
    {
        y:60* screenScaleFactor
        width: parent.width
        height: parent.height-40* screenScaleFactor
        spacing:15* screenScaleFactor

        Rectangle{
            id:tableContainer
            width: parent.width - 50*screenScaleFactor
            height: parent.height - 170*screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            color: "transparent"
            radius: 5*screenScaleFactor
            TableView{
                anchors.fill: parent
                model: tableData
                verticalScrollBarPolicy: Qt.ScrollBarAlwaysOff
                id:tableViewContainer
                headerVisible: true
                headerDelegate: Rectangle {
                    height: 50*screenScaleFactor //tableContainer.height/(tableData.count+1)
//                    width: 100 //parseInt(tableContainer.width/tableData.count)
                    color:  Constants.dialogTitleColor
                    Rectangle{
                        anchors.centerIn: parent
                        width: 24*screenScaleFactor
                        height: 24*screenScaleFactor
                        color: +styleData.value ?tableData.get(+styleData.value-1).color : "transparent"
                        radius: 3
                        Text {
                            id:colorText
                            text: styleData.value
                            color: +styleData.value ?tableData.get(+styleData.value-1).textColor :Constants.themeTextColor
                            font.family: Constants.labelFontFamilyBold
                            anchors.centerIn: parent
                        }
                    }

                }


                itemDelegate:Rectangle{
                    anchors.centerIn: parent
                    height:  50*screenScaleFactor

                    color: styleData.row%2!==0 ?  Constants.profileBtnColor : Constants.themeColor
                    Rectangle{
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        color: tableData.get(styleData.row) ? tableData.get(styleData.row).color:"transparent"
                        radius: 3
                        visible: styleData.column ===0
                        Text {
                            id:colorText1
                            text: styleData.row+1
                            color: tableData.get(styleData.row) ? tableData.get(styleData.row).textColor: Constants.themeTextColor
                            font.family: Constants.labelFontFamilyBold
                            anchors.centerIn: parent
                        }
                    }

                    BasicDialogTextField {
                        id:textField
                        text: styleData.value
                        validator: IntValidator {bottom: 1; top: 999;}
                        width: 32* screenScaleFactor
                        height:23* screenScaleFactor
                        borderW: !!text ? 0 : 1
                        itemBorderColor: !!text ? "transparent" : "red"
                        padding:0
                        enabled:styleData.column -1 !== styleData.row
                        color: (+styleData.value>800 &&enabled) ?"red":(+styleData.value<nozzleVolum&&enabled)?"red": Constants.themeTextColor
                        anchors.centerIn: parent
                        Layout.alignment: Qt.AlignVCenter
                        visible: styleData.column !==0
                
                        Keys.onEnterPressed: {
                            if(!text) return;
                            tableData.setProperty(styleData.row, `num${styleData.column}`, parseFloat(text))
                            let arr = setFlushingParams()
                            kernel_material_center.setFlushingParams(arr)
                            setTipWarning()
                        }
                        onActiveFocusChanged: {
                            if(!text) return;
                            if (!textField.activeFocus) {
                                tableData.setProperty(styleData.row, `num${styleData.column}`, parseFloat(text))
                                setTipWarning()
                            }
                        }
                    }

                }


                rowDelegate: Rectangle {
                    anchors.centerIn: parent
                    height: 30* screenScaleFactor //tableContainer.height/(tableData.count+1)
                    color: Constants.themeColor // styleData.row%2!==0 ?  Constants.profileBtnColor : Constants.themeColor
                }


            }
        }
        ColumnLayout{
            anchors.left: parent.left
            anchors.leftMargin: 20* screenScaleFactor
            Row {
                spacing: 10* screenScaleFactor
                StyledLabel {
                    id:multiplierText
                    anchors.verticalCenter: parent.verticalCenter
                    //                    width: multiplierText.contentWidth
                    leftPadding: 0/*10*/
                    rightPadding: 10//
                    color:  Constants.textColor
                    text: qsTr("multiplier:")
                    font.pointSize: Constants.labelFontPointSize_9
                }
                LeftToolSpinBox {
                    id:multiplier
                    anchors.verticalCenter: parent.verticalCenter
                    width: 115* screenScaleFactor
                    height: 28* screenScaleFactor
                    realFrom: 0.0
                    realTo: 3.0
                    realStepSize: 1
                    unitchar: ""
                    textValidator: RegExpValidator {
                        regExp:   /(\d{1,4})([.,]\d{1,2})?$/
                    }
                    onValueEdited:{
                        if(!realValue)return
                        calcTableData(1)
                        setTipWarning()
                    }
                    
                }
            }
            StyledLabel {
                width: 15* screenScaleFactor
                leftPadding: 0/*10*/
                rightPadding: 10//
                color:  Constants.textColor
                text: qsTr("Flush volume required to switch between two filaments (mm³)")
                font.pointSize: Constants.labelFontPointSize_9
            }
            StyledLabel {
                width: 15* screenScaleFactor
                leftPadding: 0/*10*/
                rightPadding: 10//
                color: tipColor
                text: qsTr("Warm reminder: It is recommended that the flushing volume be set within the range of.")+"("+ nozzleVolum+ "-800)"
                font.pointSize: Constants.labelFontPointSize_9
            }
        }

        Grid {
            width : 315*screenScaleFactor
            height: 30*screenScaleFactor
            columns: 3
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            Layout.bottomMargin: 20
            BasicDialogButton {
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Automatic calculation")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:calcTableData(4)
//                {
//                    let arr = setFlushingParams()
//                    kernel_material_center.getFlushingVolumes(colorArr,multiplier.realValue)
//                }
            }
            BasicDialogButton {
                id: basicComButton
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Ok")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked:idDialog.close()
//                {
//                    calcTableData(1)
//                    idDialog.close()
//                }
            }

            BasicDialogButton {
                id: basicComButton2
                width: 100*screenScaleFactor
                height: 30*screenScaleFactor
                text: qsTr("Cancel")
                btnRadius:15*screenScaleFactor
                btnBorderW:1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: idDialog.close()
            }

        }

    }
}
