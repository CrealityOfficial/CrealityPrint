import QtQuick 2.10
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Window 2.13

import "../components"
import "../qml"
import "../secondqml"

DockItem
{
    id: root
    title: qsTr("Personal Center")
    width: 1205 * screenScaleFactor
    height: 772 * screenScaleFactor
    panelColor: sourceTheme.background_color

    signal quitClicked()

    property int themeType: -1
    property var sourceTheme: ""

    property var currentGcodeId: 0
    property var currentModelId: 0

    property var sliceItemMap: {0:0}
    property var currentSlicePage: 1
    property var nextSlicePage: 1

    property var modelItemMap: {0:0}
    property var currentModelPage: ""
    property var nextModelPage: 1

    property var deviceItemMap: {0:0}
    property var deviceImageMap: {0:0}
    property var currentPrinterPage: 1
    property var nextPrinterPage: 1

    property string deviceOn:   "Online"
    property string deviceOff:  "Offline"
    property string tfCardOn:   "Card Inserted"
    property string tfCardOff:  "No Card Inserted"
    property string printerOn:  "Connected"
    property string printerOff: "Disconnected"

    enum RequestSliceList {
        Gft_my_upload,
        Gft_cloud
    }

    enum RequestModelList {
        Gft_purchased,
        Gft_favorited,
        Gft_my_upload
    }

    ListModel {
        id: themeModel
        ListElement {
            //Dark Theme
            background_color: "#4B4B4D"
            label_normal_color: "#CBCBCC"
            label_checked_color: "#00A3FF"

            scrollbar_color: "#7E7E84"
            split_line_color: "#606063"
            progressbar_color: "#38383B"

            treeView_plus_img: "qrc:/UI/photo/treeView_plus_dark.png"
            treeView_minus_img: "qrc:/UI/photo/treeView_minus_dark.png"
            treeView_checked_img: "qrc:/UI/photo/treeView_checked_icon.png"
            userInfo_refresh_img: "qrc:/UI/photo/userInfo_refresh_dark.png"
            userInfo_question_img: "qrc:/UI/photo/userInfo_question_dark.png"
            userInfo_refresh_hover_img: "qrc:/UI/photo/userInfo_refresh_hover.png"
            userInfo_question_hover_img: "qrc:/UI/photo/userInfo_question_hover.png"
        }
        ListElement {
            //Light Theme
            background_color: "#FFFFFF"
            label_normal_color: "#333333"
            label_checked_color: "#00A3FF"

            scrollbar_color: "#D6D6DC"
            split_line_color: "#DDDDE1"
            progressbar_color: "#D6D6DC"

            treeView_plus_img: "qrc:/UI/photo/treeView_plus_light.png"
            treeView_minus_img: "qrc:/UI/photo/treeView_minus_light.png"
            treeView_checked_img: "qrc:/UI/photo/treeView_checked_icon.png"
            userInfo_refresh_img: "qrc:/UI/photo/userInfo_refresh_light.png"
            userInfo_question_img: "qrc:/UI/photo/userInfo_question_light.png"
            userInfo_refresh_hover_img: "qrc:/UI/photo/userInfo_refresh_hover.png"
            userInfo_question_hover_img: "qrc:/UI/photo/userInfo_question_hover.png"
        }
    }

    Binding on themeType {
        value: Constants.currentTheme
    }

    onWidthChanged: {
        idMySliceList.syncSize()
        idMyModelList.syncSize()
        idMyDeviceList.syncSize()
    }

    onThemeTypeChanged: {
        sourceTheme = themeModel.get(themeType)//切换主题
    }

    ButtonGroup {
        id: subButtonGroup
        property bool btnEnabled: true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 30 * screenScaleFactor
        anchors.rightMargin: 30 * screenScaleFactor
        anchors.bottomMargin: 30 * screenScaleFactor
        anchors.topMargin: 30 * screenScaleFactor + (adsorbed ? 0 : titleHeight)

        RowLayout {
            spacing: 0
            Layout.alignment: Qt.AlignHCenter

            BaseCircularImage {
                id: idAvtorImage
                width: 60 * screenScaleFactor
                height: 60 * screenScaleFactor
            }

            Item {
                Layout.preferredWidth: 12 * screenScaleFactor
            }

            ColumnLayout {
                spacing: 8 * screenScaleFactor

                Label {
                    id: idNickNameLabel
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 14 * screenScaleFactor

                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    color: sourceTheme.label_normal_color
                    text: ""
                }

                Label {
                    id: idUserIdLabel
                    property string userid: ""
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 14 * screenScaleFactor

                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    color: sourceTheme.label_normal_color
                    text: qsTr("ID: ") + userid
                }
            }

            Item {
                Layout.preferredWidth: 50 * screenScaleFactor
            }

            Rectangle {
                Layout.preferredWidth: 1 * screenScaleFactor
                Layout.preferredHeight: 30 * screenScaleFactor
                color: sourceTheme.split_line_color
            }

            Item {
                Layout.preferredWidth: 50 * screenScaleFactor
            }

            Label {
                id: idStorageSpace
                Layout.preferredWidth: contentWidth * screenScaleFactor
                Layout.preferredHeight: 14 * screenScaleFactor

                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                color: sourceTheme.label_normal_color
                text: qsTr("Remaining Space:")
            }

            Item {
                Layout.preferredWidth: 10 * screenScaleFactor
            }

            ColumnLayout {
                spacing: 8 * screenScaleFactor

                Label {
                    id: idSpaceLabel
                    Layout.preferredWidth: 200 * screenScaleFactor
                    Layout.preferredHeight: 14 * screenScaleFactor

                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: sourceTheme.label_normal_color
                    text: ""
                }

                ProgressBar {
                    id: progressBar
                    width: 200 * screenScaleFactor
                    height: 2 * screenScaleFactor

                    from: 0
                    to: 100

                    background: Rectangle {
                        color: sourceTheme.progressbar_color
                        radius: 1
                    }

                    contentItem: Rectangle {
                        width: progressBar.visualPosition * progressBar.width
                        height: progressBar.height
                        color: "#00A3FF"
                    }
                }
            }

            Item {
                Layout.fillWidth: true
             //   Layout.minimumWidth: 457 * screenScaleFactor
//                Layout.maximumWidth: 1084 * screenScaleFactor
            }

            BasicButton {
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: 140 * screenScaleFactor
                Layout.preferredHeight: 36 * screenScaleFactor

                btnRadius: height / 2
                btnBorderW: 0
                defaultBtnBgColor: Constants.profileBtnColor
                hoveredBtnBgColor: Constants.profileBtnHoverColor
                pointSize: Constants.labelFontPointSize_9
                text: qsTr("Quit")

                onSigButtonClicked: quitClicked()
            }
        }

        //spacing
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 20 * screenScaleFactor
        }

        RowLayout {
            spacing: 0
            Layout.alignment: Qt.AlignHCenter

            ColumnLayout {
                spacing: 25 * screenScaleFactor

                RowLayout {
                    spacing: 11 * screenScaleFactor

                    Button {
                        id: idMySlicesTitle
                        Layout.preferredWidth: 80 * screenScaleFactor
                        Layout.preferredHeight: 14 * screenScaleFactor

                        checkable: true
                        background: null

                        Row {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 11 * screenScaleFactor

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                source: idMySlicesTitle.checked ? sourceTheme.treeView_minus_img : sourceTheme.treeView_plus_img
                            }

                            Label {
                                width: 56 * screenScaleFactor
                                height: 14 * screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter

                                font.family: Constants.labelFontFamily
                                font.weight: Constants.labelFontWeight
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                color: sourceTheme.label_normal_color
                                text: qsTr("My Slices")
                            }
                        }

                        onCheckedChanged: idMySlicesContent.visible = checked
                    }

                    Button {
                        Layout.preferredWidth: 16 * screenScaleFactor
                        Layout.preferredHeight: 16 * screenScaleFactor

                        background: null

                        Image {
                            anchors.centerIn: parent
                            source: parent.hovered ? sourceTheme.userInfo_question_hover_img : sourceTheme.userInfo_question_img
                        }

                        onClicked: kernel_command_center.openCloudTutorialWeb("myslices")
                    }
                }

                ColumnLayout {
                    id: idMySlicesContent
                    spacing: 15 * screenScaleFactor
                    visible: false

                    Repeater {
                        id: repeaterSlice
                        model: ListModel
                        {
                            ListElement { modelData: QT_TR_NOOP("My Uploads")    }
                            ListElement { modelData: QT_TR_NOOP("Cloud Slicing") }
                        }
                        delegate: ItemDelegate
                        {
                            id: itemSlice
                            padding: 0.00
                            checkable: true
                            background: null

                            opacity: enabled ? 1.0 : 0.7
                            enabled: subButtonGroup.btnEnabled
                            ButtonGroup.group: subButtonGroup

                            function modelFunc(page) {
                                switch(index)
                                {
                                    case 0:
                                        cxkernel_cxcloud.sliceService.loadUploadedSliceList(page, 24);
                                        break
                                    case 1:
                                        cxkernel_cxcloud.sliceService.loadCloudSliceList(page, 24);
                                        break
                                    default:
                                        break
                                }
                            }

                            contentItem: Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 1 * screenScaleFactor
                                spacing: 11 * screenScaleFactor + anchors.leftMargin

                                Rectangle {
                                    width: 11 * screenScaleFactor
                                    height: 11 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    color: "transparent"
                                    border.color: itemSlice.checked ? "transparent" : "#CBCBCC"
                                    border.width: itemSlice.checked ? 0 : 1
                                    radius: height / 2

                                    Image {
                                        anchors.centerIn: parent
                                        visible: itemSlice.checked
                                        source: sourceTheme.treeView_checked_img
                                    }
                                }

                                Label {
                                    width: 56 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.label_normal_color
                                    text: qsTr(modelData)
                                }
                            }

                            onCheckedChanged:
                            {
                                if(checked) {
                                    idMySliceView.visible = true
                                    idMyModelView.visible = false
                                    idMyDeviceView.visible = false
                                    currentSlicePage = 1; modelFunc(currentSlicePage)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 11 * screenScaleFactor

                    Button {
                        id: idMyModelsTitle
                        Layout.preferredWidth: 80 * screenScaleFactor
                        Layout.preferredHeight: 14 * screenScaleFactor

                        checkable: true
                        background: null

                        Row {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 11 * screenScaleFactor

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                source: idMyModelsTitle.checked ? sourceTheme.treeView_minus_img : sourceTheme.treeView_plus_img
                            }

                            Label {
                                width: 56 * screenScaleFactor
                                height: 14 * screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter

                                font.family: Constants.labelFontFamily
                                font.weight: Constants.labelFontWeight
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                color: sourceTheme.label_normal_color
                                text: qsTr("My Model")
                            }
                        }

                        onCheckedChanged: idMyModelsContent.visible = checked
                    }

                    Button {
                        Layout.preferredWidth: 16 * screenScaleFactor
                        Layout.preferredHeight: 16 * screenScaleFactor

                        background: null

                        Image {
                            anchors.centerIn: parent
                            source: parent.hovered ? sourceTheme.userInfo_question_hover_img : sourceTheme.userInfo_question_img
                        }

                        onClicked: kernel_command_center.openCloudTutorialWeb("mymodels")
                    }
                }

                ColumnLayout {
                    id: idMyModelsContent
                    spacing: 15 * screenScaleFactor
                    visible: false

                    Repeater {
                        id: repeaterModel
                        model: ListModel
                        {
                            ListElement { modelData: QT_TR_NOOP("My Uploads")   }
                            ListElement { modelData: QT_TR_NOOP("My Favorited") }
                            ListElement { modelData: QT_TR_NOOP("My Purchased") }
                        }
                        delegate: ItemDelegate
                        {
                            id: itemModel
                            padding: 0.00
                            checkable: true
                            background: null

                            opacity: enabled ? 1.0 : 0.7
                            enabled: subButtonGroup.btnEnabled
                            ButtonGroup.group: subButtonGroup

                            function modelFunc(page) {
                                switch(index)
                                {
                                    case 0:
                                        cxkernel_cxcloud.modelService.loadUploadedModelGroupList(page, 24)
                                        break
                                    case 1:
                                        cxkernel_cxcloud.modelService.loadCollectedModelGroupList(page, 24)
                                        break
                                    case 2:
                                        cxkernel_cxcloud.modelService.loadPurchasedModelGroupList(page, 24,"",2)
                                        break
                                    default:
                                        break
                                }
                            }

                            contentItem: Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 1 * screenScaleFactor
                                spacing: 11 * screenScaleFactor + anchors.leftMargin

                                Rectangle {
                                    width: 11 * screenScaleFactor
                                    height: 11 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    color: "transparent"
                                    border.color: itemModel.checked ? "transparent" : "#CBCBCC"
                                    border.width: itemModel.checked ? 0 : 1
                                    radius: height / 2

                                    Image {
                                        anchors.centerIn: parent
                                        visible: itemModel.checked
                                        source: sourceTheme.treeView_checked_img
                                    }
                                }

                                Label {
                                    width: 56 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.label_normal_color
                                    text: qsTr(modelData)
                                }
                            }

                            onCheckedChanged:
                            {
                                if(checked) {
                                    idMyModelView.visible = true
                                    idMySliceView.visible = false
                                    idMyDeviceView.visible = false
                                    currentModelPage = 1;
                                    nextModelPage = 1
                                    idMyModelList.children = []
                                    modelFunc(currentModelPage)
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 11 * screenScaleFactor

                    Button {
                        id: idMyDevicesTitle
                        Layout.preferredWidth: 80 * screenScaleFactor
                        Layout.preferredHeight: 14 * screenScaleFactor

                        checkable: true
                        background: null

                        Row {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 11 * screenScaleFactor

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                source: idMyDevicesTitle.checked ? sourceTheme.treeView_minus_img : sourceTheme.treeView_plus_img
                            }

                            Label {
                                width: 56 * screenScaleFactor
                                height: 14 * screenScaleFactor
                                anchors.verticalCenter: parent.verticalCenter

                                font.family: Constants.labelFontFamily
                                font.weight: Constants.labelFontWeight
                                font.pointSize: Constants.labelFontPointSize_9
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                color: sourceTheme.label_normal_color
                                text: qsTr("My Devices")
                            }
                        }

                        onCheckedChanged: idMyDevicesContent.visible = checked
                    }

                    Button {
                        Layout.preferredWidth: 16 * screenScaleFactor
                        Layout.preferredHeight: 16 * screenScaleFactor

                        background: null

                        Image {
                            anchors.centerIn: parent
                            source: parent.hovered ? sourceTheme.userInfo_refresh_hover_img : sourceTheme.userInfo_refresh_img
                        }

                        onClicked: cxkernel_cxcloud.printerService.loadPrinterList(1, 50)
                    }
                }

                ColumnLayout {
                    id: idMyDevicesContent
                    spacing: 15 * screenScaleFactor
                    visible: false

                    Repeater {
                        model: ListModel
                        {
                            ListElement { modelData: QT_TR_NOOP("My Devices") }
                        }
                        delegate: ItemDelegate
                        {
                            id: itemDevice
                            padding: 0.00
                            checkable: true
                            background: null

                            opacity: enabled ? 1.0 : 0.7
                            enabled: subButtonGroup.btnEnabled
                            ButtonGroup.group: subButtonGroup

                            contentItem: Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 1 * screenScaleFactor
                                spacing: 11 * screenScaleFactor + anchors.leftMargin

                                Rectangle {
                                    width: 11 * screenScaleFactor
                                    height: 11 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    color: "transparent"
                                    border.color: itemDevice.checked ? "transparent" : "#CBCBCC"
                                    border.width: itemDevice.checked ? 0 : 1
                                    radius: height / 2

                                    Image {
                                        anchors.centerIn: parent
                                        visible: itemDevice.checked
                                        source: sourceTheme.treeView_checked_img
                                    }
                                }

                                Label {
                                    width: 56 * screenScaleFactor
                                    height: 14 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    font.family: Constants.labelFontFamily
                                    font.weight: Constants.labelFontWeight
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    color: sourceTheme.label_normal_color
                                    text: qsTr(modelData)
                                }
                            }

                            onCheckedChanged:
                            {
                                if(checked) {
                                    idMySliceView.visible = false
                                    idMyModelView.visible = false
                                    idMyDeviceView.visible = true

                                    //modelFunc
                                    //if(isDeviceEmpty())
                                    cxkernel_cxcloud.printerService.loadPrinterList(1, 50)
                                    updateDeviceQmlData()
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }

            //spacing
            Item {
                Layout.fillWidth: true
                Layout.minimumWidth: 58 * screenScaleFactor
                Layout.maximumWidth: 175 * screenScaleFactor
            }

            Rectangle {
                color: "transparent"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumWidth: 982 * screenScaleFactor
                Layout.maximumWidth: 1476 * screenScaleFactor

//                Label {
//                    id: idEmptyLabel
//                    anchors.centerIn: parent
//                    font.weight: Font.Medium
//                    font.family: Constants.mySystemFont.name
//                    font.pointSize: Constants.labelFontPointSize_12
//                    horizontalAlignment: Text.AlignHCenter
//                    verticalAlignment: Text.AlignVCenter
//                    color: sourceTheme.label_normal_color
//                    text: "Hello world"
//                }

                BasicScrollView {
                    clip: true
                    visible: false
                    id: idMySliceView
                    anchors.fill: parent
                    hpolicyVisible: contentWidth > width
                    vpolicyVisible: contentHeight > height
                    hpolicyindicator: Rectangle {
                        radius: height / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 180 * screenScaleFactor
                        implicitHeight: 6 * screenScaleFactor
                    }
                    vpolicyindicator: Rectangle {
                        radius: width / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 6 * screenScaleFactor
                        implicitHeight: 180 * screenScaleFactor
                    }

                    Grid {
                        id: idMySliceList
                        width: parent.width
                        height: parent.height
                        spacing: 20 * screenScaleFactor
                        property int itemWidth: 225 * screenScaleFactor
                        property int itemHeight: 285 * screenScaleFactor

                        function syncSize() {
                            let item_width = itemWidth + spacing
                            let total_width = idMySliceView.width + spacing
                            idMySliceList.columns = Math.floor(total_width / item_width)
                        }
                    }

                    onVPositionChanged: {
                        if((vSize + vPosition) === 1 && currentSlicePage != nextSlicePage)
                        {
                            let checkedBtn = subButtonGroup.checkedButton
                            if(checkedBtn !== null) checkedBtn.modelFunc(nextSlicePage)
                        }
                    }
                }

                BasicScrollView {
                    clip: true
                    visible: false
                    id: idMyModelView
                    anchors.fill: parent
                    hpolicyVisible: contentWidth > width
                    vpolicyVisible: contentHeight > height
                    hpolicyindicator: Rectangle {
                        radius: height / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 180 * screenScaleFactor
                        implicitHeight: 6 * screenScaleFactor
                    }
                    vpolicyindicator: Rectangle {
                        radius: width / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 6 * screenScaleFactor
                        implicitHeight: 180 * screenScaleFactor
                    }

                    Grid {
                        id: idMyModelList
                        width: parent.width
                        height: parent.height
                        spacing: 20 * screenScaleFactor
                        property int itemWidth: 225 * screenScaleFactor
                        property int itemHeight: 285 * screenScaleFactor

                        function syncSize() {
                            let item_width = itemWidth + spacing
                            let total_width = idMyModelView.width + spacing
                            idMyModelList.columns = Math.floor(total_width / item_width)
                        }
                    }

                    onVPositionChanged: {
                        if((vSize + vPosition) === 1 && currentModelPage != nextModelPage)
                        {
                            if(nextModelPage)
                            {
                                let checkedBtn = subButtonGroup.checkedButton
                                if(checkedBtn !== null) checkedBtn.modelFunc(nextModelPage)
                            }
                        }
                    }
                }

                BasicScrollView {
                    clip: true
                    visible: false
                    id: idMyDeviceView
                    anchors.fill: parent
                    hpolicyVisible: contentWidth > width
                    vpolicyVisible: contentHeight > height
                    hpolicyindicator: Rectangle {
                        radius: height / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 180 * screenScaleFactor
                        implicitHeight: 6 * screenScaleFactor
                    }
                    vpolicyindicator: Rectangle {
                        radius: width / 2
                        color: sourceTheme.scrollbar_color
                        implicitWidth: 6 * screenScaleFactor
                        implicitHeight: 180 * screenScaleFactor
                    }

                    Grid {
                        id: idMyDeviceList
                        width: parent.width
                        height: parent.height
                        spacing: 20 * screenScaleFactor
                        property int itemWidth: 470 * screenScaleFactor
                        property int itemHeight: 285 * screenScaleFactor

                        function syncSize() {
                            let item_width = itemWidth + spacing
                            let total_width = idMyDeviceView.width + spacing
                            idMyDeviceList.columns = Math.floor(total_width / item_width)
                        }
                    }
                }
            }
        }
    }

    UploadMessageDlg {
        id: idMessageDlg
        visible: false
        cancelBtnVisible: false
        onSigOkButtonClicked: idMessageDlg.close()
    }

    ModelLibraryShareDialog {
        id: shareDlg
        visible: false
    }

    Connections {
        target: cxkernel_cxcloud.modelLibraryService

        onModelGroupUncollected: function(id) {
            uncollectModelResult(id)
        }

        onShareModelGroupSuccessed: function () {
            copyModelLinkSuccess()
        }
    }

    Connections {
        target: cxkernel_cxcloud.modelService

        onLoadUploadedModelGroupListSuccessed: function(json_string, page_index) {
            setMyModelList(json_string, page_index, UserInfoDlg.RequestModelList.Gft_my_upload)
        }

        onLoadCollectedModelGroupListSuccessed: function(json_string, page_index) {
            setMyModelList(json_string, page_index, UserInfoDlg.RequestModelList.Gft_favorited)
        }

        onLoadPurchasedModelGroupListSuccessed: function(json_string, page_index) {
            setMyModelList(json_string, page_index, UserInfoDlg.RequestModelList.Gft_purchased)
        }

        onDeleteModelGroupSuccessed: function(id) {
            deleteModelResult(id)
        }
    }

    Connections {
        target: cxkernel_cxcloud.sliceService

        onLoadUploadedSliceListSuccessed: function(json_string, page_index, page_size) {
            setMySliceList(json_string, page_index)
        }

        onLoadCloudSliceListSuccessed: function(json_string, page_index, page_size) {
            setMySliceList(json_string, page_index)
        }

        onDeleteSliceSuccessed: function(uid) {
            deleteGcodeResult(uid)
        }

        onImportSliceSuccessed: function(uid) {
            myGcodeDownloadFinished(true)
        }

        onImportSliceFailed: function(uid) {
            myGcodeDownloadFinished(false)
        }
    }

    Connections {
        target: cxkernel_cxcloud.printerService

        onLoadPrinterListSuccessed: function(json_string) {
            setPrinterImageMap(json_string)
        }

        onPrinterListInfoBotained: function(json_string) {
            setSerDeviceData(json_string)
        }

        onPrinterBaseInfoBotained: function(json_string, printer_name) {
            updateDeviceInformation_NewIOT(json_string, printer_name)
        }

        onPrinterRealtimeInfoBotained: function(json_string, printer_name) {
            updateDeviceInformation_NewIOT_RealTime(json_string, printer_name)
        }

        onPrinterPostitonInfoBotained: function(json_string, printer_name) {
            updateDeviceInformation_NewIOT_Position(json_string, printer_name)
        }

        onPrinterStateInfoBotained: function(json_string, printer_name) {
            updateDeviceInformation_NewIOT_RealTime(json_string, printer_name)
        }
    }

    //UserInfo
    function getSpaceUnit(value)
    {
        if(value > 1024 * 1024 * 1024)
        {
            let num = Number(value/(1024*1024*1024)).toFixed(3)
            let index = num.indexOf('.')
            if (index !== -1) num = num.substring(0, 2 + index + 1)

            return num + "GB"//Math.ceil(value/(1024*1024*1024))+"GB"
        }
        else if(value > 1024 * 1024)
        {
            let num = Number(value/(1024*1024)).toFixed(3)
            let index = num.indexOf('.')
            if (index !== -1) num = num.substring(0, 2 + index + 1)

            return num + "MB"//Math.ceil(value/(1024*1024))+"MB"
        }
        else if(value > 1024)
        {
            let num = Number(value/1024).toFixed(3)
            let index = num.indexOf('.')
            if (index !== -1) num = num.substring(0, 2 + index + 1)

            return num + "KB"//Math.ceil(value/1024)+"KB"
        }
        else
        {
            return value + "B"
        }
    }

    function setUserInfo()
    {
        idAvtorImage.img_src = cxkernel_cxcloud.accountService.avatar
        idUserIdLabel.userid = cxkernel_cxcloud.accountService.userId
        idNickNameLabel.text = cxkernel_cxcloud.accountService.nickName

        let usedSpace = cxkernel_cxcloud.accountService.usedStorageSpace
        let maxSpace = cxkernel_cxcloud.accountService.maxStorageSpace

        progressBar.value = Math.ceil((usedSpace/maxSpace)*100)
        idSpaceLabel.text = getSpaceUnit(usedSpace) + "/" + getSpaceUnit(maxSpace)

        kernel_sensor_analytics.trace("Personal Center", "Personal Center")
    }

    function setUserInfoDlgShow(entry)
    {
        setUserInfo()
        root.visible = true
        subButtonGroup.checkedButton = null

        if(entry === "mySlice")
        {
            idMySlicesTitle.checked = true
            idMyModelsTitle.checked = false
            idMyDevicesTitle.checked = false
            repeaterSlice.itemAt(0).checked = true
        }
        else if(entry === "myModel")
        {
            idMyModelsTitle.checked = true
            idMySlicesTitle.checked = false
            idMyDevicesTitle.checked = false
            repeaterModel.itemAt(0).checked = true
        }
    }

    //Import
    function importGcodeItem(id, name, downloadlink)
    {
        currentGcodeId = id
        idMySliceList.enabled = false
        subButtonGroup.btnEnabled = false
        cxkernel_cxcloud.sliceService.importSlice(id)
    }

    function importModelItem(id, name)
    {
        currentModelId = id
        root.visible = false

        if(!cxkernel_cxcloud.downloadService.checkModelGroupDownloaded(id))
        {
            cxkernel_cxcloud.downloadService.startModelGroupDownloadTask(id)
        }
        else
        {
            cxkernel_cxcloud.downloadService.importModelGroup(id)
        }
    }

    //Cloud / Share
    function openCloudPrintWebpage()
    {
        cxkernel_cxcloud.printerService.openCloudPrintWebpage()
    }

    function showShareDlg(groupId, target)
    {
        shareDlg.setId(groupId)
        shareDlg.trackTo(target)
    }

    //Delete / UnCollect
    function deleteGcodeItem(id)
    {
        cxkernel_cxcloud.sliceService.deleteSlice(id)
    }

    function deleteModelItem(id)
    {
        cxkernel_cxcloud.modelService.deleteModelGroup(id)
    }

    function uncollectModelItem(id)
    {
        cxkernel_cxcloud.modelLibraryService.collectModelGroup(id, false)
    }

    //Result
    function deleteGcodeResult(id)
    {
        if(sliceItemMap[id] !== undefined)
        {
            sliceItemMap[id].destroy()
            delete sliceItemMap[id]
        }
    }

    function deleteModelResult(id)
    {
        if(modelItemMap[id] !== undefined)
        {
            modelItemMap[id].destroy()
            delete modelItemMap[id]
        }
    }

    function uncollectModelResult(id)
    {
        if(modelItemMap[id] !== undefined)
        {
            modelItemMap[id].destroy()
            delete modelItemMap[id]
        }
    }

    function copyModelLinkSuccess()
    {
        idMessageDlg.msgText = qsTr("Copy link successfully!")
        idMessageDlg.alert(0)
        idMessageDlg.show()
    }

    function myGcodeDownloadFinished(state)
    {
        idMySliceList.enabled = true
        subButtonGroup.btnEnabled = true
        //if(sliceItemMap[currentGcodeId] !== undefined) sliceItemMap[currentGcodeId].setAnimatedImageStatus(false)

        if(state)
        {
            root.visible = false
        }
        else {
            idMessageDlg.msgText = qsTr("Failed to import gcode file!")
            idMessageDlg.show()
        }
    }

    //Delete component
    function deleteSliceComponent()
    {
        for(var key in sliceItemMap)
        {
            var strkey = "-%1-".arg(key)
            if(strkey !== "-0-")
            {
                sliceItemMap[key].destroy()
                delete sliceItemMap[key]
            }
            else {
                 delete sliceItemMap[key]
            }
        }
    }

    function deleteModelComponent()
    {
        for(var key in modelItemMap)
        {
            var strkey = "-%1-".arg(key)
            if(strkey !== "-0-")
            {
                modelItemMap[key].destroy()
                delete modelItemMap[key]
            }
            else {
                delete modelItemMap[key]
            }
        }
    }

    function deleteDeviceComponent()
    {
        for(var key in deviceItemMap)
        {
            var strkey = "-%1-".arg(key)
            if(strkey !== "-0-")
            {
                deviceItemMap[key].destroy()
                delete deviceItemMap[key]
            }
            else {
                 delete deviceItemMap[key]
            }
        }
    }

    function setMySliceList(strjson, page)
    {
        var componentGcode = Qt.createComponent("../secondqml/BasicUserItem.qml")
        if(componentGcode.status === Component.Ready)
        {
            var objectArray = JSON.parse(strjson)
            if(objectArray.code === 0)
            {
                currentSlicePage = page
                if(currentSlicePage == 1)
                {
                    deleteSliceComponent()
                    idMySliceView.vPosition = 0
                }

                var objResult = objectArray.result.list
                //idEmptyLabel.visible = objResult.length <= 0
                if(objResult.length === 24) nextSlicePage = currentSlicePage + 1

                let vaild_suffix_list = cxkernel_cxcloud.sliceService.vaildSuffixList

                for(var key in objResult)
                {
                    let suffix_list = objResult[key].downloadLink.split(".")
                    let vaild = false
                    for (let suffix of suffix_list) {
                        for (let vaild_suffix of vaild_suffix_list) {
                            if (suffix === vaild_suffix) {
                                vaild = true
                                break
                            }
                        }
                    }
                    if (!vaild) {
                        continue
                    }

                    var obj = componentGcode.createObject
                    (
                        idMySliceList,
                        {
                            "baseId": objResult[key].id,
                            "baseName": objResult[key].name,
                            "baseImage": objResult[key].thumbnail,
                            "downloadlink": objResult[key].downloadLink,

                            "baseType": 0,
                            "userType": 0,
                            "shareVisible": true,
                            "deleteVisible": true,
                            "printVisible": true,
                            "isPreserveAspectCrop": false
                        }
                    )
                    obj.sigShareModel.connect(showShareDlg)
                    obj.importGcodeItem.connect(importGcodeItem)
                    obj.sigPrintGcode.connect(openCloudPrintWebpage)
                    obj.deleteCurrentItem.connect(deleteGcodeItem)
                    sliceItemMap[objResult[key].id] = obj
                }
            }
        }
    }

    function setMyModelList(strjson, page, requestType)
    {
        // var page = paramList[0]; var requestType = paramList[1]
        var componentModel = Qt.createComponent("../secondqml/BasicUserItem.qml")
        if(componentModel.status === Component.Ready)
        {
            var objectArray = JSON.parse(strjson)
            if(objectArray.code === 0)
            {
                currentModelPage = page
                if(currentModelPage == "")
                {
                    deleteModelComponent()
                    idMyModelView.vPosition = 0
                }

                var objResult = objectArray.result.list
                //idEmptyLabel.visible = objResult.length <= 0
                if(objResult.length) nextModelPage++
                for(var key in objResult)
                {
                    var obj = componentModel.createObject
                    (
                        idMyModelList,
                        {
                            "baseId": objResult[key].id,
                            "baseName": objResult[key].groupName,
                            "baseImage": objResult[key].covers[0].url,
                            "modelcount": objResult[key].modelCount,

                            "baseType": 1,
                            "userType": requestType !== UserInfoDlg.RequestModelList.Gft_favorited ? 0 : 3,
                            "shareVisible": requestType !== UserInfoDlg.RequestModelList.Gft_purchased ? true: false,
                            "deleteVisible": requestType !== UserInfoDlg.RequestModelList.Gft_purchased ? true: false,
                            "printVisible": false,
                            "isPreserveAspectCrop": true
                        }
                    )
                    //Connections
                    obj.importModelItem.connect(importModelItem)
                    if(requestType !== UserInfoDlg.RequestModelList.Gft_purchased) obj.sigShareModel.connect(showShareDlg)
                    if(requestType === UserInfoDlg.RequestModelList.Gft_my_upload) obj.deleteCurrentItem.connect(deleteModelItem)
                    if(requestType === UserInfoDlg.RequestModelList.Gft_favorited) obj.deleteCurrentItem.connect(uncollectModelItem)
                    modelItemMap[objResult[key].id] = obj
                }
            }
        }
    }

    //myDevice
    function setSerDeviceData(strjson)
    {
        var objectArray = JSON.parse(strjson)
        if(objectArray.code === 0)
        {
            IotData.serDeviceNameArray = []
            var objResult = objectArray.result.list
            //idEmptyLabel.visible = objResult.length <= 0

            for(var key in objResult)
            {
                IotData.serDeviceNameArray.push(objResult[key].deviceName)
                IotData.serIotIdMap[objResult[key].deviceName] = objResult[key].iotId
                IotData.serProductkeyMap[objResult[key].deviceName] = objResult[key].productKey
                IotData.serNickNameMap[objResult[key].deviceName] = objResult[key].nickName
                IotData.serSyDidMap[objResult[key].deviceName] = objResult[key].syDid
                IotData.serSyLecenseMap[objResult[key].deviceName] = objResult[key].syLicense
                IotData.serTbIdMap[objResult[key].deviceName] = objResult[key].tbId
                IotData.serIotType[objResult[key].deviceName] = objResult[key].iotType
            }
        }

        //create obj
        var componentGcode = Qt.createComponent("../secondqml/BasicPrinterItem.qml")
        if(componentGcode.status === Component.Ready)
        {
            deleteDeviceComponent()
            for(var i = 0; i < IotData.serDeviceNameArray.length; i++)
            {
                var obj = componentGcode.createObject
                (
                    idMyDeviceList,
                    {
                        "equipmentID": IotData.serDeviceNameArray[i],
                        "printerName": IotData.serNickNameMap[IotData.serDeviceNameArray[i]],
                        "equipmentName": IotData.serNickNameMap[IotData.serDeviceNameArray[i]]
                    }
                )
                obj.itemClicked.connect(openCloudPrintWebpage)
                deviceItemMap[IotData.serDeviceNameArray[i]] = obj
                deviceItemMap[IotData.serDeviceNameArray[i]].equipmentStatus = deviceOff
                deviceItemMap[IotData.serDeviceNameArray[i]].tFCardStatus = tfCardOff
                deviceItemMap[IotData.serDeviceNameArray[i]].printerStatus = printerOff
                deviceItemMap[IotData.serDeviceNameArray[i]].printerStatusShow = printerOff
            }
            idMyDeviceList.syncSize()
        }
    }

    function setPrinterImageMap(strjson)
    {
        var objectArray = JSON.parse(strjson)
        if(objectArray.code === 0)
        {
            var objResult = objectArray.result.list
            for (var key in objResult)
            {
                deviceImageMap[objResult[key].internalName] = objResult[key].thumbnail
            }
        }
    }

    function updateDeviceStatus(strjson, productName)
    {
        var statusRet = JSON.parse(strjson)
        if(statusRet.code === 0)
        {
            if(statusRet.Data.Status === "ONLINE")
            {
                IotData.iotDeviceStatusMap[productName] = "online"
            }
            else {
                IotData.iotDeviceStatusMap[productName] = "offline"
            }
        }
        updateDeviceQmlData()
    }

    function updateDeviceInformation_NewIOT(strjson, productName)
    {
        var dataRet = JSON.parse(strjson)
        if(dataRet.code === 0)
        {
            var dataArray = dataRet.result
            for(var i in dataArray)
            {
                if(dataArray[i].key === "connect")
                {
                    if(dataArray[i].value === 1)
                    {
                        IotData.iotPrinterStatusMap[productName] = printerOn
                    }
                    else {
                        IotData.iotPrinterStatusMap[productName] = printerOff
                    }
                }
                else if(dataArray[i].key === "model")
                {
                    IotData.iotPrinterNameMap[productName] = dataArray[i].value.trim()
                }
                else if(dataArray[i].key === "tfCard")
                {
                    if(dataArray[i].value === 1)
                    {
                        IotData.iotTFCardStatusMap[productName] = tfCardOn
                    }
                    else {
                        IotData.iotTFCardStatusMap[productName] = tfCardOff
                    }
                }
                else if(dataArray[i].key === "APILicense")
                {
                    IotData.iotShangYunAPILicenseMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "DIDString")
                {
                    IotData.iotShangYunDIDStringMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "InitString")
                {
                    IotData.iotShangYunInitStringMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "video")
                {
                    IotData.iotVideoStatusMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "state")
                {
                    IotData.iotPrintStateMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "active")
                {
                    if(dataArray[i].value === true)
                    {
                        IotData.iotDeviceStatusMap[productName] = "online"
                    }
                    else {
                        IotData.iotDeviceStatusMap[productName] = "offline"
                    }
                }
                else if(dataArray[i].key === "nozzleTemp2")
                {
                    IotData.iotNozzleTempTargetMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "bedTemp2")
                {
                    IotData.iotBedTempTargetMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "curFeedratePct")
                {
                    IotData.iotCurFeedratePctTargetMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "layer")
                {
                    IotData.iotLayerPrintMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "printId")
                {
                    IotData.iotPrintIDMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "curPosition")
                {
                    IotData.iotCurPositionMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "fan")
                {
                    IotData.iotFanSwitchMap[productName] = dataArray[i].value
                }
                else if(dataArray[i].key === "netIP")
                {
                    IotData.iotNetIPMap[productName] = dataArray[i].value
                }
            }
            updateDeviceQmlData()
        }
    }

    function updateDeviceInformation_NewIOT_RealTime(strjson, productName)
    {
        var dataRet = JSON.parse(strjson)
        if(dataRet.code === 0)
        {
            var dataResult = dataRet.result

            IotData.iotPrintProgressMap[productName] = dataResult.printProgress[0].value

            IotData.iotNozzleTempMap[productName] = dataResult.nozzleTemp[0].value

            IotData.iotBedTempMap[productName] = dataResult.bedTemp[0].value

            IotData.iotCurFeedratePctMap[productName] = dataResult.curFeedratePct[0].value

            IotData.iotPrintedTimesMap[productName] = dataResult.printJobTime[0].value

            IotData.iotTimesLeftToPrintMap[productName] = dataResult.printLeftTime[0].value
        }
    }

    function updateDeviceInformation_NewIOT_Position(strjson, productName)
    {
        var dataRet = JSON.parse(strjson)
        if(dataRet.code === 0)
        {
            var dataResult = dataRet.result

            IotData.iotCurPositionMap[productName] = dataResult.curPosition
        }
    }

    function updateDeviceQmlData()
    {
        for(var i = 0; i < IotData.serDeviceNameArray.length; i++)
        {
            if(IotData.iotNetIPMap[IotData.serDeviceNameArray[i]])
            {
                deviceItemMap[IotData.serDeviceNameArray[i]].printerIp = IotData.iotNetIPMap[IotData.serDeviceNameArray[i]]
            }

            if(IotData.iotDeviceStatusMap[IotData.serDeviceNameArray[i]] === "online")
            {
                deviceItemMap[IotData.serDeviceNameArray[i]].equipmentStatus = deviceOn
            }
            else {
                deviceItemMap[IotData.serDeviceNameArray[i]].equipmentStatus = deviceOff
            }

            if(deviceItemMap[IotData.serDeviceNameArray[i]].equipmentStatus === deviceOn)
            {
                if(IotData.iotPrintStateMap[IotData.serDeviceNameArray[i]] === 1)
                {
                    deviceItemMap[IotData.serDeviceNameArray[i]].printerStatusShow = qsTr("Printing")
                    //deviceItemMap[IotData.serDeviceNameArray[i]].progressVisible =  true
                    //deviceItemMap[IotData.serDeviceNameArray[i]].progressValue =  IotData.iotPrintProgressMap[IotData.serDeviceNameArray[i]]
                }
                else if(IotData.iotPrintStateMap[IotData.serDeviceNameArray[i]] === 5)
                {
                    deviceItemMap[IotData.serDeviceNameArray[i]].printerStatusShow = qsTr("Printing pause")
                    //deviceItemMap[IotData.serDeviceNameArray[i]].progressVisible =  true
                    //deviceItemMap[IotData.serDeviceNameArray[i]].progressValue =  IotData.iotPrintProgressMap[IotData.serDeviceNameArray[i]]
                }
                else {
                    deviceItemMap[IotData.serDeviceNameArray[i]].printerStatusShow = IotData.iotPrinterStatusMap[IotData.serDeviceNameArray[i]]
                    //deviceItemMap[IotData.serDeviceNameArray[i]].progressVisible =  false
                    //deviceItemMap[IotData.serDeviceNameArray[i]].progressValue =  0
                }

                deviceItemMap[IotData.serDeviceNameArray[i]].tFCardStatus = IotData.iotTFCardStatusMap[IotData.serDeviceNameArray[i]]
                deviceItemMap[IotData.serDeviceNameArray[i]].printerStatus = IotData.iotPrinterStatusMap[IotData.serDeviceNameArray[i]]
            }
            else {
                //deviceItemMap[IotData.serDeviceNameArray[i]].progressVisible =  false
                //deviceItemMap[IotData.serDeviceNameArray[i]].progressValue =  0
                deviceItemMap[IotData.serDeviceNameArray[i]].tFCardStatus = tfCardOff
                deviceItemMap[IotData.serDeviceNameArray[i]].printerStatus = printerOff
                deviceItemMap[IotData.serDeviceNameArray[i]].printerStatusShow = printerOff
            }
            deviceItemMap[IotData.serDeviceNameArray[i]].printerImage = deviceImageMap[IotData.iotPrinterNameMap[IotData.serDeviceNameArray[i]]]//update device image

            if(deviceItemMap[IotData.serDeviceNameArray[i]].equipmentStatus === deviceOn && deviceItemMap[IotData.serDeviceNameArray[i]].tFCardStatus === tfCardOn && deviceItemMap[IotData.serDeviceNameArray[i]].printerStatus === printerOn)
            {
                deviceItemMap[IotData.serDeviceNameArray[i]].enabled = true
            }
            else {
                deviceItemMap[IotData.serDeviceNameArray[i]].enabled = false
            }
        }
    }

    function isDeviceEmpty()
    {
        for(var key in deviceItemMap)
        {
            var strkey = "-%1-".arg(key)
            if(strkey !== "-0-")
            {
                return false
            }
        }
        return true
    }
}
