import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Controls 1.4 as T
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.15

T.Slider
{
    id: control
    implicitHeight: 20
    property real realTo: 557
    property real realFrom: 0
    property real valueText: 0
    property real realStepSize: 1.0

    property real sliderHeight: 4
    property real handleWidth: 10
    property real handleHeight: 10
    property real handleBorderWidth: 0

    property bool popTipVisible: true

    //property color grooveColor: Constants.grooveColor
    property color handleColor: Constants.themeGreenColor
    property color handleBorderColor: Qt.darker(Constants.themeGreenColor,1.2)
    //property color completeColor: "cadetblue"
    property color incompleteColor: Constants.lpw_BtnBorderColor

    value: valueText
    stepSize: realStepSize
    maximumValue: realTo
    minimumValue: realFrom

    style: SliderStyle {
        //保留滑块的背景凹槽
        groove: Rectangle {
            implicitHeight: sliderHeight
            radius: sliderHeight / 2
            color: control.incompleteColor

            Rectangle {
                id: idbackgroud
                width: __handlePos / maximumValue * parent.width
                implicitHeight: parent.implicitHeight
                radius: 3
                color: Constants.themeGreenColor
//                LinearGradient {
//                    anchors.fill: parent
//                    gradient: Gradient {
//                        GradientStop { position: 1; color: Constants.themeGreenColor }
//                        GradientStop { position: 0; color: Constants.themeGreenColor }
//                    }
//                    cached : true
//                    source: idbackgroud
//                    start: Qt.point(0, 0)
//                    end: Qt.point(idbackgroud.width, 0)
//                }
            }
        }
        //保存滑块句柄的项。可以通过控件属性访问滑块
        handle: Rectangle {
            implicitWidth: handleWidth
            implicitHeight: handleHeight

            color: handleColor
            border.color: handleBorderColor
            border.width: handleBorderWidth
            radius: handleHeight / 2
        }
    }

    ToolTip {
        parent: control.handle
        visible: control.pressed && popTipVisible
        text: control.value.toFixed(1)
    }
}
