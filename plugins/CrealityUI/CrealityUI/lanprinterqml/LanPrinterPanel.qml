import QtQuick 2.13
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Item {
    id : __rootPanel
    property var curSelectPrintingIndex: -1
    property var devicesMap: undefined   //已打开的设备ID和数据映射表
    property var maxDeviceDataCount: 650

    function setRealEntryPanel(value)
    {
        if(value) lanPrintingBtnClicked()
        __lanPrinterPanel.setRealEntry(value)
    }

    function setCurGcodeFileName(fileName)
    {
        __lanPrinterPanel.curGcodeFileName = fileName
    }

    function lanPrintingBtnClicked()
    {
        curSelectPrintingIndex = -1
        __testLoad.sourceComponent = undefined
    }

    Component.onCompleted: __repeaterModel.clear()

    MouseArea
    {
        anchors.fill: parent
        focus: __rootPanel.visible
    }

    ListModel
    {
        id: __repeaterModel
    }

    Component
    {
        //设备详情页面，被共用的单例页面
        id: __test
        LanPrinterDetail {

        }
    }

    Timer {
        repeat: true
        interval: 1000
        id: idRefreshTimer
        triggeredOnStart: true

        onTriggered: {
            let devIds = Object.keys(devicesMap)
            if (devIds.length === 0 ||
                curSelectPrintingIndex < 0)
                return;

            for (let i = 0; i < devIds.length; ++i) {
                let devId = devIds[i]
                let item = printerListModel.cSourceModel.getDeviceItem(devId)  //获取设备
                var curBedTemp = item ? item.pcBedTemp : ""
                var curNozzleTemp = item ? item.pcNozzleTemp : ""
                let data = ""
                if(item && item.nozzleCount > 1){
                    var curNozzle2Temp = item.nozzle2Temp
                    var curChamberTemp = item.chamberTemp
                    data = [curBedTemp, curNozzleTemp, curNozzle2Temp, curChamberTemp]
                }else if(item.printerType === LanPrinterListLocal.RemotePrinterType.REMOTE_PRINTER_TYPE_KLIPPER4408){
                    data = [curBedTemp, curNozzleTemp, item.chamberTemp]
                }else{
                    data = [curBedTemp, curNozzleTemp]
                }
                devicesMap[devId].push(data)

                if (devicesMap[devId].length > maxDeviceDataCount)
                    devicesMap[devId].shift() // 超出最大缓存

                if (__testLoad.itemPrinterID === devId)
                    __testLoad.item.addChartData(data)
            }
        }
    }

    ColumnLayout
    {
        width: parent.width
        height: parent.height
        spacing: 0

        Rectangle
        {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 32 * screenScaleFactor
            color: Constants.lanPrinter_panel_background

            z: 1

            layer.enabled: Constants.currentTheme

            layer.effect: DropShadow {
                radius: 3
                spread: 0
                samples: 7
                color: Constants.dropShadowColor
                verticalOffset: 3 * screenScaleFactor
            }

            Row
            {
                anchors.left: parent.left
                anchors.leftMargin: 1 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1 * screenScaleFactor

                //默认的“局域网打印”按钮
                Button {
                    id: lanPrinter_list_btn
                    width: 100 * screenScaleFactor
                    height: 30 * screenScaleFactor
                    property bool isSelected: curSelectPrintingIndex < 0

                    background: Rectangle {
                        radius: 5
                        border.width: 1
                        border.color: Constants.lanPrinter_panel_border
                        color: (lanPrinter_list_btn.isSelected || lanPrinter_list_btn.hovered)
                               ? Constants.lanPrinter_panel_btn_hovered : Constants.lanPrinter_panel_btn_default
                    }

                    contentItem: Text {
                        font.weight: Font.Medium
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("LAN Printing")
                        color: (lanPrinter_list_btn.isSelected || lanPrinter_list_btn.hovered)
                               ? Constants.lanPrinter_panel_weight_txt : Constants.lanPrinter_panel_light_txt
                    }

                    onClicked: lanPrintingBtnClicked()
                }

                //可变数量的打印机按钮
                Repeater {
                    model: __repeaterModel
                    delegate: Button {
                        id: lanPrinter_detail_btn
                        height: 30 * screenScaleFactor
                        width: detailBtnRow.width + detailBtnRow.spacing * 2
                        property bool isSelected: (curSelectPrintingIndex === index)

                        background: Rectangle {
                            radius: 5
                            border.width: 1
                            border.color: Constants.lanPrinter_panel_border
                            color: (lanPrinter_detail_btn.isSelected || lanPrinter_detail_btn.hovered)
                                   ? Constants.lanPrinter_panel_btn_hovered : Constants.lanPrinter_panel_btn_default
                        }

                        Row {
                            spacing: 10 * screenScaleFactor
                            id: detailBtnRow
                            height: parent.height
                            anchors.centerIn: parent

                            Text {
                                height: parent.height
                                font.weight: Font.Medium
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                text: Key_PrinterName
                                color: (lanPrinter_detail_btn.isSelected || lanPrinter_detail_btn.hovered)
                                       ? Constants.lanPrinter_panel_weight_txt : Constants.lanPrinter_panel_light_txt
                            }

                            Image {
                                width: 8 * screenScaleFactor
                                height: 8 * screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter
                                source: Constants.currentTheme ? "qrc:/UI/photo/lanPrinterSource/closeBtn_light.svg" : "qrc:/UI/photo/lanPrinterSource/closeBtn_dark.svg"

                                MouseArea {
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor

                                    onClicked: {
                                        delete devicesMap[__testLoad.itemPrinterID]
                                        curSelectPrintingIndex = -1
                                        __testLoad.sourceComponent = undefined
                                        __repeaterModel.remove(index)
                                    }
                                }
                            }
                        }

                        onClicked: {
                            if(isSelected) return
                            curSelectPrintingIndex = index

                            if(__testLoad.sourceComponent === null)
                            {
                                __testLoad.itemPrinterID = Key_PrinterID
                                __testLoad.itemPrinterType = Key_PrinterType
                                __testLoad.sourceComponent = __test
                            }
                            else {
                                __testLoad.itemPrinterID = Key_PrinterID
                                __testLoad.item.deviceID = Key_PrinterID

                                __testLoad.itemPrinterType = Key_PrinterType
                                __testLoad.item.deviceType = Key_PrinterType

                                __testLoad.item.stopPlayVideo()
                                __testLoad.item.updateShowData(devicesMap[Key_PrinterID])
                                __testLoad.item.startPlayVideo()
                            }
                        }
                    }
                }
            }
        }

        //内容
        Item
        {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width

            z: 0

            LanPrinterListLocal
            {
                id: __lanPrinterPanel
                anchors.fill: parent

                function isModelContains(id) {
                    var result = -1
                    for(var index = 0; index < __repeaterModel.count; ++index)
                    {
                        var macAddress = __repeaterModel.get(index).Key_PrinterID
                        if(macAddress === id) result = index
                    }
                    return result
                }

                onClickDetail:
                {
                    var result = isModelContains(printerID)
                    curSelectPrintingIndex = (result < 0) ? __repeaterModel.count : result
                    if(result < 0) __repeaterModel.append({ "Key_PrinterID": printerID, "Key_PrinterName": printerName, "Key_PrinterType": printerType })

                    __testLoad.itemPrinterID = printerID
                    __testLoad.itemPrinterType = printerType
                    __testLoad.sourceComponent = __test
                }
                Loader
                {
                    id: __testLoad
                    property var itemPrinterID: ""
                    property var itemPrinterType: ""

                    focus: true
                    active: true
                    anchors.fill: parent
                    visible: status == Loader.Ready

                    onLoaded: {
                        item.deviceID = itemPrinterID
                        item.deviceType = itemPrinterType
                        if (devicesMap === undefined)
                            devicesMap = new Object
                        if (devicesMap[itemPrinterID] === undefined)
                            devicesMap[itemPrinterID] = []
                        item.updateShowData(devicesMap[itemPrinterID])
                        item.startPlayVideo()
                    }
                }
            }
        }
    }
}
