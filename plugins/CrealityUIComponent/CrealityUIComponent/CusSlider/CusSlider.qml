import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Controls 1.4 as T
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.12
import QtQuick.Layouts 1.1
import "../"
import "../CusTip"
import "../CusSpinBox"
RowLayout{
    id:sliderCom
    property var initValue: 0.3
    property string labelLeft: ""
    property int sliderWidth: 150
    property int sliderHeight: 2
    property int sliderBall: 10
    property bool isProgress: false
    property bool showRightLabel: false
    property color sliderColor: "#144F71"
    property color sliderBgColor: "#bdbebf"
    property color sliderBallColor: "#00A3FF"
    signal released(var value)
    Text {
        id: labelId
        text: labelLeft
        visible: !!labelLeft
    }
    Slider{
        id:sliderId
        value: initValue
        enabled: !isProgress
        ToolTip.text: parseInt(value*100, 10)
        ToolTip.visible: sliderId.pressed || sliderId.hovered
        background: Rectangle {
            x: sliderId.leftPadding
            y: sliderId.topPadding + sliderId.availableHeight / 2 - height / 2
            implicitWidth: sliderWidth
            implicitHeight: sliderHeight
            width: sliderId.availableWidth
            height: implicitHeight
            radius: sliderHeight
            color: sliderBgColor

            Rectangle {
                width: sliderId.visualPosition * parent.width
                height: parent.height
                color: sliderColor
                radius: sliderHeight
            }
        }

        handle: Rectangle {
            x: sliderId.leftPadding + sliderId.visualPosition * (sliderId.availableWidth - width)
            y: sliderId.topPadding + sliderId.availableHeight / 2 - height / 2
            implicitWidth: sliderBall
            implicitHeight: sliderBall
            radius: sliderBall/2
            color: sliderBallColor
            visible: !isProgress
        }
        onPressedChanged: {
            if(!pressed) released(sliderId.value)
        }
    }
    Text {
        id: valueId
        text: parseInt(sliderId.value*100, 10) + "%"
        width: 100
        visible: showRightLabel
    }
}
