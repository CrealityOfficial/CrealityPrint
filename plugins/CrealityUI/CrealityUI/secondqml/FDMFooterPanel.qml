import "../qml"
import "../secondqml"
import QtGraphicalEffects 1.12
import QtQml 2.13
import QtQuick 2.13
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13

Rectangle {
    id: root

    property int panelWidth: 460 * screenScaleFactor
    property int panelHeight: 50 * screenScaleFactor
    //Control options
    property var speedFlag: kernel_slice_model.currentStepSpeed
    property var randomstep: 0
    property var previewSlider: ""
    property bool startAnimation: false
    property color labelColor: Constants.currentTheme ? "#333333" : "#FFFFFF"
    //Display range
    property alias stepMax: stepSlider.maximumValue
    property alias currentStep: stepSlider.value
    property alias onlyLayer: idOnlyShow.isChecked
    property string slice_start_img: "qrc:/UI/photo/slice_start.svg"
    property string slice_stop_img: "qrc:/UI/photo/slice_stop.svg"
    property string layer_current_img: "qrc:/UI/photo/layer_current.svg"
    property string layer_all_img: "qrc:/UI/photo/layer_all.svg"

    signal stepSliderChange(var value)

    function update() {
        idOnlyShow.isChecked = false;
        stepMax = kernel_slice_model.steps;
        currentStep = kernel_slice_model.steps;
    }

    function onlyShowLayer() {
        if (idOnlyShow.isChecked)
            kernel_slice_model.showOnlyLayer(previewSlider.value);
        else
            kernel_slice_model.showOnlyLayer(-1);
    }

    width: panelWidth
    color: "transparent"
    height: panelHeight
    onSpeedFlagChanged: {
        if (root.speedFlag > 0)
            idTimerDown.interval = root.speedFlag;

    }

    MouseArea {
        id: wheelFilter

        anchors.fill: parent
        propagateComposedEvents: false
        onWheel: {
        }
    }
    
    Connections {
        target: kernel_kernel

        onCurrentPhaseChanged: {
            if (kernel_kernel.currentPhase != 1)
                idTimerDown.stop()
        }
    }

    Timer {
        id: idTimerDown

        repeat: true
        interval: 30
        triggeredOnStart: true
        onTriggered: {
            if (stepSlider.value < stepSlider.maximumValue) {
                stepSlider.value += 1;
                randomstep = true;
                return ;
            }
            if (previewSlider.value < previewSlider.to) {
                previewSlider.value += 1;
                stepSlider.value = 0; //idPreviewWay.currentIndex === 0 ? 0 : stepSlider.maximumValue;
                randomstep = true;
                return ;
            }
            kernel_slice_model.setAnimationState(0);
            startAnimation = !startAnimation;
            randomstep = false;
            idTimerDown.stop();
        }
    }

    Rectangle {
        width: panelWidth
        height: panelHeight
        //anchors.centerIn: parent
        color: Constants.currentTheme ? "#FFFFFF" : "#4B4B4D"
        border.color: Constants.currentTheme ? "#CBCBCC" : "#262626"
        border.width: 1
        radius: 5
        layer.enabled: true

        RowLayout {
            anchors.fill: parent
            Layout.alignment: Qt.AlignVCenter
            spacing: 5 * screenScaleFactor

            Item {
                Layout.preferredWidth: 5 * screenScaleFactor
            }

            CusImglButton {
                id: animationBtn

                Layout.preferredWidth: 28 * screenScaleFactor
                Layout.preferredHeight: 28 * screenScaleFactor
                imgWidth: 10 * screenScaleFactor
                imgHeight: 12 * screenScaleFactor
                enabledIconSource: startAnimation ? root.slice_stop_img : root.slice_start_img
                highLightIconSource: startAnimation ? root.slice_stop_img : root.slice_start_img
                pressedIconSource: startAnimation ? root.slice_stop_img : root.slice_start_img
                defaultBtnBgColor: Constants.themeGreenColor
                selectedBtnBgColor:  Constants.themeGreenColor
                hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor, 1.1) //Constants.right_panel_slice_button_hovered_color
                borderWidth: 0
                onClicked: {
                    if (previewSlider.value === previewSlider.to){
                        previewSlider.value = 0;
                        stepSlider.value = 0
                    }

                    if (startAnimation) {
                        idTimerDown.stop(); 
                        randomstep = false;
                        startAnimation = !startAnimation;
                        kernel_slice_model.setAnimationState(0);
                    } else {
                        kernel_slice_model.setAnimationState(1);
                        startAnimation = !startAnimation;
                        randomstep = true;
                        idTimerDown.start();
                    }
                }
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

            StyledSlider {
                id: stepSlider

                Layout.fillWidth: true
                Layout.preferredHeight: 28 * screenScaleFactor
                stepSize: 1
                maximumValue: 40
                sliderHeight: 4 * screenScaleFactor
                handleWidth: 18 * screenScaleFactor
                handleHeight: 18 * screenScaleFactor
                handleBorderColor: Qt.darker(Constants.themeGreenColor, 1.2)
                handleBorderWidth: 2
                onValueChanged: {
                   
                    stepInput.realValue = value;
                    stepSliderChange(value);
                    console.log(stepSlider.maximumValue,value, "maximumValuemaximumValue")
                    if (stepSlider.maximumValue > 0 && value > 0 && !startAnimation && stepSlider.maximumValue!=value)
                        kernel_slice_model.setCurrentLayerFocused(1);
                    
                    // if(value!==stepSlider.maximumValue)
                    //   kernel_slice_model.setIndicatorVisible(true)
                }
                
            }
            Shortcut {
                context: Qt.WindowShortcut
                sequence: "Right"
                onActivated:stepSlider.value++
            }
            Shortcut {
                context: Qt.WindowShortcut
                sequence: "Left"
                onActivated:stepSlider.value--
            }


            
        CusStyledSpinBox
        {
            id: stepInput
            Layout.preferredWidth: 66 * screenScaleFactor
            Layout.preferredHeight: 28 * screenScaleFactor
            decimals: 0
            realFrom:0
            realTo: stepSlider.maximumValue
            // realStepSize: 1
            realValue: stepSlider.value
            value: stepSlider.value
            stepSize: 1
            unitchar: ""
            onTextContentChanged: stepSlider.value = result
        
      }


            CusImglButton {
                id: idOnlyShow

                property bool isChecked: false

                Layout.preferredWidth: 28 * screenScaleFactor
                Layout.preferredHeight: 28 * screenScaleFactor
                imgWidth: 16 * screenScaleFactor
                imgHeight: 16 * screenScaleFactor
                defaultBtnBgColor: "transparent"
                hoveredBtnBgColor: "transparent"
                selectedBtnBgColor: "transparent"
                opacity: 1
                borderWidth: 0
                text: qsTr("Click to toggle showing only the current layer")
                cusTooltip.position: BasicTooltip.Position.TOP
                shadowEnabled: Constants.currentTheme
                enabledIconSource: isChecked ? root.layer_current_img : root.layer_all_img
                pressedIconSource: isChecked ? root.layer_current_img : root.layer_all_img
                highLightIconSource: isChecked ? root.layer_current_img : root.layer_all_img
                onClicked: {
                    isChecked = !isChecked;
                    animationBtn.enabled = !isChecked;
                    onlyShowLayer();
                }
            }

            Item {
                Layout.preferredWidth: 5 * screenScaleFactor
            }

        }

        layer.effect: DropShadow {
            radius: 8
            spread: 0.2
            samples: 17
            color: Constants.currentTheme ? "#BBBBBB" : "#333333"
        }

    }

}
