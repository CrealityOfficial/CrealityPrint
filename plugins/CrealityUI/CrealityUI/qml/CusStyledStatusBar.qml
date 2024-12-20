import QtQuick 2.12
import QtQuick.Controls 2.12
import ".."
import "../qml"

Item{
    id: statusBur
    visible: true
    property var object
    property var showProcess:true

    signal acceptButton()
    signal cancelJobButton()
    signal sigJobStart()
    signal sigJobEnd()
    property var currentPhase: 0
    property var progressMessage


    Connections {
        target: kernel_slice_flow
        onSupportStructureRequired: {
            console.log("-------------------")
           standaloneWindow.tipsItem.addMsgTipTemp(addSupport)
            if(kernel_kernel.currentPhase === 1)
                standaloneWindow.tipsItem.visible = true
        }
    }


    Connections{
        target: kernel_kernel
        onCurrentPhaseChanged:{
            if(kernel_kernel.currentPhase === 0)
                standaloneWindow.tipsItem.visible = false
        }
    }

    function addSupport(){
        cancelButton.clicked()
        kernel_kernel.setKernelPhase(0)
        standaloneWindow.tipsItem.visible = false
    }

    UploadMessageDlg {
        id: need_support_structure_dialog

        visible: false
        messageType: 0
        msgText: qsTr("The model has not been supported and may fail to print. Do you want to continue adding supports before slicing?")

        onSigOkButtonClicked: {
            cancelButton.sigButtonClicked()
            need_support_structure_dialog.visible = false
        }

        onSigCancelButtonClicked:{
            need_support_structure_dialog.visible = false
        }
    }

    Rectangle
    {
        id:progressBar
        visible: false
        color: "transparent"
        border.width: 0
        anchors.fill: parent
        radius : 0
        Rectangle
        {
            id: progressBarInside
            width: statusBur.width
            height: 40*screenScaleFactor
            color: "transparent"
            anchors.left: parent.left
            Rectangle
            {
                anchors.top: parent.top
                anchors.topMargin: 15*screenScaleFactor
                id: idProgressOut
                height: 4*screenScaleFactor
                width: parent.width
                color: Constants.progressBarBgColor
                Rectangle
                {
                    id: idProgress
                    height: idProgressOut.height
                    color: Constants.themeGreenColor
                    anchors.left: idProgressOut.left
                }
            }




//        CusButton
//        {
//            id: cancelButton
//            txtContent: qsTr("cancel")
//            width: 120*screenScaleFactor
//            height: 28*screenScaleFactor
//            radius: height/2
//            anchors.top: progressBarInside.bottom
//            anchors.topMargin: 55*screenScaleFactor
//            anchors.horizontalCenter: progressBarInside.horizontalCenter
//            normalColor: Constants.leftToolBtnColor_normal
//            hoveredColor: Constants.leftToolBtnColor_hovered
//            pressedColor: Constants.leftToolBtnColor_hovered
//            onClicked:
//            {
//                if(object) object.stop()
//                cancelJobButton()
//                //kernel_kernel.setKernelPhase(0)
//            }
        }
         Row {
                // anchors.horizontalCenter: progressBarInside.horizontalCenter
                // anchors.bottom: idProgressOut.top
                // anchors.bottomMargin: 5*screenScaleFactor
                anchors.left:parent.left
                anchors.leftMargin: parent.width/2 - 150*screenScaleFactor //progressBarInside.horizontalCenter +100*screenScaleFactor
                width:525*screenScaleFactor
                height:22*screenScaleFactor
                spacing:5*screenScaleFactor
                anchors.top: parent.top
                anchors.topMargin: -10*screenScaleFactor
                z:999
                 CusButton
                {
                    id: cancelButton
                    txtContent: qsTr("cancel")
                    width: 60*screenScaleFactor
                    height: 20*screenScaleFactor
                    radius: height/2
                    normalColor: Constants.leftToolBtnColor_normal
                    hoveredColor: Constants.leftToolBtnColor_hovered
                    pressedColor: Constants.leftToolBtnColor_hovered

                    onClicked:
                    {
                        if(object) object.stop()
                        cancelJobButton()
                        //kernel_kernel.setKernelPhase(0)
                    }
                }
                StyledLabel
                {
                    id:idSliceNumber
                    width: 50*screenScaleFactor
                    height: 18*screenScaleFactor
                    text: ""
                    color: Constants.themeGreenColor//Constants.textColor 深色浅色 字体一样
                    font.pointSize: Constants.labelFontPointSize_14 //16*screenScaleFactor
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.ExtraBold
                    visible: currentPhase === 1

                }


                StyledLabel
                {
                    id:idSliceMessage
                    width: 400*screenScaleFactor
                    height: 18*screenScaleFactor
                    text: progressMessage ? progressMessage:qsTr("Processing, Please Wait...")
                    color: "#ffffff"
                    font.pointSize: Constants.labelFontPointSize_12 //16*screenScaleFactor
                    font.family: Constants.labelFontFamilyBold
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
        width: 400
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
            anchors.leftMargin: 5
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
            width:40
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
        width: idMessage.width + 60
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
            width:40
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
            sigJobStart()
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
        sigJobEnd()
        statusBur.visible = false
        progressBar.visible = false
        cancelButton.visible = false
        progressMessage = qsTr("Processing, Please Wait...")
        controlEnabled(true)
    }

    function jobStart(details)
    {
        if(details.get("name")== "LoginJob" || details.get("name")== "AutoSaveJob")
        {
            console.log("LoginJob no need to show statusBar")
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
        idSliceNumber.text = parseInt(r*100) + "%"
        //console.log("idProgress.width r= " + r )
    }

    function showMessage(mess1,btnMess)
    {
        idopenFileMessageDlg.showDoubleBtn()
        idopenFileMessageDlg.isInfo = true
        idopenFileMessageDlg.messageText = mess1 + btnMess
        idopenFileMessageDlg.show()
    }
    function jobMessage(msg)
     {
         progressMessage = msg

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
        object.jobMessage.connect(jobMessage)
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
