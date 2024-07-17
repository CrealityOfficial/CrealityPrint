import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Templates 2.12 as T
import QtGraphicalEffects 1.12
import "../components"
import "../qml"

T.RangeSlider {
    id: control
    property color completeColor: Constants.themeGreenColor
    property color incompleteColor: Constants.currentTheme ? "#CECECF" : "#4A4A4D"

    property color handlePressColor: Qt.lighter(Constants.themeGreenColor,1.1)
    property color handleHoverColor: Qt.lighter(Constants.themeGreenColor,1.1)
    property color handleNormalColor: Constants.themeGreenColor
    property color handleBorderColor: "#00FF65" // Qt.darker(Constants.themeGreenColor,1.2)

    property bool animation: false
    property bool firstControl: true
    property bool secondControl: true
    property alias firstVisible: first_tip.visible
    property alias secondVisible: second_tip.visible
    property alias firstLabelText: idLabel1.text
    property alias secondLabelText: idLabel2.text
    property alias thirdLabelText: idLabel3.text
    property var gcodeImage: []
    property var menuType: -1
    property bool sliderClick: false
    property var setCurrentPhase: ""

    property string gcode_delete: "qrc:/UI/photo/slider/gcode_delete.svg"
    property string gcode_default: "qrc:/UI/photo/slider/gcode_default.svg"
    property string gcode_hover: "qrc:/UI/photo/slider/gcode_hover.svg"
    property string slider_block: "qrc:/UI/photo/slider/slider_block.svg"
    property string stop_default: "qrc:/UI/photo/slider/stop_default.svg"
    property string stop_hover: "qrc:/UI/photo/slider/stop_hover.svg"


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
    onSetCurrentPhaseChanged: {
        gcodeMenuContent.visible = false
        for(let f of gcodeImage){
            f.destroy();
            delete f;
        }
    }


    z:99
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
    Component{
        id: gcodeIcon
        Image {
            id: img
            property var gcodeValue: ""
            property var imgSource: gcode_default
            property var imgSourceHover: gcode_hover
            source: imgSource
            width: 35*screenScaleFactor
            height: 20*screenScaleFactor
            x: idHandle_second.x - width/2
            fillMode: Image.PreserveAspectFit
            Rectangle{
                height: parent.height
                width: parent.width/2
                anchors.left: parent.left
                anchors.leftMargin: 0
                color: "transparent"
                MouseArea{
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onExited: img.source = img.imgSource
                    onEntered: img.source = img.imgSourceHover
                    onClicked: layerSliderRange.second.value = img.gcodeValue

                }
            }
        }


    }

    DockItem{
        id: idGcodeDialog
        title: qsTr("Custom G-code")
        width: 500 * screenScaleFactor
        height: 240 * screenScaleFactor
        visible: false
        Column{
            y:50* screenScaleFactor
            width: parent.width - 40* screenScaleFactor
            height: parent.height-20* screenScaleFactor
            spacing:10* screenScaleFactor
            anchors.horizontalCenter: parent.horizontalCenter
         //   anchors.verticalCenter: parent.verticalCenter
            Text {
                y:60* screenScaleFactor
                id:tipText
                text: qsTr("Enter the custom G-code used on the current layer")
                color: Constants.textColor
                font.pointSize: Constants.labelFontPointSize_10
                font.family: Constants.labelFontFamilyBold
            }
            BasicDialogTextArea{
                id:gcodeEdit
                width: parent.width
                height: 109*screenScaleFactor
                font.pointSize:Constants.labelFontPointSize_10
                color: Constants.themeTextColor
                text: ""

            }

            Grid
            {
                width : basicComButton.width + basicComButton2.width + spacing
                height: 30*screenScaleFactor
                columns: 2
                spacing: 10
                anchors.horizontalCenter: parent.horizontalCenter
                Image {
                    id: name
                    source: "file"
                    Rectangle{

                    }
                }
                BasicDialogButton {
                    id: basicComButton
                    width: 100*screenScaleFactor
                    height: 30*screenScaleFactor
                    text: qsTr("Confirm")
                    btnRadius:15*screenScaleFactor
                    btnBorderW:1
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked:
                    {
                        let image = gcodeIcon.createObject(control)
                        image.y = JSON.parse(JSON.stringify(idHandle_second.y))
                        image.gcodeValue = JSON.parse(JSON.stringify(layerSliderRange.second.value))
                        image.imgSource = gcode_default
                        image.imgSourceHover = gcode_hover
                        gcodeImage.push(image)
                        idGcodeDialog.visible = false
                        kernel_slice_model.addCustomGcode(gcodeEdit.text)
                    }
                }

                BasicDialogButton {
                    id: basicComButton2
                    width: 100*screenScaleFactor
                    height: 30*screenScaleFactor
                    text: qsTr("Cancel")
                    btnRadius:15*screenScaleFactor
                    btnBorderW:1
                    defaultBtnBgColor: Constants.leftToolBtnColor_normal
                    hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
                    onSigButtonClicked: idGcodeDialog.visible = false

                }
            }

        }


    }

    Rectangle  {
        id: gcodeMenuLayout
        anchors.right: parent.right
        anchors.rightMargin: 150 * screenScaleFactor
        anchors.verticalCenter: idHandle_second.verticalCenter
        radius: 5
        Menu{
            id: gcodeMenuContent
            width: 130* screenScaleFactor
            MenuItem{
                text:  qsTr("Add pause printing")
                height: 28* screenScaleFactor
                width: 130 * screenScaleFactor
                onTriggered:{
                    let image = gcodeIcon.createObject(control)
                    image.y = JSON.parse(JSON.stringify(idHandle_second.y))
                    image.gcodeValue = JSON.parse(JSON.stringify(layerSliderRange.second.value))
                    image.imgSource = stop_default
                    image.imgSourceHover = stop_hover
                    gcodeImage.push(image)
                    gcodeMenuContent.visible = false
                    kernel_slice_model.addCustomGcode("M400")
                }

            }
            MenuItem{
                text:  qsTr("Add custom G-code")
                height: 28* screenScaleFactor
                width: 130 * screenScaleFactor
                onTriggered: {
                    idGcodeDialog.visible = true
                    gcodeMenuContent.visible = false
                }
            }
        }
    }



    Rectangle{
          width: 20* screenScaleFactor
          height: 20* screenScaleFactor
          anchors.left: parent.left
          anchors.leftMargin: 20 * screenScaleFactor
          anchors.verticalCenter: idHandle_second.verticalCenter
          visible: false
          id:gcodeDele
          radius: height/2
          color: dele_img.isHover ? "#FD265A" : "transparent"
          border.width: dele_img.isHover ? 0 : 1
          border.color: "#4B4B4D"
          Image {
              id:dele_img
              source: gcode_delete
              anchors.horizontalCenter: parent.horizontalCenter
              anchors.verticalCenter: parent.verticalCenter
              sourceSize.width: 8* screenScaleFactor
              sourceSize.height: 8* screenScaleFactor
              property bool isHover: false
              MouseArea{
                  hoverEnabled: true
                  anchors.fill: parent
                  cursorShape: Qt.PointingHandCursor
                  onEntered: dele_img.isHover = true
                  onExited: dele_img.isHover = false
                  onClicked: {
                      kernel_slice_model.delCustomGcode()
                      let obj = gcodeImage.find(f=>Math.abs(parseInt(f.y)-parseInt(idHandle_second.y)) < 3)
                      if(obj) {
                          obj.destroy();
                          delete obj;
                      }
                      gcodeDele.visible =false
                  }

              }
          }

      }
    second.onValueChanged: {
        gcodeDele.visible = !!gcodeImage.find(f=> Math.abs(parseInt(f.y)-parseInt(idHandle_second.y)) < 3)
    }


    //first
    Rectangle {
        id: first_tip

        width: 80 * screenScaleFactor
        height: 40 * screenScaleFactor
        visible: firstControl && (control.first.pressed || control.first.hovered)

        anchors.right: parent.right
        anchors.rightMargin: 30 * screenScaleFactor
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
        height: 48 * screenScaleFactor
        visible: !idGcodeDialog.visible && secondControl && (control.second.pressed || control.second.hovered)&& !gcodeMenuContent.visible

        anchors.right: parent.right
        anchors.rightMargin: 40 * screenScaleFactor
        anchors.verticalCenter: idHandle_second.verticalCenter

        color: Constants.currentTheme ? "#F2F2F5" : "#424B51"
        border.color: "#1E9BE2"
        border.width: 1
        radius: 5
        Column{
            anchors.fill: parent
            anchors.left: parent.left
            anchors.leftMargin: 10* screenScaleFactor
            Rectangle{
                width: parent.width
                height: parent.height/2
                color: "transparent"
                Label {
                    id: idLabel2
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    anchors.verticalCenter: parent.verticalCenter
                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                }

            }

            Rectangle{
                width: parent.width
                height: parent.height/2
                color: "transparent"
                Label {
                    id: idLabel3
                    font.family: Constants.labelFontFamily
                    font.weight: Constants.labelFontWeight
                    font.pointSize: Constants.labelFontPointSize_9
                    anchors.verticalCenter: parent.verticalCenter
                    color: Constants.currentTheme ? "#333333" : "#FFFFFF"
                }

            }

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
                if(layerSliderRange.second.value>=layerSliderRange.to)
                {
                    return;
                }
                layerSliderRange.second.value = layerSliderRange.second.value + 1
            }else{
                if(layerSliderRange.second.value<=layerSliderRange.from)
                {
                    return
                }
                layerSliderRange.second.value = layerSliderRange.second.value - 1
            }
            console.log("onwheel:"+layerSliderRange.to,layerSliderRange.first.position);


        }
        acceptedButtons: Qt.RightButton // | Qt.LeftButton
        onPressed: {
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
