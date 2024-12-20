import QtQml 2.13
import QtQuick 2.13
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.13
import QtQuick.Dialogs 1.3 as File
import "../secondqml"
import "../qml"
Window {
    id: spcpDialog
    title: "基线自动化测试工具"
    width: 800 * screenScaleFactor
    height: 742 * screenScaleFactor
    property real curIndex: 0       //生成scp / 运行scp
    property real readScpType: 0     //0:目录 1:单个文件
    property real inputTypeIndex: 0     //目录下启动类型

    property string curScpFilePath: ""      //单个文件的路径
    property real runType: 1            //generate/compare/update
    property bool vld_checked : false
    property var comBtns: [qsTranslate("SlicerParamCfgPage","Save"), qsTranslate("SlicerParamCfgPage","Reset")]
    color:Constants.themeColor
    Component.onCompleted: {
        Constants.currentTheme = 0
        id:spcpDialog.visible = true
    }
    Rectangle{
        anchors.horizontalCenter: parent.horizontalCenter
        width: 800 * screenScaleFactor
        height: 712 * screenScaleFactor
        color: "transparent"
        Item{
            id: __selectFile
            anchors.top: parent.top
            anchors.topMargin: 10 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 20 * screenScaleFactor
            height: curIndex != 3 ? 28 * screenScaleFactor : 0
            width: parent.width - 40 * screenScaleFactor
            visible : curIndex != 3
            RowLayout
            {
                spacing: 10
                anchors.left : parent.left
                anchors.leftMargin: 30
                StyledLabel {
                    text: qsTr("选择需校对的软件版本exe：")
                    color: Constants.right_panel_text_default_color
                    font.pointSize: Constants.labelFontPointSize_9
                    wrapMode: Text.NoWrap
                    font.family: Constants.labelFontFamily
                }
                CusFolderField
                {
                    id : __exepath
                    Layout.preferredWidth: 500*screenScaleFactor
                    Layout.preferredHeight: 30*screenScaleFactor
                    text:  gTestTool.c3dExePath
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Bold
                    rightPadding : image.width + 20 * screenScaleFactor
                    function callbackText(path)
                    {
                        text = path
                        gTestTool.c3dExePath = path
                    }
                }
                Button
                {
                    text: "..."
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 20
                    onClicked:
                    {
                        fileDialog.itemObj = __exepath
                        fileDialog.nameFilters =  [ "exe files (*.exe)" ]
                        fileDialog.folder = "file:///"+ (__exepath.text)
                        fileDialog.visible = true
                    }
                }
            }
        }
        Rectangle
        {
            width: parent.width - 40
            color: "lightblue"
            height: 2
            anchors.top: __selectFile.bottom
            anchors.topMargin: 10 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 20 * screenScaleFactor
        }

        Column{
            id:leftCom
            anchors.top: __selectFile.bottom
            anchors.topMargin: 28 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 20 * screenScaleFactor
            spacing: 5 * screenScaleFactor
            Repeater{
                model: [("生成脚本文件scp"), ("运行脚本文件scp")]
                delegate: CusButton{
                    width: 150 * screenScaleFactor
                    height: 36 * screenScaleFactor
                    radius: 5 * screenScaleFactor
                    txtContent: modelData
                    txtPointSize: Constants.labelFontPointSize_9
                    txtColor: "#ffffff"
                    normalColor: model.index === spcpDialog.curIndex? "#009cff" : "#4d4d52"
                    hoveredColor: "#009cff"
                    pressedColor: "#009cff"
                    onClicked:{
                        spcpDialog.curIndex = index
                        gTestTool.clearMessageText()
                    }
                }
            }
            Column
            {
                id : _runCol
                visible: curIndex == 1
                anchors.left: parent.left
                anchors.leftMargin: 20
                spacing: 5

                CusButton{
                    width: 130 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    radius: 5 * screenScaleFactor
                    txtContent: "比较基线"
                    txtPointSize: Constants.labelFontPointSize_9
                    txtColor: "#ffffff"
                    hoveredColor: "#009cff"
                    pressedColor: "#009cff"
                    isChecked : spcpDialog.runType === 1
                    onClicked:{
                        spcpDialog.runType = 1
                    }
                }
                CusButton{
                    width: 130 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    radius: 5 * screenScaleFactor
                    txtContent: "生成基线"
                    txtPointSize: Constants.labelFontPointSize_9
                    txtColor: "#ffffff"
                    hoveredColor: "#009cff"
                    pressedColor: "#009cff"
                    isChecked : spcpDialog.runType === 0
                    onClicked:{
                        spcpDialog.runType = 0

                    }
                }
                CusButton{
                    width: 130 * screenScaleFactor
                    height: 28 * screenScaleFactor
                    radius: 5 * screenScaleFactor
                    txtContent: "更新基线"
                    txtPointSize: Constants.labelFontPointSize_9
                    txtColor: "#ffffff"
                    hoveredColor: "#009cff"
                    pressedColor: "#009cff"
                    isChecked : spcpDialog.runType === 2
                    onClicked:{
                        spcpDialog.runType = 2
                    }
                }
            }
            CusButton
            {
                width: 150 * screenScaleFactor
                height: 36 * screenScaleFactor
                radius: 5 * screenScaleFactor
                txtContent: "vld内存检测"
                txtPointSize: Constants.labelFontPointSize_9
                txtColor: "#ffffff"
                normalColor: spcpDialog.curIndex == 2 ? "#009cff" : "#4d4d52"
                hoveredColor: "#009cff"
                pressedColor: "#009cff"
                enabled : unittool_vld_enabled ? true : false
                onClicked:{
                    spcpDialog.curIndex = 2
                    vld_checked = true
                    gTestTool.clearMessageText()
                }
            }
            CusButton
            {
                width: 150 * screenScaleFactor
                height: 36 * screenScaleFactor
                radius: 5 * screenScaleFactor
                txtContent: "引擎参数GCode对比"
                txtPointSize: Constants.labelFontPointSize_9
                txtColor: "#ffffff"
                normalColor: spcpDialog.curIndex == 3 ? "#009cff" : "#4d4d52"
                hoveredColor: "#009cff"
                pressedColor: "#009cff"
                onClicked:{
                    spcpDialog.curIndex = 3
                    gTestTool.clearMessageText()
                }
            }
        }
        Rectangle{
            id: sep
            width: 1 * screenScaleFactor
            height: 651 * screenScaleFactor
            color: "#4e4e53"
            anchors.left: leftCom.right
            anchors.leftMargin: 20 * screenScaleFactor
            anchors.top: leftCom.top
        }
        StackLayout{
            id:stackLayout
            anchors.left: sep.right
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.top: sep.top
            width: 570 * screenScaleFactor
            height: 651 * screenScaleFactor
            currentIndex: spcpDialog.curIndex
            Loader{
                id: loader3
                sourceComponent: __scpConfigCom
            }
            Loader{
                id: loader1
                sourceComponent: _3mfSliceCom
            }
            Loader{
                sourceComponent: _3mfSliceCom
            }
            Loader{
                sourceComponent: __gcodeCom
            }
        }
    }
    Component
    {
        id : _3mfSliceCom
        Rectangle
        {
            color: "transparent"
            ColumnLayout
            {
                spacing: 10

                Rectangle
                {
                    color: "transparent"
                    border.width: 1
                    border.color: "green"
                    Layout.preferredWidth: parent.width
                    Layout.preferredHeight: 100 * screenScaleFactor
                    Column
                    {
                        spacing: 5

                        RowLayout
                        {
                            spacing: 10
                            height: 20 * screenScaleFactor
                            StyledLabel
                            {
                                text: "脚本文件的读取方式："
                                Layout.preferredWidth: 160  * screenScaleFactor
                            }
//                            BasicCombobox
//                            {
//                                currentIndex: readScpType
//                                model:["整个目录","单一文件"]
//                                Layout.preferredWidth: 200  * screenScaleFactor
//                                Layout.preferredHeight: 28  * screenScaleFactor
//                                onActivated:
//                                {
//                                    readScpType = currentIndex
//                                }
//                            }
                            BasicRadioButton
                            {
                                text: qsTr("整个目录")
                                checked : readScpType === 0
                                font.pointSize: Constants.labelFontPointSize_10
                                font.weight: Font.bold
                                font.family: Constants.labelFontFamilyBold
                                onClicked:
                                {
                                    readScpType = 0
                                }
                            }
                            BasicRadioButton
                            {
                                text: qsTr("单一文件")
                                checked : readScpType === 1
                                font.pointSize: Constants.labelFontPointSize_10
                                font.weight: Font.bold
                                font.family: Constants.labelFontFamilyBold
                                onClicked:
                                {
                                    readScpType = 1
                                }
                            }
                        }
                        StackLayout
                        {
                            width: parent.width
                            height: 75 * screenScaleFactor
                            currentIndex: spcpDialog.readScpType
                            Loader{
                                sourceComponent:
                                ColumnLayout
                                {
                                    RowLayout
                                    {
                                        spacing: 10
                                        StyledLabel {
                                            Layout.preferredWidth: 120*screenScaleFactor
                                            text: qsTr("脚本scp目录:")
                                            color: Constants.right_panel_text_default_color
                                            font.pointSize: Constants.labelFontPointSize_9
                                            wrapMode: Text.NoWrap
                                            font.family: Constants.labelFontFamily
                                        }
                                        CusFolderField
                                        {
                                            id : __scpPath
                                            Layout.preferredWidth: 400*screenScaleFactor
                                             Layout.preferredHeight: 30*screenScaleFactor
                                            text:  gTestTool.scpPath
                                            font.family: Constants.labelFontFamilyBold
                                            font.weight: Font.Bold
                                            rightPadding : image.width + 20 * screenScaleFactor
                                            function callbackText(path)
                                            {
                                                text = path
                                                gTestTool.scpPath = path
                                            }
                                        }
                                        Button
                                        {
                                            text: "..."
                                            Layout.preferredWidth: 30
                                            Layout.preferredHeight: 20
                                            onClicked:
                                            {
                                                folderDialog.itemObj = __scpPath
                                                folderDialog.folder = "file:///"+ (__scpPath.text)
                                                folderDialog.visible = true
                                            }
                                        }
                                    }
                                    Rectangle
                                    {
                                        color: "transparent"
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 30 * screenScaleFactor

                                        RowLayout
                                        {
                                            spacing: 10
                                            height: 20 * screenScaleFactor
                                            StyledLabel
                                            {
                                                text: "脚本文件启动软件的方式："
                                                width: 200  * screenScaleFactor
                                            }
                                            BasicRadioButton
                                            {
                                                text: qsTr("仅一次启动软件运行所有脚本")
                                                checked : inputTypeIndex === 0
                                                font.pointSize: Constants.labelFontPointSize_10
                                                font.weight: Font.bold
                                                font.family: Constants.labelFontFamilyBold
                                                onClicked:
                                                {
                                                    inputTypeIndex = 0
                                                }
                                            }
                                            BasicRadioButton
                                            {
                                                text: qsTr("按脚本个数启动软件")
                                                checked : inputTypeIndex === 1
                                                font.pointSize: Constants.labelFontPointSize_10
                                                font.weight: Font.bold
                                                font.family: Constants.labelFontFamilyBold
                                                onClicked:
                                                {
                                                    inputTypeIndex = 1
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Loader{
                                id: loader3
                                sourceComponent:
                                RowLayout
                                {
                                    spacing: 10
                                    StyledLabel {
                                        text: qsTr("单个脚本scp文件路径:")
                                        color: Constants.right_panel_text_default_color
                                        font.pointSize: Constants.labelFontPointSize_9
                                        wrapMode: Text.NoWrap
                                        font.family: Constants.labelFontFamily
                                    }
                                    CusFolderField
                                    {
                                        id : _scpfilePath
                                        Layout.preferredWidth: 400*screenScaleFactor
                                        Layout.preferredHeight: 30*screenScaleFactor
                                        font.family: Constants.labelFontFamilyBold
                                        font.weight: Font.Bold
                                        rightPadding : image.width + 20 * screenScaleFactor
                                        function callbackText(path)
                                        {
                                            console.log("callbackText path",path)
                                            text = path
                                            curScpFilePath = path
                                        }
                                    }
                                    Button
                                    {
                                        text: "..."
                                        Layout.preferredWidth: 30
                                        Layout.preferredHeight: 20
                                        onClicked:
                                        {
                                            fileDialog.itemObj = _scpfilePath
                                            fileDialog.nameFilters =  [ "scp files (*.scp)" ]
                                            fileDialog.folder = "file:///"+ (_scpfilePath.text)
                                            fileDialog.visible = true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout
                {
                    spacing: 10
                    StyledLabel {
                        Layout.preferredWidth: 120*screenScaleFactor
                        text: qsTr("运行结果目录：")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        id : __compare
                        Layout.preferredWidth: 400*screenScaleFactor
                         Layout.preferredHeight: 30*screenScaleFactor
                        text:  gTestTool.compareSliceBLPath
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        function callbackText(path)
                        {
                            text = path
                            gTestTool.compareSliceBLPath = path
                        }
                    }
                    Button
                    {
                        text: "..."
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 20
                        onClicked:
                        {
                            folderDialog.itemObj = __compare
                            folderDialog.folder = "file:///"+ (__compare.text)
                            folderDialog.visible = true
                        }
                    }
                }
                RowLayout
                {
                    spacing: 10
                    StyledLabel {
                        text: qsTr("基线目录：")
                        Layout.preferredWidth: 120*screenScaleFactor
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        id : __blpath
                        Layout.preferredWidth: 400*screenScaleFactor
                        Layout.preferredHeight: 30*screenScaleFactor
                        text:  gTestTool.slicerBLPath
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Bold
                        rightPadding : image.width + 20 * screenScaleFactor
                        function callbackText(path)
                        {
                            text = path
                            gTestTool.slicerBLPath = path
                        }
                    }
                    Button
                    {
                        text: "..."
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 20
                        onClicked:
                        {
                            folderDialog.itemObj = __blpath
                            folderDialog.folder = "file:///"+ (__blpath.text)
                            folderDialog.visible = true
                        }
                    }
                }


                RowLayout
                {
                    CusButton{
                        Layout.preferredWidth: 140 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "执行"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        enabled : !(readScpType ==1 && curScpFilePath === "")
                        onClicked:
                        {
                            if(vld_checked)
                            {
                                gTestTool.vldTest()
                                return;
                            }
                            if(readScpType ===0)
                            {
                                if(inputTypeIndex ===0)
                                {
                                    gTestTool.doTest(false,spcpDialog.runType,"")
                                }
                                else
                                {
                                    gTestTool.doTestOneByOne(spcpDialog.runType)
                                }
                            }
                            else if(readScpType === 1)
                            {
                                gTestTool.doTest(false,spcpDialog.runType,curScpFilePath)
                            }
                        }
                    }

                    StyleCheckBox {
                        Layout.preferredHeight: 28 * screenScaleFactor
                        Layout.preferredWidth: 80
                        checked: gTestTool.isSimple
                        text: qsTr("简易方式")
                        onClicked: {
                            gTestTool.isSimple = checked
                        }
                    }
                }

                Rectangle
                {
                    color: "transparent"
                    border.width: 1
                    border.color :"green"
                    Layout.preferredWidth: 570 * screenScaleFactor
                    Layout.preferredHeight: 500
                    ColumnLayout
                    {
                        spacing: 2
                        RowLayout
                        {
                            spacing:0
                            StyledLabel
                            {
                                text:"结果展示："
                            }
                            Rectangle
                            {
                                color: "white"
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                            }
                        }
                        RowLayout
                        {
                            spacing:0
                            StyledLabel
                            {
                                text:"过滤条件:"
                            }
                            Item {
                                Layout.preferredHeight: 1
                                Layout.fillWidth: true
                            }
                            StyleCheckBox
                            {
                                id: __errorCheckBox
                                text: "仅错误信息"
                                Layout.minimumWidth: 80 * screenScaleFactor
                                Layout.preferredHeight: 20 * screenScaleFactor
                                checked: false
                                onToggled:
                                {
                                    gTestTool.swichMessageShow(checked)
                                }
                            }
                            StyleCheckBox
                            {
                                id: __passCheckBox
                                text: "错误列表"
                                Layout.minimumWidth: 80 * screenScaleFactor
                                Layout.preferredHeight: 20 * screenScaleFactor
                                checked: false
                                onToggled:
                                {
                                    if(checked)
                                    {
                                        gTestTool.failedFileShow()
                                    }
                                    else
                                    {
                                        gTestTool.swichMessageShow(__errorCheckBox.checked)
                                    }
                                }
                            }
                        }
                        ScrollView {
                            Layout.preferredHeight: 400
                            Layout.preferredWidth: 560
                            TextArea {
                                id: textArea
                                anchors.margins: 10
//                                width: 560
//                                height: 400

                                wrapMode: TextEdit.Wrap
                                color: "white"
                                font.family: Constants.labelFontFamily
                                font.weight: Constants.labelFontWeight
                                font.pointSize: Constants.labelFontPointSize_10
                                font.letterSpacing: 1.2
                                textFormat: TextEdit.AutoText
                                selectByMouse: true
                                text:htmlContent(gTestTool.message)
                                cursorVisible: true;
                                background: Rectangle
                                {
                                    anchors.fill : parent
                                    color: Constants.tooltipBgColor
                                    border.width:1
                                    radius: 5
                                }
                                // 将原始文本传递给微小的修改和展现字符色彩部分
                                function htmlContent(text) {
                                    var originalText = text
                                    var processedText = originalText.replace(/Pass:/g, "<span style='color:green'>Pass: </span>")
                                    var processedText2 = processedText.replace(/Failed:/g, "<span style='color:red'>Failed: </span>")
                                    var processedText3 = processedText2.replace(/\n/g, "<br>")
                                    return processedText3
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component
    {
        id : __scpConfigCom

        ColumnLayout
        {
            spacing: 10
            RowLayout
            {
                spacing: 10
                StyledLabel {
                    Layout.preferredWidth : 120
                    text: qsTr("选择3mf的目录：")
                    color: Constants.right_panel_text_default_color
                    font.pointSize: Constants.labelFontPointSize_9
                    wrapMode: Text.NoWrap
                    font.family: Constants.labelFontFamily

                }
                CusFolderField
                {
                    id : __3mfpath
                    Layout.preferredWidth: 400*screenScaleFactor
                    Layout.preferredHeight: 30*screenScaleFactor
                    text:  gTestTool._3mfPath
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Bold
                    rightPadding : image.width + 20 * screenScaleFactor
                    function callbackText(path)
                    {
                        text = path
                        gTestTool._3mfPath = path
                    }
                }
                Button
                {
                    text: "..."
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 20
                    onClicked:
                    {
                        console.log("aaaaa")
                        folderDialog.itemObj = __3mfpath
                        folderDialog.folder = "file:///"+ (__3mfpath.text)
                        folderDialog.visible = true
                    }
                }
            }
            RowLayout
            {
                spacing: 10
                StyledLabel {
                    Layout.preferredWidth : 120
                    text: qsTr("选择scp配置的目录：")
                    color: Constants.right_panel_text_default_color
                    font.pointSize: Constants.labelFontPointSize_9
                    wrapMode: Text.NoWrap
                    font.family: Constants.labelFontFamily
                }
                CusFolderField
                {
                    id : __compare
                    Layout.preferredWidth: 400*screenScaleFactor
                     Layout.preferredHeight: 30*screenScaleFactor
                    text:  gTestTool.scpPath
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Bold
                    rightPadding : image.width + 20 * screenScaleFactor
                    function callbackText(path)
                    {
                        text = path
                        gTestTool.scpPath = path
                    }
                }
                Button
                {
                    text: "..."
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 20
                    onClicked:
                    {
                        console.log("aaaaa")
                        folderDialog.itemObj = __compare
                        folderDialog.folder = "file:///"+ (__compare.text)
                        folderDialog.visible = true
                    }

                }
            }
            Rectangle
            {
                color: "transparent"
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                RowLayout
                {
                    spacing: 10
                    StyledLabel {
                        Layout.preferredWidth : 120
                        text: qsTr("scp录入基线流程：")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    StyleCheckBox {
                        id: uniformScalingCheckbox
                        checked: true
                        text: qsTr("3mf->slicer")
                        onToggled:
                        {

                        }
                    }
                }

            }

            CusButton{
                Layout.preferredWidth: 140 * screenScaleFactor
                Layout.preferredHeight: 28 * screenScaleFactor
                radius: 5 * screenScaleFactor
                txtContent: "执行生成scp"
                txtPointSize: Constants.labelFontPointSize_9
                txtColor: "#ffffff"
                onClicked:{
                    gTestTool.generateScript()
                }
            }
            Loader
            {
                Layout.preferredWidth: 570 * screenScaleFactor
                Layout.preferredHeight: 500
                sourceComponent: __textAreaCom
            }

        }
    }

    Component
    {
        id: __gcodeCom

        ColumnLayout
        {
            spacing: 10
            id : __gcodeCol
            property real gcodeType : 0
            property string _3mfFile: gTestTool.cur3mfFile
            property string _cacheFile:  ""
            Rectangle
            {
                color: "transparent"
                border.width: 1
                border.color: "green"
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: 30 * screenScaleFactor
                Row
                {
                    spacing: 10
                    StyledLabel
                    {
                        text: "生成GCode的方式:"
                        width: 100  * screenScaleFactor
                        height: 20 * screenScaleFactor
                        anchors.verticalCenter: parent.horizontalCenter
                    }
                    CusButton{
                        width: 130 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "3mf->gcode"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        hoveredColor: "#009cff"
                        pressedColor: "#009cff"
                        isChecked : __gcodeCol.gcodeType === 0
                        onClicked:{
                            __gcodeCol.gcodeType = 0
                        }
                    }
                    CusButton{
                        width: 130 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "引擎参数->gcode"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        hoveredColor: "#009cff"
                        pressedColor: "#009cff"
                        isChecked : __gcodeCol.gcodeType === 1
                        onClicked:{
                            __gcodeCol.gcodeType = 1
                        }
                    }
                }
            }
            Rectangle
            {
                color: "transparent"
                border.width: 1
                border.color: "green"
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: 140 * screenScaleFactor
                Column
                {
                    spacing: 5
                    StyledLabel
                    {
                        text: "输入："
                        width: 160  * screenScaleFactor
                    }
                    RowLayout
                    {
                        spacing: 10
                        visible:  __gcodeCol.gcodeType === 0
                        StyledLabel {
                            Layout.preferredWidth: 120*screenScaleFactor
                            text: qsTr("导入3mf文件:")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            id : __3mfFilePath
                            Layout.preferredWidth: 400*screenScaleFactor
                             Layout.preferredHeight: 30*screenScaleFactor
                            text:  gTestTool.cur3mfFile
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            function callbackText(path)
                            {
                                text = path
                                gTestTool.cur3mfFile = path
                            }
                        }
                        Button
                        {
                            text: "..."
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 20
                            onClicked:
                            {
                                fileDialog.itemObj = __3mfFilePath
                                fileDialog.nameFilters =  [ "3mf files (*.3mf)" ]
                                var url = Qt.resolvedUrl(__3mfFilePath.text)
                                var directory = url.substring(0, url.lastIndexOf('/'))
                                fileDialog.folder = "file:///"+ directory
                                fileDialog.visible = true
                            }
                        }
                    }
                    RowLayout
                    {
                        spacing: 10
                        visible:  __gcodeCol.gcodeType === 1
                        StyledLabel {
                            Layout.preferredWidth: 120*screenScaleFactor
                            text: qsTr("引擎参数文件路径:")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            id : __slicerDataPath
                            Layout.preferredWidth: 400*screenScaleFactor
                             Layout.preferredHeight: 30*screenScaleFactor
                            text:  _cacheFile
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            function callbackText(path)
                            {
                                text = path
                                _cacheFile = path
//                                gTestTool.cur3mfFile = path
                            }
                        }
                        Button
                        {
                            text: "..."
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 20
                            onClicked:
                            {
                                var url = Qt.resolvedUrl(__slicerDataPath.text)
                                var directory = url.substring(0, url.lastIndexOf('/'))
                                if(__slicerDataPath.text == "")
                                {
                                    console.log("67890")
                                    directory = Qt.resolvedUrl(gTestTool.slicerCacheDir)
                                }
                                fileDialog.folder = "file:///"+ directory
                                fileDialog.itemObj = __slicerDataPath
                                fileDialog.nameFilters =  [ "cache files (*.cxcache)" ]
                                fileDialog.visible = true
                            }
                        }
                    }
                    RowLayout
                    {
                        spacing: 10
                        StyledLabel {
                            Layout.preferredWidth: 120*screenScaleFactor
                            text: qsTr("c3d 应用程序路径:")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            id: __c3dExe
                            Layout.preferredWidth: 400*screenScaleFactor
                             Layout.preferredHeight: 30*screenScaleFactor
                            text:  gTestTool.c3dExePath
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            function callbackText(path)
                            {
                                text = path
                                gTestTool.c3dExePath = path
                            }
                        }
                        Button
                        {
                            text: "..."
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 20
                            onClicked:
                            {
                                var url = Qt.resolvedUrl(__c3dExe.text)
                                var directory = url.substring(0, url.lastIndexOf('/'))
                                fileDialog.folder = "file:///"+ directory
                                fileDialog.itemObj = __c3dExe
                                fileDialog.nameFilters =  [ "exe files (*.exe)" ]
                                fileDialog.visible = true
                            }
                        }
                    }
                    RowLayout
                    {
                        spacing: 10
                        StyledLabel {
                            Layout.preferredWidth: 120*screenScaleFactor
                            text: qsTr("orca应用程序路径:")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            id : __orcaExe
                            Layout.preferredWidth: 400*screenScaleFactor
                             Layout.preferredHeight: 30*screenScaleFactor
                            text:  gTestTool.orcaAppPath
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            function callbackText(path)
                            {
                                text = path
                                gTestTool.orcaAppPath = path
                            }
                        }
                        Button
                        {
                            text: "..."
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 20
                            onClicked:
                            {
                                var url = Qt.resolvedUrl(__orcaExe.text)
                                var directory = url.substring(0, url.lastIndexOf('/'))
                                fileDialog.folder = "file:///"+ directory
                                fileDialog.itemObj = __orcaExe
                                fileDialog.nameFilters =  [ "exe files (*.exe)" ]
                                fileDialog.visible = true
                            }
                        }
                    }
                }
            }

            Rectangle
            {
                color: "transparent"
                border.width: 1
                border.color: "green"
                Layout.preferredWidth: parent.width
                Layout.preferredHeight: 110 * screenScaleFactor
                Column
                {
                    spacing: 5
                    StyledLabel
                    {
                        text: "输出："
                        width: 160  * screenScaleFactor
                    }
                    RowLayout
                    {
                        spacing: 10
                        visible:  __gcodeCol.gcodeType === 0
                        StyledLabel {
                            Layout.preferredWidth: 120*screenScaleFactor
                            text: qsTr("导出切片引擎参数的文件目录:")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            id : __cachePath
                            Layout.preferredWidth: 400*screenScaleFactor
                             Layout.preferredHeight: 30*screenScaleFactor
                            text:  gTestTool.slicerCacheDir
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            function callbackText(path)
                            {
                                text = path
                                gTestTool.slicerCacheDir = path
                            }
                        }
                        Button
                        {
                            text: "..."
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 20
                            onClicked:
                            {
                                var directory = Qt.resolvedUrl(__cachePath.text)
                                folderDialog.folder = "file:///"+ directory
                                folderDialog.itemObj = __cachePath
                                folderDialog.visible = true
                            }
                        }
                    }
                    RowLayout
                    {
                        spacing: 10
                        StyledLabel {
                            Layout.preferredWidth: 120*screenScaleFactor
                            text: qsTr("导出gcode的目录:")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            id : __gcodePath
                            Layout.preferredWidth: 400*screenScaleFactor
                             Layout.preferredHeight: 30*screenScaleFactor
                            text:  gTestTool.gcodeDir
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            function callbackText(path)
                            {
                                text = path
                                gTestTool.gcodeDir = path
                            }
                        }
                        Button
                        {
                            text: "..."
                            Layout.preferredWidth: 30
                            Layout.preferredHeight: 20
                            onClicked:
                            {
                                var directory = Qt.resolvedUrl(__gcodePath.text)
                                folderDialog.folder = "file:///"+ directory
                                folderDialog.itemObj = __gcodePath
                                folderDialog.visible = true
                            }
                        }
                    }
                }
            }
            Rectangle
            {
                color: "transparent"
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                RowLayout
                {
                    spacing: 10
                    CusButton{
                        Layout.preferredWidth: 140 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "运行c3d 生成Gcode"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        onClicked:{
                            if(__gcodeCol.gcodeType == 0)
                            {
                                gTestTool.runGCodeCompare(__gcodeCol._3mfFile,0)
                            }
                            else if(__gcodeCol.gcodeType == 1)
                            {
                                gTestTool.runGCodeCompare(__gcodeCol._cacheFile,0)
                            }
                        }
                    }
                    CusButton{
                        Layout.preferredWidth: 140 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "运行orca 生成Gcode"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        onClicked:{
                            if(__gcodeCol.gcodeType == 0)
                            {
                                gTestTool.runGCodeCompare(__gcodeCol._3mfFile,1)
                            }
                            else if(__gcodeCol.gcodeType == 1)
                            {
                                gTestTool.runGCodeCompare(__gcodeCol._cacheFile,1)
                            }
                        }
                    }

                }

            }

            Loader
            {
                Layout.preferredWidth: 570 * screenScaleFactor
                Layout.preferredHeight: 400  * screenScaleFactor
                sourceComponent: __textAreaCom
                onLoaded:
                {
                    item.curHeight = 400
                }
            }
            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }

    Component
    {
        id : __textAreaCom
        Rectangle
        {
            color: "transparent"
            border.width: 1
            border.color :"green"
            property real curHeight: 500
            ColumnLayout
            {
                spacing: 2
                RowLayout
                {
                    spacing:0
                    StyledLabel
                    {
                        text:"结果展示："
                    }
                    Rectangle
                    {
                        color: "white"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                    }
                }
                TextArea {
                    id: textArea
                    Layout.leftMargin: 10
                    Layout.minimumHeight: curHeight - 50
                    Layout.minimumWidth: 560
                    Layout.fillHeight: true

                    wrapMode: TextEdit.Wrap
                    color: "white"
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    font.letterSpacing: 1.2
                    textFormat: TextEdit.AutoText
                    selectByMouse: true
                    text:htmlContent(gTestTool.message)
                    background: Rectangle
                    {
                        anchors.fill : parent
                        color: Constants.tooltipBgColor
                        border.width:1
                        radius: 5
                    }
                    // 将原始文本传递给微小的修改和展现字符色彩部分
                    function htmlContent(text) {
                        var originalText = text
                        var processedText = originalText.replace(/Pass:/g, "<span style='color:green'>Pass: </span>")
                        var processedText2 = processedText.replace(/Failed:/g, "<span style='color:red'>Failed: </span>")
                        var processedText3 = processedText2.replace(/\n/g, "<br>")
                        return processedText3
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
            console.log("You chose file: " + fileDialog.fileUrl.toString().replace("file:///",""))
            itemObj.callbackText(fileDialog.fileUrl.toString().replace("file:///",""))
        }
        onRejected: {
            visible = false
        }
        Component.onCompleted: visible = false
    }
}
