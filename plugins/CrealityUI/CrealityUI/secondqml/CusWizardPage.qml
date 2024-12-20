import QtQuick 2.9
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import ".."
import "../qml"

Item {
    id: pageItem
    z: 998
    anchors.fill: parent
    property string wizardName
    property string wizardDescript
    property string targetObjectName
    property int pageType: pageTypeDown

    property color maskColor: "#4B4B4B"
    property real maskOpacity: 0.75

    property rect focusRect

    readonly property int pageTypeDown: Qt.DownArrow
    readonly property int pageTypeUp: Qt.UpArrow
    readonly property int pageTypeLeft: Qt.LeftArrow
    readonly property int pageTypeRight: Qt.RightArrow

    signal nextStep()
    signal finishStep()

    property int descriptWidth: 100
    property int descriptHeight: 100
    property int descriptOffset: 0


    Component.onCompleted: {
        var rect = WizardUI.getItemGeometryToScene(targetObjectName)
        focusRect = rect
    }
    Item {
        id: focusItem
        x: focusRect.x
        y: focusRect.y
        width: focusRect.width
        height: focusRect.height
        RadialGradient {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.6; color: maskColor }
            }
            opacity: maskOpacity / 4
        }
    }
    //left
    Rectangle {
        x: 0
        y: 0
        width: focusRect.x
        height: parent.height
        color: maskColor
        opacity: maskOpacity
    }
    //right
    Rectangle {
        x: focusRect.x + focusRect.width
        y: 0
        width: pageItem.width - x
        height: parent.height
        color: maskColor
        opacity: maskOpacity
    }
    //top
    Rectangle {
        x: focusRect.x
        width: focusRect.width
        y: 0
        height: focusRect.y
        color: maskColor
        opacity: maskOpacity
    }
    //bottom
    Rectangle {
        x: focusRect.x
        width: focusRect.width
        y: focusRect.y + focusRect.height
        height: pageItem.height - y
        color: maskColor
        opacity: maskOpacity
    }
    Row {
        id: leftRow
        spacing: 10
        visible: pageType === pageTypeLeft
        z: 998
        anchors {
            left: focusItem.right
            leftMargin: 5
            verticalCenter: focusItem.verticalCenter
        }
        Image {
            source: "qrc:/UI/images/arrow-left.png"
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Row {
        id: rightRow
        spacing: 10
        //        layoutDirection: Qt.RightToLeft
        visible: pageType === pageTypeRight
        z: 998
        anchors {
            right: focusItem.left
            rightMargin: 5
            verticalCenter: focusItem.verticalCenter
        }
        //Image {
        //    source:  "qrc:/UI/images/arrow-right.png"
        //    anchors.verticalCenter: parent.verticalCenter
        //}
        Row{
            Rectangle
            {
                id: idRectTypeRight
                height: descriptHeight
                width: descriptWidth
                color: "white"
                y: descriptOffset
                BasicButton
                {
                    id: idcloseRight
                    width: 20
                    height: 20
                    btnRadius: 0
                    y: 20
                    btnBorderW:0
                    defaultBtnBgColor : "transparent"
                    anchors {
                        top: idRectTypeRight.top
                        topMargin: 10
                        right:idRectTypeRight.right
                        rightMargin:10
                    }
                    Image{
                        anchors.centerIn: parent
                        fillMode: Image.Pad
                        source: idcloseRight.hovered ? "qrc:/UI/photo/close_press.png" : "qrc:/UI/photo/close_normal.png"
                    }
                    onSigButtonClicked:
                    {
                        finishStep()
                    }
                }

                Rectangle{
                    id: titleRectRight
                    height: 26
                    width:170
                    color:"transparent"
                    anchors {
                        top: idRectTypeRight.top
                        topMargin: 20
                        left:idRectTypeRight.left
                        leftMargin:20
                    }
                    Image{
                        height: 26
                        width:170
                        source: "qrc:/UI/photo/wizardTitle.png"
                    }
                    Label {
                        id: typeRightLabel
                        z: 998
                        text: qsTr(wizardName)
                        font.pointSize: Constants.labelFontPointSize_18
                        color: "black"
                        anchors {
                            top: titleRectRight.top
                            topMargin: 5
                            left: titleRectRight.left
                        }
                    }
                }

                Text{
                    text: qsTr(wizardDescript)
                    color: "black"
                    //readOnly: true
                    width: idRectTypeUp.width
                    wrapMode: TextEdit.WordWrap
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 35
                    font.pointSize: Constants.labelFontPointSize_14
                    anchors {
                        top: titleRectRight.bottom
                        topMargin: 10
                        left:titleRectRight.left
                    }
                }

                BasicButton
                {
                    id: idNextright
                    width: 60
                    height: 22
                    btnRadius: 0
                    y: 20
                    defaultBtnBgColor : "#DDDDDD"
                    hoveredBtnBgColor: "#B5B5B5"
                    text: qsTr("Next")
                    btnTextColor : "#333333"
                    anchors {
                        bottom: idRectTypeRight.bottom
                        bottomMargin: 20
                        right:idRectTypeRight.right
                        rightMargin:10
                    }
                    onSigButtonClicked:
                    {
                        nextStep()
                    }
                }
            }
            Canvas {
                id: canvasIdRight
                property color triangleColor: "white"
                //x: idRectTypeRight.width /2
                //y: idRect.width
                width: 8
                height: 14
                onPaint: {
                    var context = getContext("2d")
                    context.lineWidth = 1
                    context.strokeStyle = "white"
                    context.fillStyle = triangleColor
                    context.beginPath();
                    context.moveTo(0, 0)
                    context.lineTo(canvasIdRight.width, canvasIdRight.height/2);
                    context.lineTo(0, canvasIdRight.height);
                    context.lineTo(0, 0);
                    context.closePath();
                    context.fill()
                    context.stroke();
                }
            }
        }
    }
    Column {
        id: downColumn
        spacing: 10
        visible: pageType === pageTypeDown
        width: 300
        z: 998
        anchors {
            bottom: focusItem.top
            bottomMargin: 5
            horizontalCenter: focusItem.horizontalCenter
        }
        Image {
            source: "qrc:/UI/images/arrow-down.png"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
    Column {
        id: upColumn
        spacing: 10
        visible: pageType === pageTypeUp
        width: 300
        z: 998
        anchors {
            top: focusItem.bottom
            topMargin: 5
            horizontalCenter: focusItem.horizontalCenter
        }
        //Image {
        //    source: "qrc:/UI/images/arrow-up.png"
        //    anchors.horizontalCenter: parent.horizontalCenter
        //}
        Column{
            Canvas {
                id: canvasId
                property color triangleColor: "white"
                x: idRectTypeUp.width /2
                //y: idRect.width
                width: 14
                height: 8

                onPaint: {
                    var context = getContext("2d")
                    context.lineWidth = 1
                    context.strokeStyle = "white"
                    context.fillStyle = triangleColor
                    context.beginPath();
                    context.moveTo(0, canvasId.height)
                    context.lineTo(canvasId.width/2, 0);
                    context.lineTo(canvasId.width, canvasId.height);
                    context.lineTo(0, canvasId.height);
                    context.closePath();
                    context.fill()
                    context.stroke();
                }
            }

            Rectangle
            {
                id: idRectTypeUp
                height: descriptHeight
                width: descriptWidth
                color: "white"
                x: descriptOffset
                BasicButton
                {
                    id: idclose
                    width: 20
                    height: 20
                    btnRadius: 0
                    y: 20
                    defaultBtnBgColor : "transparent"
                    btnBorderW:0
                    anchors {
                        top: idRectTypeUp.top
                        topMargin: 20
                        right:idRectTypeUp.right
                        rightMargin:20
                    }
                    Image{
                        anchors.centerIn: parent
                        fillMode: Image.Pad
                        source: idclose.hovered ? "qrc:/UI/photo/close_press.png" : "qrc:/UI/photo/close_normal.png"
                    }
                    onSigButtonClicked:
                    {
                        finishStep()
                    }
                }

                Rectangle{
                    id: titleRect
                    height: 26
                    width:170
                    color:"transparent"
                    anchors {
                        top: idRectTypeUp.top
                        topMargin: 20
                        left:idRectTypeUp.left
                        leftMargin:20
                    }
                    Image{
                        //anchors.centerIn: parent
                        //fillMode: Image.TileHorizontally
                        height: 26
                        width:170
                        source: "qrc:/UI/photo/wizardTitle.png"
                    }
                    Label {
                        id: typeUpLabel
                        z: 998
                        text: qsTr(wizardName)
                        font.pointSize: Constants.labelFontPointSize_18
                        color: "black"
                        anchors {
                            top: titleRect.top
                            topMargin: 5
                            left: titleRect.left
                        }
                    }
                }

                Text{
                    text: qsTr(wizardDescript)
                    color: "black"
                    //readOnly: true
                    width: idRectTypeUp.width - 20
                    wrapMode: TextEdit.WordWrap
                    lineHeightMode: Text.FixedHeight
                    lineHeight: 35
                    font.pointSize: Constants.labelFontPointSize_14
                    anchors {
                        top: titleRect.bottom
                        topMargin: 10
                        left:titleRect.left
                    }
                }

                BasicButton
                {
                    id: idNext
                    width: 60
                    height: 22
                    btnRadius: 0
                    y: 20
                    defaultBtnBgColor : "#DDDDDD"
                    hoveredBtnBgColor: "#B5B5B5"
                    text: qsTr("Next")
                    btnTextColor : "#333333"
                    anchors {
                        bottom: idRectTypeUp.bottom
                        bottomMargin: 20
                        right:idRectTypeUp.right
                        rightMargin:20
                    }
                    onSigButtonClicked:
                    {
                        nextStep()
                    }
                }
            }
        }
    }
}
