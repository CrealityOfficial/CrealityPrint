import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0 as QQC2
import CrealityUI 1.0
import "qrc:/CrealityUI"
import ".."
import "../qml"

Column{
    property var contentPanelHeight: 175*2 + 50//100
    property var isOpen: false
    property var isFoldPanelCameraShow: false
    property var seriesName: "CR-6"
    property var deviceNum: 10
    property var printingCnt: 5
    property var waitingCnt: 3
    property var finishCnt: 5
    property var seriesId: ""
    property var isBtnVisible: true

    property var childTaskItemMap: {0 : 0}
    property var isWaitItem: false

    signal sigFoldPanelConformStatus(var data, var status)
    signal sigFoldPanelControlChildTaskStatus(var dnName, var operation)
    signal sigPanelVidelBtnClick()

    visible: {
        isWaitItem ? updateWaitingContentPanelHeight() :updateContentPanelHeight()
        if((IotData.taskListBtnStatus === "printing")&&(printingCnt != 0))
        {
            return true
        }
        // else if((IotData.taskListBtnStatus === "waiting")&&(waitingCnt != 0))
        // {
        //     return true
        // }
        else if((IotData.taskListBtnStatus === "idling")&&(finishCnt != 0))
        {
            return true
        }
        else if((IotData.taskListBtnStatus === "waiting"))
        {
            return true
        }
        else{
            return false
        }
    }

    function findWaitingCntValueInChildTaskItemMap()
    {
        var cnt = 0
        for(var key in childTaskItemMap)
        {
            if(childTaskItemMap[key].printStatus === 1){
                cnt++
            }
        }
        return cnt
    }

    function updateWaitingContentPanelHeight()
    {
        idChildTaskItemGrid.rows = Math.ceil(findWaitingCntValueInChildTaskItemMap() / 3)
        contentPanelHeight = Math.ceil(findWaitingCntValueInChildTaskItemMap() / 3) * 175 + (Math.ceil(findWaitingCntValueInChildTaskItemMap() / 3) - 1) * 7 + 20
    }

    function updateContentPanelHeight()
    {
        if((IotData.taskListBtnStatus === "printing")&&(printingCnt != 0))
        {
            idChildTaskItemGrid.rows = Math.ceil(printingCnt / 3)
            contentPanelHeight = Math.ceil(printingCnt / 3) * 175 + (Math.ceil(printingCnt / 3) - 1) * 7 + 20
        }
        else if((IotData.taskListBtnStatus === "waiting"))
        {
            idChildTaskItemGrid.rows = Math.ceil(waitingCnt / 3)
            contentPanelHeight = Math.ceil(waitingCnt / 3) * 175 + (Math.ceil(waitingCnt / 3) - 1) * 7 + 20
        }
        else if((IotData.taskListBtnStatus === "idling")&&(finishCnt != 0))
        {
            idChildTaskItemGrid.rows = Math.ceil(finishCnt / 3)
            contentPanelHeight = Math.ceil(finishCnt / 3) * 175 + (Math.ceil(finishCnt / 3) - 1) * 7 + 20
        }
        else{}


    }

    function updateQMLData(id)
    {
        seriesName = IotData.groupTaskSeriesNameMap[id]
        deviceNum = IotData.groupTaskDeviceNumMap[id]
        printingCnt = IotData.groupTaskPrintingCntMap[id]
        waitingCnt = IotData.groupTaskWaitingCntMap[id]
        finishCnt = IotData.groupTaskFinishCntMap[id]
        isFoldPanelCameraShow = IotData.groupVideoMap[id]
    }

    function updateCreateChildTaskItem(series_id)
    {
        var componentGcode = Qt.createComponent("../secondqml/GroupPrintChildTaskItem.qml")
        if(componentGcode.status === Component.Ready)
        {
            for(var seriesKey in IotData.groupDeviceModelTypeTaskMap)
            {
                if(seriesKey == series_id)
                {
                    for(var key in IotData.groupDeviceModelTypeTaskMap[series_id])
                    {
                        if(findChildTaskIdByKey(IotData.groupDeviceModelTypeTaskMap[series_id][key]))
                        {
                            continue
                        }
                        else
                        {
                            var obj = componentGcode.createObject(idChildTaskItemGrid, {"deviceName": IotData.groupChildTaskPrintNameMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "gcodeName": IotData.groupChildTaskGcodeNameMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "progressBarValue": IotData.groupChildTaskPrintProgressMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "printStatus": IotData.groupChildTaskStatusMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "printTimes": IotData.groupChildTaskPrintTimeMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "printImgSource": IotData.groupChildTaskPrintPictureMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "dnName": IotData.groupChildDeviceNameMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "isCameraShow": IotData.groupChildTaskVideoMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]],
                                                                      "taskId": IotData.groupDeviceModelTypeTaskMap[series_id][key],
                                                                      "isWaitingTaskItem": isWaitItem,
                                                                      "printLeftTimes": IotData.groupChildTaskPrintLeftTimeMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]]})
                            obj.sigConformPrinterStatus.connect(conformPrinterStatus)
                            obj.sigControlChildTaskStatus.connect(controlChildTaskStatus)
                            childTaskItemMap[IotData.groupDeviceModelTypeTaskMap[series_id][key]] = obj
                        }
                    }

                    for(var id in childTaskItemMap)
                    {
                        if(isWaitItem)
                        {
                            if(findDestoryTaskId(id, series_id))
                            {
                                continue
                            }
                            else
                            {
                                var strkey = "-%1-".arg(id)
                                if(strkey != "-0-")
                                {
                                    childTaskItemMap[id].destroy()
                                    delete childTaskItemMap[id]
                                }
                                else{
                                    delete childTaskItemMap[id]
                                }
                            }
                        }
                        else{
                            if(findDestoryTaskIdByKey(id, series_id))
                            {
                                continue
                            }
                            else
                            {
                                var strkey = "-%1-".arg(id)
                                if(strkey != "-0-")
                                {
                                    childTaskItemMap[id].destroy()
                                    delete childTaskItemMap[id]
                                }
                                else{
                                    delete childTaskItemMap[id]
                                }
                            }
                        }
                    }

                    for(var id in childTaskItemMap){
                        childTaskItemMap[id].updateQMLData(id)
                    }

                }
            }
            //idChildTaskItemGrid.rows = Math.ceil(Object.keys(childTaskItemMap).length / 3)
            //contentPanelHeight = idChildTaskItemGrid.rows * 175 + (idChildTaskItemGrid.rows - 1) * 7 + 20
            isWaitItem ? updateWaitingContentPanelHeight() :updateContentPanelHeight()
        }
    }

    function controlChildTaskStatus(dnName, operation)
    {
        sigFoldPanelControlChildTaskStatus(dnName, operation)
    }

    function conformPrinterStatus(value, status)
    {
        sigFoldPanelConformStatus(value, status)
    }

    function findChildTaskIdByKey(data)
    {

        for(var id in childTaskItemMap)
        {
            if(data == id)
            {
                return true
            }
        }
        return false
    }

    function findDestoryTaskIdByKey(taskId, seriesId)
    {
        for(var key in IotData.groupDeviceModelTypeTaskMap[seriesId])
        {
            if(taskId == IotData.groupDeviceModelTypeTaskMap[seriesId][key])
            {
                return true
            }
        }
        return false
    }
    function findDestoryTaskId(taskId)
    {
        for(var i = 0; i < IotData.groupTaskDeviceModelTypeArray.length; i++)
        {
            for(var key in IotData.groupDeviceModelTypeTaskMap[IotData.groupTaskDeviceModelTypeArray[i]])
            {
                if(taskId == IotData.groupDeviceModelTypeTaskMap[IotData.groupTaskDeviceModelTypeArray[i]][key])
                {
                    return true
                }
            }
        }
        return false
    }

    function deleteChildTaskItemMap()
    {
        for(var key in childTaskItemMap)
        {
            var strkey = "-%1-".arg(key)
            if(strkey != "-0-")
            {
                childTaskItemMap[key].destroy()
                delete childTaskItemMap[key]
            }
            else{
                delete childTaskItemMap[key]
            }
        }
    }

    width: 935
    height: isOpen ? (40 + contentPanelHeight) : 40
    Button{
        id: idTitleBtn
        width: parent.width
        height: isBtnVisible ? 40 : 0
        visible: isBtnVisible
        //
        //padding : 0
        MouseArea{
            //anchors.fill: parent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 90
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            hoverEnabled:true
            onClicked:
            {
                isOpen = !isOpen
            }
        }
        contentItem: Item{
            width: parent.width
            height: parent.height
            z:idTitleBtn.z -1
            Row{
                anchors.left: parent.left
                anchors.leftMargin: 10
                width: parent.width - 10
                height: parent.height
                spacing: 8
                Rectangle{
                    anchors{
                        //horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    width: 6
                    height: 6
                    radius: 3
                    color: isOpen ? "#1E9BE2" : (idTitleBtn.hovered ? "#1E9BE2" : "#FCB39F")
                }
                StyledLabel{
                    width: 100
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: seriesName
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignLeft
                }
                StyledLabel{
                    width: 100
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: qsTr("Device Count") + ": "
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignRight
                }
                StyledLabel{
                    width: 20
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: deviceNum
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignLeft
                }
                StyledLabel{
                    width: 100
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: qsTr("Printing") + ": "
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignRight
                }
                StyledLabel{
                    width: 20
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: printingCnt
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignLeft
                }
                StyledLabel{
                    width: 100
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: qsTr("Waiting") + ": "
                    visible: false
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignRight
                }
                StyledLabel{
                    width: 20
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    visible: false
                    text: waitingCnt
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignLeft
                }
                StyledLabel{
                    width: 100
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: qsTr("Allocating") + ": "
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignRight
                }
                StyledLabel{
                    width: 20
                    height: parent.height
                    font.pointSize: Constants.labelFontPointSize_12
                    color: "#333333"
                    text: finishCnt
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment:Text.AlignLeft
                }

                Rectangle{
                    width: 320 - 120 - 16 + 120
                    height: parent.height
                    color: "transparent"
                }
                CusSkinButton_Image{
                    anchors{
                        //horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    z:idTitleBtn.z + 1
                    width: 12
                    height: 15
                    btnImgUrl: isFoldPanelCameraShow ? "qrc:/UI/photo/print_camera_h.png" : "qrc:/UI/photo/print_camera.png"
                    enabled: isFoldPanelCameraShow ? true : false
                    onClicked:
                    {
                        //video
                        sigPanelVidelBtnClick()
                    }
                }
                Rectangle{
                    width: 2
                    height: parent.height
                    color: "transparent"
                }
                CusSkinButton_Image{
                    anchors{
                        //horizontalCenter: parent.horizontalCenter
                        verticalCenter: parent.verticalCenter
                    }
                    width: 11
                    height: 6
                    btnImgUrl: isOpen ? "qrc:/UI/photo/group_print_task_item_fold_arrow_up.png" : "qrc:/UI/photo/group_print_task_item_fold_arrow.png"
                    onClicked:
                    {
                        isOpen = !isOpen
                    }
                }
            }
        }
        background:Rectangle{
            width: parent.width
            height: parent.height
            color: isOpen ? "#DFF3FF" : (idTitleBtn.hovered ? "#DFF3FF" : "#FFFFFF")
        }
    }
    Rectangle{
        id: idPanelRct
        visible: isOpen
        width: parent.width
        height: contentPanelHeight
        
        Grid{
            id: idChildTaskItemGrid
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.left: parent.left
            anchors.leftMargin: 10
            width: parent.width - 20
            height: parent.height - 20
            spacing: 7
            columns: 3
            rows: 2
            // GroupPrintChildTaskItem{}
            // GroupPrintChildTaskItem{}
            // GroupPrintChildTaskItem{}
            // GroupPrintChildTaskItem{}
            // GroupPrintChildTaskItem{}
            // GroupPrintChildTaskItem{}
        }
    }
}
