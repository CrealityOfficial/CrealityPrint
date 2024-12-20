import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."
import "../qml"

BasicNoTitleDialog{
    id: statusBur
    visible: false
    //width: (parent.width * 0.46) < 450 ? 450 : (parent.width * 0.46) //890
    //height: (parent.height * 0.074) < 60 ? 60 : parent.height * 0.074  //80
    property var object
    property var showProcess:true

    signal acceptButton()
    signal cancelJobButton()
    onVisibleChanged:
    {
        if(visible)
        {
            statusBur.x = standaloneWindow.x + (standaloneWindow.width - statusBur.width) / 2
            statusBur.y = standaloneWindow.y + (standaloneWindow.height - statusBur.height - 20 *  screenScaleFactor)
        }
    }
    Rectangle
    {
        id:progressBar
        visible: false
        //width: parent.width
        //height: statusBur.height
        color: Constants.itemBackgroundColor
//        border.color: Constants.rightPanelTabNormalColor
        border.width: 1*screenScaleFactor
        anchors.fill: parent
        radius : 0
        Rectangle
        {
            id: progressBarInside
            width: statusBur.width - 170*screenScaleFactor
            height:40*screenScaleFactor
            color: Constants.itemBackgroundColor
            anchors.left: parent.left
            anchors.leftMargin: 20*screenScaleFactor
            anchors.top: parent.top
            anchors.topMargin: statusBur.height * 0.3
            Rectangle
            {
                id: idProgressOut
                height: 4*screenScaleFactor
                width: parent.width
                color: Constants.progressBarBgColor
                Rectangle
                {
                    id: idProgress
                    height: 4*screenScaleFactor
                    color:"#1E9BE2"
                    anchors.left: idProgressOut.left
                }
            }


            StyledLabel
            {
                id:idSliceMessage
                x:0
                y:5*screenScaleFactor
                width: 176*screenScaleFactor
                height: 18*screenScaleFactor
                text: qsTr("Processing, Please Wait...")
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_16
                anchors.horizontalCenter: progressBarInside.horizontalCenter
                anchors.verticalCenter: progressBarInside.verticalCenter
                anchors.verticalCenterOffset: 5*screenScaleFactor
            }
        }


        BasicButton
        {
            id: cancelButton
            text: qsTr("cancel")
            width: 120*screenScaleFactor
            height: 28*screenScaleFactor
            btnRadius: 14*screenScaleFactor
            anchors.left: progressBarInside.right
            anchors.leftMargin: 25*screenScaleFactor
            anchors.top: progressBar.top
            anchors.topMargin: statusBur.height * 0.3 - 5
            defaultBtnBgColor : Constants.buttonColor
            hoveredBtnBgColor : Constants.hoveredColor

            onSigButtonClicked:
            {
                if(object) object.stop()
                cancelJobButton()
                //kernel_kernel.setKernelPhase(0)
            }
        }

    }

    Rectangle
    {
        id: idSavefinishShow
        objectName: "SavefinishShow"
        anchors.right: statusBur.right
        anchors.rightMargin: 0
        height: parent.height
        width: 400*screenScaleFactor
        color: "transparent"
        visible: false
        StyledLabel
        {
            id:idFirstMessage
            x:0
            y:5
            width: contentWidth
            height: idSavefinishShow.height
            text: qsTr("Save Finish")
            color: Constants.textColor
        }
        StyledLabel
        {
            id : idSecondMessage
            anchors.left: idFirstMessage.right
            anchors.leftMargin: 5*screenScaleFactor
            y:5
            text: qsTr("Open fileDir")
            width: contentWidth
            height: idSavefinishShow.height
            font.underline: true
            color: "#3968E9"

            MouseArea
            {
                anchors.fill: parent
                onClicked:
                {
                    acceptButton()
                }
            }
        }
        Button
        {
            id: cancelButton2
            text: qsTr(" X ")
            width:40*screenScaleFactor
            height: idSavefinishShow.height
            anchors.right: idSavefinishShow.right
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            onClicked:
            {
                idSavefinishShow.visible = false
            }
        }
    }

        Item
    {
        id: idMessShow
        anchors.right: statusBur.right
        anchors.rightMargin: 0
        height: parent.height
        width: idMessage.width + 60*screenScaleFactor
//        color: "transparent"
        visible: false
        property var receiver
        property alias textMess : idMessage.text
        StyledLabel
        {
            id:idMessage
            x:0
            y:5
            width: contentWidth
            height: idMessShow.height
         //   text: textMess
            color: Constants.textColor
        }

        Button
        {
            id: cancelBtn
            text: qsTr(" X ")
            width:40*screenScaleFactor
            height: idMessShow.height
            anchors.right: idMessShow.right
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            onClicked:
            {
                idMessShow.visible = false
            }
        }
    }

    BasicMessageDialog
    {
        id: idopenFileMessageDlg
        objectName: "openFileMessageDlg"
        onAccept:
        {
            acceptButton()
        }
        onCancel:
        {
            //do nothing
        }
    }


    function jobsStart()
    {
        if(showProcess == true)
        {
            statusBur.visible = true
            progressBar.visible = true
            cancelButton.visible = true
            idProgress.width = 0
            idSavefinishShow.visible = false
            /*idAdaptShow.visible = false*/
            controlEnabled(false)
        }
    }

    function jobsEnd()
    {
        statusBur.visible = false
        progressBar.visible = false
        cancelButton.visible = false
        controlEnabled(true)
    }

    function jobStart(details)
    {
        if(details.get("name")== "LoginJob" || details.get("name")== "AutoSaveJob")
        {
            statusBur.visible = false
            showProcess = false
        }
        else
        {
            statusBur.visible = true
            showProcess = true
        }
        idProgress.width = 0
        controlEnabled(false)
    }

    function jobEnd(details)
    {
        //idJob.text = details.get("status")
        controlEnabled(true)
    }

    function jobProgress(r)
    {
        idProgress.width = r * idProgressOut.width//(progressBar.width-160)
        //console.log("idProgress.width r= " + r )
    }
    function showMessage(mess1,btnMess)
    {
        idopenFileMessageDlg.showDoubleBtn()
        idopenFileMessageDlg.isInfo = true
        idopenFileMessageDlg.messageText = mess1 + btnMess
        idopenFileMessageDlg.show()
    }

    function bind(bindObject)
    {
        object = bindObject
        //console.log(object)
        object.jobsStart.connect(jobsStart)
        object.jobsEnd.connect(jobsEnd)
        object.jobStart.connect(jobStart)
        //object.jobEnd.connect(jobEnd)
        object.jobProgress.connect(jobProgress)
    }

    function showJobFinishMessage(receiver,textMessage)
    {
        console.log("receiver =" + receiver)
        //idMessShow.textMess = textMessage
        console.log("idMessShow.textMess =" + idMessShow.textMess)
        //idMessShow.visible = true
        idMessShow.receiver = receiver
    }

    function controlEnabled(bEnabled)
    {
        if(!Constants.bIsWizardShowing)
        {
            Constants.bLeftToolBarEnabled = bEnabled
            Constants.bRightPanelEnabled = bEnabled
            Constants.bMenuBarEnabled = bEnabled
            Constants.bGLViewEnabled = bEnabled
        }
    }
}
