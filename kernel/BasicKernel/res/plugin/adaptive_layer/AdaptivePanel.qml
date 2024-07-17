import CrealityUI 1.0
import QtQuick 2.10
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.13
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

LeftPanelDialog {
    id: control

    property var com

    title: qsTranslate("CusAdaptiveLayerTs","Adaptive Layer Thickness")
    width: 340 * screenScaleFactor
    height: 230 * screenScaleFactor
    Component.onCompleted: {
    }
    function execute()
    {}
    //捕获鼠标点击空白地方的事件
    MouseArea {
        anchors.fill: parent
    }

    Item {
        anchors.fill: parent.panelArea

        ColumnLayout {
            spacing: 10 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 21
            anchors.right: parent.right
            anchors.rightMargin: 21

            RowLayout {
                spacing: 5 * screenScaleFactor
                Layout.topMargin: 20 * screenScaleFactor
                BasicDialogButton
                {
                    Layout.preferredWidth: 60 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor

                    text: qsTranslate("CusAdaptiveLayerTs","Adaptive")//qsTr("自适应")
                    btnRadius: height/2
                    opacity: enabled ? 1 : 0.8
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    font.pointSize: Constants.labelFontPointSize_9
                    onSigButtonClicked:
                    {
                        com.adapted()
                    }
                }
                StyledLabel {
                    text: qsTranslate("CusAdaptiveLayerTs","Quality/Speed")//qsTr("细节/速度")
                    Layout.fillWidth: true
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    font.pointSize: Constants.labelFontPointSize_9
                }
                StyledSlider {
                    id: stepSlider
                    Layout.fillWidth: true
                    Layout.minimumWidth: 65 * screenScaleFactor
                    Layout.maximumWidth: 120 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    stepSize: 1
                    maximumValue: 100
                    realTo: 100
                    realFrom: 0
                    valueText: com ? com.speedVal * 100 : 2
                    sliderHeight: 4 * screenScaleFactor
                    handleWidth: 12 * screenScaleFactor
                    handleHeight: 12 * screenScaleFactor
                    handleBorderColor: Qt.darker(Constants.themeGreenColor, 1.2)
                    handleBorderWidth: 2
                    popTipVisible : false
                    onValueChanged: {
                        stepInput.value = value
                        if(com)
                            com.speedVal = value / 100
                    }
                }

                LiSpinBox {
                    id: stepInput

                    Layout.preferredWidth: 60 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    radius: 5
                    realFrom: 0
                    realTo: 1
                    realStepSize: 0.01
                    realValue: stepSlider.value
                    value: stepSlider.value
                    decimals: 2
                    unitchar: ""
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    font.pointSize: Constants.labelFontPointSize_9
                    textObj.selectionColor: "#1E9BE2"
                    onValueEdited: stepSlider.value = realValue *100
                }


            }
            RowLayout {
                spacing: 5 * screenScaleFactor
                Layout.topMargin: 10 * screenScaleFactor
                BasicDialogButton
                {
                    Layout.preferredWidth: 60 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor

                    text: qsTranslate("CusAdaptiveLayerTs","Smooth")//qsTr("平滑模式")
                    btnRadius: height/2
                    opacity: enabled ? 1 : 0.8
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    font.pointSize: Constants.labelFontPointSize_9
                    onSigButtonClicked:
                    {
                        com.smooth()
                    }
                }
                StyledLabel {
                    text: qsTranslate("CusAdaptiveLayerTs","Radius") //qsTr("半径")
                    Layout.fillWidth: true
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    font.pointSize: Constants.labelFontPointSize_9
                }
                StyledSlider {
//                    id: stepSlider
                    id : __radiusSlider
                    Layout.fillWidth: true
                    Layout.minimumWidth: 96 * screenScaleFactor
                    Layout.maximumWidth: 120 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    stepSize: 1
                    realTo: 10
                    realFrom: 1
                    valueText: com ? com.radiusVal : 3
                    sliderHeight: 4 * screenScaleFactor
                    handleWidth: 12 * screenScaleFactor
                    handleHeight: 12 * screenScaleFactor
                    handleBorderColor: Qt.darker(Constants.themeGreenColor, 1.2)
                    handleBorderWidth: 2
                    popTipVisible : false
                    onValueChanged: {
                        __radiusSpin.value = value
                        if(com)
                            com.radiusVal = value
                    }
                }

                LiSpinBox {
//                    id: stepInput
                    id : __radiusSpin
                    Layout.preferredWidth: 60 * screenScaleFactor
                    Layout.preferredHeight: 28 * screenScaleFactor
                    radius: 5
                    realFrom: 1
                    realTo: 10
                    realStepSize: 1
                    realValue: __radiusSlider.value
                    value: __radiusSlider.value
                    decimals: 0
                    unitchar: ""
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    font.pointSize: Constants.labelFontPointSize_9
                    textObj.selectionColor: "#1E9BE2"
                    onValueEdited: __radiusSlider.value = realValue
                }


            }
            RowLayout {
                width: parent.width

                StyleCheckBox {
                    Layout.preferredHeight: 28 * screenScaleFactor
                    Layout.fillWidth: true
                    checked: com ? com.keepMin : true
                    text: qsTranslate("CusAdaptiveLayerTs","Keep Min")//qsTr("保留最小")
                    font.family: Constants.labelFontFamilyBold
                    font.weight: Font.Medium
                    onClicked: {
                        com.keepMin = checked
                    }
                }
            }
//            Rectangle
//            {
//                width: parent.width
//                height : 1
//                color: "#6C6C70"
//            }
            RowLayout {
                width: parent.width
                enabled : true
                KeyboardPanel
                {
                    id : __keyboard
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
//                   contextList: [
//                       "Shift + " + qsTranslate("SeamPaintingPanel","Right mouse button") + ":",qsTr("平滑")
//                       ,"Shift + " + qsTranslate("SeamPaintingPanel","Left mouse button") + ":",qsTr("设置到基础层高")
//                       ,qsTranslate("SeamPaintingPanel","Right mouse button")  + ":",qsTr("增大层高")
//                        ,qsTranslate("SeamPaintingPanel","Left mouse button") + ":", "减小层高"
//                        ,qsTranslate("SeamPaintingPanel","Wheel") + ":", "增大/缩小编辑区域"]
                   onClicked:
                   {
                        com.reset()
                   }
                }
            }

        }
    }

}
