import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3 as File
import Qt.labs.platform 1.1 as File2
import "../qml"
import "../components"
import "../secondqml"
BasicDialogV4 {
    id: root

    modal: false
    width: 500 * screenScaleFactor
    height: {
        320 * screenScaleFactor
    }
    titleHeight: 30 * screenScaleFactor
    title: qsTr("引擎数据配置")
    maxBtnVis: false
    property real curType: 0
    property string _3mffile: ""
    property string cacheFile : ""
    property string gcodeFile : ""
    onVisibleChanged: {}

    bdContentItem: ColumnLayout {
        spacing: 10 * screenScaleFactor
        Item
        {
            Layout.topMargin:  10 * screenScaleFactor
            Layout.leftMargin:  10 * screenScaleFactor
            Layout.rightMargin: 10 * screenScaleFactor
            Layout.minimumHeight:  250 * screenScaleFactor
            Layout.maximumHeight: Layout.minimumHeight
            Layout.fillWidth: true
            Column
            {
                anchors.centerIn: parent
                spacing:  10 * screenScaleFactor
                Row
                {
                    spacing : 10
                    CusButton{
                        width: 130 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "保存引擎数据"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        hoveredColor: "#009cff"
                        pressedColor: "#009cff"
                        isChecked : root.curType === 0
                        onClicked:{
                            root.curType = 0
                        }
                    }
                    CusButton{
                        width: 130 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "读取引擎数据"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        hoveredColor: "#009cff"
                        pressedColor: "#009cff"
                        isChecked : root.curType == 1
                        onClicked:{
                            root.curType = 1

                        }
                    }

                }

                StackLayout{
                    id:stackLayout
                    width: 460 * screenScaleFactor
                    height: 200 * screenScaleFactor
                    currentIndex: root.curType
                    Loader{
                        id: loader3
                        sourceComponent: __com1
                    }
                    Loader{
                        id: loader1
                        sourceComponent: __com2
                    }
                }
                
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
    Component
    {
        id : __com1
        Column
        {
            spacing : 10
            Item
            {
                height: 50*screenScaleFactor
                width: 450*screenScaleFactor
                Column
                {
                    spacing: 5 *screenScaleFactor
                    StyledLabel {
                        text: qsTr("3mf File")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        width: 450*screenScaleFactor
                        height: 30*screenScaleFactor
                        text:  ""
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        onClicked:
                        {
                            fileDialog.itemObj = this
                            fileDialog.nameFilters =  [ "3mf files (*.3mf)" ]
                            fileDialog.folder = "file:///"+ (text)
                            fileDialog.visible = true
                        }
                        function callbackText(path)
                        {
                            text = path
                            _3mffile = path
                        }
                    }
                }

            }
            Item
            {
                height: 50*screenScaleFactor
                width: 450*screenScaleFactor
                Column
                {
                    spacing: 5 *screenScaleFactor
                    StyledLabel {
                        text: qsTr("Silce Cache Dir")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        width: 450*screenScaleFactor
                        height: 30*screenScaleFactor
                        text:  kernel_unittest.sliceCachePath
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        onClicked:
                        {
                            folderDialog.itemObj = this
                            folderDialog.folder = "file:///"+ (text)
                            folderDialog.visible = true
                        }
                        function callbackText(path)
                        {
                            text = path
                            kernel_unittest.sliceCachePath = path
                        }
                    }
                }

            }
            Item
            {
                height: 50*screenScaleFactor
                width: 450*screenScaleFactor
                Column
                {
                    spacing: 5 *screenScaleFactor
                    StyledLabel {
                        text: qsTr("Gcode OutName")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        width: 450*screenScaleFactor
                        height: 30*screenScaleFactor
                        text:  kernel_unittest.gcodePath
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        onClicked:
                        {
                            folderDialog.itemObj = this
                            folderDialog.folder = "file:///"+ (text)
                            folderDialog.visible = true
                        }
                        function callbackText(path)
                        {
                            text = path
                            kernel_unittest.gcodePath = path
                        }
                    }
                }

            }
            Row
            {
                spacing: 5* screenScaleFactor
                height: 28 * screenScaleFactor
                BasicButton {
                    width: 150 * screenScaleFactor

                    height: 28 * screenScaleFactor
                    btnRadius: height / 2
                    btnBorderW: 0
                    defaultBtnBgColor: Constants.profileBtnColor
                    hoveredBtnBgColor: Constants.profileBtnHoverColor
                    text: "Run"
                    onSigButtonClicked: {
                        kernel_unittest.sliceGcodeFrom3mf(_3mffile)
                    }
                }
            }
        }
    }
    Component
    {
        id : __com2
        Column
        {
            spacing : 10
            Item
            {
                height: 50*screenScaleFactor
                width: 450*screenScaleFactor
                Column
                {
                    spacing: 5 *screenScaleFactor
                    StyledLabel {
                        text: qsTr("Slicer Cache File")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        width: 450*screenScaleFactor
                        height: 30*screenScaleFactor
                        text:  ""
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        onClicked:
                        {
                            fileDialog.itemObj = this
                            fileDialog.nameFilters =  [ "cache files (*.cxcache)" ]
                            fileDialog.folder = "file:///"+ (text)
                            fileDialog.visible = true
                        }
                        function callbackText(path)
                        {
                            text = path
                            cacheFile = path
                        }
                    }
                }

            }

            Item
            {
                height: 50*screenScaleFactor
                width: 450*screenScaleFactor
                Column
                {
                    spacing: 5 *screenScaleFactor
                    StyledLabel {
                        text: qsTr("Gcode OutName")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        width: 450*screenScaleFactor
                        height: 30*screenScaleFactor
                        text:  ""
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        onClicked:
                        {
                            saveDialog.itemObj = this
                            saveDialog.nameFilters =  [ "gcode files (*.gcode)" ]
                            saveDialog.folder = "file:///"+ (text)
                            saveDialog.visible = true
                        }
                        function callbackText(path)
                        {
                            text = path
                            gcodeFile = path
                        }
                    }
                }

            }
            Row
            {
                spacing: 5* screenScaleFactor
                height: 28 * screenScaleFactor
                BasicButton {
                    width: 150 * screenScaleFactor

                    height: 28 * screenScaleFactor
                    btnRadius: height / 2
                    btnBorderW: 0
                    defaultBtnBgColor: Constants.profileBtnColor
                    hoveredBtnBgColor: Constants.profileBtnHoverColor
                    text: "Run"
                    onSigButtonClicked: {
                        kernel_slice_flow.slicerCacheToGcode(cacheFile,gcodeFile)
                    }
                }
            }
        }

    }

    File.FileDialog {
        id: folderDialog
        title: "Please choose path"
        property var itemObj: null
        selectFolder : true
        selectExisting : true
        onAccepted: {
            console.log("You chose folder: " + folderDialog.folder.toString().replace("file:///",""))
            itemObj.callbackText(folderDialog.folder.toString().replace("file:///",""))
        }
        onRejected: {
            console.log("FileDialog Canceled")
            visible = false
        }
        Component.onCompleted: visible = false
    }
    File.FileDialog {
        id: fileDialog
        title: "Please choose file"
        property var itemObj: null
        selectMultiple : false
        onAccepted: {
            console.log("You chose files: " + fileDialog.fileUrls)
            console.log("You chose file: " + fileDialog.fileUrl.toString().replace("file:///",""))
            itemObj.callbackText(fileDialog.fileUrl.toString().replace("file:///",""))
        }
        onRejected: {
            visible = false
        }
        Component.onCompleted: visible = false
    }

    File2.FileDialog {
        id: saveDialog
        title: qsTr("Export")
        fileMode: File2.FileDialog.SaveFile
        nameFilters: ["Profiles (*.gcode)", "All files (*)"]
        defaultSuffix : ".gcode"
        property var itemObj: null
        onAccepted: {
            console.log("You chose file: " + saveDialog.file.toString().replace("file:///",""))
            itemObj.callbackText(saveDialog.file.toString().replace("file:///",""))
        }
        onRejected: {
            visible = false
        }
        Component.onCompleted: visible = false
    }


}
