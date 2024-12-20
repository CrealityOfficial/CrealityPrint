import QtQuick 2.0
import QtQuick.Controls 2.12
import Qt.labs.platform 1.1
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "../qml"

BasicDialog{
    id: idDialog
    width: 600
    height: 433 - 28 - 5
    titleHeight : 30
    title: qsTr("Upload Models")
    property var progressValue: 0.0
    property bool m_isLocalFile: false
    signal modelUpload(var categoryId, var groupName, var groupDesc, var isShare, var isOriginal, var modelType, var license, var isCombine)
    signal viewMyModel()

    function upload(is_local_file) {
        m_isLocalFile = is_local_file
        if (m_isLocalFile) {
            local_files_selected_dialog.open()
        } else {
            show()
        }
        idModelTypeModel.clear()
        idModelTypeCombobox.currentIndex = 0
        idModelTypeModel.append({"key":1, "modelData":qsTr("Rendering Blue")})
        idModelTypeModel.append({"key":2, "modelData":qsTr("Rendering Light Green")})
        idModelTypeModel.append({"key":3, "modelData":qsTr("Rendering Green")})
        idModelTypeModel.append({"key":4, "modelData":qsTr("Rendering Yellow")})
        idModelTypeModel.append({"key":5, "modelData":qsTr("Rendering Pink")})
    }

    FileDialog {
        id: local_files_selected_dialog
        fileMode: FileDialog.OpenFiles
        options: {
            if(Qt.platform.os == "linux")
                return FileDialog.DontUseNativeDialog
            else
                return FileDialog.ReadOnly
        }
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            idDialog.show()
        }
    }

    onDialogClosed:
    {
        initData()
    }

    Connections {
        target: cxkernel_cxcloud.modelService

        // onUploadSelectModelsSuccessed: function(json_string, is_local_file) {
        //     idDialog.showUploadModelDialog(json_string, is_local_file)
        // }

        onUploadModelGroupSuccessed: function() {
            idDialog.uploadSuccess()
        }

        onUploadModelGroupProgressUpdated: function(progress) {
            idDialog.updateProgressValue(progress)
        }
    }

    function initData()
    {
        idModelGroupInput.text = ""
        idDescText.text = ""
        progressValue = 0
        idOriginalCheckBox.checked = false
        idLicenseCombobox.currentIndex = 0
        idShareCheckBox.checked = false
        idCombineCheckBox.checked = true
        idDialog.height = 433 - 28 - 5

        idLogoImageColumn.visible = true
        grid_wrapper.visible = true
        idBtnGroup.visible = true
        idprogressBar.visible = false
        idUploadSuccess.visible = false
    }

    function uploadModelGroupFailed()
    {
        initData()
    }

    function uploadSuccess()
    {
        idLogoImageColumn.visible = false
        grid_wrapper.visible = false
        idBtnGroup.visible = false
        idprogressBar.visible = false
        idUploadSuccess.visible = true
    }

    function updateProgressValue(value)
    {
        progressValue = value
    }

    function showUploadModelDialog(data, isLocalFile){
        idGroupTypeModel.clear()
        idGroupTypeCombobox.currentIndex = 0
        var objectArray = JSON.parse(data);
        var objResult = objectArray.result.list;
        for ( var key in objResult ) {
            idGroupTypeModel.append({"key": objResult[key].id, "modelData": objResult[key].name})
            console.log("showUploadModelDialog ModelGroup Key : " + objResult[key].id + " " + objResult[key].name)
        }
        m_isLocalFile = isLocalFile
        //update idModelTypeModel list when language changed
        idModelTypeModel.clear()
        idModelTypeCombobox.currentIndex = 0
        idModelTypeModel.append({"key":1, "modelData":qsTr("Rendering Blue")})
        idModelTypeModel.append({"key":2, "modelData":qsTr("Rendering Light Green")})
        idModelTypeModel.append({"key":3, "modelData":qsTr("Rendering Green")})
        idModelTypeModel.append({"key":4, "modelData":qsTr("Rendering Yellow")})
        idModelTypeModel.append({"key":5, "modelData":qsTr("Rendering Pink")})

        show()
    }

    Column{
        id: idLogoImageColumn
        y: 30
        Rectangle {
            id:logoRect
            x: (idDialog.width - logoRect.width)/2
            width: idDialog.width
            height: 74
            color: "transparent"
            Row
            {
                width: logoImage.width + idText.contentWidth
                anchors{
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                spacing: 10
                Image {
                    id : logoImage
                    height:sourceSize.height
                    width: sourceSize.width
                    source: "qrc:/UI/photo/cloud_logo.svg"
                }
                StyledLabel
                {
                    id:idText
                    height:logoImage.sourceSize.height
                    text: qsTr("Creality Cloud")
                    font.pointSize: Constants.labelFontPointSize_18
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignLeft
                }
            }
        }

        Item {
            width: idLogoImageColumn.width - 2
            height : 2

            Rectangle {
                x: 6
                width:idDialog.width - 12
                height: 2
                color: Constants.splitLineColor
                Rectangle {
                    width: parent.width
                    height: 1
                    color: Constants.splitLineColorDark
                }
            }
        }
    }
    Column{
        id: grid_wrapper
        anchors.top: idLogoImageColumn.bottom
        anchors.topMargin: 25
        anchors.bottomMargin: 25
        anchors.left: parent.left
        anchors.leftMargin: 55
        anchors.rightMargin: 55
        width: idDialog.width
        spacing: 5
        Row{
            StyledLabel {
                id: idGroupNameLabel
                width:100
                height:28
                text: qsTr("Group Name:")
                font.pointSize: Constants.labelFontPointSize_10
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }
            BasicDialogTextField {
                id: idModelGroupInput
                font.pointSize: Constants.labelFontPointSize_10
                placeholderText: qsTr("Please enter the model group name")
                baseValidator:RegExpValidator { regExp: /^\S{100}$/ }
                width: grid_wrapper.width-idGroupNameLabel.width-110
                height : 28
                text: ""
            }
        }
        Row{
            StyledLabel {
                id: idGroupDescLabel
                width:100
                height:28
                text: qsTr("Description:")
                font.pointSize: Constants.labelFontPointSize_10
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }
            BasicScrollView {
                id: idScrollView
                width: grid_wrapper.width-idGroupNameLabel.width-110
                height: 56
                TextArea {
                    id:idDescText
                    width: idScrollView.width
                    height:idScrollView.height
                    wrapMode: TextEdit.Wrap
                    placeholderText: qsTr("Please enter the model group description")
                    placeholderTextColor: Qt.lighter(Constants.textColor, 0.8)
                    text: ""
                    color: Qt.lighter(Constants.textColor, 0.8)//Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_10
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight

                }
                background: Rectangle {
                    //radius: 5
                    border.color: Constants.dialogItemRectBgBorderColor
                    color: Constants.dialogItemRectBgColor//"transparent"
                }
            }
        }
        Row{
            StyledLabel {
                id: idGroupType
                width:100
                height:28
                text: qsTr("Group Type:")
                font.pointSize: Constants.labelFontPointSize_10
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }
            BasicCombobox{
                id: idGroupTypeCombobox
                height:28
                width: grid_wrapper.width-idGroupNameLabel.width-110
                font.pointSize: Constants.labelFontPointSize_10
                popPadding : 2
                currentIndex: 0
                // model: ListModel {
                //     id: idGroupTypeModel
                // }
                model: cxkernel_cxcloud.modelLibraryService.modelGroupCategoryModelList
                textRole: "model_name"
            }
        }
        Row{
            StyledLabel {
                id: idModelType
                width:100
                height:28
                text: qsTr("Rendering color:")
                font.pointSize: Constants.labelFontPointSize_10
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }
            BasicCombobox{
                id: idModelTypeCombobox
                height:28
                width: grid_wrapper.width-idGroupNameLabel.width-110
                font.pointSize: Constants.labelFontPointSize_10
                popPadding : 2
                currentIndex: 0
                model: ListModel {
                    id: idModelTypeModel
                }
            }
        }
        Row{
            spacing: 30
            x: 100
            StyleCheckBox
            {
                id :idOriginalCheckBox
                width: 100
                height: 18
                text: qsTr("Original")
                visible: true
                checked: false
                onCheckedChanged:
                {
                    if(idOriginalCheckBox.checked === true)
                    {
                        idDialog.height = 433
                    }
                    else{
                        idDialog.height = 433 - 28 - 5
                    }
                }
            }
            StyleCheckBox
            {
                id :idShareCheckBox
                width: 100
                height: 18
                text: qsTr("Share")
                visible: true
            }
            StyleCheckBox
            {
                id :idCombineCheckBox
                width: 100
                height: 18
                text: cxTr("Combintion")
                visible: !m_isLocalFile
                checked: true
            }
        }
        Row{
            id: idLicenseRow
            spacing:4
            visible: idOriginalCheckBox.checked
            StyledLabel {
                id: idLicenseLabel
                width:100-4
                height:28
                text: qsTr("License Type:")
                font.pointSize: Constants.labelFontPointSize_10
                verticalAlignment: Qt.AlignVCenter
                horizontalAlignment: Qt.AlignLeft
            }
            BasicCombobox{
                id: idLicenseCombobox
                height:28
                width: grid_wrapper.width-idGroupNameLabel.width-110
                font.pointSize: Constants.labelFontPointSize_10
                popPadding : 2
                currentIndex: 0
                model: ListModel {
                    id: idLicenseModel
                    ListElement{text : "CC BY";}
                    ListElement{text : "CC BY-SA";}
                    ListElement{text : "CC BY-NC";}
                    ListElement{text : "CC BY-NC-SA";}
                    ListElement{text : "CC BY-ND";}
                    ListElement{text : "CC BY-NC-ND";}
                    ListElement{text : "CC0";}
                }
            }
            CusSkinButton_Image{
                y: 5
                width: 16
                height: 16
                btnImgNormal: "qrc:/UI/photo/model_library_license.png"
                btnImgHovered: "qrc:/UI/photo/model_library_license_h.png"
                btnImgPressed: "qrc:/UI/photo/model_library_license_h.png"

                onClicked:
                {
                    idLicenseDesDlg.visible = true
                }
            }
        }
    }
    Row{
        id: idBtnGroup
        anchors{
            horizontalCenter: parent.horizontalCenter
            //verticalCenter: parent.verticalCenter
        }
        anchors.top: grid_wrapper.bottom
        anchors.topMargin: 25
        spacing: 10
        BasicButton{
            width: 125
            height: 28
            btnRadius:3
            btnBorderW:0
            defaultBtnBgColor: Constants.profileBtnColor
            hoveredBtnBgColor: Constants.profileBtnHoverColor
            text: qsTr("Upload")
            enabled: (idModelGroupInput.text != "")&&(idDescText.text != "")
            onSigButtonClicked:
            {
                idLogoImageColumn.visible = false
                grid_wrapper.visible = false
                idBtnGroup.visible = false
                idDialog.height = 223
                idprogressBar.visible = true
                idUploadSuccess.visible = false

                let is_local_file = idDialog.m_isLocalFile
                let is_original = idOriginalCheckBox.checked
                let is_share = idShareCheckBox.checked
                let is_combine = is_local_file ? false : idCombineCheckBox.checked
                let color_index = idModelTypeModel.get(idModelTypeCombobox.currentIndex).key
                let category_uid = idGroupTypeCombobox.model.get(idGroupTypeCombobox.currentIndex).model_uid
                let group_name = idModelGroupInput.text
                let group_description = idDescText.text
                let license = !idLicenseRow.visible ? "" : idLicenseModel.get(idLicenseCombobox.currentIndex).text
                let file_list = []
                const url_head = Qt.platform.os === "windows" ? "" : "/"
                if (is_local_file) {
                    for (let file_url of local_files_selected_dialog.files) {
                        file_list[file_list.length] = file_url.toString().replace("file:///", url_head)
                    }
                }

                cxkernel_cxcloud.modelService.uploadModelGroup(is_local_file,
                                                               is_original,
                                                               is_share,
                                                               is_combine,
                                                               color_index,
                                                               category_uid,
                                                               group_name,
                                                               group_description,
                                                               license,
                                                               file_list)
            }
        }
        BasicButton{
            width: 125
            height: 28
            btnRadius:3
            btnBorderW:0
            defaultBtnBgColor: Constants.profileBtnColor
            hoveredBtnBgColor: Constants.profileBtnHoverColor
            text: qsTr("Cancel")
            onSigButtonClicked:
            {
                idDialog.close();
            }
        }
    }
    Column{
        id: idprogressBar
        x: 96
        y: 108
        visible: false
        spacing: 10
        StyledLabel{
            anchors{
                horizontalCenter: parent.horizontalCenter
                //verticalCenter: parent.verticalCenter
            }
            text: progressValue + "%"
            font.pointSize: Constants.labelFontPointSize_10
        }
        ProgressBar{
            id: progressBar
            from: 0
            to:100
            value: progressValue
            width: 408
            height: 3

            background: Rectangle {
                implicitWidth: progressBar.width
                implicitHeight: progressBar.height
                color: "#303030"
            }

            contentItem: Item {
                Rectangle {
                    width: progressBar.visualPosition * progressBar.width
                    height: progressBar.height
                    color: "#1E9BE2"
                }
            }
        }
    }

    Rectangle{
        id: idUploadSuccess
        visible: false
        y: 35
        width: idDialog.width
        height: idDialog.height - titleHeight
        color: "transparent"
        Column{
            anchors{
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
            spacing: 27
            Row{
                spacing: 10
                Image{
                    id: idFinishImage
                    height:sourceSize.height
                    width: sourceSize.width
                    source: "qrc:/UI/photo/upload_success_image.png"//"qrc:/UI/photo/UploadSuccess.png"
                }
                StyledLabel
                {
                    id:idFinishText
                    height:logoImage.sourceSize.height
                    text: qsTr("Uploaded Successfully")
                    font.pointSize: Constants.labelFontPointSize_12
                    verticalAlignment: Qt.AlignVCenter
                    horizontalAlignment: Qt.AlignLeft
                }
            }
            BasicButton{
                anchors{
                    horizontalCenter: parent.horizontalCenter
                    //verticalCenter: parent.verticalCenter
                }
                width: 125
                height: 28
                btnRadius:0
                btnBorderW:0
                pointSize: Constants.labelFontPointSize_10
                text: qsTr("View My Model")
                onSigButtonClicked:
                {
                    viewMyModel()
                    // initDialogQMLData()
                }
            }
        }

    }

    LicenseDescriptionDlg{
        id:idLicenseDesDlg
        visible:false
    }
}
