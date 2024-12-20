import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Styles 1.4
import QtQml.Models 2.15
import "../secondqml"

TreeView {
    property real rowHeight: 24 * screenScaleFactor
    property var columnsName: ["support", "Seams", "color", "modelAdptiveLayer"]
    property var columnWidth: [160, 30, 70]
    property real itemWidth: columnWidth[0]
    property real initContentY: 0
    property var currentEditRow
    property var preIndex
    id: cusTree

    TableViewColumn {
        id: col0
        role: "itemData" // 设置角色对应的数据显示方式
        title: qsTranslate("PrintParameterPanel", "Name")
        width: columnWidth[0]* screenScaleFactor
        movable: false
        onWidthChanged: {
            itemWidth = width
            console.log("width111111 = ", itemWidth)
        }
    }

    TableViewColumn {
        id: col1
        role: "itemData"
        title: ""
        width: columnWidth[1] * screenScaleFactor
        resizable: false
        movable: false
    }

    TableViewColumn {
        id: col2
        role: "itemData"
        title: qsTranslate("PrintParameterPanel", "Filament")
        width: columnWidth[2] * screenScaleFactor
        movable: false
    }

    TableViewColumn {
        id: supportColumn
        role: "itemData"
        title: ""
        movable: false
    }

    style: TreeViewStyle{
        id: treeStyle
        branchDelegate: Item{
            width: 10* screenScaleFactor
            height: cusTree.rowHeight
            Image {
                property string downImgSource: styleData.isExpanded ? Constants.downBtnImgSource: Constants.rightBtnImgSource
                id: idImg
                anchors.centerIn: parent
                sourceSize: styleData.isExpanded ? Qt.size(7 * screenScaleFactor, 4 * screenScaleFactor) : Qt.size(4 * screenScaleFactor, 7 * screenScaleFactor)
                source: downImgSource
                anchors.left: parent.left
                anchors.leftMargin: 10 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        handle:Rectangle{
            opacity: 0.8
            radius: width / 2
            implicitWidth: 6 * screenScaleFactor
            implicitHeight: 6 * screenScaleFactor
            color: Constants.currentTheme ? "#D4D4DA" : "#7E7E84"
        }
        scrollBarBackground: Item{
            implicitWidth: 6 * screenScaleFactor
            implicitHeight: 6 * screenScaleFactor
        }
        corner:Item{
        }
        frame: Rectangle{
            radius: 5
            color:"transparent"
        }
        decrementControl:Item{

        }
        incrementControl:Item{
        }


        rowDelegate:Rectangle {
            id: itemDel
            width: cusTree.rowHeight
            height: rowHeight
            color: styleData.selected ?  (cusTree.currentEditRow !== styleData.index ? "grey" : Constants.themeColor_New) : Constants.right_panel_bgColor
        }

        itemDelegate: Item{
            width: 100 * screenScaleFactor
            height: cusTree.rowHeight
            Loader{
                property var styleDataL : styleData
                anchors.fill: parent
                sourceComponent: styleDataL.value.objectType !== undefined ?
                                     choiseItemDelegate(styleData.column, styleDataL.value.objectType, styleData.row) : null

                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    acceptedButtons: Qt.RightButton

                    onClicked: {
                        mouse.accepted = false
                        var menuType = cusTree.model.rightMenuType(styleData.index)
                        kernel_rclick_menu_data.updateState();
                        if (kernel_rclick_menu_data.menuType === 0) {
                            noModelMenu.popup();
                        } else if (!kernel_model_selector.wipeTowerSelected) {
                            if (kernel_rclick_menu_data.menuType === 1)
                                singleModelMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 2)
                                singleReliefMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 3)
                                multiModelsMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 4)
                                normalPartMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 5)
                                modifierPartMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 6)
                                specialPartMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 7)
                                normalReliefMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 8)
                                modifierReliefMenu.popup();
                            else if (kernel_rclick_menu_data.menuType === 9)
                                specialReliefMenu.popup();
                        }
                    }
                }

            }
        }
        headerDelegate: CusRoundedBg{
            y: 0 * screenScaleFactor
            color:Constants.right_panel_bgColor
            borderWidth: 1 * screenScaleFactor
            borderColor: "transparent"
            height: cusTree.rowHeight
            leftTop: styleData.column === 0
            rightTop: styleData.column === 6
            Rectangle{
                width: parent.width
                height: 1 * screenScaleFactor
                radius: 5 * screenScaleFactor
                border.width: 0
                color: Constants.right_panel_border_default_color
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle{
                width: 1 * screenScaleFactor
                height: parent.height
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                border.width: 0
                color: Constants.right_panel_border_default_color
            }

            Rectangle{
                width: parent.width
                height: 1 * screenScaleFactor
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                border.width: 0
                color: Constants.right_panel_border_default_color
            }

            Text {
                anchors.centerIn: parent
                font.pointSize: Constants.labelFontPointSize_9
                color: Constants.themeTextColor
                text: styleData.value
                font.family: Constants.labelFontFamily
            }
        }
    }
    selection: cusTree.model.treeSelectModel
    selectionMode: SelectionMode.ExtendedSelection
    backgroundVisible: false

    function isItemVisible(totalRow){
        if(totalRow*rowHeight - cusTree.contentItem.contentY > rowHeight &&
                totalRow*rowHeight - cusTree.contentItem.contentY < cusTree.contentItem.height){
            return true
        }else{
            return false
        }
    }

    function isIndexExpand(itemIndex){
        console.log("itemIndex = ", itemIndex)
        let res = isExpanded(itemIndex)
        console.log("res = ", res)
        return res;
    }

    Connections{
        target: cusTree.model.treeSelectModel
        function onSigSelectionChanged(lastIndex){
            let rowTotalCount = cusTree.model.findModelIndexTotalRow(lastIndex)
            let res = isItemVisible(rowTotalCount)
            if(!res)
            {
                let dstY = rowTotalCount*rowHeight - cusTree.contentItem.height
                if(dstY < rowHeight*(-1)){
                    dstY = rowHeight*(-1)
                }

                dstY = dstY + flickableItem.originY + 24
                cusTree.contentItem.contentY = dstY
            }
        }
    }

    onClicked: {
        console.log("index111 = ", index)
        console.log("index.row = ", index.column)
        if(preIndex !== index){
            currentEditRow = undefined
        }else{
            //            currentEditRow = index
        }

        preIndex = index
    }

    onPressAndHold: {
        console.log("onPressAndHold contentY = ", cusTree.contentItem.contentY)
    }


    Connections{
        target: kernel_objects_list_data
        function onSigExpandIndex(expandIndex){
            console.log("expandIndex = ", expandIndex)
            cusTree.expand(expandIndex)
        }
    }

    Connections{
        target: cusTree.model.treeSelectModel
        function onSigSelectionError(selectModelIndexs, errCode){
            if(errCode === 0){
                warningMsg1.msgText = qsTr("For multi-selection, object/part types must match.")
            }else if(errCode === 1){
                warningMsg1.msgText = qsTr("If the first selection is a part, the second selection must also be a part in the same object.")
            }

            warningMsg1.show()
        }
    }



    UploadMessageDlg {
        property var selections
        id: warningMsg1
        msgText: qsTr("If the first selection is a part, the second selection must also be a part in the same object.")
        cancelBtnVisible: false
        visible: false
    }

    function choiseItemDelegate(column, objectType){
        switch(column){
        case 0:
            return objectType ===0 ? itemColumn1Plate :
                                     (objectType === 1 ? itemColumn1Group : itemColumn1Model);
        case 1:
            return objectType ===1 ? hideCom : null;
        case 2:
            return objectType ===1 || objectType ===2 ? itemColumn2 : null
        case 3:
            return objectType ===1 ? itemColumn3 : null
            //        case 4:
            //            return objectType ===1 ? itemColumn4: null
            //        case 5:
            //            return objectType ===1 ? itemColumn5 : null
            //        case 6:
            //            return objectType ===1 ? itemColumn6 : null
        default:
            return typeCom
        }
    }

    function getIconByModelType(modelType, isHovered, isRelief){
        switch(modelType){
        case 0:
            return isHovered ? (isRelief ? Constants.normalReliefD : Constants.normalD) : (isRelief ? Constants.normalRelief : Constants.normal)
        case 1:
            return isHovered ? (isRelief ? Constants.negativeReliefD : Constants.negativeD) : (isRelief ? Constants.negativeRelief : Constants.negative)
        case 2:
            return isHovered ? (isRelief ? Constants.modifierReliefD : Constants.modifierD) : (isRelief ? Constants.modifierRelief : Constants.modifier)
        case 3:
            return isHovered ? Constants.supportDefenderD : Constants.supportDefender
        case 4:
            return isHovered ? Constants.supportGeneratorD : Constants.supportGenerator
        default:
            return Constants.normalD
        }
    }

    Component{
        id: typeCom
        Item{
            Text{
                anchors.centerIn: parent
                text: styleDataL.value
            }
        }
    }

    Component{
        id: hideCom
        Item{
            Image{
                id: idAreaImage
                property string visImg: Constants.currentTheme? "qrc:/UI/photo/rightDrawer/update/show_light_default.svg" : "qrc:/UI/photo/modelVis_d.svg"
                property string noVisImg: Constants.currentTheme? "qrc:/UI/photo/rightDrawer/update/hidden_light_default.svg" : "qrc:/UI/photo/modelNoVis_d.svg"
                property string imgPath: styleDataL.value.itemData.isVisible ? visImg : noVisImg
                sourceSize: Qt.size(12*screenScaleFactor, 7*screenScaleFactor)
                source: imgPath
                anchors.centerIn: parent
            }
            MouseArea{
                hoverEnabled: true
                anchors.fill: parent
                onClicked: {
                    styleDataL.value.itemData.isVisible = !styleDataL.value.itemData.isVisible
                }
            }
        }
    }

    Component{
        id: itemColumn1Group
        Item{
            id: delRec
            MouseArea{
                enabled: styleDataL.selected
                anchors.fill: parent
                onClicked: {
                    currentEditRow = styleDataL.index
                }
            }

            StackLayout{
                id: colGroupStack
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                currentIndex: styleDataL.index === cusTree.currentEditRow ? 1 : 0
                Text{
                    id: groupNameText
                    text: styleDataL.value.itemData ? styleDataL.value.itemData.groupName : ""
                    font.pointSize: Constants.labelFontPointSize_9
                    color: Constants.themeTextColor
                    font.family: Constants.labelFontFamily
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                }

                BasicDialogTextField {
                    id: groupEdit
                    color: Constants.themeTextColor
                    itemBorderColor: itemBorderHoverdColor
                    font.pointSize: Constants.labelFontPointSize_9
                    text: groupNameText.text
                    validator: RegExpValidator{}
                    onEditingFinished: {
                        if(groupEdit.text){
                            styleDataL.value.itemData.groupName = groupEdit.text
                            cusTree.currentEditRow = undefined
                        }else{
                            groupEdit.text = Qt.binding(function(){ return groupNameText.text; })
                        }
                    }
                    onActiveFocusChanged: {
                        if(!activeFocus){
                            if(groupEdit.text){
                                styleDataL.value.itemData.groupName = groupEdit.text
                                cusTree.currentEditRow = undefined
                            }else{
                                groupEdit.text = Qt.binding(function(){ return groupNameText.text; })
                            }
                        }
                    }
                }
            }
        }
    }

    Component{
        id: itemColumn1Model
        Item{
            MouseArea{
                enabled: styleDataL.selected
                anchors.fill: parent
                onClicked: {
                    currentEditRow = styleDataL.index
                }
            }
            StackLayout{
                id: colModelStack
                anchors.fill: parent
                currentIndex: styleDataL.index === cusTree.currentEditRow ? 1 : 0
                Row{
                    spacing: 6 * screenScaleFactor
                    Image{
                        anchors.verticalCenter: parent.verticalCenter
                        source:  getIconByModelType(styleDataL.value.itemData.modelNType, styleDataL.selected, styleDataL.value.itemData.isRelief)
                    }
                    CusText{
                        id: modelNameText
                        anchors.verticalCenter: parent.verticalCenter
                        fontWidth: itemWidth- 70*screenScaleFactor
                        fontText:styleDataL.value.itemData.name
                        font.pointSize: Constants.labelFontPointSize_9
                        fontColor: Constants.themeTextColor
                        hAlign: 0
                    }
                }

                BasicDialogTextField {
                    id: modelNameEdit
                    color: Constants.themeTextColor
                    font.pointSize: Constants.labelFontPointSize_9
                    itemBorderHoverdColor: itemBorderColor
                    text: modelNameText.fontText
                    validator: RegExpValidator{}
                    onEditingFinished: {
                        if(modelNameEdit.text){
                            styleDataL.value.itemData.name = modelNameEdit.text
                            cusTree.currentEditRow = undefined
                        }else{
                            modelNameEdit.text = Qt.binding(function(){ return modelNameText.fontText; })
                        }
                    }
                    onActiveFocusChanged: {
                        if(!activeFocus){
                            if(modelNameEdit.text){
                                styleDataL.value.itemData.name = modelNameEdit.text
                                cusTree.currentEditRow = undefined
                            }else{
                                modelNameEdit.text = Qt.binding(function(){ return modelNameText.fontText; })
                            }
                        }
                    }
                }
            }
        }
    }

    Component{
        id: itemColumn1Plate
        Item{
            id: delRec
            enabled: styleDataL.value.itemData !== -1
            MouseArea{
                enabled: styleDataL.selected
                anchors.fill: parent
                onClicked: {
                    currentEditRow = styleDataL.index
                }
            }
            StackLayout{
                id: textStack
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                currentIndex: styleDataL.index === cusTree.currentEditRow ? 1 : 0

                Text{
                    id: plateNameText
                    text:{
                        if(styleDataL.value.itemData === -1 ){
                            qsTranslate("ModelListPop", "Outside Plate")
                        }else{
                            var plateIndex = qsTranslate("ModelListPop", "Plate")  + " " + (styleDataL.value.itemData.index + 1)
                            var plateName =   styleDataL.value.itemData.name ? "(" + styleDataL.value.itemData.name + ")" : ""
                            return plateIndex + plateName
                        }
                    }
                    font.pointSize: Constants.labelFontPointSize_9
                    color: Constants.themeTextColor
                    font.family: Constants.labelFontFamily
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignLeft
                }

                BasicDialogTextField {
                    id: plateEdit
                    color: Constants.themeTextColor
                    font.pointSize: Constants.labelFontPointSize_9
                    itemBorderHoverdColor: itemBorderColor
                    text: styleDataL.value.itemData.name
                    validator: RegExpValidator{}
                    onEditingFinished: {
                        styleDataL.value.itemData.name = plateEdit.text
                        cusTree.currentEditRow = undefined
                    }
                    onActiveFocusChanged: {
                        if(!activeFocus){
                            styleDataL.value.itemData.name = plateEdit.text
                            cusTree.currentEditRow = undefined
                        }
                    }
                }
            }
        }
    }

    Component{
        id: itemColumn2
        Item{
            Rectangle{
                property bool isHover: false
                property int colorIndex: styleDataL.value.itemData.defaultColorIndex
                id: colorPickBtn
                radius: 9
                visible: !(styleDataL.value.itemData.modelNType===1 ||
                           styleDataL.value.itemData.modelNType===3 ||
                           styleDataL.value.itemData.modelNType===4 ||
                           styleDataL.value.itemData.modelNType===5 ||
                           styleDataL.value.itemData.modelNType===6 ||
                           styleDataL.value.itemData.modelNType===7)
                border.width: styleDataL.selected ? 1*screenScaleFactor : 0*screenScaleFactor
                border.color: "#ffffff"
                width: 30*screenScaleFactor
                height: 18*screenScaleFactor
                anchors.centerIn: parent
                color: currentMachine.extruderObject(colorIndex).color
                onColorChanged: {
                    numText.color = Constants.setTextColor(color)
                }

                Text{
                    id: numText
                    anchors.centerIn: parent
                    text: styleDataL.value.itemData.defaultColorIndex + 1
                    Component.onCompleted: {
                        Constants.setTextColor(colorPickBtn.color)
                    }
                    onTextChanged: {
                        Constants.setTextColor(colorPickBtn.color)
                    }
                }

                Popup {
                    property real modelIndex: 0
                    property var curentItem: styleDataL.value.itemData

                    id:colorPic
                    width: colorListView.count *34*screenScaleFactor + 15*screenScaleFactor
                    height: 50*screenScaleFactor
                    x: colorPickBtn.x + width
                    y: colorPickBtn.y + colorPickBtn.height
                    contentItem: ListView{
                        id: colorListView
                        clip: true
                        spacing: 5*screenScaleFactor
                        model: kernel_parameter_manager.currentMachineObject.extrudersModel
                        orientation: ListView.Horizontal
                        delegate: Rectangle{
                            id: rec
                            property bool isHover: false
                            width: 27*screenScaleFactor
                            height: 27*screenScaleFactor
                            radius:3*screenScaleFactor
                            color: !isHover ? "transparent" : Constants.themeGreenColor
                            Rectangle{
                                anchors.fill: parent
                                anchors.margins: 2*screenScaleFactor
                                border.width:  1*screenScaleFactor
                                border.color: "#000000"
                                color: model.extruderColor
                                radius:3*screenScaleFactor
                                onColorChanged: {
                                    colorText.color = Constants.setTextColor(color)
                                }
                                Text{
                                    id: colorText
                                    anchors.centerIn: parent
                                    text: model.index + 1
                                    Component.onCompleted:  color = Constants.setTextColor(parent.color)
                                }
                            }

                            MouseArea{
                                hoverEnabled: true
                                anchors.fill: parent
                                onClicked: {
                                    styleDataL.value.itemData.defaultColorIndex = model.index
                                    styleDataL.value.setColorIndex(model.index)
                                    colorPic.close()
                                }
                                onExited: {
                                    rec.isHover = false
                                }
                                onEntered: {
                                    rec.isHover = true
                                }
                            }
                        }
                    }
                    background: Rectangle{
                        radius: 5 * screenScaleFactor
                        color:Constants.dialogItemRectBgColor
                        border.width:1 * screenScaleFactor
                        border.color: "#262626"
                    }
                    onVisibleChanged: {
                        if(visible){
                            colorListView.model = kernel_parameter_manager.currentMachineObject.extrudersModel
                        }
                    }
                }

                MouseArea{
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        colorPic.x = mouse.x - colorPic.width
                        colorPic.y = colorPickBtn.y +25* screenScaleFactor
                        colorPic.visible = true
                    }
                    onExited: {
                        colorPickBtn.isHover = false
                    }
                    onEntered: {
                        colorPickBtn.isHover = true
                        console.log("+++index = ", model.index)
                    }
                }
            }
        }
    }

    Component{
        id: itemColumn3
        Row{
            anchors.left: parent.left
            anchors.leftMargin: 12 * screenScaleFactor
            spacing: 12 * screenScaleFactor
            enabled: styleDataL.selected && kernel_kernel.currentPhase === 0
            Image{
                id: supportImage
                visible: styleDataL.value.itemData.hasSupportsData
                sourceSize.width: 14 * screenScaleFactor
                sourceSize.height: 14 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/UI/photo/leftBar/support_pressed.svg"
                MouseArea{
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        topToolBar.switchMakeSupport()
                    }
                    onEntered: {
                        supportImage.source = "qrc:/UI/photo/leftBar/support_dark.svg"
                    }
                    onExited: {
                        supportImage.source = "qrc:/UI/photo/leftBar/support_pressed.svg"
                    }
                }
            }

            Image{
                id: zImage
                visible: styleDataL.value.itemData.hasSeamsData
                sourceSize.width: 14 * screenScaleFactor
                sourceSize.height: 14 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/UI/photo/leftBar/seam_painting_p.svg"
                MouseArea{
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        standaloneWindow.topToolBar.switchMakeZ()
                    }
                    onEntered: {
                        zImage.source = "qrc:/UI/photo/leftBar/seam_painting_n.svg"
                    }
                    onExited: {
                        zImage.source = "qrc:/UI/photo/leftBar/seam_painting_p.svg"
                    }
                }
            }

            Image{
                id: colorImage
                visible: styleDataL.value.itemData.hasColors
                sourceSize.width: 14 * screenScaleFactor
                sourceSize.height: 14 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/UI/photo/colorMode.svg"
                MouseArea{
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        topToolBar.switchMakeColor()
                    }
                    onEntered: {
                        colorImage.source = "qrc:/UI/photo/colorMode_d.svg"
                    }
                    onExited: {
                        colorImage.source = "qrc:/UI/photo/colorMode.svg"
                    }
                }
            }

            Image{
                id: adaptivelayerImage
                visible: styleDataL.value.itemData.adaptiveLayerData
                sourceSize.width: 14 * screenScaleFactor
                sourceSize.height: 14 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/UI/photo/leftBar/adaptivelayer_p.svg"
                MouseArea{
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        topToolBar.switchMakeAdaptiveLayer()
                    }
                    onEntered: {
                        adaptivelayerImage.source = "qrc:/UI/photo/leftBar/adaptivelayer_p.svg"
                    }
                    onExited: {
                        adaptivelayerImage.source = "qrc:/UI/photo/leftBar/adaptivelayer_p.svg"
                    }
                }
            }

        }
    }
    onModelChanged:{
        console.log("onModelChanged == ", onModelChanged)
    }

    Component.onCompleted:{
        cusTree.model.setTreeView(cusTree)
        initContentY = cusTree.contentItem.contentY
        console.log("initContentY == ", initContentY)
    }
}

