import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
import "../qml"
import "../components"

BasicDialogV4 {
    id: root

    modal: false
    width: 500 * screenScaleFactor
    height: {
        200 * screenScaleFactor
    }
    titleHeight: 30 * screenScaleFactor
    title: qsTr("Test Config")
    maxBtnVis: false
    property real curType: kernel_unittest.slicerUnitType()
    onVisibleChanged: {}

    bdContentItem: ColumnLayout {
        anchors.horizontalCenter: parent
        spacing: 10 * screenScaleFactor
        Item
        {
            Layout.topMargin:  10 * screenScaleFactor
            Layout.leftMargin:  10 * screenScaleFactor
            Layout.rightMargin: 10 * screenScaleFactor
            Layout.minimumHeight:  120 * screenScaleFactor
            Layout.maximumHeight: Layout.minimumHeight
            Layout.fillWidth: true
            Column
            {
                anchors.centerIn: parent
                spacing:  10 * screenScaleFactor
                Item
                {
                    height: 50*screenScaleFactor
                    width: 450*screenScaleFactor
                    Column
                    {
                        spacing: 5 *screenScaleFactor
                        StyledLabel {
                            text: qsTr("BaseLine Cache Path")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            width: 450*screenScaleFactor
                            height: 30*screenScaleFactor
                            text:  kernel_unittest.slicerBaseLinePath
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            onClicked:
                            {
                                console.log("aaaaa")
                                fileDialog.itemObj = this
                                fileDialog.folder = "file:///"+ (text)
                                fileDialog.visible = true
                            }
                            function callbackText(path)
                            {
                                text = path
                                kernel_unittest.slicerBaseLinePath = path
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
                            text: qsTr("BaseLine Compare Error Path")
                            color: Constants.right_panel_text_default_color
                            font.pointSize: Constants.labelFontPointSize_9
                            wrapMode: Text.NoWrap
                            font.family: Constants.labelFontFamily
                        }
                        CusFolderField
                        {
                            width: 450*screenScaleFactor
                            height: 30*screenScaleFactor
                            text:  kernel_unittest.compareErrorPath
                            font.family: Constants.labelFontFamilyBold
                            font.weight: Font.Bold
                            rightPadding : image.width + 20 * screenScaleFactor
                            onClicked:
                            {
                                fileDialog.itemObj = this
                                fileDialog.folder = "file:///"+ (text)
                                fileDialog.visible = true
                            }
                            function callbackText(path)
                            {
                                text = path
                                kernel_unittest.compareErrorPath = path
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
                        text: "Ganerate SlicerBL"
                        btnSelected: curType === 1
                        onSigButtonClicked: {
                            console.log("Generate")
                            kernel_unittest.generateTest()
                            curType = 1
                        }
                    }

                    BasicButton {
                        width: 150 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        btnRadius: height / 2
                        btnBorderW: 0
                        defaultBtnBgColor: Constants.profileBtnColor
                        hoveredBtnBgColor: Constants.profileBtnHoverColor
                        text: "Compare SlicerBL"
                        btnSelected: curType === 2
                        onSigButtonClicked: {
                            console.log("Compare")
                            kernel_unittest.compareTest()
                            curType = 2
                        }
                    }

                    BasicButton {
                        width: 150 * screenScaleFactor
                        height: 28 * screenScaleFactor
                        btnRadius: height / 2
                        btnBorderW: 0
                        defaultBtnBgColor: Constants.profileBtnColor
                        hoveredBtnBgColor: Constants.profileBtnHoverColor
                        text: "Update SlicerBL"
                        btnSelected: curType === 3
                        onSigButtonClicked: {
                            console.log("Update")
                            kernel_unittest.uploadTest()
                            curType = 3
                        }
                    }
                }


            }


        }

        Item {
            Layout.fillHeight: true
        }
    }

    FileDialog {
        id: fileDialog
        title: "You chose folder"
        property var itemObj: null

        selectFolder : true
        onAccepted: {
            console.log("You chose folder: " + fileDialog.fileUrl.toString().replace("file:///",""))
            itemObj.callbackText(fileDialog.fileUrl.toString().replace("file:///",""))
        }
        onRejected: {
            console.log("FileDialog Canceled")
            visible = false
        }
        Component.onCompleted: visible = false
    }
}
