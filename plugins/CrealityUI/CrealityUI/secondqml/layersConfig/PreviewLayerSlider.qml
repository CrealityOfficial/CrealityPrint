import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "./"
import "../"
import "../../qml"
import "../../components"

Slider {
    id: control
    property color completeColor: Constants.themeGreenColor
    property color incompleteColor: Constants.currentTheme ? "#CECECF" : "#4A4A4D"

    property color handlePressColor: Qt.lighter(Constants.themeGreenColor,1.1)
    property color handleHoverColor: Qt.lighter(Constants.themeGreenColor,1.1)
    property color handleNormalColor: Constants.themeGreenColor
    property color handleBorderColor: "#00FF65" // Qt.darker(Constants.themeGreenColor,1.2)

    property bool animation: false
    property bool secondControl: true
    property alias secondVisible: second_tip.visible
    property var tagMap: new Map()
    property var locationMap: new Map()
    property var menuType: -1
    property bool sliderClick: false

    property string slider_block: "qrc:/UI/photo/slider/slider_block.svg"
    property string stop_default: "qrc:/UI/photo/slider/stop_default.svg"
    property string stop_hover: "qrc:/UI/photo/slider/stop_hover.svg"

    property bool hasSpecificExtruder: printerSettings && 
                                       printerSettings.specificExtruderHeights.length > 0 && 
                                       !kernel_slice_flow.isPreviewGCodeFile

    topPadding: 0

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            18 * screenScaleFactor + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             18 * screenScaleFactor + topPadding + bottomPadding)
    // implicitWidth: implicitBackgroundWidth + leftInset + rightInset
    // implicitHeight: implicitBackgroundHeight + topInset + bottomInset

    wheelEnabled: true
    z:99

    property var gcodeMenuContent: idPreviewSliderMenu
    property var printer: kernel_reusable_cache.printerManager.selectedPrinter
    property var printerSettings
    property var sliceAttain: kernel_slice_flow.sliceAttain ? kernel_slice_flow.sliceAttain : kernel_slice_flow.cacheSliceAttain

    onPrinterChanged: {
        printerSettings = printer ? printer.settingsObject : undefined
        // let attain = requestAttain()
        // if (attain != sliceAttain)
        //     sliceAttain = attain
    }

    // Connections {
    //     target: printer

    //     onSignalAttainChanged: {
    //         let attain = requestAttain()
    //         if (attain != sliceAttain)
    //             sliceAttain = attain
    //     }
    // }

    property bool needReloadConfig: true

    onSliceAttainChanged: {
        if (!sliceAttain) {
            return;
        } else {
            to = sliceAttain.layers()
            value = kernel_slice_model.currentLayer
            // value = to
        }

        reloadConfigTag()
    }

    onValueChanged: {
        // /* 根据stepSize校正值 */
        // let stepNum = parseInt(value / stepSize)
        // let correctedValue = stepNum * stepSize
        // if (correctedValue != value) {
        //     value = correctedValue
        // }

        checkIfEnableDeleteTag()
    }

    function requestAttain() {
        let attain = kernel_slice_flow.sliceAttain
        // if (printer) {
        //     attain = printer.attain
        //     if (!attain)
        //         attain = printer.sliceCache
        // } 
        return attain
    }

    function reloadConfigTag() {
        clearMap()

        if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
            return;

        printerSettings.reload();

        console.log("reloadConfigTag")
        let tempList = printerSettings.pauseLayers
        for (let height of tempList) {
            let layer = printerSettings.heightLayer(height)
            if (layer == -1)
                continue;

            printerSettings.correctLayer(height, layer)

            let tagY = layerLocation(layer)
            if (tagY < control.topPadding) {
                continue;
            }
                
            // console.log("pause: layer-" + layer + ", height-" + height + ", tagY-" + tagY)   
            let image = idConfigTag.createObject(control)
            image.type = "pause"
            image.y = tagY
            updateImageMap(layer, image);
        }

        tempList = printerSettings.customGCodeLayers
        for (let height of tempList) {
            let layer = printerSettings.heightLayer(height)
            if (layer == -1)
                continue;
                
            printerSettings.correctLayer(height, layer)

            let tagY = layerLocation(layer)
            if (tagY < control.topPadding) {
                continue;
            }
                
            // console.log("gcode: layer-" + layer + ", height-" + height + ", tagY-" + tagY)    
            let image = idConfigTag.createObject(control)
            image.tips = qsTr("Custom G-code") + ':"' + printerSettings.customGCode(layer) + '"'
            image.type = "gcode"
            image.y = tagY
            updateImageMap(layer, image);
        }

        tempList = printerSettings.specificExtruderHeights
        for (let height of tempList) {
            let layer = printerSettings.heightLayer(height)
            if (layer == -1)
                continue;
                
            printerSettings.correctLayer(height, layer)

            let tagY = layerLocation(layer)
            if (tagY < control.topPadding) {
                continue;
            }

            // console.log("extruder: layer-" + layer + ", height-" + height + ", tagY-" + tagY)   
            let image = idConfigTag.createObject(control)
            image.type = "color"
            image.y = tagY
            updateImageMap(layer, image);
        }
    }

    function layerLocation(layer) {
        let actualHeight = control.availableHeight - idHandle_second.height
        let totalLayer = control.to - control.from;
        let result = control.topPadding + (control.to - layer) / totalLayer * actualHeight
        // console.log("calculate location *************")
        // console.log("idHandle_second: " + idHandle_second.y + ", " + idHandle_second.height)
        // console.log("layer: " + layer)
        // console.log("control.to: " + control.to)
        // console.log("visualPosition: " + control.visualPosition)
        // console.log(actualHeight)
        // console.log("result: " + result)
        return parseInt(result)
    }

    // function heightToLayer(height) {
    //     let actualHeight = control.availableHeight - idHandle_second.height
    //     let totalHeight = control.sliceAttain.layerHeight(control.to).toFixed(2)
    //     let totalLayer = control.to - control.from + 1;
    //     let percent = height / totalHeight
    //     let layer = parseInt(totalLayer * percent + 0.5)
    //     console.log("heightToLayer: " + actualHeight + ", " + totalHeight + ", " + totalLayer + ", " + percent + ", " + (totalLayer * percent) + ", " + layer)
    //     return layer
    // }

    function checkIfEnableDeleteTag() {
        let layer = value
        if (tagMap[layer]) { /* 显示X按钮 */
            idDeleteTag.visible = true
            idDeleteTag.y = tagMap[layer].y
        } else {
            idDeleteTag.visible = false
        }
    }

    function updateImageMap(layer, image) {
        image.value = layer
        image.visible = true

        let item = tagMap[layer]
        if (item) {
            item.destroy()
            tagMap[layer] = undefined;
            locationMap[layer] = undefined;
        }

        tagMap[layer] = image
        locationMap[layer] = image.y

        checkIfEnableDeleteTag()
    }

    function addPause() {
        if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
            return 
            
        let layer = control.value
        // control.second.value = layer
        let image = idConfigTag.createObject(control)
        image.type = "pause"
        // image.y = layerLocation(layer) - image.height / 2
        image.y = layerLocation(layer)
        updateImageMap(layer, image);

        printerSettings.setLayerPause(layer)
    }

    function setExtruder(extruder) {                
        if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
            return 

        let layer = control.value
        // control.second.value = layer
        let image = idConfigTag.createObject(control)
        image.type = "color"
        image.y = layerLocation(layer)
        updateImageMap(layer, image);

        printerSettings.setLayerExtruder(layer, extruder + 1);
    }

    function setCustomGCode(gcode) {                
        if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
            return 

        let layer = control.value
        // control.second.value = layer
        let image = idConfigTag.createObject(control)
        image.tips = qsTr("Custom G-code") + ':"' + gcode + '"'
        image.type = "gcode"
        image.y = layerLocation(layer)
        updateImageMap(layer, image);

        printerSettings.setLayerGCode(layer, gcode);
    }

    function removeConfig() {
        if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
            return 
        let layer = parseInt(control.value)
        let item = tagMap[layer]
        if (item) {
            item.destroy()
            tagMap[layer] = undefined;
            locationMap[layer] = undefined;
        }
        
        printerSettings.removeLayerConfig(layer);
        checkIfEnableDeleteTag()
    }

    function clearMap() {
        for (var layer in tagMap) {
            let item = tagMap[layer]
            if (item) {
                item.destroy()
            }
        }
        tagMap = new Map
        locationMap = new Map
    }

    function clearConfig() {
        clearMap()

        if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
            return 

        printerSettings.clearLayerConifg();
    }

    handle: Rectangle {
        id: idHandle_second
        visible: secondControl

        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))

        width: 18 * screenScaleFactor
        height: 18 * screenScaleFactor

        color: control.pressed || control.hovered ? handleHoverColor : handleNormalColor
        border.color: handleBorderColor
        border.width: 2
        radius: height / 2
        z:99
        Image {
            source: slider_block
            width: 8* screenScaleFactor
            height: 8* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            sourceSize.width:  8* screenScaleFactor
            sourceSize.height: 8* screenScaleFactor
        }
    }

    background: Rectangle {
        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: 9 * screenScaleFactor + (control.horizontal ? (control.availableHeight - height) / 2 : 0)

        width: control.horizontal ? control.availableWidth : implicitWidth
        height: control.horizontal ? (implicitHeight - 18 * screenScaleFactor) : (control.availableHeight - 18 * screenScaleFactor)

        implicitWidth: control.horizontal ? 200 : 4
        implicitHeight: (control.horizontal ? 4 : (200 - 18)) * screenScaleFactor

        color: control.completeColor
        opacity: control.opacity
        scale: control.horizontal && control.mirrored ? -1 : 1
        radius: 2

        //first
        Rectangle {
            id: idRect_first
            visible: firstControl && !control.hasSpecificExtruder
            y: control.horizontal ? 0 : control.first.visualPosition * parent.height
            width: control.horizontal ? control.position * parent.width : 4 //parent.width
            height: control.horizontal ? parent.height : control.first.position * parent.height

            color: control.incompleteColor
            opacity: control.opacity
            radius: 2
        }

        //second
        Rectangle {
            id: idRect_second
            visible: secondControl && !control.hasSpecificExtruder
            width: control.horizontal ? control.position * parent.width : 4 //parent.width
            height: control.horizontal ? parent.height : control.visualPosition * parent.height

            color: control.incompleteColor
            opacity: control.opacity
            radius: 2
        }

        Rectangle {
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            visible: control.hasSpecificExtruder
            radius: 2
            color: "transparent"
            // opacity: control.opacity

            MutilColorMask {
                anchors.fill: parent
                
                radius: parent.radius

                printerSettings: control.printerSettings 
                totalLayers: control.to
                control: control

                // totalHeight: control.availableHeight

            }
        }

    }
    Component{
        id: idConfigTag

        ConfigTag {
            x: idHandle_second.x - width / 2
            onClicked: {
                control.value = value
            }
        }

    }

    CustomGCodeDialog{
        id: idGcodeDialog
        onOk: {
            control.setCustomGCode(gcode)
        }
    }

    JumpLayerDialog {
        id: idJumpLayerDialog

        onOk: {
            control.value = layer
        }
    }

    Rectangle  {
        id: gcodeMenuLayout
        anchors.right: parent.right
        anchors.rightMargin: idPreviewSliderMenu.width + 25 * screenScaleFactor
        anchors.verticalCenter: idHandle_second.verticalCenter
        radius: 5

        PreviewSliderMenu {
            id: idPreviewSliderMenu

            onRequestAddPause: {
                control.addPause()
            }

            onRequestGCodeDialog: {
                if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
                    return 

                idGcodeDialog.clear()
                idGcodeDialog.visible = true
                gcodeMenuContent.visible = false
            }

            onRequestJumpLayerDialog: {
                idJumpLayerDialog.visible = true
                idJumpLayerDialog.init(control.from, control.to, control.value)
            }

            onRequsetChangeExtruder: {
                control.setExtruder(index)
            }

        }

        EditCustomGCodeMenu {
            id: idEditCustomGCodeMenu

            onRequestGCodeDialog: {
                if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile)
                    return 

                let gcode = printerSettings.customGCode(control.value)
                idGcodeDialog.init(gcode)
                idGcodeDialog.visible = true
                gcodeMenuContent.visible = false
            }

            onRequestDeleteGCode: {
                control.removeConfig();
            }

        }

        ChangeLayerFilamentMenu {
            id: idChangeLayerFilamentMenu

            onRequestDeletePause: {
                control.removeConfig();
            }

            onRequsetChangeExtruder: {
                control.setExtruder(index)
            }
        }

        CancelPauseMenu {
            id: idCancelPauseMenu

            onRequestDeletePause: {
                control.removeConfig()
            }
        }
    }

    DeleteTag {
        id: idDeleteTag

        anchors.left: parent.left
        anchors.leftMargin: 30 * screenScaleFactor
        anchors.verticalCenter: idHandle_second.verticalCenter

        onRequestDelete: {
            control.removeConfig();
        }

    }

    //first
    BubbleTip {
        id: first_tip
        width: 105 * screenScaleFactor
        height: 48 * screenScaleFactor
        visible: control.sliceAttain

        anchors.right: parent.right
        anchors.rightMargin: 30 * screenScaleFactor
        anchors.top: control.bottom
        anchors.topMargin: -13 * screenScaleFactor

        // bubbleColor: Constants.currentTheme ? "#F2F2F5" : "#424B51"
        bubbleColor: Constants.currentTheme ? "#424B51" : "#424B51"
        tailAnchor: "top"

        Column{
            anchors.fill: parent
            anchors.left: parent.left
            anchors.leftMargin: 10* screenScaleFactor

            Text {
                // id: idLabel2
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                width: parent.width
                height: parent.height/2
                verticalAlignment: Text.AlignVCenter
                // color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                color: "#FFFFFF"
                text: qsTr("Layer") + ": 1"
            }

            Text {
                // id: idLabel3
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                width: parent.width
                height: parent.height/2
                verticalAlignment: Text.AlignVCenter
                // color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                color: "#FFFFFF"
                text: qsTr("Height") + ": " + control.sliceAttain.layerHeight(1).toFixed(labelDecimal)
            }
        }
    }

    //second
    BubbleTip {
        id: second_tip
        width: 105 * screenScaleFactor
        height: 48 * screenScaleFactor
        visible: control.sliceAttain

        anchors.right: parent.right
        anchors.rightMargin: 30 * screenScaleFactor
        anchors.bottom: idHandle_second.verticalCenter
        radius: 5
        // bubbleColor: Constants.currentTheme ? "#F2F2F5" : "#424B51"
        bubbleColor: Constants.currentTheme ? "#424B51" : "#424B51"
        tailAnchor: "bottom"

        Column{ 
            anchors.fill: parent
            anchors.left: parent.left
            anchors.leftMargin: 10* screenScaleFactor

            Text {
                id: idLabel2
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                width: parent.width
                height: parent.height/2
                verticalAlignment: Text.AlignVCenter
                // color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                color: "#FFFFFF"
                text: qsTr("Layer") + ": " + parseInt(control.value) 
            }

            Text {
                id: idLabel3
                font.family: Constants.labelFontFamily
                font.weight: Constants.labelFontWeight
                font.pointSize: Constants.labelFontPointSize_9
                width: parent.width
                height: parent.height/2
                verticalAlignment: Text.AlignVCenter
                // color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                color: "#FFFFFF"
                text: qsTr("Height") + ": " + control.sliceAttain.layerHeight(control.value).toFixed(labelDecimal)
            }

        }
    }

    MouseArea {
        anchors.fill: parent
        onWheel: {
            if(wheel.angleDelta.y > 0)
            {
                if(layerSliderRange.value>=layerSliderRange.to)
                {
                    return;
                }
                layerSliderRange.value = layerSliderRange.value + 1
            }else{
                if(layerSliderRange.value<=layerSliderRange.from)
                {
                    return
                }
                layerSliderRange.value = layerSliderRange.value - 1
            }


        }
        acceptedButtons: Qt.RightButton // | Qt.LeftButton
        onPressed: {
            let layer = control.value
            if (!printerSettings || kernel_slice_flow.isPreviewGCodeFile) 
                return;

            if (gcodeMenuContent.updateState)
                gcodeMenuContent.updateState()

            if (printerSettings.isPauseLayer(layer)) {
                gcodeMenuContent = idCancelPauseMenu
            } else if (printerSettings.hasSpecificExtruder(layer)) {
                gcodeMenuContent = idChangeLayerFilamentMenu
            } else if (printerSettings.useCustomGCode(layer)) {
                gcodeMenuContent = idEditCustomGCodeMenu
            } else {
                gcodeMenuContent = idPreviewSliderMenu
            }

            //forward mouse event
            gcodeMenuContent.visible = true
//            sliderClick = true
            mouse.accepted = true
        }
        onReleased: {
            // forward mouse event
            mouse.accepted = false
        }
    }
}
