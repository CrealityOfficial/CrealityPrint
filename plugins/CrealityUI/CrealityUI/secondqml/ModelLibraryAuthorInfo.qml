import QtQml 2.13

import QtQuick 2.10
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

import Qt.labs.platform 1.1
import "../components"
import "../qml"
DockItem {
    width: 1282 * screenScaleFactor
    height: 960 * screenScaleFactor
    title: qsTr("Author")
    property int currentModelLibraryPage: 1
    property int nextModelLibraryPage: 1
    property int nextSearchLibraryPage: 1
    property int currentModelLibrarySize: 24
    property var idGroupMap: {0:0}
    property string currentGroupId: ""
    property string currentModelId: ""
    signal sigAuthorClick()
    property var idImageMap: {0:0}
    property var idModelMap: {0:0}
    property string authorId: ""
    property int currentFilterTypeId: 3
    property bool isSearch: false
    property var idGroupSearchMap: {0:0}
  //  readonly property bool isSearchMode: model_type_repeater.model === search_result_model

    property var tipDialog: null
    property var shareDialog: null

    id: root

    // to MainWindow
    signal requestLogin()
    signal requestToShowDownloadTip()
    function getPageModelList() {
        let next = isSearch? nextSearchLibraryPage: nextModelLibraryPage
        cxkernel_cxcloud.modelLibraryService.loadModelGroupAuthorModelList(
                    authorId,
                    next,
                    currentModelLibrarySize,
                    currentFilterTypeId,
                    search_edit.text
                    )
    }

    function setModelList(strJson, page) {
        let componentGroupItem = Qt.createComponent("ModelLibraryItem.qml")
        if (componentGroupItem.status === Component.Ready) {
            let json_root = JSON.parse(strJson)
            if (json_root.code !== 0) return
            let dom
            if(isSearch){
                if(nextSearchLibraryPage === 1) group_search_list.children = []
                nextSearchLibraryPage++
                dom = group_search_list
            }else {
                nextModelLibraryPage++
                dom = group_list
            }
            let object_list = json_root.result.list
            for (let object of object_list) {
                const params = {
                    "width"           : dom.itemWidth,
                    "height"          : dom.itemHeight,
                    "groupId"         : object.id,
                    "groupName"       : object.groupName,
                    "groupImage"      : !object.covers.length  ? "" : object.covers[0].url,
                    "authorName"      : object.userInfo.nickName,
                    "authorHead"      : object.userInfo.avatar,
                    "modelCount"      : object.modelCount,
                    "totalPrice"      : object.totalPrice,
                    "collected"       : object.isCollection,
                    "restricted"      : object.maturityRating == "restricted",
                    "createdTimestamp": object.createTime,
                    "tipDialog"       : root.tipDialog,
                }
                authorId = object.userInfo.userId
                let model_item = componentGroupItem.createObject(dom, params)
                model_item.sigButtonDownClicked.connect(onGroupDownloadButtonClicked)
                model_item.sigButtonClicked.connect(onGroupButtonClicked)
                model_item.sigDownloadedTip.connect(onSigDownloadedTip)
                model_item.sigLoginTip.connect(onSigLoginTip)
               if(isSearch){
                   idGroupSearchMap[object.id] = model_item
               }else {
                   idGroupMap[object.id] = model_item
               }
            }
        }
    }

    function clearMap(map) {
        for (let key in map) {
            if(map[key]){
                map[key].destroy()
                delete map[key]
            }

        }
    }
    function clearGrid() {
            // 将Grid的子项数组重新分配为空数组
            group_list.children = []
        }
    function setGroupInfo(json_string) {
        let json_root = JSON.parse(json_string)
        if (json_root.code === 0) {
            let group_info = json_root.result.userInfo
            let id_group_map = isSearch ? idGroupSearchMap : idGroupMap
            nickName.text = group_info.nickName
            root.title =  group_info.nickName
            author_image.img_src = group_info.userAvatar
            authorId = group_info.userId
            modelSharedCount.text = "%1 %2 %3".arg(qsTr("total")).arg(group_info.modelSharedCount).arg(qsTr("Model"))
            let author_info_count_array = ["followCount", "fansCount", "modelLikeCount","collectCount"]
             author_info_count_array.forEach((f,idx)=>author_info_count.setProperty(idx,"count", group_info[f]))

        }
    }

    Connections{
        target: cxkernel_cxcloud.modelLibraryService
        onLoadModelGroupAuthorModelListSuccessed: (json_string, page_index) => setModelList(json_string, page_index)
        onLoadModelGroupAuthorInfoSuccessed:(json_string)=> setGroupInfo(json_string)
    }
    onVisibleChanged: {
        if(visible)
        group_list.children = []
        group_search_list.children = []
        nextModelLibraryPage=1
        nextSearchLibraryPage= 1
        model_author_layout.visible =true
        author_model_info.visible = false
    }
    function onGroupButtonClicked(group_id) {
        if (currentGroupId !== group_id) {
            currentGroupId = group_id
        } else {
            currentGroupIdChanged()
        }
    }
    function onGroupDownloadButtonClicked(group_id) {
        cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(group_id)
        root.requestToShowDownloadTip()
      //  root.visible = false
    }
    function onSigDownloadedTip() {
        goto_download_center_tip.visible = true
    }
    function onSigLoginTip() {
        requestLogin()
    }
    onCurrentGroupIdChanged: {
        let id_group_map = isSearch ? idGroupSearchMap : idGroupMap

        if (currentGroupId === "") {
            clearMap(id_group_map)
            currentModelId = ""
            return
        }
        let group_item = id_group_map[currentGroupId]
        if (!group_item) {
            return
        }

        let group_id          = group_item.groupId
        let group_name        = group_item.groupName
        let group_image       = group_item.groupImage
        let model_count       = group_item.modelCount
        let author_name       = group_item.authorName
        let author_head       = group_item.authorHead
        let total_price       = group_item.totalPrice
        let collected         = group_item.collected
        let created_timestamp = group_item.createdTimestamp * 1000

        cxkernel_cxcloud.modelLibraryService.loadModelGroupInfo(group_item.groupId)
        cxkernel_cxcloud.modelLibraryService.loadModelGroupFileListInfo(group_id, model_count)
        model_author_layout.visible =false
        author_model_info.visible = true


        // write to history
//        cxkernel_cxcloud.modelLibraryService.pushHistoryModelGroup(String(group_id),
//                                                                   String(group_name),
//                                                                   String(group_image),
//                                                                   Number(total_price),
//                                                                   String(author_name),
//                                                                   String(author_head))
    }
    onCurrentModelIdChanged: {
        if (currentModelId === "") {
            clearMap(idModelMap)
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
    }
    onWidthChanged: {
        group_list.syncSize()
      //  group_search_list.syncSize()
    }
    Button {
        id: back_button
        width: 76 * screenScaleFactor
        height: 28 * screenScaleFactor
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 20* screenScaleFactor
        anchors.topMargin:  40 * screenScaleFactor
        visible:  author_model_info.visible
        background: Rectangle {
            anchors.fill: parent
            radius: height / 2
            border.width: 1
            border.color: back_button.hovered ?Constants.themeGreenColor
                                              : Constants.model_library_back_button_border_default_color
            color: back_button.hovered ? Constants.themeGreenColor
                                       : Constants.model_library_back_button_default_color
            RowLayout {
                anchors.fill: parent
                spacing: 5 * screenScaleFactor
                Image {
                    Layout.alignment: Qt.AlignCenter
                    Layout.leftMargin: 20 * screenScaleFactor
                    sourceSize.width: 6 * screenScaleFactor
                    sourceSize.height: 10 * screenScaleFactor
                    source: back_button.hovered ? Constants.model_library_back_button_checked_image
                                                : Constants.model_library_back_button_default_image
                }
                Text {
                    Layout.alignment: Qt.AlignCenter
                    Layout.rightMargin: 20 * screenScaleFactor
                    text: qsTr("Back")
                    font.family: Constants.labelFontFamily
                    font.pointSize: Constants.labelFontPointSize_9
                    color: back_button.hovered ? Constants.model_library_back_button_text_checked_color
                                               : Constants.model_library_back_button_text_default_color
                }
            }
        }
        onClicked: {
            author_model_info.visible = false
            model_author_layout.visible =true
        }
    }

    Column{
        id:model_author_layout
        anchors.fill: parent
      //  anchors.bottomMargin: 40 * screenScaleFactor
        anchors.topMargin: root.currentTitleHeight + 30 * screenScaleFactor
        anchors.leftMargin: 40 * screenScaleFactor
        anchors.rightMargin: 40 * screenScaleFactor //anchors.leftMargin / 2 - group_view.verticalWidth / 2
        Rectangle{
            width: parent.width
            height: 100* screenScaleFactor
            color:"transparent"
            anchors.margins: 20* screenScaleFactor
            Row{
                height: parent.height
                anchors.left: parent.left
                BaseCircularImage {
                    id:author_image
                    width: 80* screenScaleFactor
                    height: 80* screenScaleFactor
                    radiusImg: height/2
                    isPreserveAspectCrop: true
                    img_src: ""
                }

                Column{
                    anchors.left: author_image.right
                    anchors.leftMargin: 10* screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 5* screenScaleFactor
                    Text {
                        id:nickName
                        text: ""
                        font.family: Constants.labelFontFamilyBold
                        color: Constants.themeTextColor
                        font.pointSize: 10* screenScaleFactor
                    }
                    Text {
                        id:modelSharedCount
                        text: ""
                        font.pointSize: 9* screenScaleFactor
                        color: Constants.model_library_type_button_text_default_color
                    }
                }

            }
            Row{
                height: parent.height
                anchors.right: parent.right
                anchors.rightMargin: 10* screenScaleFactor
                ListModel{
                    id:author_info_count
                    ListElement{ count:0;label:qsTr("Following") }
                    ListElement{ count:0;label:qsTr("Followers") }
                    ListElement{ count:0;label:qsTr("Likes") }
                    ListElement{ count:0;label:qsTr("Favorite");noline:true }
                }

                Repeater{
                    model: author_info_count
                    delegate: Row{
                        height: parent.height
                        Column{
                            id: follow_num
                            anchors.verticalCenter: parent.verticalCenter
                            width: 100* screenScaleFactor
                            spacing: 8* screenScaleFactor
                            Text {
                                text: count
                                anchors.horizontalCenter: parent.horizontalCenter
                                font.family: Constants.labelFontFamilyBold
                                font.pointSize: 12* screenScaleFactor
                                color: "#BABABA"
                            }
                            Text {
                                text: label
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
                            visible: !noline
                            anchors.verticalCenter: parent.verticalCenter

                        }

                    }

                }

            }
        }

        RowLayout{
            ListModel {
                id: selectType
                ListElement{ modelText: qsTr("Latest upload"); type: 3 }
                ListElement{ modelText: qsTr("Most liked"); type: 1 }
                ListElement{ modelText: qsTr("Best seller"); type: 5 }
                ListElement{ modelText: qsTr("Downloads"); type: 7 }
                ListElement{ modelText: qsTr("Most makes"); type: 6 }
            }
            readonly property int buttonHeight: 36 * screenScaleFactor
            id:model_type_select_flow
            width: parent.width
            Layout.fillWidth: true
            Layout.minimumHeight: buttonHeight

            Flow{
                ButtonGroup {
                    id: model_type_select_group
                }
                Layout.fillWidth: true
                Layout.minimumHeight: model_type_select_flow.buttonHeight
                spacing: 10 * screenScaleFactor
                Repeater{
                    id:selectBtn
                    model: selectType
                    delegate: Button {
                        width: model_type_text1.contentWidth + 20 * screenScaleFactor
                        height: model_type_select_flow.buttonHeight

                        checkable: true

                        background: Rectangle {
                            anchors.fill: parent
                            radius: 5 * screenScaleFactor
                            color: parent.checked || parent.hovered ? Constants.themeGreenColor
                                                                    : Constants.model_library_type_button_default_color
                        }

                        Text {
                            id: model_type_text1

                            anchors.centerIn: parent
                            text: modelText
                            color: parent.checked || parent.hovered ? Constants.model_library_type_button_text_checked_color
                                                                    : Constants.model_library_type_button_text_default_color
                            font.pointSize: Constants.labelFontPointSize_10
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Black
                        }

                        onToggled: {
                            if (!toggled) return
//                            clearMap(idGroupMap)
//                            group_search_view.visible = false
//                            group_view.visible = true
//                            group_view.vPosition = 0
                            clearGrid()
                            nextModelLibraryPage = 1
                            currentFilterTypeId = type
                            getPageModelList()
                        }

                        Component.onCompleted: {
                            model_type_select_group.addButton(this)
                            checked = type === currentFilterTypeId
                        }
                    }

                }
            }

            Button {
                id: clear_search_button

                enabled: search_edit.text !== ""

                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.minimumWidth: 36 * screenScaleFactor
                Layout.minimumHeight: Layout.minimumWidth

                background: Rectangle {
                    anchors.fill: parent

                    visible: parent.enabled

                    radius: width / 2
                    color: parent.hovered ? Constants.model_library_border_color : "transparent"

                    Image {
                        anchors.centerIn: parent

                        width: 18 * screenScaleFactor
                        height: 12 * screenScaleFactor

                        fillMode: Image.PreserveAspectFit

                        source: "qrc:/UI/photo/model_library_detail_back.png"
                    }
                }

                onPressed: {
                    search_edit.text = ""
                 //   model_type_repeater.model = model_type_model
                    group_view.visible = true
                    isSearch = false
                    group_search_view.visible = false
                }
            }

            BasicLoginTextEdit {
                id: search_edit

                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.minimumWidth: 260 * screenScaleFactor
                Layout.minimumHeight: 36 * screenScaleFactor

                radius: Layout.minimumHeight / 2

                text: ""
                property string prevText: ""

                placeholderText: qsTr("Enter Model Name")
                font.family: Constants.labelFontFamily
                font.pointSize: Constants.labelFontPointSize_10
                imgPadding: 12 * screenScaleFactor
                headImageSrc: hovered ? Constants.searchBtnImg_d : Constants.searchBtnImg
                tailImageSrc: hovered
                              && search_edit.text != "" ? Constants.clearBtnImg : ""
                hoveredTailImageSrc: Constants.clearBtnImg_d

                onEditingFinished: {
                    if (search_edit.text == "") {
                        clear_search_button.onPressed()
                    } else {
                        search_button.sigButtonClicked()
                    }
                }

                onTailBtnClicked: {
                    search_edit.text = ""
                    search_button.sigButtonClicked()
                }
            }

            BasicButton {
                id: search_button

                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.minimumWidth: 66 * screenScaleFactor
                Layout.minimumHeight: 36 * screenScaleFactor

                enabled: search_edit.text != ""

                btnRadius: Layout.minimumHeight / 2
                btnBorderW: 0

                text: qsTr("Search")
                pointSize: Constants.labelFontPointSize_10
                btnTextColor: "#ffffff"

                defaultBtnBgColor: Constants.themeGreenColor
                hoveredBtnBgColor:  Qt.lighter(Constants.themeGreenColor,1.2)

                onSigButtonClicked: {
                    if (search_edit.text == "") {
                        clear_search_button.onPressed()
                        return
                    }
                  //  model_type_repeater.model = search_result_model
                    group_view.visible = false
                    group_search_view.visible = true

                    isSearch = true
                    nextSearchLibraryPage = 1
                    if (search_edit.text == search_edit.prevText) return
                    search_edit.prevText = search_edit.text
                    group_search_view.vPosition = 0
                    getPageModelList()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height - 180* screenScaleFactor
            property int topPadding: 10 * screenScaleFactor

            color: root.panelColor

            BasicScrollView {
                id: group_view
                visible: true
                anchors.fill: parent
                anchors.topMargin: parent.topPadding
          //      rightPadding: library_groups_layout.anchors.rightMargin
                clip: true
                vpolicyVisible:group_list.height > height
                Grid {
                    id: group_list
                    width: parent.width
                    height: parent.height
                    spacing: 5 * screenScaleFactor
                    columns: 4

                    property int itemWidth: 286 * screenScaleFactor
                    property int itemHeight: 360 * screenScaleFactor

                    function syncSize() {
                        let total_width = group_view.width + spacing
                        let item_width = itemWidth + spacing
                        columns = Math.floor(total_width / item_width)
                    }
                }

                onVPositionChanged: {

                    if ((vSize + vPosition) === 1) {
                         getPageModelList()
                    }
                }
            }

            BasicScrollView {
                id: group_search_view
                visible: false
                anchors.fill: parent
                anchors.topMargin: parent.topPadding
           //     rightPadding: library_groups_layout.anchors.rightMargin
                clip: true
                vpolicyVisible: group_search_list.height > 400 *2* screenScaleFactor
                Grid {
                    id: group_search_list
                    width: parent.width
                    height: parent.height
                    spacing: 10 * screenScaleFactor
                    columns: 4

                    property int itemWidth: 286 * screenScaleFactor
                    property int itemHeight: 360 * screenScaleFactor

                    function syncSize() {
                        let total_width = group_search_view.width + spacing
                        let item_width = itemWidth + spacing
                        columns = Math.floor(total_width / item_width)
                    }
                }

                onVPositionChanged: {
                    if ((vSize + vPosition) === 1) {
                       getPageModelList()
                    }
                }
            }
        }
    }
    ModelLibraryAuthorModelInfo{
       id:author_model_info
       shareDialog: root.shareDialog
       authorId: authorId
       visible: false
    }
    UploadMessageDlg {
        id: goto_download_center_tip
        visible: false
        msgText: qsTr("The Model is in the download task list. Please check the download center.")
        cancelBtnVisible: false
        ignoreBtnVisible: false
        onSigOkButtonClicked: {
            root.requestToShowDownloadTip()
            this.visible = false
        }
    }

}
