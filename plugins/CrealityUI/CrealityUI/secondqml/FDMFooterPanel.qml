import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import QtGraphicalEffects 1.12
import QtQml 2.13

import "../qml"
import "../secondqml"

Rectangle {
    id: root

    color: "transparent"
    height: panelHeight
    property int panelWidth: 828 * screenScaleFactor
    property int panelHeight: 104 * screenScaleFactor

    //Control options
    property var speedFlag: kernel_slice_model.currentStepSpeed
    property var randomstep: 0
    property var previewSlider: ""

    property bool startAnimation: false
    property color labelColor: Constants.currentTheme ? "#333333" : "#FFFFFF"

    //Display range
    property alias stepMax: stepSlider.maximumValue
    property alias currentStep: stepSlider.value
    property alias onlyLayer: idOnlyShow.checked
    signal stepSliderChange(var value)

    function update()
    {
        idOnlyShow.checked = false
        stepMax = kernel_slice_model.steps
        currentStep = kernel_slice_model.steps
    }

    function onlyShowLayer()
    {
        if(idOnlyShow.checked)
        {
			let value = layerBox.realValue
			previewSlider.second.value = value
			kernel_slice_model.showOnlyLayer(value)
        }
        else {
			kernel_slice_model.showOnlyLayer(-1)
		}
    }

    onSpeedFlagChanged: {
        if (root.speedFlag > 0) {
            idTimerDown.interval = 3000 / (speedSlider.value * root.speedFlag)
        }
    }

    MouseArea {
        id: wheelFilter
        anchors.fill: parent
        propagateComposedEvents: false
        onWheel: {}
    }

    Timer {
        id: idTimerDown
        repeat: true
        interval: 3000 / speedSlider.value
        triggeredOnStart: true

        onTriggered: {
            if (stepSlider.value < stepSlider.maximumValue && idPreviewWay.currentIndex === 0) {
                stepSlider.value += 1
                randomstep = true
                return
            }
            
            if (previewSlider.second.value < previewSlider.to) {
                previewSlider.second.value += 1
                stepSlider.value = idPreviewWay.currentIndex === 0 ? 0 : stepSlider.maximumValue
                randomstep = true
                return
            }

            kernel_slice_model.setAnimationState(0)
            startAnimation = !startAnimation
            randomstep = false
            idTimerDown.stop()
        }
    }

    Rectangle {
        width: panelWidth
        height: panelHeight
        anchors.centerIn: parent

        color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
        border.color: Constants.currentTheme ? "#CBCBCC" : "#262626"
        border.width: 1
        radius: 5

        layer.enabled: true

        layer.effect: DropShadow {
            radius: 8
            spread: 0.2
            samples: 17
            color: Constants.currentTheme ? "#BBBBBB" : "#333333"
        }

        RowLayout {
            x: 20 * screenScaleFactor
            y: 10 * screenScaleFactor
            width: parent.width - x * 2
            height: parent.height - y * 2
            spacing: 20 * screenScaleFactor

            ColumnLayout {
                spacing: 10 * screenScaleFactor

                BasicDialogButton {
                    id: animationBtn
                    Layout.preferredWidth: 150 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor

                    text: startAnimation ? qsTr("Suspend") : qsTr("Start")
                    btnTextColor: Constants.right_panel_slice_text_default_color
                    defaultBtnBgColor : Constants.right_panel_slice_button_default_color
                    hoveredBtnBgColor : Constants.right_panel_slice_button_hovered_color
                    btnBorderW: 0
                    btnRadius: 5

                    onSigButtonClicked:
                    {
                        if(previewSlider.second.value === previewSlider.to)
                        {
                            previewSlider.second.value = 0
                        }

                        //startOrSuspend

                        if(startAnimation)
                        {
                            idTimerDown.stop()
                            randomstep = false
                            startAnimation = !startAnimation
                            kernel_slice_model.setAnimationState(0)
                        }
                        else {
                            kernel_slice_model.setAnimationState(1)
                            startAnimation = !startAnimation
                            randomstep = true
                            idTimerDown.start()
                        }
                    }
                }

                RowLayout {
                    spacing: 10 * screenScaleFactor

                    Label {
                        Layout.preferredWidth: 60 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor

                        font.pointSize: Constants.labelFontPointSize_9
                        font.family: Constants.labelFontFamily
                        font.weight: Constants.labelFontWeight
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("Print Speed")
                        color: labelColor
                    }

                    StyledSlider {
                        id: speedSlider
                        Layout.preferredWidth: 80 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor

                        value: 50
                        minimumValue: 1
                        maximumValue: 150
                        sliderHeight: 2 * screenScaleFactor
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 1 * screenScaleFactor
                Layout.preferredHeight: 80 * screenScaleFactor
                color: Constants.currentTheme ? "#CBCBCC" : "#606062"
            }

            ColumnLayout {
                spacing: 10 * screenScaleFactor

                Label {
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor

                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Preview Way")
                    color: labelColor
                }

                Label {
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor

                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("Steps Number")
                    color: labelColor
                }
            }

            ColumnLayout {
                spacing: 10 * screenScaleFactor

                RowLayout {
                    spacing: 10 * screenScaleFactor

                    ComboBox {
                        id: idPreviewWay
                        Layout.preferredWidth: 140 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor

                        displayText: qsTr(currentText)

                        currentIndex: 0

                        model: ListModel {
                            ListElement { modelData: QT_TR_NOOP("Each Step")  }
                            ListElement { modelData: QT_TR_NOOP("Each Layer") }
                        }

                        delegate: ItemDelegate {
                            width: idPreviewWay.width
                            height: idPreviewWay.height
                            contentItem: Rectangle {
                                anchors.fill: parent
                                color: (idPreviewWay.highlightedIndex == index) ? "#1E9BE2" : Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"

                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 8 * screenScaleFactor
                                    anchors.verticalCenter: parent.verticalCenter

                                    font.weight: Font.Medium
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                                    text: qsTr(modelData)
                                }
                            }
                        }

                        indicator: Rectangle {
                            id: previewway_indicator

                            readonly property bool focused: parent.hovered || parent.popup.opened

                            anchors.right: parent.right
                            anchors.rightMargin: 10 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            color: "transparent"

                            Rectangle {
                                anchors.centerIn: parent
                                width: 16 * screenScaleFactor
                                height: 10 * screenScaleFactor
                                radius: 3 * screenScaleFactor
                                color: previewway_indicator.focused ? Constants.cmbIndicatorRectColor_pressed_basic
                                                                    : Constants.cmbIndicatorRectColor_basic
                                Image {
                                    anchors.centerIn: parent
                                    width: 7 * screenScaleFactor
                                    height: 5 * screenScaleFactor
                                    source: previewway_indicator.focused ? Constants.downBtnImgSource_d
                                                                         : Constants.downBtnImgSource
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        contentItem: Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 8 * screenScaleFactor
                            anchors.verticalCenter: parent.verticalCenter

                            font.weight: Font.Medium
                            font.family: Constants.mySystemFont.name
                            font.pointSize: Constants.labelFontPointSize_9
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                            text: idPreviewWay.displayText
                        }

                        background: Rectangle {
                            color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                            border.color: Constants.currentTheme ? "#D6D6DC" : "#6E6E72"
                            border.width: 1 * screenScaleFactor
                            radius: 5 * screenScaleFactor
                        }

                        popup: Popup {
                            y: idPreviewWay.height
                            width: idPreviewWay.width
                            implicitHeight: contentItem.implicitHeight + bottomPadding

                            leftPadding: 1; rightPadding: 1
                            bottomPadding: 1; topPadding: 0

                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: idPreviewWay.delegateModel
                            }

                            background: Rectangle {
                                color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
                                border.color: Constants.currentTheme ? "#D6D6DC" : "#6E6E72"
                                border.width: 1
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.preferredWidth: 1 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        color: Constants.currentTheme ? "#CBCBCC" : "#606062"
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        spacing: 10 * screenScaleFactor

                        CheckBox {
                            id: idOnlyShow
                            padding: 0
                            checked: false
                            contentItem: null

                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: 14 * screenScaleFactor
                            Layout.preferredHeight: 14 * screenScaleFactor

                            indicator: Image {
                                x: 3 * screenScaleFactor
                                y: 4 * screenScaleFactor
                                source: "qrc:/UI/photo/checkIcon.png"
                                visible: parent.checked
                            }

                            background: Rectangle {
                                color: parent.checked ? "#1E9BE2" : "transparent"
                                border.color: parent.hovered ? "#1E9BE2" : Constants.currentTheme ? "#CECECF" : "#6E6E72"
                                border.width: parent.checked ? 0 : 1
                                radius: 3
                            }

                            onCheckedChanged: {
                                animationBtn.enabled = !checked
                                //previewSlider.enabled = !checked
                                onlyShowLayer()
                            }
                        }

                        Label {
                            Layout.preferredWidth: 56 * screenScaleFactor
                            Layout.preferredHeight: 28 * screenScaleFactor

                            font.family: Constants.labelFontFamily
                            font.weight: Constants.labelFontWeight
                            font.pointSize: Constants.labelFontPointSize_9
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: qsTr("Only Show")
                            color: labelColor
                        }

                        StyledSpinBox {
                            id: layerBox
                            Layout.preferredWidth: 193 * screenScaleFactor
                            Layout.preferredHeight: 28 * screenScaleFactor
                            opacity: enabled ? 1.0 : 0.5
                            enabled: idOnlyShow.checked
                            radius: 5

                            realFrom: 1
                            realTo: previewSlider.to
                            realStepSize: 1
                            realValue: previewSlider.second.value
                            decimals: 0

                            unitchar: qsTr("Layer")
                            unitColor: labelColor
                            font.family: Constants.labelFontFamily
                            font.weight: Constants.labelFontWeight
                            font.pointSize: Constants.labelFontPointSize_9

                            textObj.color: labelColor
                            textObj.selectionColor: "#1E9BE2"
                            textObj.validator: IntValidator { bottom: 1; top: 99999 }

                            onValueEdited: onlyShowLayer()
                            onValueChanged:{
                                if(previewSlider.to>0)
                                {
                                     kernel_slice_model.setCurrentLayerFocused(false)
                                }
                               
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 10 * screenScaleFactor

                    StyledSlider {
                        id: stepSlider
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28 * screenScaleFactor

                        stepSize: 1
                        maximumValue: 40
                        sliderHeight: 4 * screenScaleFactor
                        handleWidth: 18 * screenScaleFactor
                        handleHeight: 18 * screenScaleFactor
                        handleBorderColor: "#7DEEFF"
                        handleBorderWidth: 2

                        onValueChanged:
                        {
                            stepInput.value = value
                            stepSliderChange(value)
                            if(maximumValue>0 && value >0 && !startAnimation && maximumValue!=value)
                            {
                                kernel_slice_model.setCurrentLayerFocused(true)
                            }
                            //console.log("++++++++++++++++++++++++++"+value)
                            //kernel_slice_model.setCurrentLayerFocused(true)
                        }
                        onPressedChanged:
                        {
                            if(previewSlider.to>0)
                            {
                                //kernel_slice_model.setCurrentLayerFocused(pressed)
                            }
							
                        }
                    }

                    StyledSpinBox {
                        id: stepInput
                        Layout.preferredWidth: 86 * screenScaleFactor
                        Layout.preferredHeight: 28 * screenScaleFactor
                        radius: 5

                        realFrom: 0
                        realTo: stepSlider.maximumValue
                        realStepSize: 1
                        realValue: stepSlider.value
                        value: stepSlider.value
                        decimals: 0

                        unitchar: ""
                        unitColor: labelColor
                        font.family: Constants.labelFontFamily
                        font.weight: Constants.labelFontWeight
                        font.pointSize: Constants.labelFontPointSize_9

                        textObj.color: labelColor
                        textObj.selectionColor: "#1E9BE2"
                        textObj.validator: IntValidator { bottom: 1; top: 99999 }

                        onValueEdited: stepSlider.value = realValue
                    }
                }
            }
        }
    }
}
