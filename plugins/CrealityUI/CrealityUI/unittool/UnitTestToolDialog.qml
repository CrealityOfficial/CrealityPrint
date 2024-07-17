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
    title: qsTranslate("SlicerParamCfgPage","Resin Config Tool")
    width: 800 * screenScaleFactor
    height: 742 * screenScaleFactor
    property real curIndex: 0
    property real radioIndex: 0
    property string curFilePath: ""
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
            height: 28 * screenScaleFactor
            width: parent.width - 40 * screenScaleFactor
            Row
            {
                spacing : 10 * screenScaleFactor
                BasicRadioButton
                {
                    id: __normalMode
                    text: qsTr("English")
                    checked : radioIndex === 0
                    font.pointSize: Constants.labelFontPointSize_10
                    font.weight: Font.bold
                    font.family: Constants.labelFontFamilyBold
                    onClicked:
                    {
                      radioIndex = 0
                       gTestTool.changeLanguage(0)
                    }
                }

                BasicRadioButton
                {
                    id : __dynaxMode
                    text: qsTr("Chinese/中文")
                    checked : radioIndex === 1
                    font.pointSize: Constants.labelFontPointSize_10
                    font.weight: Font.bold
                    font.family: Constants.labelFontFamilyBold
                    onClicked:
                    {
                        radioIndex = 1
                        gTestTool.changeLanguage(1)
                    }
                }
            }
            Column{
                id:leftCom
                anchors.top: __selectFile.bottom
                anchors.topMargin: 10 * screenScaleFactor
                anchors.left: parent.left
                anchors.leftMargin: 20 * screenScaleFactor
                spacing: 10 * screenScaleFactor
                Repeater{
                    model: [qsTranslate("SlicerParamCfgPage","3mf->slice Test"), qsTranslate("SlicerParamCfgPage","xxxxx")]
                    delegate: CusButton{
                        width: 140 * screenScaleFactor
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
                        }
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
                anchors.leftMargin: 20 * screenScaleFactor
                anchors.top: sep.top
                width: 580 * screenScaleFactor
                height: 651 * screenScaleFactor
                currentIndex: spcpDialog.curIndex
                Loader{
                    id: loader1
                    sourceComponent: _3mfSliceCom
                }
                Loader{
                    id: loader3
                    sourceComponent: _xxxCom
                }

            }
        }
    }
    Component
    {
        id : _3mfSliceCom
        Rectangle
        {
            color: "transparent"
            border.width: 1
            border.color: "blue"
            ColumnLayout
            {
                spacing: 10

                RowLayout
                {
                    spacing: 10
                    StyledLabel {
                        text: qsTr("3mf 路径")
                        color: Constants.right_panel_text_default_color
                        font.pointSize: Constants.labelFontPointSize_9
                        wrapMode: Text.NoWrap
                        font.family: Constants.labelFontFamily
                    }
                    CusFolderField
                    {
                        id : __3mfPath
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
                            console.log("aaaaa =",__3mfPath.text)
                            fileDialog.itemObj = __3mfPath
                            fileDialog.folder = "file:///"+ (__3mfPath.text)
                            fileDialog.visible = true
                        }
                    }
                }
                RowLayout
                {
                    spacing: 10
                    StyledLabel {
                        text: qsTr("BL 路径")
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
                            console.log("aaaaa")
                            fileDialog.itemObj = __blpath
                            fileDialog.folder = "file:///"+ (__blpath.text)
                            fileDialog.visible = true
                        }

                    }
                }
                RowLayout
                {
                    spacing: 10
                    StyledLabel {
                        text: qsTr("Compare 结果路径")
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
                            console.log("aaaaa")
                            fileDialog.itemObj = __compare
                            fileDialog.folder = "file:///"+ (__compare.text)
                            fileDialog.visible = true
                        }

                    }
                }

                Item
                {
                    Layout.fillHeight: true
                }

                RowLayout
                {
                    spacing: 10
                    Layout.preferredHeight:36
                    CusButton{
                        Layout.preferredWidth: 140 * screenScaleFactor
                        Layout.preferredHeight: 36 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "比较测试"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        onClicked:{
                            gTestTool.doTest(1)
                        }
                    }
                    CusButton{
                        Layout.preferredWidth: 120 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "生成BL"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        onClicked:{
                            gTestTool.doTest(0)
                        }
                    }
                    CusButton{
                        Layout.preferredWidth: 120 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        radius: 5 * screenScaleFactor
                        txtContent: "更新BL"
                        txtPointSize: Constants.labelFontPointSize_9
                        txtColor: "#ffffff"
                        onClicked:{
                            gTestTool.doTest(2)
                        }
                    }
                }


                TextArea{
                    id: idTextArea
                    Layout.minimumHeight: 450
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    wrapMode: TextEdit.WordWrap
                    color: "white"
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_10
                    text : gTestTool.message
                    selectByMouse: true
                    background: Rectangle
                    {
                        anchors.fill : parent
                        color: Constants.tooltipBgColor
                        border.width:1
                    }
                }
            }

        }
    }

    Component
    {
        id : _xxxCom
        Rectangle
        {
            color: "transparent"
            border.width: 1
            border.color: "blue"
        }
    }

    File.FileDialog {
        id: fileDialog
        title: "Please choose path"
        property var itemObj: null
        selectFolder : true
        onAccepted: {
            console.log("You chose folder: " + fileDialog.folder.toString().replace("file:///",""))
            itemObj.callbackText(fileDialog.folder.toString().replace("file:///",""))
        }
        onRejected: {
            console.log("FileDialog Canceled")
            visible = false
        }
        Component.onCompleted: visible = false
    }

}
