import QtQml 2.13
import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0
import QtQuick.Window 2.0
import Qt.labs.platform 1.1
import "../qml"

Item {
    id: root
    anchors.fill: parent
    //title: qsTr("Model Library")
    signal requestToShowDownloadTip()
    property string img_collect_d: Constants.currentTheme ? "qrc:/UI/photo/model_info/collect.svg" : "qrc:/UI/photo/model_info/collect_d.svg"
    property string img_collect_s: "qrc:/UI/photo/model_info/collect_s.svg"
    property string img_like_d: Constants.currentTheme ? "qrc:/UI/photo/model_info/like.svg" : "qrc:/UI/photo/model_info/like_d.svg"
    property string img_like_s: "qrc:/UI/photo/model_info/like_s.svg"
    property string img_share_s: "qrc:/UI/photo/model_info/share_s.svg"
    property string img_share_d:  Constants.currentTheme ? "qrc:/UI/photo/model_info/share.svg":"qrc:/UI/photo/model_info/share_d.svg"

    property string img_download_all_d:  Constants.currentTheme ?"qrc:/UI/photo/model_info/download_all.svg" :"qrc:/UI/photo/model_info/download_all_d.svg"
    property string img_download_one_d:  Constants.currentTheme ? "qrc:/UI/photo/model_info/download_one.svg":"qrc:/UI/photo/model_info/download_one_d.svg"
    property string img_import_all_d:  Constants.currentTheme ?"qrc:/UI/photo/model_info/import_all.svg": "qrc:/UI/photo/model_info/import_all_d.svg"
    property string img_import_one_d: Constants.currentTheme ? "qrc:/UI/photo/model_info/import_one.svg":"qrc:/UI/photo/model_info/import_one_d.svg"
    property string currentModelId: ""
    property string authorId: ""
    property var idImageMap: {0:0}
    property var idModelMap: {0:0}
    property var idGroupMap: {0:0}
    property var idGroupSearchMap: {0:0}
    property bool isSearchMode: false

    property var shareDialog: null

    function setGroupInfo(json_string) {
        const json_root = JSON.parse(json_string)
        if (json_root.code !== 0) {
            return
        }

        const group_info = json_root.result.groupItem
        if (group_info.id !== currentGroupId) {
            return
        }

        switch (group_info.license) {
            case "CC BY": {
                license_image.source = "qrc:/UI/photo/license_by.png"
                break
            }
            case "CC BY-SA": {
                license_image.source = "qrc:/UI/photo/license_by_sa.png"
                break
            }
            case "CC BY-NC": {
                license_image.source = "qrc:/UI/photo/license_by_nc.png"
                break
            }
            case "CC BY-NC-SA": {
                license_image.source = "qrc:/UI/photo/license_by_nc_sa.png"
                break
            }
            case "CC BY-ND": {
                license_image.source = "qrc:/UI/photo/license_by_nd.png"
                break
            }
            case "CC0": {
                license_image.source = "qrc:/UI/photo/license_cc0.png"
                break
            }
            default: {
                license_image.source = ""
                break
            }
        }

        license_image.visible = license_image.source !== ""

        const user_info = group_info.userInfo
        author_name_label.text = user_info.nickName
        group_name_label.text = group_info.groupName
        author_head_image.img_src = user_info.avatar

        group_upload__category.categoryName = group_info.categoryName
        group_like_btn.isLike = group_info.isLike
        group_like_btn.likeCount = group_info.likeCount

        group_collect_btn.isCollect = group_info.isCollection
        group_collect_btn.collectCount = group_info.collectionCount

        group_share_btn.shareCount = group_info.shareCount
        group_download_btn.downloadCount = group_info.downloadCount

        group_upload_time_label.text = "%1 : %2".arg(qsTr("UploadedTime"))
                .arg(Qt.formatDateTime(new Date(Number(group_info.createTime*1000)), "yyyy-MM-dd hh:mm"))

    }
    function clearMap(map) {
        for (let key in map) {
            if(map[key]){
                map[key].destroy()
                delete map[key]
            }

        }
    }
    function setGroupDetail(json_string) {
        let componentImageItem = Qt.createComponent("BasicImageButton.qml")
        let componentModelItem = Qt.createComponent("ModelLibraryDetailIem.qml")
        if (componentImageItem.status !== Component.Ready ||
                componentImageItem.status !== Component.Ready) {
            return
        }
        let json_root = JSON.parse(json_string)
        if (json_root.code === 0) {
            let model_info_list = json_root.result.list
             if (model_info_list.length && (model_info_list[0].modelId).localeCompare(currentGroupId) !== 0) return
             clearMap(idImageMap)
             clearMap(idModelMap)
             currentModelId = ""
            let is_first_model = true
            for (let model_info of model_info_list) {
                let group_id   = model_info.modelId
                let model_id   = model_info.id
                let model_name = model_info.fileName
                let model_size = model_info.fileSize
                let image_url  = model_info.coverUrl
                // init image button
                let image_item = componentImageItem.createObject(image_list, {
                                                                     "id"       : model_id,
                                                                     "keystr"   : model_id,
                                                                     "btnRadius": 10 * screenScaleFactor,
                                                                     "btnImgUrl": image_url
                                                                 })
                image_item.sigBtnClicked.connect(onModelButtonClicked)
                idImageMap[model_id] = image_item
                // init model button
                let model_item = componentModelItem.createObject(model_list, {
                                                                     "groupId"  : group_id,
                                                                     "modelName": model_name,
                                                                     "modelSize": model_size,
                                                                     "modelId"  : model_id,
                                                                      "btnImgUrl": image_url
                                                                 })
                model_item.sigBtnDetailClicked.connect(onModelButtonClicked)
                model_item.sigBtnDownLoadDetailModel.connect(onModelDownloadButtonClicked)
                model_item.sigBtnImportDetailModel.connect(onModelImportButtonClicked)
                model_item.sigBtnImportDirectModel.connect(onModelImportDirectButtonClicked)
                model_item.sigDownloadedTip.connect(onSigDownloadedTip)
             //   model_item.sigLoginTip.connect(onSigLoginTip)
                idModelMap[model_id] = model_item
                if (is_first_model) {
                    currentModelId = model_id
                    is_first_model = false
                }
            }
        }
    }
    function onModelButtonClicked(model_id) {
        currentModelId = model_id
    }
    function onModelDownloadButtonClicked(model_id) {
        let id_group_map = isSearchMode ? idGroupSearchMap : idGroupMap
        currentModelId = model_id
        cxkernel_cxcloud.downloadService.startModelDownloadTask(currentGroupId, currentModelId)
        root.requestToShowDownloadTip()
      //  root.visible = false
    }
    function onModelImportButtonClicked(model_id) {
        let id_group_map = isSearchMode ? idGroupSearchMap : idGroupMap

        currentModelId = model_id
        cxkernel_cxcloud.downloadService.startModelDownloadTask(currentGroupId, currentModelId,true)
        //  root.requestToShowDownloadTip()
     //   root.visible = false
    }

    function onModelImportDirectButtonClicked(group_id, model_id) {
        let id_group_map = isSearchMode ? idGroupSearchMap : idGroupMap

        cxkernel_cxcloud.downloadService.importModelCache(group_id,model_id)
     //   root.visible = false
    }

    onCurrentModelIdChanged: {
        if (currentModelId === "") {
            clearMap(idModelMap)
            clearMap(idImageMap)
            return
        }
        for (let model_key in idModelMap) {
            let model_item = idModelMap[model_key]
            let selected = model_key === currentModelId
            model_item.btnIsSelected = selected
            if (selected) {
                let visiableSize = model_list_scroll.vSize
                let virtualHeight = model_list_scroll.height / visiableSize
                let itemPosition = model_item.y / virtualHeight
                let position = model_list_scroll.vPosition
                /* 当前所选项不在列表的显示范围时，把列表滑动到所选项位置 */
                if (itemPosition < position ||
                        itemPosition > position + visiableSize) {
                    model_list_scroll.vPosition = Math.min(1 - visiableSize, itemPosition)
                }
            }
        }
        current_model_image.source = ""
        for (let image_key in idImageMap) {
            let image_item = idImageMap[image_key]
            let selected = image_key === currentModelId
            image_item.btnSelect = selected
            if (selected) {
                current_model_image.source = image_item.btnImgUrl
            }
        }
    }

    Connections{
        target: cxkernel_cxcloud.modelLibraryService
        onLoadModelGroupInfoSuccessed:(json_string)=>setGroupInfo(json_string)
        onLoadModelGroupFileListInfoSuccessed: (json_string) => setGroupDetail(json_string)
    }

    ColumnLayout{
        id: author_library_models_layout
        anchors.fill: parent
        anchors.topMargin: 40 * screenScaleFactor
        anchors.leftMargin: 20 * screenScaleFactor
        anchors.rightMargin: anchors.leftMargin
        anchors.bottomMargin: 30 * screenScaleFactor
        spacing: 10 * screenScaleFactor
        Item {
            Layout.minimumWidth: 76 * screenScaleFactor
            Layout.minimumHeight: 28 * screenScaleFactor
        }
        RowLayout {
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.minimumHeight: 500 * screenScaleFactor
            Layout.maximumHeight: Layout.minimumHeight
            spacing: 10 * screenScaleFactor
            Rectangle {
                Layout.minimumWidth: 442 * screenScaleFactor
                Layout.fillHeight: true
                border.width: 1
                border.color: Constants.model_library_border_color
                radius: 10 * screenScaleFactor
                color: "#FFFFFF"
                ColumnLayout {
                    anchors.fill: parent
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 400 * screenScaleFactor
                        color: "transparent"
                        Image {
                            id: current_model_image
                            anchors.fill: parent
                            mipmap: true
                            smooth: true
                            cache: false
                            asynchronous: true
                            fillMode: Image.PreserveAspectFit
                            source: ""
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.topMargin: 20 * screenScaleFactor
                            anchors.bottomMargin: anchors.topMargin
                            anchors.leftMargin: 10 * screenScaleFactor
                            anchors.rightMargin: anchors.leftMargin
                            spacing: 10 * screenScaleFactor
                            Button {
                                id: model_image_left_button
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                Layout.minimumWidth: 30 * screenScaleFactor
                                Layout.minimumHeight: Layout.minimumWidth
                                //enabled: image_list_view.hPosition !== 0
                                background: Rectangle {
                                    anchors.fill: parent
                                    radius: width / 2
                                    color: model_image_left_button.hovered ? "#E2F5FF" : "transparent"
                                    Image {
                                        anchors.centerIn: parent
                                        visible: model_image_left_button.enabled
                                        width: 6 * screenScaleFactor
                                        height: 10 * screenScaleFactor
                                        source: "qrc:/UI/photo/model_library_detail_left.png"
                                    }
                                }
                                onClicked: {
                                    let new_pos = image_list_view.hPosition - 1 / countMapSize(idImageMap)
                                    image_list_view.hPosition = Math.max(0, new_pos)
                                    // 查找上一个modelId
                                    let currentIndex
                                    var ids = Object.keys(idModelMap)
                                    for (var i = ids.length - 1; i >= 0; i--) {
                                        if (ids[i] === currentModelId)
                                            currentIndex = i
                                    }
                                    // 切换到上一个modelId
                                    let nextIndex = Math.max(0, currentIndex - 1);
                                    currentModelId = ids[nextIndex];
                                }
                            }
                            Rectangle {
                                id: image_list_rect
                                Layout.fillWidth: true
                                Layout.minimumHeight: 60 * screenScaleFactor
                                BasicScrollView {
                                    id: image_list_view
                                    anchors.fill: parent
                                    anchors.margins: 0
                                    wheelEnabled: false
                                    clip: true
                                    vpolicyVisible: false
                                    background: Rectangle {
                                        color: "transparent"
                                    }
                                    Row {
                                        id: image_list
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 0
                                        spacing: 10 * screenScaleFactor
                                    }
                                }
                            }
                            Button {
                                id: model_image_right_button
                                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                Layout.minimumWidth: 30 * screenScaleFactor
                                Layout.minimumHeight: Layout.minimumWidth
                                //enabled: image_list_view.hPosition !== 1 - image_list_view.hSize
                                background: Rectangle {
                                    anchors.fill: parent
                                    radius: width / 2
                                    color: model_image_right_button.hovered ? "#E2F5FF" : "transparent"
                                    Image {
                                        anchors.centerIn: parent
                                        visible: true
                                        width: 6 * screenScaleFactor
                                        height: 10 * screenScaleFactor
                                        source: "qrc:/UI/photo/model_library_detail_right.png"
                                    }
                                }
                                onClicked: {
                                    let new_pos = image_list_view.hPosition + 1 / countMapSize(idImageMap)
                                    image_list_view.hPosition = Math.min(1 - image_list_view.hSize, new_pos)
                                    // 查找下一个modelId
                                    let currentIndex
                                    var ids = Object.keys(idModelMap)
                                    for (var i = ids.length - 1; i >= 0; i--) {
                                        if (ids[i] === currentModelId)
                                            currentIndex = i
                                    }
                                    let nextIndex = Math.min(ids.length - 1, currentIndex + 1);
                                    currentModelId = ids[nextIndex];
                                }
                            }
                        }
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border.width: 1
                border.color: Constants.model_library_border_color
                radius: 10 * screenScaleFactor
                color: "transparent"
                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: 30 * screenScaleFactor
                    anchors.bottomMargin: anchors.topMargin
                    anchors.leftMargin: 20 * screenScaleFactor
                    anchors.rightMargin: anchors.leftMargin
                    Rectangle{
                        width: parent.width
                        height: 30* screenScaleFactor
                        color:"transparent"
                        Layout.fillWidth: true
                        Column{
                            height: parent.height
                            anchors.left: parent.left
                            StyledLabel {
                                id: group_name_label
                                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                                Layout.minimumHeight: 20 * screenScaleFactor
                                elide: Text.ElideRight
                                text: ""
                                color: Constants.model_library_special_text_color
                                font.weight: "Bold"
                                font.pointSize: Constants.labelFontPointSize_12
                            }
                            RowLayout{
                                StyledLabel {
                                    id: group_upload__category
                                    property string categoryName: ""
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                                    Layout.minimumHeight: 20 * screenScaleFactor
                                    text:  "%1 : %2".arg(qsTr("Category")).arg(categoryName)
                                    color: Constants.model_library_general_text_color
                                    font.pointSize: Constants.labelFontPointSize_9
                                }
                                StyledLabel {
                                    id: group_upload_i
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                                    Layout.minimumHeight: 20 * screenScaleFactor
                                    text: "/"
                                    color: Constants.model_library_general_text_color
                                    font.pointSize: Constants.labelFontPointSize_9
                                }
                                StyledLabel {
                                    id: group_upload_time_label
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                                    Layout.minimumHeight: 20 * screenScaleFactor
                                    text: ""
                                    color: Constants.model_library_general_text_color
                                    font.pointSize: Constants.labelFontPointSize_9
                                }
                            }
                        }

                        Row{
                            height: parent.height
                            anchors.right: parent.right
                            Column{
                                id:group_like_btn
                                anchors.verticalCenter: parent.verticalCenter
                                width: 76* screenScaleFactor
                                spacing: 8* screenScaleFactor
                                property bool isLike: false
                                property int likeCount: 0
                                Image {
                                    source: parent.isLike ? img_like_s: img_like_d
                                    width: 15* screenScaleFactor
                                    height: 15* screenScaleFactor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    MouseArea{
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (!cxkernel_cxcloud.accountService.logined) {
                                                onSigLoginTip()
                                                return
                                            }
                                            cxkernel_cxcloud.modelLibraryService.likeModelGroup(currentGroupId, !group_like_btn.isLike)
                                        }
                                    }
                                }
                                Text {
                                    text: parent.likeCount
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    font.pointSize: 10* screenScaleFactor
                                    color: "#BABABA"
                                }

                            }
                            Rectangle{
                                width: 1* screenScaleFactor
                                height: 20* screenScaleFactor
                                color: "#55555B"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Column{
                                anchors.verticalCenter: parent.verticalCenter
                                width: 76* screenScaleFactor
                                spacing: 8* screenScaleFactor
                                id:group_collect_btn
                                property bool isCollect: false
                                property int collectCount: 0
                                Image {
                                    source: parent.isCollect ? img_collect_s: img_collect_d
                                    width: 15* screenScaleFactor
                                    height: 15* screenScaleFactor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    MouseArea{
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (!cxkernel_cxcloud.accountService.logined) {
                                                onSigLoginTip()
                                                return
                                            }
                                            cxkernel_cxcloud.modelLibraryService.collectModelGroup(currentGroupId, !group_collect_btn.isCollect)
                                        }
                                    }
                                }
                                Text {
                                    text: parent.collectCount
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    font.pointSize: 10* screenScaleFactor
                                    color: "#BABABA"
                                }
                            }
                            Rectangle{
                                id:line
                                width: 1* screenScaleFactor
                                height: 20* screenScaleFactor
                                color: "#55555B"
                                anchors.verticalCenter: parent.verticalCenter

                            }
                            Column{
                                anchors.verticalCenter: parent.verticalCenter
                                width: 76* screenScaleFactor
                                spacing: 8* screenScaleFactor
                                id:group_share_btn
                                property int shareCount: 0
                                Image {
                                    source: img_share_d
                                    width: 15* screenScaleFactor
                                    height: 15* screenScaleFactor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    MouseArea{
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (!cxkernel_cxcloud.accountService.logined) {
                                                onSigLoginTip()
                                                return
                                            }
                                            root.shareDialog.share(currentGroupId, group_share_btn)
                                        }
                                    }
                                }
                                Text {
                                    text: parent.shareCount
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    font.pointSize: 10* screenScaleFactor
                                    color: "#BABABA"
                                }
                            }
                            Rectangle{
                                width: 1* screenScaleFactor
                                height: 20* screenScaleFactor
                                color: "#55555B"
                                anchors.verticalCenter: parent.verticalCenter

                            }
                            Column{
                                anchors.verticalCenter: parent.verticalCenter
                                width: 76* screenScaleFactor
                                spacing: 8* screenScaleFactor
                                id:group_download_btn
                                property int downloadCount: 0
                                Image {
                                    source: img_download_one_d
                                    width: 15* screenScaleFactor
                                    height: 15* screenScaleFactor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    MouseArea{
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            let group_id = currentGroupId
                                            let id_group_map = isSearchMode ? idGroupSearchMap : idGroupMap

                                            if (!cxkernel_cxcloud.downloadService.checkModelGroupDownloadable(group_id)) {
                                                onSigDownloadedTip()
                                                return
                                            }
                                            if (!cxkernel_cxcloud.accountService.logined) {
                                                cxkernel_cxcloud.downloadService.makeModelGroupDownloadPromise(group_id)
                                                onSigLoginTip()
                                                return
                                            }
                                            cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(group_id)
                                            root.requestToShowDownloadTip()
                                        //    root.visible = false

                                        }
                                    }
                                }
                                Text {
                                    text: parent.downloadCount
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    font.pointSize: 10* screenScaleFactor
                                    color: "#BABABA"
                                }
                            }

                        }

                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.minimumHeight: 85 * screenScaleFactor
                        spacing: 8 * screenScaleFactor
                        BaseCircularImage {
                            id: author_head_image
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.minimumWidth: 60 * screenScaleFactor
                            Layout.minimumHeight: Layout.minimumWidth
                            img_src: ""
                        }
                        StyledLabel {
                            id: author_name_label
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.fillWidth: true
                            Layout.minimumHeight: author_head_image.Layout.minimumHeight
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            text: ""
                            color: Constants.model_library_special_text_color
                            font.pointSize: Constants.labelFontPointSize_12
                        }

                        ColumnLayout{
                            RowLayout{
                                 spacing: 10* screenScaleFactor
                                BasicDialogButton {
                                    id: download_group_button
                                    Layout.alignment: Qt.AlignCenter
                                    Layout.minimumWidth: 130 * screenScaleFactor
                                    Layout.minimumHeight: 36 * screenScaleFactor
                                    btnBorderW: 0
                                    btnRadius: Layout.minimumHeight / 2
                                    borderColor:Constants.model_library_action_button_border_checked_color
                                    isAnimatedImage: false
                                    imgWidth: 14 * screenScaleFactor
                                    imgHeight: 14 * screenScaleFactor
                                    iconImage.fillMode: Image.PreserveAspectFit
                                    pressedIconSource: img_download_all_d
                                    defaultBtnBgColor:Constants.themeGreenColor
                                    hoveredBtnBgColor: hovered ? Qt.lighter(Constants.themeGreenColor,1.1) : Constants.themeGreenColor
                                    text: qsTr("DownLoad All")
                                    btnTextColor: Constants.model_library_action_button_text_checked_color
                                    onSigButtonClicked: {
                                        let group_id = currentGroupId
                                        let id_group_map = isSearchMode ? idGroupSearchMap : idGroupMap

                                        if (!cxkernel_cxcloud.downloadService.checkModelGroupDownloadable(group_id)) {
                                            onSigDownloadedTip()
                                            return
                                        }
                                        if (!cxkernel_cxcloud.accountService.logined) {
                                            cxkernel_cxcloud.downloadService.makeModelGroupDownloadPromise(group_id)
                                            onSigLoginTip()
                                            return
                                        }
                                        cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(group_id)
                                        root.requestToShowDownloadTip()
                                    //    root.visible = false
                                    }
                                }

                                BasicDialogButton {
                                    id: download_group_button1
                                    Layout.alignment: Qt.AlignCenter
                                    Layout.minimumWidth: 130 * screenScaleFactor
                                    Layout.minimumHeight: 36 * screenScaleFactor
                                    btnBorderW: 0
                                    btnRadius: Layout.minimumHeight / 2
                                    borderColor:Constants.model_library_action_button_border_checked_color
                                    isAnimatedImage: false
                                    imgWidth: 14 * screenScaleFactor
                                    imgHeight: 14 * screenScaleFactor
                                    iconImage.fillMode: Image.PreserveAspectFit
                                    pressedIconSource:  img_import_all_d
                                    defaultBtnBgColor: Constants.themeGreenColor
                                    hoveredBtnBgColor: hovered ? Qt.lighter(Constants.themeGreenColor,1.1) : Constants.themeGreenColor
                                    text: qsTr("Import all")
                                    btnTextColor: Constants.model_library_action_button_text_checked_color
                                    onSigButtonClicked: {
                                        let group_id = currentGroupId
                                        let id_group_map = isSearchMode ? idGroupSearchMap : idGroupMap

                                        if (!cxkernel_cxcloud.accountService.logined) {
                                            cxkernel_cxcloud.downloadService.makeModelGroupDownloadPromise(group_id, true)
                                            onSigLoginTip()
                                            return
                                        }

                                        if (cxkernel_cxcloud.downloadService.checkModelGroupDownloaded(group_id)) {
                                            cxkernel_cxcloud.downloadService.importModelGroupCache(group_id)
                                        } else {
                                            cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(group_id, true)
                                        }
                                  //      root.visible = false
                                    }
                                }

                            }

                        }
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        height: 20 * screenScaleFactor
                        color: "transparent"
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        height: 1
                        color: Constants.model_library_border_color
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        height: 20 * screenScaleFactor
                        color: "transparent"
                    }
                    StyledLabel {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.minimumHeight: 20 * screenScaleFactor
                        color: Constants.model_library_special_text_color
                        text: qsTr("Model List")
                        font.family: Constants.labelFontFamily
                        font.pointSize: Constants.labelFontPointSize_12
                    }
                    Rectangle {
                        id: model_list_rect
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        color: "transparent"
                        BasicScrollView {
                            id: model_list_scroll
                            anchors.fill: parent
                            anchors.rightMargin: -15 * screenScaleFactor
                            clip: true
                            vpolicyVisible: model_list.height > model_list_rect.height
                            background: Rectangle {
                                color: "transparent"
                            }
                            Column {
                                id: model_list
                                width: model_list_rect.width
                                spacing: 10 * screenScaleFactor
                            }
                        }
                    }
                }
            }
        }
        Rectangle {
            id: group_license_rect
            Layout.minimumHeight: 100 * screenScaleFactor
            Layout.fillWidth: true
            height: 123*screenScaleFactor
            color: "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.margins: 20 * screenScaleFactor
                ColumnLayout {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 15 * screenScaleFactor
                    RowLayout {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 10 * screenScaleFactor
                        StyledLabel {
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.minimumHeight: 18 * screenScaleFactor
                            font.pointSize: Constants.labelFontPointSize_12
                            color: Constants.model_library_special_text_color
                            text: qsTr("Creative Commons License")
                        }
                        Button {
                            id: license_button
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                            Layout.minimumWidth: 18 * screenScaleFactor
                            Layout.minimumHeight: Layout.minimumWidth
                            background: Rectangle {
                                anchors.fill: parent
                                radius: license_button.Layout.minimumWidth / 2
                                color: license_button.hovered ? Qt.lighter(Constants.themeGreenColor,1.1) : Constants.themeGreenColor
                                Image {
                                    anchors.fill: parent
                                    anchors.centerIn: parent
                                    anchors.margins: 4 * screenScaleFactor
                                    fillMode: Image.PreserveAspectFit
                                    source: license_button.hovered ? Constants.model_library_license_checked_image : Constants.model_library_license_default_image
                                }
                            }
                            onClicked: {
                                license_dialog.visible = true
                            }
                        }
                    }
                    StyledLabel {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.pointSize: Constants.labelFontPointSize_9
                        color: Constants.model_library_general_text_color
                        text: qsTr("Please check the copyright information in the description.")
                    }
                }
                Image {
                    id: license_image
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    Layout.minimumWidth: 172 * screenScaleFactor
                    Layout.minimumHeight: 60 * screenScaleFactor
                    mipmap: true
                    smooth: true
                    cache: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/UI/photo/license_by_nc.png"
                }
            }
        }
        Item{
            Layout.fillHeight: true
        }
}
    LicenseDescriptionDlg {
        id: license_dialog
        visible: false
    }
}
