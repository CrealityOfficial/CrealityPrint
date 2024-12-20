import "../components"
import "../qml"
import "../secondqml"
import QtQml 2.13
import QtQuick 2.10
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

DockItem {
    id: root

    property int themeType: -1
    property var sourceTheme: ""
    property bool initNozzleCheckCount: true

    signal checkPrinterClicked()

    function showLeastOnePrinterMsg() {
        if (kernel_parameter_manager.machinesCount === 0) {
            addLeastOnePrinterTip.visible = true;
            return ;
        }
        this.visible = false;
    }

    title: qsTr("Add Printer")
    modality: Qt.ApplicationModal
    width: 1214 * screenScaleFactor
    height: 730 * screenScaleFactor
    panelColor: sourceTheme.background_color
    onVisibleChanged: {
        if (visible) {
        }
    }
    onThemeTypeChanged: sourceTheme = themeModel.get(themeType) //切换主题

    UploadMessageDlg {
        id: addLeastOnePrinterTip

        centerToWindow: false
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        visible: false
        msgText: qsTr("Please add at least one printer")
        cancelBtnVisible: false
        ignoreBtnVisible: false
        onSigOkButtonClicked: {
            this.visible = false;
        }
    }

       UploadMessageDlg {
        id: searchTip

        centerToWindow: false
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        visible: false
        msgText: qsTr("This model is not available yet")
        cancelBtnVisible: false
        ignoreBtnVisible: false
        onSigOkButtonClicked: this.visible = false;
    }

    AddPrinterDlgChoseNuzzle {
        id: idAddPrinterDlg

        onChoseAccepted: Qt.callLater(function() {
            root.close();
        })
    }

    ListModel {
        id: collectModel
    }

    ListModel {
        id: themeModel

        ListElement {
            //Dark Theme
            background_color: "#4B4B4D"
            text_light_color: "#CBCBCC"
            text_weight_color: "#333333"
            text_disable_color: "#999999"
            scrollbar_color: "#7E7E84"
            item_border_color: "#7A7A7F"
            btn_default_color: "#4B4B4D"
            btn_hovered_color: "#7A7A7F"
            textedit_text_color: "#FEFEFE"
            textedit_selection_color: "#1E9BE2"
            treeview_indicator_border: "#CBCBCC"
            treeview_text_selection_color: "#00A3FF"
            search_btn_img: "qrc:/UI/photo/search.png"
            treeView_plus_img: "qrc:/UI/photo/treeView_plus_dark.png"
            treeView_minus_img: "qrc:/UI/photo/treeView_minus_dark.png"
            treeView_checked_img: "qrc:/UI/photo/treeView_checked_icon.png"
        }

        ListElement {
            //Light Theme
            background_color: "#FFFFFF"
            text_light_color: "#333333"
            text_weight_color: "#333333"
            text_disable_color: "#999999"
            scrollbar_color: "#D6D6DC"
            item_border_color: "#CBCBCC"
            btn_default_color: "#FFFFFF"
            btn_hovered_color: "#CBCBCC"
            textedit_text_color: "#333333"
            textedit_selection_color: "#00A3FF"
            treeview_indicator_border: "#CBCBCC"
            treeview_text_selection_color: "#00A3FF"
            search_btn_img: "qrc:/UI/photo/search.png"
            treeView_plus_img: "qrc:/UI/photo/treeView_plus_light.png"
            treeView_minus_img: "qrc:/UI/photo/treeView_minus_light.png"
            treeView_checked_img: "qrc:/UI/photo/treeView_checked_icon.png"
        }

    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: root.currentTitleHeight + anchors.bottomMargin

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.topMargin: 20 * screenScaleFactor

            RowLayout {
                Layout.leftMargin: 20 * screenScaleFactor
                Layout.alignment: Qt.AlignLeft

                BasicLoginTextEdit {
                    id: search_edit

                    property string prevText: ""

                    function searchPrinter() {
                        if (!search_edit.text)
                            return ;

                        let search_text = search_edit.text.toLowerCase();
                        let typeNameList = kernel_add_printer_model.typeNameList;
                        for (let item of typeNameList) {
                            let printerList = kernel_add_printer_model.getPrinterNameList(item);
                            for (let printer of printerList) {
                                let lowerCaseName = printer.toLowerCase();
                                if (lowerCaseName.startsWith(search_text)){
                                     kernel_add_printer_model.selectPrinter(item, printer);
                                     return
                                }
                            }
                        }
                        searchTip.visible = true
                    }

                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: 280 * screenScaleFactor
                    Layout.preferredHeight: 30 * screenScaleFactor
                    radius: height / 2
                    text: ""
                    placeholderText: qsTr("Search")
                    font.family: Constants.labelFontFamily
                    font.pointSize: Constants.labelFontPointSize_9
                    imgPadding: 12 * screenScaleFactor
                    headImageSrc: hovered ? Constants.searchBtnImg_d : Constants.searchBtnImg
                    tailImageSrc: hovered && search_edit.text != "" ? Constants.clearBtnImg : ""
                    hoveredTailImageSrc: Constants.clearBtnImg_d
                    itemBorderHoverdColor: Constants.dialogItemRectBgBorderColor
                    onEditingFinished: searchPrinter()
                    onTailBtnClicked: search_edit.text = ""
                    verticalAlignment:Text.AlignVCenter
                    clip: true
                    antialiasing: true

                    CusRoundedBg {
                        height: parent.height
                        width: 60 * screenScaleFactor
                        rightTop: true
                        rightBottom: true
                        txtText: qsTr("Search")
                        anchors.right: parent.right
                        anchors.rightMargin: 0
                        radius: (height / 2)
                        color: Constants.dialogTitleColor
                        txtColor: Constants.manager_printer_label_color

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: search_edit.searchPrinter()
                        }

                    }

                }

            }

            Item {
                Layout.fillWidth: true
            }

            Row{
                Layout.rightMargin: 40 * screenScaleFactor
                Layout.alignment: Qt.AlignRight
                visible:screenScaleFactor>1
                spacing:10 * screenScaleFactor
                BasicDialogButton {

                width: 80 * screenScaleFactor
                height: 28 * screenScaleFactor
                text: qsTr("Ok")
                btnRadius: 15 * screenScaleFactor
                btnBorderW: 1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    let havePrinter = false;
                    for (let i = 0; i < collectModel.count; i++) {
                        let current = collectModel.get(i).changeCheck;
                        let init = collectModel.get(i).initCheck;
                        let name = collectModel.get(i).name;
                        let nozzle = collectModel.get(i).nozzle;
                        if (current)
                            havePrinter = true;

                        if (init !== current) {
                            if (current) {
                                kernel_parameter_manager.addMachine(name, nozzle);
                            } else {
                                var machineObject = kernel_parameter_manager.getMachineObjectByName(name, nozzle);
                                kernel_parameter_manager.removeMachine(machineObject);
                            }
                        }
                    }
                    if (!havePrinter){
                        showLeastOnePrinterMsg()
                        return
                    }

                    collectModel.clear();
                    initNozzleCheckCount = true;
                    root.close();
                }
            }

            BasicDialogButton {

                width: 80 * screenScaleFactor
                height: 28 * screenScaleFactor
                text: qsTr("Cancel")
                btnRadius: 15 * screenScaleFactor
                btnBorderW: 1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                onSigButtonClicked: {
                    if (kernel_parameter_manager.machinesCount === 0) {
                        addLeastOnePrinterTip.visible = true;
                        return
                    }
                    for (let i = 0; i < itemRepeater.count; ++i) {
                        itemRepeater.itemAt(i).reset();
                    }
                    root.close();
                }
            }

            }

            BasicDialogButton {
                //                onSigButtonClicked:addPrinterStep.visible = true

                visible: false
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 30 * screenScaleFactor
                Layout.preferredHeight: 30 * screenScaleFactor
                Layout.preferredWidth: 80 * screenScaleFactor
                text: qsTr("New")
                btnRadius: 15 * screenScaleFactor
                btnBorderW: 1
                defaultBtnBgColor: Constants.leftToolBtnColor_normal
                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            }

        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 10 * screenScaleFactor
        }

        RowLayout {
            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: 30 * screenScaleFactor
            }

            ColumnLayout {
                BasicScrollView {
                    id: idListScrollView

                    clip: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 159 * screenScaleFactor
                    Layout.maximumWidth: 185 * screenScaleFactor
                    hpolicyVisible: contentWidth > width
                    vpolicyVisible: contentHeight > height

                    Column {
                        width: 125 * screenScaleFactor
                        spacing: 0

                        Repeater {
                            model: kernel_add_printer_model.typeNameList

                            delegate: Column {
                                id: type_list

                                width: 125 * screenScaleFactor
                                spacing: 0

                                RowLayout {
                                    spacing: 5

                                    AddPrinterButton {
                                        id: type_button

                                        property var checkCount: 0

                                        function checkTypeCount() {
                                            let list = kernel_parameter_manager.machineNameList;
                                            checkCount = 0;
                                            let currentTypeList = kernel_add_printer_model.getPrinterNameList(modelData);
                                            for (let item of currentTypeList) {
                                                for (let m of list) {
                                                    let idex = m.indexOf("-");
                                                    let name = m.substring(0, idex);
                                                    if (name === item)
                                                        checkCount++;

                                                }
                                            }
                                            return checkCount;
                                        }

                                        typeName: modelData
                                        subButtonDefaultImage: root.sourceTheme.treeView_plus_img
                                        subButtonSelectedImage: root.sourceTheme.treeView_minus_img
                                        textColor: root.sourceTheme.text_light_color
                                        Layout.alignment: Qt.AlignVCenter

                                        Connections {
                                            function onMachineListChange() {
                                                type_button.checkTypeCount();
                                            }

                                            target: kernel_parameter_manager
                                        }

                                        BasicTooltip {
                                            visible: parent.hovered
                                            position: BasicTooltip.Position.RIGHT
                                            text: qsTr("Selected quantity") + ": " + type_button.checkTypeCount()
                                        }

                                    }

                                    CusImglButton {
                                        Layout.preferredWidth: 14 * screenScaleFactor
                                        Layout.preferredHeight: 14 * screenScaleFactor
                                        btnRadius: height / 2
                                        borderWidth: 0
                                        state: "imgOnly"
                                        cusTooltip.text: qsTr("Clear series")
                                        enabledIconSource: "qrc:/UI/photo/addPrinterSource/clear_printer-default.svg"
                                        pressedIconSource: "qrc:/UI/photo/addPrinterSource/clear_printer-hover.svg"
                                        highLightIconSource: "qrc:/UI/photo/addPrinterSource/clear_printer-hover.svg"
                                        defaultBtnBgColor: Constants.leftBtnBgColor_normal
                                        hoveredBtnBgColor: Constants.leftBtnBgColor_normal
                                        selectedBtnBgColor: Constants.leftBtnBgColor_normal
                                        btnTextColor: Constants.leftTextColor
                                        btnTextColor_d: Constants.leftTextColor_d
                                        visible: type_button.isOpened && checkCountText.checkCount > 0
                                        onClicked: {
                                            let list = kernel_parameter_manager.machineNameList;
                                            let currentTypeList = kernel_add_printer_model.getPrinterNameList(modelData);
                                            for (let item of currentTypeList) {
                                                for (let m of list) {
                                                    let idex = m.indexOf("_");
                                                    let name = m.substring(0, idex);
                                                    let nozzle = m.substring(idex + 1).replace(" nozzle", "");
                                                    console.log(name, nozzle, m, m === item, 111);
                                                    if (name === item) {
                                                        let codeName = kernel_parameter_manager.getMachineCodeName(name);
                                                        console.log(name, nozzle, codeName, 222);
                                                        var machineObject = kernel_parameter_manager.getMachineObjectByName(codeName, nozzle);
                                                        kernel_parameter_manager.removeMachine(machineObject);
                                                    }
                                                }
                                            }
                                        }
                                    }

                                }

                                Column {
                                    width: 125 * screenScaleFactor
                                    spacing: 0

                                    Repeater {
                                        model: kernel_add_printer_model.getPrinterNameList(type_button.typeName)

                                        delegate: RowLayout {
                                            visible: type_button.isOpened

                                            Text {
                                                id: checkCountImage

                                                property var isCheck: false

                                                function initCheck() {
                                                    isCheck = false;
                                                    let codeName = kernel_parameter_manager.getMachineCodeName(modelData);
                                                    var extruder = kernel_parameter_manager.extruderSupportDiameters(codeName, 0);
                                                    for (let i = 0; i < extruder.length; i++) {
                                                        let machine = kernel_parameter_manager.machineNozzleSelectList(modelData);
                                                        if (machine.includes(extruder[i]))
                                                            isCheck = true;

                                                    }
                                                    return isCheck;
                                                }

                                                text: type_button.isOpened && initCheck() ? "√" : "  "
                                                color: Constants.right_panel_text_default_color
                                                Layout.alignment: Qt.AlignVCenter
                                                verticalAlignment: Text.AlignVCenter
                                                horizontalAlignment: Text.AlignRight
                                                font.weight: Font.ExtraBold
                                                font.pointSize: Constants.labelFontPointSize_8
                                                Layout.preferredWidth: 30 * screenScaleFactor

                                                Connections {
                                                    function onMachineListChange() {
                                                        checkCountImage.initCheck();
                                                    }

                                                    target: kernel_parameter_manager
                                                }

                                            }

                                            AddPrinterButton {
                                                id: printer_button

                                                visible: type_button.isOpened
                                                typeName: type_button.typeName
                                                printerName: modelData
                                                textColor: root.sourceTheme.text_light_color
                                                Layout.alignment: Qt.AlignVCenter
                                            }

                                        }

                                    }

                                }

                            }

                        }

                    }

                    hpolicyindicator: Rectangle {
                        radius: height / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 200 * screenScaleFactor
                        implicitHeight: 6 * screenScaleFactor
                    }

                    vpolicyindicator: Rectangle {
                        radius: width / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 6 * screenScaleFactor
                        implicitHeight: 200 * screenScaleFactor
                    }

                }

            }

            Item {
                Layout.preferredWidth: 30 * screenScaleFactor
                Layout.fillHeight: true
            }

            Rectangle {
                color: Constants.right_panel_item_default_color
                width: 920 * screenScaleFactor
                height: 560 * screenScaleFactor
                radius: 5 * screenScaleFactor

                BasicScrollView {
                    id: idModelScrollView

                    clip: true
                    anchors.top: parent.top
                    anchors.topMargin: 20 * screenScaleFactor
                    anchors.left: parent.left
                    anchors.leftMargin: 20 * screenScaleFactor
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 20 * screenScaleFactor
                    anchors.fill: parent
                    hpolicyVisible: false
                    vpolicyVisible: contentHeight > height

                    Flow {
                        width: idModelScrollView.width
                        spacing: 20 * screenScaleFactor

                        Repeater {
                            //  nozzleSize   : model_nozzle_size
                            //          addEnabled   : !model_added

                            id: itemRepeater

                            model: kernel_add_printer_model

                            delegate: AddPrinterViewItem {
                                property var model_index:-1
                                imageUrl: model_image_url
                                printerName: model_name
                                printerCodeName: model_code_name
                                machineDepth: model_depth
                                machineWidth: model_width
                                machineHeight: model_height
                                onAddMachine: {
                                    idAddPrinterDlg.machineName = printerName;
                                    idAddPrinterDlg.nozzleNum = model_nozzle_count;
                                    idAddPrinterDlg.isGranular = model_granular;
                                    idAddPrinterDlg.height = 600 * screenScaleFactor;
                                    idAddPrinterDlg.visible = true;
                                }
                                onAddPrinter: (printer, nozzle, initCheck, changeCheck) => {
                                        var bFind = false;
                                        for (let i = 0; i < collectModel.count; i++) {
                                            if (collectModel.get(i).name === printer && collectModel.get(i).nozzle === nozzle){
                                                collectModel.setProperty(i, "changeCheck", changeCheck);
                                                bFind = true;
                                                break;
                                            }
                                        }
                                        if (!bFind) {
                                            collectModel.append({
                                                "name": printer,
                                                "nozzle": nozzle,
                                                "initCheck": initCheck,
                                                "changeCheck": changeCheck
                                            });
                                        }
                                }
                            }

                        }

                    }

                    hpolicyindicator: Rectangle {
                        radius: height / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 200 * screenScaleFactor
                        implicitHeight: 6 * screenScaleFactor
                    }

                    vpolicyindicator: Rectangle {
                        radius: width / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 6 * screenScaleFactor
                        implicitHeight: 200 * screenScaleFactor
                    }

                }

            }

        }

        // Grid {
        //     visible:screenScaleFactor<=1
        //     columns: 3
        //     spacing: 10
        //     Layout.alignment: Qt.AlignHCenter
        //     Layout.bottomMargin: 15 * screenScaleFactor

        //     //            BasicDialogButton {
        //     //                width: 100*screenScaleFactor
        //     //                height: 30*screenScaleFactor
        //     //                text: qsTr("新建")
        //     //                btnRadius:15*screenScaleFactor
        //     //                btnBorderW:1
        //     //                defaultBtnBgColor: Constants.leftToolBtnColor_normal
        //     //                hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
        //     //                onSigButtonClicked:addPrinterStep.visible = true
        //     //            }
        //     BasicDialogButton {
        //         id: basicComButton

        //         width: 80 * screenScaleFactor
        //         height: 28 * screenScaleFactor
        //         text: qsTr("Ok")
        //         btnRadius: 15 * screenScaleFactor
        //         btnBorderW: 1
        //         defaultBtnBgColor: Constants.leftToolBtnColor_normal
        //         hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
        //         onSigButtonClicked: {
        //             let remove_machines = [];
        //             let havePrinter = false;
        //             for (let i = 0; i < collectModel.count; i++) {
        //                 let current = collectModel.get(i).changeCheck;
        //                 let init = collectModel.get(i).initCheck;
        //                 let name = collectModel.get(i).name;
        //                 let nozzle = collectModel.get(i).nozzle;
        //                 if (current)
        //                     havePrinter = true;

        //                 if (init !== current) {
        //                     if (current) {
        //                         kernel_parameter_manager.addMachine(name, nozzle);
        //                     } else {
        //                         var machineObject = kernel_parameter_manager.getMachineObjectByName(name, nozzle);
        //                         remove_machines.push(machineObject);
        //                     }
        //                 }
        //             }
        //             for (let i = 0; i < remove_machines.length; i++) {
        //                 kernel_parameter_manager.removeMachine(remove_machines[i]);
        //             }
        //             collectModel.clear();
        //             initNozzleCheckCount = true;
        //             if (!havePrinter){
        //                 showLeastOnePrinterMsg()
        //                 return
        //             }
            
        //             root.close();
        //         }
        //     }

        //     BasicDialogButton {
        //         id: basicComButton2

        //         width: 80 * screenScaleFactor
        //         height: 28 * screenScaleFactor
        //         text: qsTr("Cancel")
        //         btnRadius: 15 * screenScaleFactor
        //         btnBorderW: 1
        //         defaultBtnBgColor: Constants.leftToolBtnColor_normal
        //         hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
        //         onSigButtonClicked: {
        //             if (kernel_parameter_manager.machinesCount === 0) {
        //                 addLeastOnePrinterTip.visible = true;
        //                 return
        //             }
        //             for (let i = 0; i < itemRepeater.count; ++i) {
        //                 itemRepeater.itemAt(i).reset();
        //             }
        //             root.close();
        //         }
        //     }

        // }

    }

    Binding on themeType {
        value: Constants.currentTheme
    }

}
