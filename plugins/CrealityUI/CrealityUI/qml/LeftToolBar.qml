import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Rectangle
{
    id: root
    color: Constants.leftBtnBgColor_normal
    border.width: 1 * screenScaleFactor
    border.color: Constants.leftbar_btn_border_color
    width: 50 * screenScaleFactor
    height:  button_background.height+2
    radius: 5 * screenScaleFactor

    property var topLeftButton: null

    property var selectItem: ""

    property int count: 0
    property int btnIndex: 0

    property bool showPop: true

    property alias mymodel: listView.model
    property alias leftLoader: content
    property alias otherModel: otherOpt.repeaterModel

    property string noSelect: ""
    property string currentSource: ""
    property string currentEnableIcon: "" //用于后面判断

//    layer.enabled: Constants.currentTheme

//    layer.effect: DropShadow {
//        radius: 3
//        spread: 0
//        samples: 7
//        color: Constants.dropShadowColor
//        verticalOffset: 3 * screenScaleFactor
//    }

    onVisibleChanged: if(!visible) focus = true

    MouseArea {
        focus: true
        anchors.fill: parent
    }

    function switchMoveMode()
    {
        switchMode(0)
    }


    function switchMode(btnIndex)
    {
        var item = listView.itemAtIndex(btnIndex)
        if(selectItem === item)return
        if(!item.bottonSelected)
            item.clicked()
    }
    //c++调用
    function switchPickMode()
    {
        switchMode(0)
    }

    //c++调用
    function pButtomBtnClick()
    {
        var item = listView.itemAtIndex(5)
        if(!item.bottonSelected) item.clicked()
    }

    ColumnLayout {
        spacing: 0
        anchors.centerIn: parent

//        Item
//        {
//            Layout.preferredWidth: 48 * screenScaleFactor
//            Layout.preferredHeight: 10 * screenScaleFactor

//            Image {
//                anchors.centerIn: parent
//                width: 19 *screenScaleFactor
//                height: 4 * screenScaleFactor
//                source: "qrc:/UI/photo/leftBar/horizontalDragBtn.svg"
//            }
//        }

        Rectangle
        {
            id: button_background
            color: Constants.themeColor
            radius: 5 * screenScaleFactor

            Layout.preferredWidth: 48 * screenScaleFactor
            Layout.preferredHeight: childrenRect.height

            ColumnLayout {
                spacing: 1 * screenScaleFactor

                ListView {
                    id: listView
                    Layout.preferredWidth: 48 * screenScaleFactor
                    Layout.preferredHeight: contentHeight
                    property bool haveOpt: false

                    interactive: false
                    spacing: 1 * screenScaleFactor

                    GlobalShortcut{
                        funcItem: root.enabled && !kernel_kernel.currentPhase ? root : null
                        shortcutType: 0
                    }

                    Connections {//模型数量更改的连接信号
                        target: kernel_modelspace

                        onSigAddModel:{

                        }

                        onSigRemoveModel:{
                            if(kernel_modelspace.modelNNum === 0){
                                //1.弹窗置空
                                content.source = ""
                                otherBtn.checked = false
                                //2.当前选项清空
                                listView.currentIndex = -1
                            }
                        }
                    }

                    delegate: CusImglButton {
                        width: 48 * screenScaleFactor
                        height: 48 * screenScaleFactor//PickMode unavailable
                        imgWidth: 24 * screenScaleFactor
                        imgHeight: 24 * screenScaleFactor
                        borderWidth: 0
                        state: "imgOnly"
                        text: name
                        focus: true
                        cusTooltip.position: BasicTooltip.Position.RIGHT
                        enabledIconSource: enabledIcon
                        pressedIconSource: pressedIcon
                        highLightIconSource: pressedIcon
                        defaultBtnBgColor: "transparent"
                        hoveredBtnBgColor: Constants.leftBtnBgColor_selected
                        selectedBtnBgColor: Constants.leftBtnBgColor_selected
                        btnTextColor: Constants.leftTextColor
                        btnTextColor_d : Constants.leftTextColor_d
                        property var tindex
                        property var bitem: item
                        //                        visible: model.index !== 0
                        opacity: 1
                        onClicked:
                        {
                            if (kernel_kernelui.commandIndex == model.index) {
                                kernel_kernelui.commandIndex = -1
                            }
                            else {
                                kernel_kernelui.commandIndex = model.index
                                listView.currentIndex = model.index

                                if (bottonSelected == false) {
                                    bottonSelected = true
                                    //初始化item
                                    item.execute()

                                    content.source = source
                                    content.item.com = item

                                    if(model.index === 5)
                                    {   // 支撑
                                        content.item.execute()
                                    }
                                    listView.haveOpt = true
                                }
                            }
                        }

                        Connections {
                            target: kernel_kernelui

                            onCommandIndexChanged: {
                                if (model.index == kernel_kernelui.commandIndex) {

                                } else {
                                    if (bottonSelected && kernel_kernelui.commandIndex == -1)
                                    {
                                        listView.haveOpt = false
                                    }
                                    bottonSelected = false
                                }
                            }

                        }
                    }
                }

                CusImglButton {
                    id: otherBtn
                    Layout.preferredWidth: 48 * screenScaleFactor
                    Layout.preferredHeight: 48 * screenScaleFactor

                    checkable: true
                    checked: false
                    bottonSelected: checked
                    opacity:1
                    
                    imgWidth: 24 * screenScaleFactor
                    imgHeight: 24 * screenScaleFactor
                    borderWidth: 0

                    state: "imgOnly"
                    text: qsTr("Others")
                    cusTooltip.position: BasicTooltip.Position.RIGHT

                    enabledIconSource: Constants.leftbar_other_btn_icon_default
                    pressedIconSource: Constants.leftbar_other_btn_icon_checked
                    highLightIconSource: Constants.leftbar_other_btn_icon_checked

                    defaultBtnBgColor: "transparent"
                    hoveredBtnBgColor: Constants.leftBtnBgColor_selected
                    selectedBtnBgColor: Constants.leftBtnBgColor_selected

                    btnTextColor: Constants.leftTextColor
                    btnTextColor_d : Constants.leftTextColor_d
                }
            }
        }
    }

    OtherOptPanel {
        id: otherOpt

        // parent(root) 的 DropShadow 导致 child(otherOpt, content) 无法显示,
        // 解除 otherOpt, content 与 root 的父子对象关系以解决这个问题.
        parent: root.parent

        x: content.x
        y: root.y + button_background.y + otherBtn.y

        buttonEnabled: root.enabled
        visible: otherBtn.checked && root.visible
        bindBtn: otherBtn
        closeButtonVisible: true
        loader: content

        onClosed: otherBtn.checked = false

        onClicked: function(index) {
            kernel_kernelui.otherCommandIndex = index
            clickFunction(otherModel.get(index).enabledIcon,
                        otherModel.get(index).source,
                        otherModel.get(index),
                        index)
        }
    }

    Component {
        id: supportCom
        CusSupportPanel {
            onVisibleChanged: if(!visible) content.sourceComponent = null
        }
    }

    Loader {
        id: content

        parent: root.parent

        anchors.top: root.topLeftButton.top
        anchors.topMargin: kernel_kernelui.commandIndex !== 3 ? 0
                                                       : parent.mapFromGlobal(listView.itemAtIndex(3).mapToGlobal(0, -root.topLeftButton.y)).y
        anchors.left: root.right
        anchors.leftMargin: 10 * screenScaleFactor

        // visible: kernel_model_selector.selectedNum > 0 && root.visible
        visible: listView.haveOpt || otherOpt.haveOpt

        onLoaded: {
            if(selectItem && selectItem.bitem && content.item.com)
            {
                content.item.com = selectItem.bitem
            }
        }

        onVisibleChanged: {
        }

        onYChanged: {
        }

    }

    function clickFunction(enabledIcon, source, item, index)
    {
        Constants.leftShowType = 0

        item.execute()

        content.source = item.checkSelect() ? source : noSelect
        currentSource = source
        currentEnableIcon = enabledIcon

        if(source.length > 0)
        {
            content.item.com = item
            content.item.execute()
        }
    }
}
