import CrealityUI 1.0
import Qt.labs.platform 1.1 as MacMenu
import Qt.labs.platform 1.0
import Qt.labs.settings 1.1
import QtQml 2.13
import QtQuick 2.13
import QtQuick.Controls 2.0
import QtQuick.Controls 2.5 as QQC25
import QtQuick.Layouts 1.3
import QtQuick.Scene3D 2.13
import QtQuick.Window 2.2
import com.cxsw.SceneEditor3D 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/cxcloud"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/secondqml/frameless"
import "qrc:/CrealityUI/secondqml/printer"

Item {
    id: root

    // property int sliceType: 0 // 0:单盘/所有盘可用。 1: 单盘可用。 2:  所有盘可用。 3：禁用所有
    property int sliceType: {
        if (kernel_slice_flow.isPreviewGCodeFile) {
            return 3
        } else {
            return (kernel_slice_flow.isSlicing ||
                    kernel_printer_manager.selectedPrinter.dirty &&
                    !standaloneWindow.isErrorExist &&
                    kernel_printer_manager.selectedPrinter.modelsInsideCount > 0) ? 0 : 2
        }
    }
    
    

    signal usbButtonClicked()
    signal wifiButtonClicked()
    signal uploadButtonClicked()
    signal exportButtonClicked()

    function startSlice() {
        //        idMachineManager.item.close();
        if (kernel_kernel.currentPhase === 0 || kernel_kernel.currentPhase === 2) {
            //添加设置colorList的接口
            topToolBar.switchPickMode();
            let colorList = kernel_parameter_manager.currentMachineObject.extrudersModel.colorList();
            kernel_slice_model.setNozzleColorList(colorList);
            kernel_slice_flow.setStartAction(0);
            kernel_kernel.setKernelPhase(1);
            // kernel_slice_flow.slice(kernel_slice_flow.sliceIndex);
        } else if (kernel_kernel.currentPhase === 1) {
            topToolBar.switchPickMode();
            kernel_slice_flow.slice(kernel_slice_flow.sliceIndex);
        }
    }

    visible: kernel_kernel.currentPhase !== 2

    Rectangle {
        id: btnGroupBox

        property bool setEnble: !parent.sliceType || parent.sliceType === selDisk.type
        property bool showExtraButtons: false

        signal usbButtonClicked()
        signal wifiButtonClicked()
        signal uploadButtonClicked()
        signal exportButtonClicked()

        width: 216 * screenScaleFactor
        height: contentColumn.height + 2 * contentColumn.spacing
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70 * screenScaleFactor
        color: Constants.currentTheme ? Qt.rgba(0, 0, 0, 0.08) : btnGroupBox.showExtraButtons ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(0, 0, 0, 0.1)
        border.width: 0
        radius: 10 * screenScaleFactor


        Column {
            id: contentColumn

            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10 * screenScaleFactor
            // anchors.horizontalCenter: parent.horizontalCenter

            Popup{
                width: 216 * screenScaleFactor
                id: diskMenu
                visible: btnGroupBox.showExtraButtons
                height:100 * screenScaleFactor
                y:-110 * screenScaleFactor
                x:-170* screenScaleFactor
                parent:arrowBtnArea
                closePolicy:Popup.CloseOnReleaseOutsideParent
                background: CusRoundedBg {
                    color:  Constants.currentTheme ? Qt.rgba(0, 0, 0, 0.08) : btnGroupBox.showExtraButtons ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(0, 0, 0, 0.1)
                    radius:10 * screenScaleFactor
                    allRadius:true
                    borderWidth:0
                }
                
                contentItem:Column {
                    state: "single"
                    states: [
                        State {
                            name: "single"

                            PropertyChanges {
                                target: selDisk
                                selText: singleDisk.text
                                type: 1
                            }

                        },
                        State {
                            name: "multi"

                            PropertyChanges {
                                target: selDisk
                                selText: multiDisk.text
                                type: 2
                            }

                        }
                    ]

                    CusRoundedBg {
                        property bool isHover: false
                        width: 150* screenScaleFactor
                        height: 36 * screenScaleFactor
                        leftTop:true
                        rightTop:true
                        radius: 10* screenScaleFactor
                        color: isHover ? Constants.themeGreenColor : Constants.currentTheme ? "#ffffff" : "#59595D"

                        Text {
                            id: singleDisk

                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.family: Constants.labelFontFamily
                            font.weight: Font.ExtraBold
                            font.pointSize: Constants.labelFontPointSize_10
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Slice Plate")
                            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                        }

                        MouseArea {
                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                // diskMenu.state = "single";
                                selDisk.selText = singleDisk.text
                                selDisk.type = 1

                                diskMenu.visible = false
                            }
                            onExited: parent.isHover = false
                            onEntered: parent.isHover = true
                        }

                    }

                    CusRoundedBg {
                        property bool isHover: false
                        width: 150* screenScaleFactor
                        height: 36 * screenScaleFactor
                        radius:10 * screenScaleFactor
                        rightBottom:true
                        leftBottom:true
                        color: isHover ? Constants.themeGreenColor : Constants.currentTheme ? "#ffffff" : "#59595D"
                        Text {
                            id: multiDisk

                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                            font.family: Constants.labelFontFamily
                            font.weight: Font.ExtraBold
                            font.pointSize: Constants.labelFontPointSize_10
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Slice all")
                            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                        }

                        MouseArea {
                            hoverEnabled: true
                            anchors.fill: parent
                            cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                // diskMenu.state = "multi";
                                selDisk.selText = multiDisk.text
                                selDisk.type = 2
                                diskMenu.visible = false
                            }
                            onExited: parent.isHover = false
                            onEntered: parent.isHover = true
                        }

                    }

                }
            }

            Rectangle {
                id: selDisk

                property string selText: ""
                property int type: 1

                width: 150 * screenScaleFactor
                height: 36 * screenScaleFactor
                color: btnGroupBox.setEnble ? Constants.themeGreenColor : Constants.currentTheme ? "#ffffff" : "#59595D"
                // border.color: Constants.currentTheme ? "#CECECF" : "transparent"
                border.width: 0
                radius: 5

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Font.ExtraBold
                    font.pointSize: Constants.labelFontPointSize_10
                    anchors.horizontalCenter: parent.horizontalCenter
                    verticalAlignment: Text.AlignVCenter
                    text: parent.selText
                    color: Constants.currentTheme ? btnGroupBox.setEnble ? "#FFFFFF" : "#333333" : "#FFFFFF"
                }

                MouseArea {
                    id: uploadBtnArea

                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!btnGroupBox.setEnble)
                            return ;

                        if (parent.type === 1) {
                            startSlice();
                            dock_context.closeAllItem();
                        } else {
                            if (kernel_kernel.currentPhase === 1) {
                                kernel_slice_flow.sliceAll();
                            } else {
                                kernel_slice_flow.setStartAction(1);
                                kernel_kernel.setKernelPhase(1);
                            }
                            //
                        }
                    }
                }

            }

        }

        Rectangle {
            width: 36 * screenScaleFactor
            height: 36 * screenScaleFactor
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10 * screenScaleFactor
            anchors.bottomMargin: 10 * screenScaleFactor
            color: arrowBtnArea.containsMouse ? Constants.currentTheme ? "#F2F2F5" : "#6E6E73" : Constants.currentTheme ? "#FFFFFF" : "#59595D"
            border.color: Constants.currentTheme ? "#CECECF" : "transparent"
            border.width: 0
            radius: 5

            Image {
                property string box_upArrow: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/fold_light_default.svg" : "qrc:/UI/photo/rightDrawer/box_upArrow_dark.svg"
                property string box_downArrow: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/expand_light_default.svg" : "qrc:/UI/photo/rightDrawer/box_downArrow_dark.svg"

                anchors.centerIn: parent
                source: diskMenu.visible ? box_downArrow : box_upArrow
            }

            MouseArea {
                id: arrowBtnArea

                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: diskMenu.visible = !diskMenu.visible
            }

        }

    }

    Rectangle {
        id: printMenu

        property bool showExtraButtons: false
        // property bool setEnble: kernel_slice_flow.sliceAttain && !kernel_printer_manager.selectedPrinter.dirty
        property bool setEnble: kernel_slice_flow.isPreviewGCodeFile || kernel_printer_manager.selectedPrinter.isSliced
        property string selText: printData.get(0).textId
        property int selType: printData.get(0).type

        signal usbButtonClicked()
        signal wifiButtonClicked()
        signal uploadButtonClicked()
        signal exportButtonClicked()

        width: 216 * screenScaleFactor
        height: printContentColumn.height + 2 * printContentColumn.spacing
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10 * screenScaleFactor
        color: Constants.currentTheme ? Qt.rgba(0, 0, 0, 0.08) : printMenu.showExtraButtons ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(0, 0, 0, 0.1)
        border.width: 0
        radius: 10 * screenScaleFactor

        ListModel {
            id: printData

            ListElement {
                textId: qsTr("LAN Printing")
                type: 0
            }

            ListElement {
                textId: qsTr("Export to Local")
                type: 1
            }

            ListElement {
                textId: qsTr("Upload to Crealitycloud")
                type: 2
            }

        }

        Column {
            id: printContentColumn

            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10 * screenScaleFactor
            
            Popup{
                id: printState
                width: 216 * screenScaleFactor
                visible: printMenu.showExtraButtons
                parent:printArrowBtnArea
                closePolicy:Popup.CloseOnReleaseOutsideParent
                height:130 * screenScaleFactor
                y:-140 * screenScaleFactor
                x:-170* screenScaleFactor
                background: CusRoundedBg {
                    color:  Constants.currentTheme ? Qt.rgba(0, 0, 0, 0.08) : btnGroupBox.showExtraButtons ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(0, 0, 0, 0.1)
                    radius:10 * screenScaleFactor
                    allRadius:true
                    borderWidth:0
                }
                contentItem:Column {
                    Repeater {
                        model: printData

                        delegate: CusRoundedBg {
                            property bool isHover: false
                            leftTop:model.index === 0
                            rightTop:model.index ===0
                            leftBottom:model.index ===2// printData.count() - 1
                            rightBottom:model.index === 2//printData.count() - 1
                            width: 150 * screenScaleFactor
                            height: 36 * screenScaleFactor
                            radius:10 * screenScaleFactor
                            borderWidth:0
                            //color: printMenu.selType === type ? Constants.themeGreenColor : Constants.currentTheme ? "#ffffff" : "#59595D"
                            color: isHover ? Constants.themeGreenColor : Constants.currentTheme ? "#ffffff" : "#59595D"

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                                font.family: Constants.labelFontFamily
                                font.weight: Font.ExtraBold
                                font.pointSize: Constants.labelFontPointSize_10
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                text: model.textId
                                color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                            }

                            MouseArea {
                                hoverEnabled: true
                                anchors.fill: parent
                                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    printMenu.selText = model.textId;
                                    printMenu.selType = model.type;
                                    printMenu.showExtraButtons = false;
                                    printState.visible  = false
                                }
                                onExited: parent.isHover = false
                                onEntered: parent.isHover = true
                            }

                        }

                    }

                }

            }

            Rectangle {
                id: selPrint

                property string selText: printMenu.selText
                property int type: printMenu.selType

                width: 150 * screenScaleFactor
                height: 36 * screenScaleFactor
                color: printMenu.setEnble ? Constants.themeGreenColor : Constants.currentTheme ? "#ffffff" : "#59595D"
                border.color: Constants.currentTheme ? "#CECECF" : "transparent"
                border.width: 0
                radius: 5

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    font.family: Constants.labelFontFamily
                    font.weight: Font.ExtraBold
                    font.pointSize: Constants.labelFontPointSize_10
                    anchors.horizontalCenter: parent.horizontalCenter
                    verticalAlignment: Text.AlignVCenter
                    text: parent.selText
                    color: Constants.currentTheme ? printMenu.setEnble ? "#FFFFFF" : "#333333" : "#FFFFFF"
                }

                MouseArea {
                    id: uploadBtnArea1

                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!printMenu.setEnble)
                            return ;

                        switch (parent.type) {
                        case 0:
                            wifiButtonClicked();
                            break;
                        case 1:
                            exportButtonClicked();
                            break;
                        case 2:
                            uploadButtonClicked();
                            break;
                        }
                    }
                }

            }

        }

        Rectangle {
            width: 36 * screenScaleFactor
            height: 36 * screenScaleFactor
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10 * screenScaleFactor
            anchors.bottomMargin: 10 * screenScaleFactor
            color: printArrowBtnArea.containsMouse ? Constants.currentTheme ? "#F2F2F5" : "#6E6E73" : Constants.currentTheme ? "#FFFFFF" : "#59595D"
            border.color: Constants.currentTheme ? "#CECECF" : "transparent"
            border.width: 0
            radius: 5

            Image {
                property string box_upArrow: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/fold_light_default.svg" : "qrc:/UI/photo/rightDrawer/box_upArrow_dark.svg"
                property string box_downArrow: Constants.currentTheme ? "qrc:/UI/photo/rightDrawer/update/expand_light_default.svg" : "qrc:/UI/photo/rightDrawer/box_downArrow_dark.svg"

                anchors.centerIn: parent
                source: printState.visible ? box_downArrow : box_upArrow
            }

            MouseArea {
                id: printArrowBtnArea

                hoverEnabled: true
                anchors.fill: parent
                cursorShape: containsMouse ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: printState.visible = !printState.visible
            }

        }

    }

}
