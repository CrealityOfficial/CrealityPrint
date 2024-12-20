import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "../qml"

T.RangeSlider {
    id: control
    property color completeColor: Constants.themeGreenColor
    property color incompleteColor: Constants.currentTheme ? "#CECECF" : "#4A4A4D"

    property color handlePressColor: Qt.lighter(Constants.themeGreenColor,1.1)
    property color handleHoverColor: Qt.lighter(Constants.themeGreenColor,1.1)
    property color handleNormalColor: Constants.themeGreenColor
    property color handleBorderColor: "#00FF65"



    property bool animation: false
    property bool firstControl: true
    property bool secondControl: true
    property alias firstVisible: first_tip.visible
    property alias secondVisible: second_tip.visible
    property alias firstLabelText: idLabel1.text
    property alias secondLabelText: idLabel2.text

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding)

    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleHeight + topPadding + bottomPadding)
    wheelEnabled: true
    first.onMoved:{
        if(!firstControl){
            second.value = first.value
            first.value = 0
        }
    }
    first.handle: Rectangle {
        id: idHandle_first
        visible: firstControl

        x: control.leftPadding + (control.horizontal ? control.first.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.first.visualPosition * (control.availableHeight - height))

        width: 18 * screenScaleFactor
        height: 18 * screenScaleFactor

        color: control.first.pressed || control.first.hovered ? handleHoverColor : handleNormalColor
        border.color: handleBorderColor
        border.width: 2
        radius: height / 2
    }

    second.handle: Rectangle {
        id: idHandle_second
        visible: secondControl

        x: control.leftPadding + (control.horizontal ? control.second.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.second.visualPosition * (control.availableHeight - height))

        width: 18 * screenScaleFactor
        height: 18 * screenScaleFactor

        color: control.second.pressed || control.second.hovered ? handleHoverColor : handleNormalColor
        border.color: handleBorderColor
        border.width: 2
        radius: height / 2
    }

    background: Rectangle {
        x: control.leftPadding + (control.horizontal ? 0 : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : 0)

        width: control.horizontal ? control.availableWidth : implicitWidth
        height: control.horizontal ? implicitHeight : control.availableHeight

        implicitWidth: control.horizontal ? 200 : 4
        implicitHeight: control.horizontal ? 4 : 200

        color: control.completeColor
        opacity: control.opacity
        scale: control.horizontal && control.mirrored ? -1 : 1
        radius: 2

        //first
        Rectangle {
            id: idRect_first
            visible: firstControl
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
            visible: secondControl
            width: control.horizontal ? control.position * parent.width : 4 //parent.width
            height: control.horizontal ? parent.height : control.second.visualPosition * parent.height

            color: control.incompleteColor
            opacity: control.opacity
            radius: 2
        }

    }

    //first
    Rectangle {
        id: first_tip

        width: 80 * screenScaleFactor
        height: 40 * screenScaleFactor
        visible: firstControl && (control.first.pressed || control.first.hovered)

        anchors.right: parent.right
        anchors.rightMargin: 40 * screenScaleFactor
        anchors.verticalCenter: idHandle_first.verticalCenter

        color: Constants.currentTheme ? "#F2F2F5" : "#424B51"
        border.color: "#1E9BE2"
        border.width: 1
        radius: 5

        Label {
            id: idLabel1
            anchors.centerIn: parent
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_9
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
        }

        Canvas {
            width: 5 * screenScaleFactor
            height: 10 * screenScaleFactor

            anchors.left: parent.right
            anchors.verticalCenter: parent.verticalCenter

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                //Draw path
                ctx.beginPath()
                ctx.moveTo(width, height / 2)
                ctx.lineTo(0, 0)
                ctx.lineTo(0, height)
                ctx.closePath()
                ctx.fillStyle = "#1E9BE2"; ctx.fill()
            }
        }
    }

    //second
    Rectangle {
        id: second_tip

        width: 80 * screenScaleFactor
        height: 40 * screenScaleFactor
        visible: secondControl && (control.second.pressed || control.second.hovered)

        anchors.right: parent.right
        anchors.rightMargin: 40 * screenScaleFactor
        anchors.verticalCenter: idHandle_second.verticalCenter

        color: Constants.currentTheme ? "#F2F2F5" : "#424B51"
        border.color: "#1E9BE2"
        border.width: 1
        radius: 5

        Label {
            id: idLabel2
            anchors.centerIn: parent
            font.family: Constants.labelFontFamily
            font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_9
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: Constants.currentTheme ? "#333333" : "#FFFFFF"
        }

        Canvas {
            width: 5 * screenScaleFactor
            height: 10 * screenScaleFactor

            anchors.left: parent.right
            anchors.verticalCenter: parent.verticalCenter

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                //Draw path
                ctx.beginPath()
                ctx.moveTo(width, height / 2)
                ctx.lineTo(0, 0)
                ctx.lineTo(0, height)
                ctx.closePath()
                ctx.fillStyle = "#1E9BE2"; ctx.fill()
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        onWheel: {
                    if(wheel.angleDelta.y > 0)
                    {
                        if(layerModelRange.second.value>=layerModelRange.to)
                        {
                            return;
                        }
                        layerModelRange.second.value = layerModelRange.second.value + 1
                    }else{
                        if(layerModelRange.second.value<=layerModelRange.from)
                        {
                            return
                        }
                        layerModelRange.second.value = layerModelRange.second.value - 1
                    }
                    console.log("onwheel:"+layerModelRange.to);


        }
        onPressed: {
                                // forward mouse event
           mouse.accepted = false
        }
        onReleased: {
                                // forward mouse event
                                mouse.accepted = false
        }
    }
}
