import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0 as QQC2
import CrealityUI 1.0
import "qrc:/CrealityUI"
import ".."
import "../qml"

Rectangle{
    width: 977
    height: 742

    property var foldingPanelMap: {0: 0}
    //property var clusterDeviceWorkingCnt: 0
    // property var clusterDeviceIdlingCnt: 0
    // property var clusterTaskPrintingCnt: 0
    // property var clusterTaskWaitingCnt: 0

    signal sigTaskItemConformStatus(var data, var status)
    signal sigTaskItemControlChildTaskStatus(var dnName, var operation)
    signal sigReportOperation(var operation)
    signal sigFoldPanelVidelBtnClick()

    function statisticsDeviceReport()
    {
        IotData.clusterDeviceWorkingCnt = 0
        IotData.clusterDeviceIdlingCnt = 0
        var clusterAllNum = 0
        for(var i = 0; i < IotData.groupDeviceModelTypeArray.length; i++)
        {
            clusterAllNum += IotData.groupDeviceCountMap[IotData.groupDeviceModelTypeArray[i]]
            IotData.clusterDeviceWorkingCnt += IotData.groupPrintingCntMap[IotData.groupDeviceModelTypeArray[i]]
        }
        IotData.clusterDeviceIdlingCnt = clusterAllNum - IotData.clusterDeviceWorkingCnt
    }

    function statisticsTaskReport()
    {
        IotData.clusterTaskPrintingCnt = 0
        IotData.clusterTaskWaitingCnt = 0
        IotData.clusterTaskAllocatingCnt = 0
        for(var key in foldingPanelMap)
        {
            IotData.clusterTaskPrintingCnt += IotData.groupTaskPrintingCntMap[key]
            IotData.clusterTaskAllocatingCnt += IotData.groupTaskFinishCntMap[key]
            //IotData.clusterTaskWaitingCnt += IotData.groupTaskWaitingCntMap[key]
        }
        
        IotData.clusterTaskWaitingCnt = idPrintWaitingTask.findWaitingCntValueInChildTaskItemMap()
    }

    function updateChildTaskInfo()
    {
        for(var key in foldingPanelMap)
        {
            foldingPanelMap[key].updateQMLData(key)
            foldingPanelMap[key].updateCreateChildTaskItem(key)
        }

        for(var i = 0; i < IotData.groupTaskDeviceModelTypeArray.length; i++)
        {
            idPrintWaitingTask.updateCreateChildTaskItem(IotData.groupTaskDeviceModelTypeArray[i])
            idPrintWaitingTask.updateWaitingContentPanelHeight()
        }

        //update
        statisticsTaskReport()
    }

    function createTaskFoldingPanelComponent()
    {
        var componentGcode = Qt.createComponent("../secondqml/GroupPrintTaskFoldingPanel.qml")
        if(componentGcode.status === Component.Ready)
        {
            deleteFoldingPanelMap()
            for(var i = 0; i < IotData.groupTaskDeviceModelTypeArray.length; i++)
            {
                var obj = componentGcode.createObject(idPrintTaskList, {"seriesName": IotData.groupTaskSeriesNameMap[IotData.groupTaskDeviceModelTypeArray[i]],
                                                          "deviceNum": IotData.groupTaskDeviceNumMap[IotData.groupTaskDeviceModelTypeArray[i]],
                                                          "printingCnt": IotData.groupTaskPrintingCntMap[IotData.groupTaskDeviceModelTypeArray[i]],
                                                          "waitingCnt": IotData.groupTaskWaitingCntMap[IotData.groupTaskDeviceModelTypeArray[i]],
                                                          "finishCnt": IotData.groupTaskFinishCntMap[IotData.groupTaskDeviceModelTypeArray[i]],
                                                          "isFoldPanelCameraShow": IotData.groupVideoMap[IotData.groupTaskDeviceModelTypeArray[i]],
                                                          "seriesId": IotData.groupTaskDeviceModelTypeArray[i]})
                obj.sigFoldPanelConformStatus.connect(foldPanelConformStatus)
                obj.sigFoldPanelControlChildTaskStatus.connect(foldPanelControlChildTaskStatus)
                obj.sigPanelVidelBtnClick.connect(foldPanelVidelBtnClick)
                foldingPanelMap[IotData.groupTaskDeviceModelTypeArray[i]] = obj
            }
        }
    }

    function foldPanelVidelBtnClick()
    {
        sigFoldPanelVidelBtnClick()
    }

    function foldPanelControlChildTaskStatus(dnName, operation)
    {
        sigTaskItemControlChildTaskStatus(dnName, operation)
    }

    function foldPanelConformStatus(value, status)
    {
        sigTaskItemConformStatus(value, status)
    }

    function deleteFoldingPanelMap()
    {
        for(var key in foldingPanelMap)
        {
            var strkey = "-%1-".arg(key)
            if(strkey != "-0-")
            {
                foldingPanelMap[key].deleteChildTaskItemMap() //delete foldingPanel child
                
                foldingPanelMap[key].destroy()
                delete foldingPanelMap[key]
            }
            else{
                delete foldingPanelMap[key]
            }
        }
    }
    
    Rectangle{
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 20

        width: parent.width - 20 - 20
        height: parent.height - 20 -20
        Column{
            width: parent.width
            height: parent.height
            spacing: 6
            Rectangle{
                width: parent.width
                height: 150
                Row{
                    spacing: 10
                    Rectangle{
                        width: 360
                        height:150
                        color:"#EDF8FF"
                        Row{
                            width: parent.width
                            height:parent.height
                            Rectangle{
                                width: parent.width-220
                                height:parent.height
                                color:"transparent"
                                Column{
                                    y:5
                                    width: parent.width
                                    height:parent.height
                                    StyledLabel{
                                        x:15
                                        width: parent.width
                                        height: 28
                                        font.pointSize: Constants.labelFontPointSize_12
                                        text: qsTr("Device")
                                        font.weight: "Bold"
                                        color:"#333333"
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                    }
                                    StyledLabel{
                                        x:15
                                        width: parent.width
                                        height: 20
                                        font.pointSize: Constants.labelFontPointSize_10
                                        text: qsTr("Number of your devices")
                                        font.weight: "Bold"
                                        color:"#333333"
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                        elide: Text.ElideRight
                                    }
                                    Item{
                                        width: parent.width
                                        height: 60
                                    }
                                    StyledLabel{
                                        x:15
                                        width: parent.width
                                        height: 20
                                        font.pointSize: Constants.labelFontPointSize_10
                                        text: qsTr("+Create more tasks")
                                        font.weight: "Bold"
                                        color:"#1E9BE2"
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                        elide: Text.ElideRight
                                        MouseArea
                                        {
                                            anchors.fill: parent
                                            onClicked:
                                            {
                                                console.log("xxxxxxxxxxxxxxxxxxxx")
                                                sigReportOperation("addmoretask")
                                            }
                                        }
                                    }
                                }
                            }
                            Rectangle{
                                width: 220
                                height:150
                                color:"transparent"
                                BasicPieSeriesItem {
                                    id: devicePieSeries
                                    bgColor: "#EDF8FF"
                                    firstValue: IotData.clusterDeviceWorkingCnt
                                    firstLable: qsTr("Online") + " " + firstValue
                                    secondValue: IotData.clusterDeviceIdlingCnt
                                    secondLable: qsTr("Idle") + " " + secondValue
                                }
                            }
                        }
                    }
                    Rectangle{
                        width: 360+10
                        height:150
                        color:"#EDF8FF"
                        Row{
                            width: parent.width
                            height:parent.height
                            Rectangle{
                                width: parent.width-250
                                height:parent.height
                                color:"transparent"
                                Column{
                                    y:5
                                    width: parent.width
                                    height:parent.height
                                    StyledLabel{
                                        x:15
                                        width: parent.width
                                        height: 28
                                        font.pointSize: Constants.labelFontPointSize_12
                                        text: qsTr("File")
                                        font.weight: "Bold"
                                        color:"#333333"
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                    }
                                    StyledLabel{
                                        x:15
                                        width: parent.width
                                        height: 20
                                        font.pointSize: Constants.labelFontPointSize_10
                                        text: qsTr("Number of files you printed")
                                        font.weight: "Bold"
                                        color:"#333333"
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                        elide: Text.ElideRight
                                    }
                                    Item{
                                        width: parent.width
                                        height: 60
                                    }
                                    StyledLabel{
                                        x:15
                                        width: parent.width
                                        height: 20
                                        font.pointSize: Constants.labelFontPointSize_10
                                        text: qsTr("+Add more print files")
                                        font.weight: "Bold"
                                        color:"#1E9BE2"
                                        verticalAlignment: Qt.AlignVCenter
                                        horizontalAlignment: Qt.AlignLeft
                                        elide: Text.ElideRight
                                        MouseArea
                                        {
                                            anchors.fill: parent
                                            onClicked:
                                            {
                                                console.log("xxxxxxxxxxxxxxxxxxxx")
                                                sigReportOperation("addmorefile")
                                            }
                                        }
                                    }
                                }
                            }
                            Rectangle{
                                width: 250
                                height:150
                                color:"transparent"
                                BasicPieSeriesItem {
                                    id: filePieSeries
                                    bgColor: "#EDF8FF"
                                    firstValue: IotData.clusterTaskPrintingCnt
                                    firstLable: qsTr("Printing") + " " + firstValue
                                    secondValue: IotData.clusterTaskWaitingCnt
                                    secondLable: qsTr("Waiting") + " " + secondValue
                                }
                            }
                        }
                    }
                    Rectangle{
                        width: 195 - 10//285
                        height:150
                        color:"#EDF8FF"
                        Column{
                            y:5
                            width: parent.width
                            height:parent.height
                            StyledLabel{
                                x:15
                                width: parent.width
                                height: 28
                                font.pointSize: Constants.labelFontPointSize_12
                                text: qsTr("Team")
                                font.weight: "Bold"
                                color:"#333333"
                                verticalAlignment: Qt.AlignVCenter
                                horizontalAlignment: Qt.AlignLeft
                            }
                            StyledLabel{
                                x:15
                                width: parent.width
                                height: 20
                                font.pointSize: Constants.labelFontPointSize_10
                                text: qsTr("Your team information")
                                font.weight: "Bold"
                                color:"#333333"
                                verticalAlignment: Qt.AlignVCenter
                                horizontalAlignment: Qt.AlignLeft
                                elide: Text.ElideRight
                            }
                            Item{
                                width: parent.width
                                height: 60
                            }
                            StyledLabel{
                                x:15
                                width: parent.width
                                height: 20
                                font.pointSize: Constants.labelFontPointSize_10
                                text: qsTr("Get to know our team")
                                font.weight: "Bold"
                                color:"#1E9BE2"
                                verticalAlignment: Qt.AlignVCenter
                                horizontalAlignment: Qt.AlignLeft
                                elide: Text.ElideRight
                                MouseArea
                                {
                                    anchors.fill: parent
                                    onClicked:
                                    {
                                        console.log("xxxxxxxxxxxxxxxxxxxx")
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Rectangle{
                width: parent.width
                height: 1
                Row{
                    visible: false
                    width: parent.width
                    height: parent.height
                    StyledLabel{
                        width: 160
                        height: parent.height
                        font.pointSize: Constants.labelFontPointSize_14
                        color: "#333333"
                        text: qsTr("Sort And Filter")
                        font.weight: "Bold"
                        verticalAlignment: Text.AlignVCenter
                        //horizontalAlignment:Text.AlignHCenter
                    }
                }
                //
            }
            Rectangle{
                width: parent.width
                height: parent.height - 150 - 40 - 12
                border.width: 1
                border.color: "#DDDDDD"
                BasicScrollView{
                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: parent.height - 2
                    clip : true
                    visible: {
                        //return true
                        if(IotData.taskListBtnStatus != "waiting")
                        {
                            return true
                        }
                        else{
                            return false
                        }
                    }
                    Column{
                        id: idPrintTaskList
                        width: parent.width
                        //spacing: 2
                        //GroupPrintTaskFoldingPanel{}
                    }
                }
                BasicScrollView{
                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: parent.height - 2
                    clip : true
                    visible: {
                        //return false
                        if(IotData.taskListBtnStatus === "waiting")
                        {
                            return true
                        }
                        else{
                            return false
                        }
                    }
                    GroupPrintTaskFoldingPanel{
                        id: idPrintWaitingTask
                        isBtnVisible: false
                        isWaitItem: true
                        isOpen: true

                        onSigFoldPanelConformStatus:
                        {
                            sigTaskItemConformStatus(data, status)
                        }
                    }
                }
            }
        }
    }
}
