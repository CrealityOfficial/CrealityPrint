import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

import "../qml"
import "../secondqml"

Rectangle
{
    id: root
    color: Constants.leftBarBgColor
    width: (48 + 2) * screenScaleFactor
    height: (10 + 2) * screenScaleFactor + button_background.height
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

    layer.enabled: Constants.currentTheme

    layer.effect: DropShadow {
        radius: 3
        spread: 0
        samples: 7
        color: Constants.dropShadowColor
        verticalOffset: 3 * screenScaleFactor
    }

    onVisibleChanged: if(!visible) focus = true

    MouseArea {
        focus: true
        anchors.fill: parent
    }

    function switchMoveMode()
    {
        var item = listView.itemAtIndex(0)
        if(selectItem === item)return
        if(!item.bottonSelected)
            item.clicked()
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
        var item = listView.itemAtIndex(1)
        if(selectItem === item) return
        if(!item.bottonSelected) item.clicked()
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

        Item
        {
            Layout.preferredWidth: 48 * screenScaleFactor
            Layout.preferredHeight: 10 * screenScaleFactor

            Image {
                anchors.centerIn: parent
                width: 19 *screenScaleFactor
                height: 4 * screenScaleFactor
                source: "qrc:/UI/photo/leftBar/horizontalDragBtn.svg"
            }
        }

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

                    interactive: false
                    spacing: 1 * screenScaleFactor

                    GlobalShortcut{
                        funcItem: root.enabled && !kernel_kernel.currentPhase ? root : null
                        shortcutType: 0
                    }

                    Connections {//模型数量更改的连接信号
                        target: kernel_modelspace

                        onSigAddModel:{
                            //如果添加的是第一个模型才执行
                            let beginIndex = 0
                            if(kernel_modelspace.modelNNum === 1)
                            {
                                //1.默认选中添加的模型
                                kernel_model_list_data.checkOne(kernel_modelspace.modelNNum - 1)
                                //2.默认回归到索引为0（移动按钮）的按钮
                                listView.currentIndex = beginIndex
                                //3.调用execute
                                listView.model.get(beginIndex).saveCall()
                                //4.设置Loader的页面url
                                content.source = listView.model.get(beginIndex).source
                                //5.传递c++对象到qml用于交互
                                content.item.com = listView.model.get(beginIndex)
                            }
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
                        bottonSelected: listView.currentIndex === model.index
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
                        highLightIconSource: hoveredIcon
                        defaultBtnBgColor: "transparent"
                        hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
                        selectedBtnBgColor: Constants.leftBtnBgColor_selected
                        btnTextColor: Constants.leftTextColor
                        btnTextColor_d : Constants.leftTextColor_d
                        property var tindex
                        property var bitem: item
                        //                        visible: model.index !== 0
                        opacity: enabled ? 1 : 0.5
                        onClicked:
                        {
                            otherOpt.curIndex = -1

                            if(listView.currentIndex === model.index)
                            {
                                switchMoveMode()
                                return
                            }

                            //1.切换当前索引
                            listView.currentIndex = model.index

                            //2.初始化item
                            item.execute()

                            //3.模型变量赋值
                            if(model.index === 5)
                            {
                                //支撑按钮
                                supportUICpp.execute()
                                content.sourceComponent = supportCom
                            }
                            else {
                                content.source = source
                                content.item.com = item
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
                    opacity: enabled ? 1 : 0.5
                    
                    imgWidth: 24 * screenScaleFactor
                    imgHeight: 24 * screenScaleFactor
                    borderWidth: 0

                    state: "imgOnly"
                    text: qsTr("Others")
                    cusTooltip.position: BasicTooltip.Position.RIGHT

                    enabledIconSource: Constants.leftbar_other_btn_icon_default
                    pressedIconSource: Constants.leftbar_other_btn_icon_checked
                    highLightIconSource: Constants.leftbar_other_btn_icon_hovered

                    defaultBtnBgColor: "transparent"
                    hoveredBtnBgColor: Constants.leftBtnBgColor_hovered
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

        onClosed: otherBtn.checked = false

        onClicked: function(index, com) {
            if (index == -1) {
                // index = 0
                // com = listView.itemAtIndex(0)
                // listView.itemAtIndex(0).clicked()
                // listView.itemAtIndex(0).bottonSelected = true
                switchMoveMode()
                return
            }

            listView.currentIndex = -1
            clickFunction(otherModel.get(index).enabledIcon,
                          otherModel.get(index).source,
                          otherModel.get(index),
                          index,
                          com)
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
        anchors.topMargin: listView.currentIndex !== 3 ? 0
                                                       : listView.itemAtIndex(3).mapToGlobal(0, -root.topLeftButton.y).y
        anchors.left: root.right
        anchors.leftMargin: 10 * screenScaleFactor

        visible: kernel_model_selector.selectedNum > 0 && root.visible

        onLoaded: {
            if(selectItem && selectItem.bitem && content.item.com)
            {
                content.item.com = selectItem.bitem
            }
        }

        onVisibleChanged: {
            console.log("+++++++onVisibleChanged = ", visible)
        }

        onYChanged: {
            console.log("+++++++Y = ", y)
        }

        //        MouseArea{
        //            anchors.fill: parent
        //        }
    }

    function clickFunction(enabledIcon, source, item, index, thisItem)
    {
        Constants.leftShowType = 0
        //支撑按钮
        if(index === (9-3))
        {
            supportUICpp.execute()
            content.sourceComponent = supportCom
        }

        item.execute()
        if(selectItem) selectItem.bottonSelected = false

        selectItem = thisItem
        selectItem.bottonSelected = true

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
