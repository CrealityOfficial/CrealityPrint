import QtGraphicalEffects 1.12
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

Rectangle {
    id: root

    property var topLeftButton: null
    property var selectItem: ""
    property int count: 0
    property int btnIndex: 0
    property bool showPop: true
    property alias mainmodel: mainlistView.model
    property alias othermodel: otherlistView.model
    property alias drawmodel: drawlistView.model
    property alias leftLoader: content
    property string noSelect: ""
    property string currentSource: ""
    property string currentEnableIcon: "" //用于后面判断
    property string dividing_line: Constants.currentTheme ? "#AAAAB0" : "#454548"
    property bool panelVisible: false
    property bool printerSelected: false
    property bool machineTabClose: false
    property var toolSelect: ""

    enum DrawListIndex
    {
        Item_Support =0,
        Item_Z,
        Item_Adaptive,
        Item_Color,
        Item_Relief

    }

    function setOperatePanelVisible(vis) {
        panelVisible = vis;
    }

    function checkDrawItemStatus()
    {
        console.log("checkDrawItemStatus")
        if (kernel_parameter_manager.curExtruderCount() > 1)
            drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color).visible = true;
        else
            drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color).visible = false;
    }

    function switchModeById(btnIndex) {
        let listView;
        let offset = 0;
        if (btnIndex < 10) {
            listView = mainlistView;
            offset = 0;
        } else if (btnIndex >= 10 && btnIndex < 30) {
            listView = otherlistView;
            offset = 10;
        } else {
            listView = drawlistView;
            offset = 30;
        }
        var item = listView.itemAt(btnIndex - offset);
        //        if (selectItem === item)
        //            return ;

        if (!item.bottonSelected)
            item.clicked();

    }

    function switchMakeAdaptiveLayer()
    {
        var item = drawlistView.itemAt(CToolBar.DrawListIndex.Item_Adaptive);
        item.clicked();

    }

    function switchMakeColor() {
        var item = drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color);
        item.clicked();

    }

    function switchMakeZ() {
        var item = drawlistView.itemAt(CToolBar.DrawListIndex.Item_Z);
        item.clicked();
    }

    function switchMakeSupport() {
        var item = drawlistView.itemAt(CToolBar.DrawListIndex.Item_Support);
        item.clicked();
    }

    //c++调用
    function switchPickMode() {
        kernel_kernelui.commandIndex = -1;
    }

    //c++调用
    function pButtomBtnClick() {
        var item = listView.itemAtIndex(5);
        if (!item.bottonSelected)
            item.clicked();
    }

    function switchToolMode(listView, btnIndex) {
        var item = listView.itemAt(btnIndex);
        //        if (selectItem === item)
        //            return ;

        if (!item.bottonSelected && item.enabled){
            item.clicked();
            toolSelect = item;
        }
    }

    height: button_background.height + 2 * screenScaleFactor
    color: Constants.currentTheme ? Qt.rgba(255, 255, 255, 0.5) : Qt.rgba(0, 0, 0, 0.5)

    GlobalShortcut {
        funcItem: !kernel_kernel.currentPhase ? root : null
        shortcutType: 0
        listView: mainlistView
    }

    GlobalShortcut {
        funcItem: !kernel_kernel.currentPhase ? root : null
        shortcutType: 1
        listView: otherlistView
    }

    Item {
        id: setEscShortcut

        Shortcut {
            context: Qt.WindowShortcut
            sequence: "ESC"
            onActivated: {
                for(var i=0;i<mainlistView.count;i++){
                    if(mainlistView.itemAt(i).bottonSelected) mainlistView.itemAt(i).clicked()
                }
                for(var i=0;i<otherlistView.count;i++){
                    if(otherlistView.itemAt(i).bottonSelected) otherlistView.itemAt(i).clicked()
                }
                for(var i=0;i<drawlistView.count;i++){
                    if(drawlistView.itemAt(i).bottonSelected) drawlistView.itemAt(i).clicked()
                }

            }
        }

    }

    onVisibleChanged: function() {
        if (!visible) {
            focus = true
        }
    }

    Component.onCompleted: function() {
        idMachineManager.open()
    }

    state: "disabled"
    states: [
        State {
            name: "disabled"
        },
        State {
            //other

            name: "selected"

            PropertyChanges {
                target: drawlistView
                enabled: true
            }

            PropertyChanges {
                target: mainlistView.itemAt(0)
                enabled: true
            }

            PropertyChanges {
                target: mainlistView.itemAt(1)
                enabled: true
            }

            PropertyChanges {
                target: mainlistView.itemAt(2)
                enabled: true
            }

            PropertyChanges {
                target: mainlistView.itemAt(3)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(0)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(1)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(2)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(3)
                enabled: kernel_model_selector.selectedObjectsNum >= 1
            }

            PropertyChanges {
                target: otherlistView.itemAt(4)
                enabled: kernel_model_selector.selectedObjectsNum >= 1
            }

            PropertyChanges {
                target: otherlistView.itemAt(5)
                enabled: kernel_model_selector.selectedObjectsNum >= 1
            }

            PropertyChanges {
                target: otherlistView.itemAt(6)
                enabled: kernel_model_selector.selectedObjectsNum >= 1
            }

            PropertyChanges {
                target: otherlistView.itemAt(7)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(8)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(9)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(10)
                enabled: true
            }
            //draw

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Support)
                enabled: kernel_model_selector.selectedObjectsNum == 1
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Z)
                enabled: kernel_model_selector.selectedObjectsNum == 1
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Adaptive)
                enabled: kernel_model_selector.selectedGroupsNum == 1
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color)
                enabled: kernel_model_selector.selectedObjectsNum == 1
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Relief)
                enabled: true
            }
        },
        State {
            //other

            name: "unselected"

            PropertyChanges {
                target: mainlistView.itemAt(2)
                enabled: true
            }

            PropertyChanges {
                target: mainlistView.itemAt(3)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(0)
                enabled: false
            }

            PropertyChanges {
                target: otherlistView.itemAt(1)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(2)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(3)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(4)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(5)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(6)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(7)
                enabled: true
            }

            PropertyChanges {
                target: otherlistView.itemAt(8)
                enabled: false
            }

            PropertyChanges {
                target: otherlistView.itemAt(9)
                enabled: false
            }

            PropertyChanges {
                target: otherlistView.itemAt(10)
                enabled: false
            }
            //draw

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Support)
                enabled: false
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Z)
                enabled: false
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Adaptive)
                enabled: false
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color)
                enabled: false
            }

            PropertyChanges {
                target: drawlistView.itemAt(CToolBar.DrawListIndex.Item_Relief)
                enabled: true
            }
        }
    ]

    MouseArea {
        focus: true
        anchors.fill: parent
    }

    ButtonGroup {
        id: button_group
        exclusive: false
    }

    RowLayout {
        id: button_background

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: spacing
        anchors.verticalCenter: parent.verticalCenter

        spacing: 10 * screenScaleFactor
        height: buttonWidth + spacing * 2

        readonly property real buttonWidth: {
            const button_count = button_group.buttons.length
            const spacing_count = button_count + 6

            const total_spacing_width = spacing * spacing_count
            const total_button_width = width - total_spacing_width

            return Math.min(total_button_width / button_count, 40 * screenScaleFactor)
        }

        readonly property real imageWidth: buttonWidth / 40 * 34

        CusImglButton {
            id: printerBtn
            property var tindex: -2
            ButtonGroup.group: button_group
            Layout.preferredWidth: button_background.buttonWidth
            Layout.preferredHeight: button_background.buttonWidth
            checkable: true
            checked: false
            bottonSelected: idMachineManager.visible /*kernel_kernelui.commandIndex === tindex && */ //kernel_kernel.isDefaultVisScene && kernel_kernel.currentPhase === 0 && kernel_parameter_manager.functionType === 0
            opacity: 1
            imgWidth: button_background.imageWidth
            imgHeight: button_background.imageWidth
            borderWidth: bottonSelected ? 0 : hovered ? 2 : 1
            state: "imgOnly"
            text: qsTr("Printer")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            enabledIconSource: Constants.currentTheme ? "qrc:/UI/photo/cToolBar/printer_light_default.svg" : "qrc:/UI/photo/cToolBar/printer_dark_default.svg"
            pressedIconSource:  "qrc:/UI/photo/cToolBar/printer_dark_press.svg"
            highLightIconSource: enabledIconSource
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            hoverBorderColor: Constants.themeGreenColor
            selectedBtnBgColor: Constants.themeGreenColor
            borderColor: Constants.currentTheme ? "#AAAAB0": "#505052"
            btnTextColor: Constants.leftTextColor
            btnTextColor_d: Constants.leftTextColor_d
            // bottonSelected: kernel_kernelui.commandIndex == tindex
            onClicked: {
                kernel_kernelui.commandIndex = -1
                kernel_kernelui.otherCommandIdex = -1
                idMachineManager.open()
                root.machineTabClose = false
            }
            Component.onCompleted: {
                kernel_kernelui.commandIndex = -1

                if (kernel_parameter_manager.curExtruderCount() > 1)
                    drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color).visible = true;
                else
                    drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color).visible = false;
            }
        }

        Rectangle{
            Layout.preferredWidth: 1 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor
            color: root.dividing_line
        }

        CusImglButton {
            ButtonGroup.group: button_group
            Layout.preferredWidth: button_background.buttonWidth
            Layout.preferredHeight: button_background.buttonWidth
            imgWidth: button_background.imageWidth
            imgHeight: button_background.imageWidth
            text: qsTr("Import Model")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: false
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            hoverBorderColor: Constants.themeGreenColor
            selectedBtnBgColor: Constants.themeGreenColor
            borderColor: Constants.currentTheme ? "#AAAAB0": "#505052"
            opacity: 1
            borderWidth: bottonSelected ? 0 : hovered ? 2 : 1
            enabledIconSource:  Constants.currentTheme ? "qrc:/UI/photo/cToolBar/openFile_light_default.svg" : "qrc:/UI/photo/cToolBar/openFile_dark_default.svg"
            pressedIconSource:enabledIconSource
            highLightIconSource: enabledIconSource
            onClicked: kernel_kernel.openFile();
        }

        CusImglButton {
            id: addPlateBtn

            ButtonGroup.group: button_group
            Layout.preferredWidth: button_background.buttonWidth
            Layout.preferredHeight: button_background.buttonWidth
            imgWidth: button_background.imageWidth
            imgHeight: button_background.imageWidth
            text: qsTr("Add Plate")
            cusTooltip.position: BasicTooltip.Position.BOTTOM
            shadowEnabled: false
            defaultBtnBgColor: "transparent"
            hoveredBtnBgColor: "transparent"
            hoverBorderColor: Constants.themeGreenColor
            selectedBtnBgColor: Constants.themeGreenColor
            borderColor: Constants.currentTheme ? "#AAAAB0": "#505052"
            opacity: 1
            borderWidth: bottonSelected ? 0 : hovered ? 2 : 1
            enabledIconSource: Constants.currentTheme ? "qrc:/UI/photo/cToolBar/addPlate_light_default.svg" : "qrc:/UI/photo/cToolBar/addPlate_dark_default.svg"
            pressedIconSource: enabledIconSource
            highLightIconSource: enabledIconSource
            onClicked: {
                kernel_kernel.addPrinter();
            }
        }

        Rectangle{
            Layout.preferredWidth: 1 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor
            color: root.dividing_line
        }

        Repeater {
            id: mainlistView

            delegate: CusImglButton {
                property var tindex: index
                property var bitem: item

                ButtonGroup.group: button_group
                Layout.preferredWidth: button_background.buttonWidth
                Layout.preferredHeight: button_background.buttonWidth
                //width: button_background.buttonWidth
                //height: button_background.buttonWidth
                imgWidth: button_background.imageWidth
                imgHeight: button_background.imageWidth
                borderWidth: bottonSelected ? 0 : hovered ? 2 : 1
                state: "imgOnly"
                text: name
                focus: true
                cusTooltip.position: BasicTooltip.Position.BOTTOM
                enabledIconSource: enabledIcon
                pressedIconSource: pressedIcon
                highLightIconSource: enabledIcon
                disabledIconSource: disabledIcon
                defaultBtnBgColor: "transparent"
                hoverBorderColor: Constants.themeGreenColor
                hoveredBtnBgColor: "transparent"
                selectedBtnBgColor: Constants.themeGreenColor
                borderColor: Constants.currentTheme ? "#AAAAB0": "#505052"
                btnTextColor: Constants.leftTextColor
                btnTextColor_d: Constants.leftTextColor_d
                bottonSelected: kernel_kernelui.commandIndex === tindex
                enabled: {
                    if (index == 2 || index == 3) {
                        return true;
                    } else {
                        return false;
                    }
                }
                //                        visible: model.index !== 0
                opacity: 1
                onClicked: {
                    root.selectItem = this
                    printerBtn.bottonSelected =  false

                    if (bottonSelected == false) {
                        //bottonSelected = true;
                        kernel_kernelui.commandIndex = index;
                        //初始化item
                        item.execute();
                        content.source = source;
                        content.item.com = item;
                        root.panelVisible = true;
                        if (source.length > 0) {
                            content.item.com = item;
                            content.item.execute();
                        }
                    } else {
                        //                                bottonSelected = false;
                        kernel_kernelui.commandIndex = -1;
                    }
                }

                Connections {
                    //bottonSelected = false;

                    function onCommandIndexChanged() {
                        if (tindex === kernel_kernelui.commandIndex) {
                        } else {
                            if (bottonSelected && kernel_kernelui.commandIndex == -1)
                                root.panelVisible = false;

                        }
                    }

                    target: kernel_kernelui
                }

            }

        }

        Rectangle{
            Layout.preferredWidth: 1 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor
            color: root.dividing_line
        }

        Repeater {
            id: otherlistView

            delegate: CusImglButton {
                property var tindex: index + 10
                property var bitem: item

                ButtonGroup.group: button_group
                Layout.preferredWidth: button_background.buttonWidth
                Layout.preferredHeight: button_background.buttonWidth
                imgWidth: button_background.imageWidth
                imgHeight: button_background.imageWidth
                borderWidth: bottonSelected ? 0 : hovered ? 2 : 1
                state: "imgOnly"
                text: name
                focus: true
                cusTooltip.position: BasicTooltip.Position.BOTTOM
                enabledIconSource: enabledIcon
                pressedIconSource: pressedIcon
                highLightIconSource: enabledIcon
                disabledIconSource: disabledIcon
                hoveredBtnBgColor: "transparent"
                defaultBtnBgColor: "transparent"
                hoverBorderColor: Constants.themeGreenColor
                selectedBtnBgColor: Constants.themeGreenColor
                borderColor: Constants.currentTheme ? "#AAAAB0": "#505052"
                btnTextColor: Constants.leftTextColor
                btnTextColor_d: Constants.leftTextColor_d
                bottonSelected: kernel_kernelui.commandIndex == tindex
                enabled: {
                    if (index == 1 || index == 2 || index == 3 ||
                        index == 4 || index == 5 || index == 6 || index == 7) {
                        return true;
                    } else {
                        return false;
                    }
                }
                //                        visible: model.index !== 0
                opacity: 1
                onClicked: {
                    root.selectItem = this
                    printerBtn.bottonSelected =  false

                    if (bottonSelected == false) {
                        //bottonSelected = true;
                        kernel_kernelui.commandIndex = index + 10;
                        //初始化item
                        item.execute();
                        content.source = source;
                        content.item.com = item;
                        root.panelVisible = true;
                        if (source.length > 0) {
                            content.item.com = item;
                            content.item.execute();
                        }
                    } else {
                        //bottonSelected = false;
                        kernel_kernelui.commandIndex = -1;
                    }
                }

                Connections {
                    //bottonSelected = false;

                    function onCommandIndexChanged() {
                        if (tindex === kernel_kernelui.commandIndex) {
                        } else {
                            if (bottonSelected && kernel_kernelui.commandIndex == -1)
                                root.panelVisible = false;

                        }
                    }

                    target: kernel_kernelui
                }
            }

            onItemAdded: {
                console.log("otherlistView add " + index);
                root.state = "disabled"
                root.state = "unselected"
            }
        }

        Rectangle{
            Layout.preferredWidth: 1 * screenScaleFactor
            Layout.preferredHeight: 20 * screenScaleFactor
            color: root.dividing_line
        }

        Repeater {
            id: drawlistView

            delegate: CusImglButton {
                property var tindex: index + 30
                property var bitem: item

                ButtonGroup.group: button_group
                Layout.preferredWidth: button_background.buttonWidth
                Layout.preferredHeight: button_background.buttonWidth
                imgWidth: button_background.imageWidth
                imgHeight: button_background.imageWidth
                borderWidth: bottonSelected ? 0 : hovered ? 2 : 1
                state: "imgOnly"
                text: name
                focus: true
                cusTooltip.position: BasicTooltip.Position.BOTTOM
                enabledIconSource: enabledIcon
                pressedIconSource: pressedIcon
                highLightIconSource: enabledIcon
                disabledIconSource: disabledIcon
                defaultBtnBgColor: "transparent"
                hoveredBtnBgColor: "transparent"
                hoverBorderColor: Constants.themeGreenColor
                selectedBtnBgColor: Constants.themeGreenColor
                btnTextColor: Constants.leftTextColor
                btnTextColor_d: Constants.leftTextColor_d
                borderColor: Constants.currentTheme ? "#AAAAB0": "#505052"
                bottonSelected: kernel_kernelui.commandIndex == tindex
                enabled: false
                opacity: 1
                onClicked: {
                    //                            idMachineManager.y = Qt.binding(function() { return root.height })
                    root.selectItem = this
                    if(model.index === CToolBar.DrawListIndex.Item_Support ||
                            model.index  === CToolBar.DrawListIndex.Item_Adaptive){
                        printerBtn.bottonSelected = false
                    }else{
                        // printerBtn.bottonSelected =  Qt.binding(function() {
                        //     return kernel_kernel.isDefaultVisScene && kernel_kernel.currentPhase === 0 &&
                        //             kernel_parameter_manager.functionType === 0})
                    }

                    if (bottonSelected == false) {
                        kernel_kernelui.commandIndex = index + 30;
                        //初始化item
                        item.execute();
                        content.source = source;
                        content.item.com = item;
                        root.panelVisible = true;
                        if (source.length > 0) {
                            content.item.com = item;
                            content.item.execute();
                        }
                    } else {
                        //bottonSelected = false;
                        kernel_kernelui.commandIndex = -1;
                    }
                }

                Connections {
                    //bottonSelected = false;

                    function onCommandIndexChanged() {
                        if (tindex === kernel_kernelui.commandIndex) {
                        } else {
                            if (bottonSelected && kernel_kernelui.commandIndex == -1)
                                root.panelVisible = false;

                            content.source = "";
                        }
                    }

                    target: kernel_kernelui
                }

                Component.onCompleted: {
                    if (model.index === CToolBar.DrawListIndex.Item_Relief) {
                        enabled = Qt.binding(_ => kernel_model_selector.selectedNum <= 1)
                    }
                }
            }

            onItemAdded: {
                root.state = "disabled"
                root.state = "unselected"
            }
        }

        Item {
            Layout.fillWidth: true
        }
    }

    Connections {
        //otherBtn.checked = false;
        //2.当前选项清空
        //drawlistView.currentIndex = -1;
        //mainlistView.currentIndex = -1;
        //otherlistView.currentIndex = -1;
        //1.弹窗置空

        target: kernel_modelspace
        onSigAddModel: {
            if (kernel_model_selector.selectedNum == 0)
                root.state = "unselected";

        }
        onSigRemoveModel: {
            if (kernel_modelspace.modelNNum === 0)
                root.state = "unselected";

        }
    }

    Connections {
        target: kernel_model_selector
        onSelectedNumChanged: {
            root.state = kernel_model_selector.selectedNum > 0 ? "selected" : "unselected"

            if (selectItem === drawlistView.itemAt(CToolBar.DrawListIndex.Item_Adaptive)) {
                if (selectItem.bottonSelected && kernel_model_selector.selectedNum != 1) {
                    selectItem.clicked()
                }
            } else if (selectItem === drawlistView.itemAt(CToolBar.DrawListIndex.Item_Relief)) {
                if (selectItem.bottonSelected && kernel_model_selector.selectedNum > 1) {
                    selectItem.clicked()
                }
            }
        }
    }

    Connections {
        target: kernel_parameter_manager
        onCurrentColorsChanged: {
            if (kernel_parameter_manager.curExtruderCount() > 1)
                drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color).visible = true;
            else
                drawlistView.itemAt(CToolBar.DrawListIndex.Item_Color).visible = false;
        }
    }

    Loader {
        id: content
        parent: c3droot
        anchors.top: parent.top
        anchors.topMargin: 60 * screenScaleFactor
        anchors.left: parent.left
        anchors.leftMargin: 10 * screenScaleFactor
        visible: root.panelVisible && kernel_kernel.currentPhase === 0
        //换料塔不需要控制这个 enbaled
        onLoaded: {
            if (selectItem && selectItem.bitem && content.item.com)
                content.item.com = selectItem.bitem;

        }

        onVisibleChanged: {
        }
        onYChanged: {
        }
    }

    CMachineTab {
        id: idMachineManager
        visible:  !root.machineTabClose && kernel_kernelui.commandIndex === -1 && kernel_kernel.currentPhase === 0
        width: 296 * screenScaleFactor
        height: 140 * screenScaleFactor
        x: 0
        y: parent.height
        onVisibleChanged: printerBtn.bottonSelected = visible
        onClosing: root.machineTabClose = true
    }
}
