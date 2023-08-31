import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0 as QQC2
import CrealityUI 1.0
import QtQml 2.13
import "qrc:/CrealityUI"
import ".."
import "../qml"

Rectangle{
    property var isMouseEntered: false
    property var isCameraShow: false

    property var deviceName: "Ender-3 S1-01"
    property var gcodeName: "Ender-3 S1_Tower1.gcode"
    property var progressBarValue: 0
    property var printStatus: 4
    property var printTimes: 0
    property var printLeftTimes: 0
    property var printImgSource: "qrc:/UI/photo/group_print_child_task_wait.png"
    property var dnName: ""
    property var printId: ""

    property var taskId: ""
    property var isWaitingTaskItem: false

    signal sigConformPrinterStatus(var name, var status)
    signal sigControlChildTaskStatus(var name, var operation) //operation: pause; continue; stop

    visible: {
        if(isWaitingTaskItem === true)
        {
            //return true
            if((printStatus === 1) && (IotData.taskListBtnStatus === "waiting"))
            {
                return true
            }
            else{
                return false
            }
        }
        else{
            if((printStatus === 2) && (IotData.taskListBtnStatus === "printing"))
            {
                return true
            }
            else if((printStatus === 5) && (IotData.taskListBtnStatus === "idling"))
            {
                return true
            }
            else if((printStatus === 4) && (IotData.taskListBtnStatus === "idling"))
            {
                return true
            }
            else{
                return false
            }
        }
    }

    Timer{
        id: idConformIdleTimer
        interval: 1000
        repeat: false
        onTriggered:{
            sigConformPrinterStatus(dnName, "clusteridle")
        }
    }
    Timer{
        id: idTaskPauseTimer
        interval: 1000
        repeat: false
        onTriggered:{
            sigControlChildTaskStatus(dnName, "pause")
        }
    }
    Timer{
        id: idTaskContinueTimer
        interval: 1000
        repeat: false
        onTriggered:{
            sigControlChildTaskStatus(dnName, "continue")
        }
    }
    Timer{
        id: idTaskStopTimer
        interval: 1000
        repeat: false
        onTriggered:{
            sigControlChildTaskStatus(dnName, "stop")
        }
    }
    Timer{
        id: idTaskCancleTimer
        interval: 1000
        repeat: false
        onTriggered:{
            sigConformPrinterStatus(taskId, "clustercancle")
        }
    }

    function updateQMLData(id)
    {
        deviceName = IotData.groupChildTaskPrintNameMap[id]
        gcodeName = IotData.groupChildTaskGcodeNameMap[id]
        printStatus = IotData.groupChildTaskStatusMap[id]
        printTimes = getTimeShowFromTime(IotData.groupChildTaskPrintTimeMap[id])
        printImgSource = IotData.groupChildTaskPrintPictureMap[id]
        dnName = IotData.groupChildDeviceNameMap[id]
        printLeftTimes = getTimeShowFromTime(IotData.groupChildTaskPrintLeftTimeMap[id])
        isCameraShow = IotData.groupChildTaskVideoMap[id]
        printId = IotData.groupChildPrintIdMap[id]

        progressBarValue = IotData.groupChildTaskPrintProgressMap[id]

        IotData.taskListGcodeNameByDn[dnName] = gcodeName //save gcodeName for video show
    }

    function getTimeShowFromTime(tmp)
    {
        var timeStr = ""
        if(tmp == 0)
        {
            timeStr = ""
        }
        else if(tmp < 60)
        {
            timeStr = tmp + "s"
            
        }
        else if(tmp > 60 && tmp < 60 * 60)
        {
            timeStr = Math.floor(tmp/60) + "min" + (tmp - Math.floor(tmp/60) * 60) + "s"
        }
        else if(tmp > 60 * 60)
        {
            timeStr = Math.floor(tmp/(60 * 60)) + "h" + Math.floor((tmp - Math.floor(tmp/(60 * 60)) * 3600)/60) + "min"
        }

        return timeStr
    }

    width: 300
    height: 175

    MouseArea{
        anchors.fill: parent
        hoverEnabled:true
        onEntered:
        {
            isMouseEntered = true
        }
        onExited:
        {
            isMouseEntered = false
        }
    }

    border.color: isMouseEntered ? "#1E9BE2" : "#9C9C9C"
    border.width: 1

    Rectangle{
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.left: parent.left
        anchors.leftMargin: 10
        width: parent.width - 2 - 20
        height: parent.height  - 10

        Column{
            width: parent.width
            height: parent.height
            spacing: 10
            Rectangle{
                width: parent.width
                height: parent.height - 24 - 10
                Row{
                    width: parent.width
                    height: parent.height
                    spacing: 35
                    Rectangle{
                        width: 150
                        height: parent.height

                        Column{
                            width: parent.width
                            height: parent.height
                            Rectangle{
                                width: parent.width
                                height: 15
                                color: "transparent"
                            }
                            StyledLabel{
                                width: parent.width
                                height: 22
                                font.pointSize: Constants.labelFontPointSize_14
                                color: "#333333"
                                text: deviceName
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment:Text.AlignLeft
                            }
                            StyledLabel{
                                width: parent.width
                                height: 21
                                font.pointSize: Constants.labelFontPointSize_10
                                color: "#333333"
                                text: gcodeName
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment:Text.AlignLeft
                            }
                            Rectangle{
                                width: parent.width
                                height: 8
                                color: "transparent"
                            }
                            Column{
                                width: parent.width
                                height: 24
                                Row{
                                    width: parent.width
                                    height: 20
                                    visible: (printStatus === 4 || printStatus === 5) ? false : true
                                    StyledLabel{
                                        width: 100
                                        height: 21
                                        font.pointSize: Constants.labelFontPointSize_10
                                        color: "#333333"
                                        text: {
                                            if(printStatus === 1)
                                            {
                                                return qsTr("Waiting")
                                            }
                                            else if(printStatus === 2)
                                            {
                                                return qsTr("Printing")
                                            }
                                            else if(printStatus === 3)
                                            {
                                                return ""
                                            }
                                            else if(printStatus === 5)
                                            {
                                                return qsTr("Confirm Idle")
                                            }
                                            else if(printStatus === 4)
                                            {
                                                return ""
                                            }
                                        }
                                        //"printing..."
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment:Text.AlignLeft
                                    }
                                    StyledLabel{
                                        width: parent.width - 100
                                        height: 21
                                        font.pointSize: Constants.labelFontPointSize_10
                                        color: {
                                            if(progressBar.value == 0)
                                            {
                                                return "#999999"
                                            }
                                            else if(progressBar.value == 100)
                                            {
                                                return "#3ACE2F"
                                            }
                                            else{
                                                return "#1E9BE2"
                                            }
                                        }//"#333333"
                                        text: progressBar.value + " %"
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment:Text.AlignRight
                                    }
                                }
                                ProgressBar{
                                    id: progressBar
                                    from: 0
                                    to:100
                                    value: progressBarValue//50
                                    width: parent.width
                                    height: 4
                                    visible: (printStatus === 4 || printStatus === 5) ? false : true

                                    background: Rectangle {   
                                        implicitWidth: progressBar.width
                                        implicitHeight: progressBar.height
                                        color: "#ECECEC"
                                    }

                                    contentItem: Item {  
                                        Rectangle {
                                            width: progressBar.visualPosition * progressBar.width
                                            height: progressBar.height
                                            color: {
                                                if(progressBar.value == 0)
                                                {
                                                    return "#ECECEC"
                                                }
                                                else if(progressBar.value == 100)
                                                {
                                                    return "#3ACE2F"
                                                }
                                                else{
                                                    return "#1E9BE2"
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            Rectangle{
                                width: parent.width
                                height: 50
                                Column{
                                    anchors.top: parent.top
                                    anchors.topMargin: 10
                                    width: parent.width
                                    height: parent.height - 20
                                    spacing: 6
                                    Row{
                                        width: parent.width
                                        height: 12
                                        visible: (printStatus === 4) ? false : true
                                        StyledLabel{
                                            id: idPrintTimeLabel
                                            width: idPrintTimeLabel.contentWidth
                                            height: parent.height
                                            font.pointSize: Constants.labelFontPointSize_10
                                            color: "#333333"
                                            text: qsTr("Print Time") + ": "
                                            //elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment:Text.AlignLeft
                                        }
                                        StyledLabel{
                                            id: idPrintTimeValueLabel
                                            width: idPrintTimeValueLabel.contentWidth
                                            height: parent.height
                                            font.pointSize: Constants.labelFontPointSize_10
                                            color: "#333333"
                                            text: printTimes
                                            //elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment:Text.AlignLeft
                                        }
                                    }
                                    Row{
                                        width: parent.width
                                        height: 12
                                        visible: (printStatus === 4) ? false : true
                                        StyledLabel{
                                            id: idRemainTimeLabel
                                            width: idRemainTimeLabel.contentWidth
                                            height: parent.height
                                            font.pointSize: Constants.labelFontPointSize_10
                                            color: "#333333"
                                            text: qsTr("Remain Time") + ": "
                                            //elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment:Text.AlignLeft
                                        }
                                        StyledLabel{
                                            id: idRemainTimeValueLabel
                                            width: idRemainTimeValueLabel.contentWidth
                                            height: parent.height
                                            font.pointSize: Constants.labelFontPointSize_10
                                            color: "#333333"
                                            text: printLeftTimes
                                            //elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment:Text.AlignLeft
                                        }
                                    }
                                    Row{
                                        visible: (printStatus === 4) ? true : false
                                        width: parent.width
                                        height: 12
                                        StyledLabel{
                                            id: idTotalTimeLabel
                                            width: idTotalTimeLabel.contentWidth
                                            height: parent.height
                                            font.pointSize: Constants.labelFontPointSize_10
                                            color: "#333333"
                                            text: qsTr("Total Print Time") + ": "
                                            //elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment:Text.AlignLeft
                                        }
                                        StyledLabel{
                                            id: idTotalTimeValueLabel
                                            width: idTotalTimeValueLabel.contentWidth
                                            height: parent.height
                                            font.pointSize: Constants.labelFontPointSize_10
                                            color: "#333333"
                                            text: printTimes//qsTr("3h35min")
                                            //elide: Text.ElideRight
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment:Text.AlignLeft
                                        }
                                    }
                                }
                            }
                        }

                    }
                    Rectangle{
                        width: parent.width - 150 - 35
                        height: parent.height
                        Column{
                            width: parent.width
                            height: parent.height
                            Rectangle{
                                width: parent.width
                                height: 6
                                color: "transparent"
                            }
                            Row{
                                width: parent.width
                                height: 15
                                spacing: 10
                                Rectangle{
                                    width: 48
                                    height: parent.height
                                    color: "transparent"
                                }
                                CusSkinButton_Image{
                                    width: 12
                                    height: 15
                                    btnImgUrl: isCameraShow ? "qrc:/UI/photo/print_camera_h.png" : "qrc:/UI/photo/print_camera.png"
                                    enabled:isCameraShow
                                    onClicked:
                                    {
                                        //video
                                        sigControlChildTaskStatus(dnName, "viewprintdetail")
                                    }
                                }
                                CusSkinButton_Image{
                                    width: 14
                                    height: 15
                                    btnImgUrl: "qrc:/UI/photo/group_print_child_task_share_h.png"
                                    visible: false
                                    onClicked:
                                    {
                                        //share
                                    }
                                }
                            }
                            Rectangle{
                                width: parent.width
                                height: 11
                                color: "transparent"
                            }
                            Rectangle{
                                //id: idDefaultPrinterImg
                                z: +1
                                width: (printStatus === 1) ? 100 : 82
                                height: (printStatus === 1) ? 121 : 80  
                                //color: "#ECECEC"
                                Image{
                                    width: parent.width
                                    height: parent.height  
                                    mipmap: true
                                    smooth: true
                                    cache: false
                                    asynchronous: true
                                    fillMode: Image.PreserveAspectFit
                                    source: (printStatus === 1) ? "qrc:/UI/photo/group_print_child_task_wait.png" : printImgSource//"qrc:/UI/photo/machineImage_Printer_Box.png"
                                }
                            }
                        }
                    }
                }

            }
            Rectangle{
                z: -1
                width: parent.width
                height: 24
                Row{
                    width: parent.width
                    height: parent.height
                    spacing: 10
                    BasicButton{
                        id: idViewBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: (printStatus === 2) ? true : false 
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("View")
                        onSigButtonClicked:
                        {
                            sigControlChildTaskStatus(dnName, "viewprintdetail")
                        }
                    }
                    BasicButton{
                        id: idPauseBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: ((printStatus === 2)&&(IotData.iotPrintStateMap[dnName] == 1)) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("Pause")
                        onSigButtonClicked:
                        {
                            //sigControlChildTaskStatus(dnName, "pause")
                            idTaskPauseTimer.restart()
                        }
                    }
                    BasicButton{
                        id: idContinueBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: ((printStatus === 2)&&(IotData.iotPrintStateMap[dnName] == 5)) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("Continue")
                        onSigButtonClicked:
                        {
                            //sigControlChildTaskStatus(dnName, "continue")
                            idTaskContinueTimer.restart()
                        }
                    }
                    BasicButton{
                        id: idStopBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: (printStatus === 2) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("Stop")
                        onSigButtonClicked:
                        {
                            //sigControlChildTaskStatus(dnName, "stop")
                            idTaskStopTimer.restart()
                        }
                    }
                    BasicButton{
                        id: idCancelBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: (printStatus === 1) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("Cancel")
                        onSigButtonClicked:
                        {
                            //sigConformPrinterStatus(taskId, "clustercancle")
                            idTaskCancleTimer.restart()
                        }
                    }
                    BasicButton{
                        id: idConformIdleBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: (printStatus === 5) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("Confirm Idle")
                        onSigButtonClicked:
                        {
                            //sigConformPrinterStatus(dnName, "clusteridle")
                            idConformIdleTimer.restart()
                        }
                    }
                    BasicButton{
                        id: idPrintCompletedBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: (printStatus === 4) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("Completed")
                        onSigButtonClicked:
                        {
                            //sigConformPrinterStatus(dnName, "clusteridle")
                            idConformIdleTimer.restart()
                        }
                    }
                    BasicButton{
                        id: idViewReportBtn
                        width: 86
                        height: parent.height
                        btnRadius: 3
                        btnBorderW: 0
                        pointSize: Constants.labelFontPointSize_9
                        visible: ((printStatus === 4)) ? true : false
                        defaultBtnBgColor: "#ECECEC"
                        hoveredBtnBgColor: "#1E9BE2"
                        btnTextColor: hovered ? "#FFFFFF" : "#333333"
                        text: qsTr("View Report")
                        onSigButtonClicked:
                        {
                            sigConformPrinterStatus(printId, "viewprintreport")
                        }
                    }
                }
            }
        }
    }
}
