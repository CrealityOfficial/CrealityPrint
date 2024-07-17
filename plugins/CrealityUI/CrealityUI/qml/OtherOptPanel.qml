import QtQuick 2.10
import QtQuick.Controls 2.13
import QtGraphicalEffects 1.12
import QtQuick.Layouts 1.13
import "../secondqml"

LeftPanelDialog {
    property int curIndex: -1 //其他按钮页面的当前选择索引
    property alias repeater: repeaterItem
    property alias repeaterModel: repeaterItem.model
    property var selectItem
    property bool buttonEnabled: true
    property var bindBtn //点击之后弹窗的btn
    property var loader
    property bool haveOpt: false

    id:control

    width: 332 *screenScaleFactor 
    height: 180 *screenScaleFactor
    // width: 272 *screenScaleFactor
    // height: 180 *screenScaleFactor
    title : qsTr("Others")

    onCloseButtonClicked: {
        closed()
    }

    layer.enabled: true
    layer.effect:
        DropShadow {
        verticalOffset: 2
        //        samples: 5
        color:Constants.dropShadowColor
    }

    signal clicked(int index)
    signal closed()

    function switchMode(btnIndex)
    {
        var item = repeaterItem.itemAt(btnIndex)
        if(control.selectItem === item)return
        if(!item.bottonSelected)
            item.clicked()
    }

    GlobalShortcut{
        funcItem: buttonEnabled && !kernel_kernel.currentPhase ? control : null
        shortcutType: 1
    }

    Flow {
        anchors.fill: control.panelArea
        anchors.margins: 20 * screenScaleFactor
        spacing: 10 * screenScaleFactor
        Repeater{
            id:repeaterItem
            delegate: CusImglButton{
                id:otherBtn
                
                width: 50*screenScaleFactor
                height: 50*screenScaleFactor
                imgWidth : 24*screenScaleFactor
                imgHeight : 24*screenScaleFactor

                enabled: control.buttonEnabled

                text: name
                allowTooltip : true
                cusTooltip.text: name
                btnTextColor : Constants.leftTextColor
                btnTextColor_d : Constants.leftTextColor_d

                enabledIconSource:enabledIcon
                pressedIconSource:pressedIcon
                highLightIconSource:pressedIcon

                borderColor: Constants.leftbar_btn_border_color
                hoverBorderColor: Constants.leftbar_btn_border_color
                selectBorderColor: "transparent"

                defaultBtnBgColor: "transparent"
                hoveredBtnBgColor: Constants.leftBtnBgColor_selected
                selectedBtnBgColor: Constants.leftBtnBgColor_selected

                // bottonSelected: kernel_kernelui.otherCommandIndex == index

                property var commandIndex: index


                Connections {
                    target: kernel_kernelui

                    onOtherCommandIndexChanged: {
                        if (index == kernel_kernelui.otherCommandIndex) {
                            if (bottonSelected == false) {
                                bottonSelected = true
                                control.clicked(index)
                                haveOpt = true
                            }
                        } else {
                            if (bottonSelected == true && kernel_kernelui.otherCommandIndex == -1) {
                                haveOpt = false
                            }
                            bottonSelected = false
                        }

                    }
                }


                onClicked: {
                    if (kernel_kernelui.otherCommandIndex == index)
                        kernel_kernelui.otherCommandIndex = -1
                    else 
                        kernel_kernelui.otherCommandIndex = index
                }
            }
        }
    }
}
