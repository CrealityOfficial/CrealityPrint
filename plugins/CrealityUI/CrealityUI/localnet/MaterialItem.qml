import QtQuick 2.13
import QtCharts 2.13
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtGraphicalEffects 1.12
import Qt.labs.platform 1.1
import "qrc:/CrealityUI/secondqml/frameless"

import "../qml"
import "../secondqml"

Rectangle{
 
    property var text_normal_color:Constants.currentTheme? "#FFFFFF":"#333333"
    property var popup_background_color:Constants.currentTheme? "#FFFFFF":"#28282B"
    property var background_border:Constants.currentTheme? "#D6D6DC":"#0F0F10"
    property var title_background_color:Constants.currentTheme? "#FFFFFF":"#37373B"
    property var img_upArrow: Constants.currentTheme? "qrc:/UI/photo/lanPrinterSource/upArrow.svg":"qrc:/UI/photo/lanPrinterSource/upArrow.svg"
    property var img_downArrow: Constants.currentTheme? "qrc:/UI/photo/lanPrinterSource/downArrow.svg":"qrc:/UI/photo/lanPrinterSource/downArrow.svg"
    property var img_closeBtn: Constants.currentTheme? "qrc:/UI/photo/lanPrinterSource/closeBtn_light.svg":"qrc:/UI/photo/lanPrinterSource/closeBtn_dark.svg"
    property var deviceItem: ""
    // width:932 * screenScaleFactor
    // height:400* screenScaleFactor
    ColumnLayout {
        spacing: 0
        id: mateialItem
        visible:!!deviceItem.materialBoxList
        property bool showPopup: true
        property string currentUnit: "10"
        anchors.fill:parent
        CusRoundedBg {
            leftTop: true
            rightTop: true
            clickedable: false
            borderWidth: 1
            borderColor: background_border
            color: title_background_color
            radius: 5

            Layout.fillWidth: true
            Layout.minimumWidth: 626 * screenScaleFactor
            Layout.maximumWidth: 932 * screenScaleFactor
            Layout.preferredHeight: 40 * screenScaleFactor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20 * screenScaleFactor
                anchors.rightMargin: 20 * screenScaleFactor
                spacing: 10 * screenScaleFactor

                Image {
                    Layout.preferredWidth: 11 * screenScaleFactor
                    Layout.preferredHeight: 14 * screenScaleFactor
                    source: "qrc:/UI/photo/lanPrinterSource/material.svg"
                }

                Text {
                    Layout.preferredWidth: contentWidth * screenScaleFactor
                    Layout.preferredHeight: 14 * screenScaleFactor

                    verticalAlignment: Text.AlignVCenter
                    font.weight: Font.Bold
                    font.family: Constants.mySystemFont.name
                    font.pointSize: Constants.labelFontPointSize_9
                    text: qsTr("耗材")
                    color: text_normal_color
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Item {
                    Layout.preferredWidth: 20 * screenScaleFactor
                    Layout.preferredHeight: 12 * screenScaleFactor

                    Image {
                        anchors.centerIn: parent
                        width: 10 * screenScaleFactor
                        height: 6 * screenScaleFactor
                        source: mateialItem.showPopup ? img_upArrow : img_downArrow
                    }

                    MouseArea {
                        hoverEnabled: true
                        anchors.fill: parent
                        cursorShape: containsMouse?Qt.PointingHandCursor:Qt.ArrowCursor

                        onClicked: {
                            mateialItem.showPopup = !mateialItem.showPopup
                            materialPopup.visible = mateialItem.showPopup
                        }
                    }
                }
            }
        }


        CusRoundedBg {
            id: materialPopup
            visible: true
            leftBottom: true
            rightBottom: true
            clickedable: false
            borderWidth: 1
            borderColor:background_border
            color: popup_background_color
            radius: 5

            Layout.fillWidth: true
            Layout.minimumWidth: 626 * screenScaleFactor
            Layout.maximumWidth: 932 * screenScaleFactor
            implicitHeight: 330* screenScaleFactor //colLayoutItem.height + 30 * screenScaleFactor


            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: parent.forceActiveFocus()
            }
            Rectangle
            {
                color: "transparent"
                anchors.fill:parent
                ColumnLayout{
                    anchors.left:parent.left
                    anchors.leftMargin:60 * screenScaleFactor
                    anchors.top:parent.top
                    anchors.topMargin:60 * screenScaleFactor
                    RowLayout{
                        id:boxList
                        spacing:10 * screenScaleFactor
                        Rectangle{
                            color: "#37373B"
                            radius:5 * screenScaleFactor
                            border.width:1
                            border.color: "#515157"
                            width:89* screenScaleFactor
                            height:209* screenScaleFactor
                            ColumnLayout{
                                anchors.fill:parent
                                Text {
                                    id:boxName
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignHCenter
                                    anchors.top:parent.top
                                    anchors.topMargin:40* screenScaleFactor
                                    color: text_normal_color
                                    text: qsTr("料架")
                                    anchors.horizontalCenter:parent.horizontalCenter
                                }
                                Rectangle{
                                    id:materialBox
                                    width:57* screenScaleFactor
                                    height:57* screenScaleFactor
                                    radius:height/2
                                    color: "#505156"
                                    border.width:2
                                    border.color: "#686870"
                                    anchors.top:boxName.bottom
                                    anchors.topMargin:8* screenScaleFactor
                                    anchors.horizontalCenter:parent.horizontalCenter
                                    Rectangle{
                                        visible:deviceItem.materialBoxList.rack
                                        width:57* screenScaleFactor
                                        height:57* screenScaleFactor
                                        color:deviceItem.materialBoxList.color
                                        anchors.centerIn:parent
                                        radius:height/2
                                        Rectangle{
                                            anchors.centerIn:parent
                                            width:13
                                            height:13
                                            radius:height/2
                                            color: "#37373B"
                                        }
                                    }
                                    Text {
                                        visible:!deviceItem.materialBoxList.rack
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_12
                                        color: text_normal_color
                                        text: qsTr("/")
                                        anchors.centerIn:parent
                                    }
                                }



                                Text {
                                    id:rackTypeId
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    horizontalAlignment: Text.AlignHCenter
                                    anchors.top:materialBox.bottom
                                    anchors.topMargin:18* screenScaleFactor
                                    color: text_normal_color
                                    text: deviceItem.materialBoxList.type
                                    anchors.horizontalCenter:parent.horizontalCenter
                                }

                                Image{
                                    visible:deviceItem.materialBoxList.rack
                                    anchors.top:rackTypeId.bottom
                                    anchors.topMargin:14* screenScaleFactor
                                    width:21* screenScaleFactor
                                    height:16* screenScaleFactor
                                    source: root.editBoxMsg
                                    sourceSize:Qt.size(width,height)
                                    anchors.horizontalCenter:parent.horizontalCenter
                                    MouseArea{
                                        anchors.fill:parent
                                        cursorShape:Qt.PointingHandCursor
                                        onClicked: {
                                            boxMap.materialItem={
                                                materialType:deviceItem.materialBoxList.type,
                                                materialColor:deviceItem.materialBoxList.color,
                                                name:deviceItem.materialBoxList.name,
                                                vendor:deviceItem.materialBoxList.vendor,
                                                minTemp:deviceItem.materialBoxList.minTemp,
                                                maxTemp:deviceItem.materialBoxList.maxTemp,
                                                pressure:deviceItem.materialBoxList.pressure,

                                            }
                                            materialMsg.visible = true
                                        }
                                    }
                                }

                            }
                        }
                        Rectangle{
                            id:boxMap
                            color: "#37373B"
                            radius:5 * screenScaleFactor
                            border.width:1
                            border.color: "#515157"
                            height:209*screenScaleFactor
                            width:353*screenScaleFactor
                            clip:true
                            property int curBoxId;
                            property int curMaterialId;
                            property var materialItem:null
                            ColumnLayout{
                                anchors.fill:parent
                                RowLayout{
                                    Layout.fillWidth:true
                                    Layout.preferredHeight:parent.height-30* screenScaleFactor
                                    Repeater{
                                        model:deviceItem.materialBoxList
                                        delegate: Rectangle{
                                            color: "#37373B"
                                            width:80* screenScaleFactor
                                            height:204* screenScaleFactor
                                            MouseArea{
                                                anchors.fill:parent
                                                onClicked: {
                                                    deviceItem.materialBoxList.getMaterialSelected(materialBoxId,materialId)
                                                    boxMap.curBoxId = materialBoxId
                                                    boxMap.curMaterialId = materialId
                                                }
                                            }
                                            ColumnLayout{
                                                anchors.fill:parent
                                                Image{
                                                    id:boxName
                                                    anchors.top:parent.top
                                                    anchors.topMargin:27* screenScaleFactor
                                                    width:28* screenScaleFactor
                                                    height:28* screenScaleFactor
                                                    source:selected? root.materialBoxUpdate:root.materialBoxUpdate_disable
                                                    sourceSize:Qt.size(width,height)
                                                    anchors.horizontalCenter:parent.horizontalCenter
                                                    Behavior on rotation {
                                                        PropertyAnimation {
                                                            duration: 500
                                                            from: 0
                                                            to: 360
                                                        }
                                                    }
                                                    MouseArea{
                                                        anchors.fill:parent
                                                        cursorShape:Qt.PointingHandCursor
                                                        onClicked:{
                                                            boxName.rotation = 0;
                                                            boxName.rotation = 360;
                                                            deviceItem.materialBoxList.refreshBox(deviceItem.pcIpAddr,materialBoxId,materialId)
                                                        }
                                                    }

                                                }
                                                Text {
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_8
                                                    color: selected ? "#FFFFFF": "#999999"
                                                    text:  materialBoxId+["A","B","C", "D"][materialId]
                                                    anchors.left:boxName.left
                                                    anchors.leftMargin:boxName.width/4
                                                    anchors.top:boxName.top
                                                    anchors.topMargin:boxName.height/4

                                                }
                                                Rectangle{
                                                    id:materialBox
                                                    width:57* screenScaleFactor
                                                    height:57* screenScaleFactor
                                                    radius:height/2
                                                    color: "#505156"
                                                    border.width:2
                                                    border.color:  selected ? "#FFFFFF": "#999999"
                                                    anchors.top:boxName.bottom
                                                    anchors.topMargin:8* screenScaleFactor
                                                    anchors.horizontalCenter:parent.horizontalCenter
                                                    Rectangle{
                                                        visible:false
                                                        width:parent.width
                                                        height:width
                                                        color: "#1DAEFF"
                                                        anchors.centerIn:parent
                                                        radius:height/2
                                                        Rectangle{
                                                            anchors.centerIn:parent
                                                            width:13
                                                            height:13
                                                            radius:height/2
                                                            color: "#37373B"
                                                        }
                                                    }
                                                    Rectangle{
                                                        visible:true
                                                        width: (+percent/100)*57* screenScaleFactor
                                                        height:width
                                                        color: materialColor
                                                        anchors.centerIn:parent
                                                        radius:height/2
                                                        Rectangle{
                                                            anchors.centerIn:parent
                                                            width:13
                                                            height:13
                                                            radius:height/2
                                                            color: "#37373B"
                                                        }
                                                    }
                                                    Text {
                                                        visible:false
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_12
                                                        color: text_normal_color
                                                        text: qsTr("/")
                                                        anchors.centerIn:parent
                                                    }

                                                    Text {
                                                        visible:false
                                                        font.weight: Font.Bold
                                                        font.family: Constants.mySystemFont.name
                                                        font.pointSize: Constants.labelFontPointSize_12
                                                        color: text_normal_color
                                                        text: qsTr("?")
                                                        anchors.centerIn:parent
                                                    }

                                                }
                                                Canvas {
                                                    anchors.top: materialBox.bottom
                                                    anchors.topMargin: 4 * screenScaleFactor
                                                    anchors.horizontalCenter:parent.horizontalCenter
                                                    visible: used
                                                    width: 14 * screenScaleFactor
                                                    height: 8 * screenScaleFactor

                                                    contextType: "2d"

                                                    onPaint: {
                                                        const context = getContext("2d")
                                                        context.clearRect(0, 0, width, height);
                                                        context.beginPath();
                                                        if (!root.down) {
                                                            context.moveTo(0, 0);
                                                            context.lineTo(width, 0);
                                                            context.lineTo(width / 2, height);
                                                        } else {
                                                            context.moveTo(0, height);
                                                            context.lineTo(width, height);
                                                            context.lineTo(width / 2, 0);
                                                        }
                                                        context.closePath();
                                                        context.fillStyle = materialColor
                                                        context.fill();
                                                    }
                                                }
                                                Text {
                                                    id:materialName
                                                    font.weight: Font.Bold
                                                    font.family: Constants.mySystemFont.name
                                                    font.pointSize: Constants.labelFontPointSize_9
                                                    horizontalAlignment: Text.AlignHCenter
                                                    anchors.top:materialBox.bottom
                                                    anchors.topMargin:18* screenScaleFactor
                                                    color:  selected ? "#FFFFFF": "#999999"
                                                    text: materialType
                                                    anchors.horizontalCenter:parent.horizontalCenter
                                                }
                                                Image{
                                                    visible:+model.state == 2
                                                    anchors.top:materialName.bottom
                                                    anchors.topMargin:14* screenScaleFactor
                                                    width:21* screenScaleFactor
                                                    height:16* screenScaleFactor
                                                    source: root.showBoxMsg
                                                    sourceSize:Qt.size(width,height)
                                                    anchors.horizontalCenter:parent.horizontalCenter
                                                    MouseArea{
                                                        anchors.fill:parent
                                                        cursorShape:Qt.PointingHandCursor
                                                        onClicked: {
                                                            boxMap.materialItem={
                                                                materialBoxId,
                                                                materialId,
                                                                materialType,
                                                                materialColor,
                                                                name,
                                                                vendor,
                                                                minTemp,
                                                                maxTemp,
                                                                pressure,
                                                                rfid,
                                                                state:model.state
                                                            }
                                                            materialMsg.visible = true
                                                        }
                                                    }
                                                }

                                                Image{
                                                    visible:+model.state== 1
                                                    anchors.top:materialName.bottom
                                                    anchors.topMargin:14* screenScaleFactor
                                                    width:21* screenScaleFactor
                                                    height:16* screenScaleFactor
                                                    source: root.editBoxMsg
                                                    sourceSize:Qt.size(width,height)
                                                    anchors.horizontalCenter:parent.horizontalCenter
                                                    MouseArea{
                                                        anchors.fill:parent
                                                        cursorShape:Qt.PointingHandCursor
                                                        onClicked: {
                                                            boxMap.materialItem={
                                                                materialBoxId,
                                                                materialId,
                                                                materialType,
                                                                materialColor,
                                                                name,
                                                                vendor,
                                                                minTemp,
                                                                maxTemp,
                                                                pressure,
                                                                rfid,
                                                                state:model.state
                                                            }
                                                            materialMsg.visible = true
                                                        }
                                                    }
                                                }

                                            }
                                        }
                                    }
                                }

                                RowLayout{
                                    spacing:10 * screenScaleFactor
                                    Layout.alignment:Qt.AlignRight
                                    Rectangle{
                                        Layout.preferredHeight:22 * screenScaleFactor
                                        Layout.preferredWidth: 44 * screenScaleFactor
                                        radius:5
                                        color: "#515157"
                                        Text{
                                            anchors.centerIn:parent
                                            text:"AUTO"
                                            color: Constants.themeGreenColor
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_10
                                        }
                                    }
                                    CusRoundedBg{
                                        Layout.preferredHeight:22 * screenScaleFactor
                                        Layout.preferredWidth: 44 * screenScaleFactor
                                        rightBottom:true
                                        color: "#515157"
                                        radius: 5* screenScaleFactor
                                        Layout.alignment:Qt.AlignRight
                                        RowLayout{
                                            Layout.fillWidth:true
                                            Layout.fillHeight:true
                                            property var humidity:+deviceItem.materialBoxList.humidity
                                            Image{
                                                width:11* screenScaleFactor
                                                height:14* screenScaleFactor
                                                source:{
                                                    if(parent.humidity>=0&&parent.humidity<39) { return humidity_normal }
                                                    if(parent.humidity>=40&&parent.humidity<59) { return humidity_height }
                                                    if(parent.humidity>=59&&parent.humidity<=100) { return humidity_heightest }
                                                }
                                                Layout.leftMargin:8* screenScaleFactor
                                                Layout.topMargin: 3* screenScaleFactor
                                            }
                                            Text{
                                                text: parent.humidity
                                                color: Constants.themeGreenColor
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_8
                                                Layout.topMargin: 3* screenScaleFactor
                                            }
                                        }
                                        MouseArea{
                                            anchors.fill:parent
                                            onClicked:humidityMsg.visible=true
                                        }
                                    }
                                }


                            }

                        }
                        ColumnLayout{
                            anchors.left:boxMap.right
                            anchors.leftMargin:10 * screenScaleFactor
                            anchors.top:parent.top
                            anchors.topMargin:10* screenScaleFactor
                            spacing:20 * screenScaleFactor
                            Repeater{
                                model:listBox
                                delegate:ColumnLayout{
                                    spacing:4 * screenScaleFactor
                                    Canvas {
                                        anchors.top: materialBox.bottom
                                        anchors.topMargin: 4 * screenScaleFactor

                                        visible:current === 3
                                        width: 11 * screenScaleFactor
                                        height: 6 * screenScaleFactor
                                        contextType: "2d"
                                        onPaint: {
                                            const context = getContext("2d")
                                            context.clearRect(0, 0, width, height);
                                            context.beginPath();
                                            if (!root.down) {
                                                context.moveTo(0, 0);
                                                context.lineTo(width, 0);
                                                context.lineTo(width / 2, height);
                                            } else {
                                                context.moveTo(0, height);
                                                context.lineTo(width, height);
                                                context.lineTo(width / 2, 0);
                                            }
                                            context.closePath();
                                            context.fillStyle = "#FFFFFF"
                                            context.fill();
                                        }
                                    }
                                    Rectangle{
                                        width:62 * screenScaleFactor
                                        height:24 * screenScaleFactor
                                        color:"#37373B"
                                        radius:5 * screenScaleFactor
                                        border.width: 1 * screenScaleFactor
                                        border.color:isHover? Constants.themeGreenColor :"#515157"
                                        property bool isHover:false
                                        MouseArea{
                                            anchors.fill:parent
                                            hoverEnabled:true
                                            onEntered:parent.isHover = true
                                            onExited:parent.isHover = false
                                        }
                                        RowLayout{
                                            anchors.fill:parent
                                            spacing: 3 * screenScaleFactor
                                            anchors.left:parent.left
                                            anchors.leftMargin:3* screenScaleFactor
                                            anchors.right:parnet.right
                                            anchors.rightMargin:3* screenScaleFactor
                                            property var colorModel
                                            Component.onCompleted:{
                                                colorModel = color.split(",")
                                            }
                                            Repeater{
                                                model:parent.colorModel
                                                delegate:Rectangle{
                                                    width:10 * screenScaleFactor
                                                    height:width
                                                    radius:width/2
                                                    color:modelData
                                                }

                                            }
                                        }
                                    }
                                }
                            }

                        }

                    }
                    RowLayout{

                        anchors.top:boxList.bottom
                        anchors.topMargin:10* screenScaleFactor
                        RowLayout{
                            spacing:5 * screenScaleFactor
                            Layout.alignment:Qt.AlignTop
                            Rectangle{
                                width:10 * screenScaleFactor
                                height:width
                                color: Constants.themeGreenColor
                                radius:height/2
                            }
                            Text {
                                font.weight: Font.Bold
                                font.family: Constants.mySystemFont.name
                                font.pointSize: Constants.labelFontPointSize_9
                                color: text_normal_color
                                text: feedState[deviceItem.feedState]
                            }
                        }
                        Item{
                            Layout.fillWidth:true
                        }
                        RowLayout{
                            spacing:10 * screenScaleFactor
                            CusImglButton{
                                Layout.preferredWidth: 48 * screenScaleFactor
                                Layout.preferredHeight: 32 * screenScaleFactor
                                imgWidth: 26 * screenScaleFactor
                                imgHeight: 24 * screenScaleFactor
                                defaultBtnBgColor: "transparent"
                                hoveredBtnBgColor: "transparent"
                                selectedBtnBgColor: "transparent"
                                opacity: 1
                                borderWidth: 0
                                state: "imgOnly"
                                enabledIconSource: root.materialBoxSetting
                                pressedIconSource:root.materialBoxSetting
                                highLightIconSource: root.materialBoxSetting
                                onClicked:boxMsg.visible = true

                            }
                            BasicDialogButton {
                                Layout.preferredWidth: 80*screenScaleFactor
                                Layout.preferredHeight: 32*screenScaleFactor
                                text: qsTr("指南")
                                btnRadius:5*screenScaleFactor
                                btnBorderW:0
                                defaultBtnBgColor:"#515157"
                                hoveredBtnBgColor: Qt.lighter(defaultBtnBgColor,1.2)
                                onSigButtonClicked:deviceItem.materialBoxList.toMultiColorGuide()
                            }
                            BasicDialogButton {
                                Layout.preferredWidth: 80*screenScaleFactor
                                Layout.preferredHeight: 32*screenScaleFactor
                                text: qsTr("进料")
                                btnRadius:5*screenScaleFactor
                                btnBorderW:0
                                enabled:deviceItem.feedState==0
                                defaultBtnBgColor: Constants.themeGreenColor
                                hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                onSigButtonClicked:{
                                    deviceItem.materialBoxList.feedInOrOutMaterial(deviceItem.pcIpAddr,boxMap.curBoxId,boxMap.curMaterialId,"1")
                                }

                            }
                            BasicDialogButton {
                                Layout.preferredWidth: 80*screenScaleFactor
                                Layout.preferredHeight: 32*screenScaleFactor
                                text: qsTr("退料")
                                enabled:deviceItem.feedState==0
                                btnRadius:5*screenScaleFactor
                                btnBorderW:0
                                defaultBtnBgColor: Constants.themeGreenColor
                                hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                onSigButtonClicked:{
                                    deviceItem.materialBoxList.feedInOrOutMaterial(deviceItem.pcIpAddr,boxMap.curBoxId,boxMap.curMaterialId,"0")
                                }

                            }
                        }
                    }
                }
                Rectangle{
                    id:materialMsg
                    anchors.fill:parent
                    color:popup_background_color
                    visible:false
                    property var materialItem:null
                    property var materialChangeData:{}
                    onVisibleChanged:{
                        if(visible){
                            materialMsg.materialItem = boxMap.materialItem
                            materialMsg.materialChangeData = JSON.parse(JSON.stringify(boxMap.materialItem))
                        }
                    }
                    Rectangle{
                        width:524* screenScaleFactor
                        height:255* screenScaleFactor
                        color: "#37373B"
                        radius:5* screenScaleFactor
                        border.width:1
                        border.color: "#515157"
                        anchors.left:parent.left
                        anchors.leftMargin:60 * screenScaleFactor
                        anchors.top:parent.top
                        anchors.topMargin:35 * screenScaleFactor
                        ColumnLayout{
                            spacing:5 * screenScaleFactor
                            anchors.fill:parent
                            RowLayout{
                                Layout.fillWidth:true
                                Layout.topMargin:10 * screenScaleFactor
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "品牌"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                Text {
                                    Layout.rightMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text:  materialMsg.materialItem.vendor || "--"
                                }
                            }
                            RowLayout{
                                Layout.fillWidth:true
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "类型"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                Text {
                                    Layout.rightMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text:  materialMsg.materialItem.materialType || "--"
                                }
                            }
                            RowLayout{
                                Layout.fillWidth:true
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "名称"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                Text {
                                    Layout.rightMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text:  materialMsg.materialItem.name || "--"
                                }
                            }
                            RowLayout{
                                Layout.fillWidth:true
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "颜色"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                Rectangle{
                                    id:materialColorId
                                    width:40
                                    height:20
                                    radius:height/2
                                    color:materialMsg.materialItem.materialColor
                                    Layout.rightMargin:20 * screenScaleFactor
                                    MouseArea{
                                        anchors.fill:parent
                                        onClicked:{
                                            if(+materialMsg.materialItem.state !== 1) return;
                                            var color = kernel_kernelui.showColorDialog(materialMsg.materialItem.materialColor, frameLessView);
                                            materialColorId.color = color
                                            materialMsg.materialChangeData.materialColor = color
                                        }
                                    }
                                }
                            }

                            RowLayout{
                                Layout.fillWidth:true
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "喷嘴温度"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                Text {
                                    Layout.rightMargin:20 * screenScaleFactor
                                    visible:+materialMsg.materialItem.state !== 1
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: materialMsg.materialItem.minTemp+"~"+materialMsg.materialItem.maxTemp
                                }
                                RowLayout{
                                    Layout.rightMargin:20 * screenScaleFactor
                                    visible:+materialMsg.materialItem.state === 1
                                    BasicDialogTextField{
                                        id:minTempId
                                        Layout.preferredWidth: 50* screenScaleFactor
                                        Layout.preferredHeight: 20* screenScaleFactor
                                        radius: 5* screenScaleFactor
                                        text:materialMsg.materialItem.minTemp
                                        unitChar: qsTr("°C")
                                        onTextChanged:materialMsg.materialChangeData.minTemp = text

                                    }
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: text_normal_color
                                        text: "~"
                                    }
                                    BasicDialogTextField{
                                        id:maxTempId
                                        Layout.preferredWidth: 50* screenScaleFactor
                                        Layout.preferredHeight: 20* screenScaleFactor
                                        radius: 5* screenScaleFactor
                                        text:materialMsg.materialItem.maxTemp
                                        unitChar:  qsTr("°C")
                                        onTextChanged:materialMsg.materialChangeData.maxTemp = text
                                    }
                                }
                            }

                            RowLayout{
                                Layout.fillWidth:true
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "压力提前距离"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                Text {
                                    visible:+materialMsg.materialItem.state !== 1
                                    Layout.rightMargin:20 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text:  materialMsg.materialItem.pressure || "--"
                                }
                                BasicDialogTextField{
                                    id:pressureId
                                    Layout.rightMargin:20 * screenScaleFactor
                                    visible:+materialMsg.materialItem.state === 1
                                    Layout.preferredWidth: 50* screenScaleFactor
                                    Layout.preferredHeight: 20* screenScaleFactor
                                    radius: 5* screenScaleFactor
                                    text:materialMsg.materialItem.pressure
                                    onTextChanged:materialMsg.materialChangeData.pressure = text
                                }
                            }

                            RowLayout{
                                Layout.fillWidth:true
                                Text {
                                    Layout.leftMargin:20 * screenScaleFactor
                                    visible:+materialMsg.materialItem.state !== 1
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: "#CBCBCC"
                                    text: "RFID耗材信息不支持修改"
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                RowLayout{
                                    // Layout.preferredWidth:150*screenScaleFactor
                                    Layout.rightMargin:20 * screenScaleFactor
                                    BasicDialogButton {
                                        Layout.preferredWidth: 50*screenScaleFactor
                                        Layout.preferredHeight: 28*screenScaleFactor
                                        text: qsTr("重置")
                                        visible:+materialMsg.materialItem.state === 1
                                        btnRadius:5*screenScaleFactor
                                        btnBorderW:0
                                        defaultBtnBgColor: Constants.themeGreenColor
                                        hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                        onSigButtonClicked: {
                                            minTempId.text = boxMap.materialItem.minTemp
                                            maxTempId.text = boxMap.materialItem.maxTemp
                                            pressureId.text= boxMap.materialItem.pressure
                                            materialColorId.color = boxMap.materialItem.materialColor
                                        }

                                    }
                                    BasicDialogButton {
                                        Layout.preferredWidth: 50*screenScaleFactor
                                        Layout.preferredHeight: 28*screenScaleFactor
                                        text: qsTr("返回")
                                        btnRadius:5*screenScaleFactor
                                        btnBorderW:0
                                        defaultBtnBgColor: Constants.themeGreenColor
                                        hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                        onSigButtonClicked: {
                                            if(+materialMsg.materialItem.state === 1){
                                                let obj = {}
                                                obj["boxId"] = +materialMsg.materialChangeData.materialBoxId
                                                obj["id"] =  +materialMsg.materialChangeData.materialId
                                                obj["vendor"] =  materialMsg.materialChangeData.vendor
                                                obj["type"] =  materialMsg.materialChangeData.materialType
                                                obj["name"] =  materialMsg.materialChangeData.name
                                                obj["color"] =  materialMsg.materialChangeData.materialColor
                                                obj["minTemp"] =  +materialMsg.materialChangeData.minTemp
                                                obj["maxTemp"] = + materialMsg.materialChangeData.maxTemp
                                                obj["pressure"] = +materialMsg.materialChangeData.pressure
                                                obj["rfid"] =   materialMsg.materialChangeData.rfid
                                                deviceItem.materialBoxList.modifyMaterial(deviceItem.pcIpAddr,obj)
                                            }
                                            materialMsg.visible = false
                                        }

                                    }

                                }
                            }
                        }
                    }
                }

                Rectangle{
                    id:boxMsg
                    anchors.fill:parent
                    color:popup_background_color
                    visible:false
                    property var autoRefill:false
                    property var cAutoFeed:false
                    property var cSelfTest:false
                    property var cAutoUpdateFilament:false
                    Rectangle{
                        width:524* screenScaleFactor
                        height:255* screenScaleFactor
                        color: "#37373B"
                        radius:5* screenScaleFactor
                        border.width:1
                        border.color: "#515157"
                        anchors.left:parent.left
                        anchors.leftMargin:60 * screenScaleFactor
                        anchors.top:parent.top
                        anchors.topMargin:35 * screenScaleFactor
                        ColumnLayout{
                            spacing:8 * screenScaleFactor
                            anchors.fill:parent
                            ColumnLayout{

                                Layout.leftMargin:20 * screenScaleFactor
                                Layout.rightMargin:20 * screenScaleFactor
                                Layout.topMargin:10 * screenScaleFactor
                                Text {
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "料盒设置"
                                }
                                Rectangle{
                                    Layout.fillWidth:true
                                    Layout.preferredHeight:1
                                    color: "#515157"
                                }

                            }
                            RowLayout{
                                Layout.fillWidth:true
                                ColumnLayout{
                                    Layout.leftMargin:20 * screenScaleFactor
                                    Layout.rightMargin:20 * screenScaleFactor
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: text_normal_color
                                        text: "插入时检测"
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: "#999999"
                                        text: "当插入新的料线时，系统自动读取料线信息，大约需要20秒。"
                                    }
                                }
                                Item{
                                    Layout.fillWidth:true
                                }

                                CusSwitchBtn{
                                    Layout.alignment:Qt.AlignRight
                                    Layout.rightMargin:20*screenScaleFactor
                                    Layout.preferredWidth: 50*screenScaleFactor
                                    Layout.preferredHeight: 28*screenScaleFactor
                                    btnHeight: 22*screenScaleFactor
                                    Layout.bottomMargin: 8*screenScaleFactor
                                    onCheckedChanged:boxMsg.cAutoFeed = checked
                                    onVisibleChanged: {
                                        if(visible) checked = !!+deviceItem.materialBoxList.cAutoFeed

                                        console.log(deviceItem.materialBoxList.cAutoFeed,"cAutoFeed")
                                    }
                                }

                            }
                            RowLayout{
                                Layout.fillWidth:true
                                ColumnLayout{
                                    Layout.leftMargin:20 * screenScaleFactor
                                    Layout.rightMargin:20 * screenScaleFactor
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: text_normal_color
                                        text: "开机时检测"
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color:"#999999"
                                        text: "每次开机时，系统自动读取料线信息，大约需要1分钟。"
                                    }
                                }
                                Item{
                                    Layout.fillWidth:true
                                }

                                CusSwitchBtn{
                                    Layout.alignment:Qt.AlignRight
                                    Layout.rightMargin:20*screenScaleFactor
                                    Layout.preferredWidth: 50*screenScaleFactor
                                    Layout.preferredHeight: 28*screenScaleFactor
                                    btnHeight: 22*screenScaleFactor
                                    Layout.bottomMargin: 8*screenScaleFactor
                                    onCheckedChanged:boxMsg.cSelfTest = checked
                                    onVisibleChanged: {
                                        if(visible) checked = !!+deviceItem.materialBoxList.cSelfTest
                                        console.log(deviceItem.materialBoxList.cSelfTest,"cSelfTest")
                                    }
                                }

                            }
                            RowLayout{
                                Layout.fillWidth:true
                                ColumnLayout{
                                    Layout.leftMargin:20 * screenScaleFactor
                                    Layout.rightMargin:20 * screenScaleFactor
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: text_normal_color
                                        text: "系统自动续料"
                                    }

                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color:"#999999"
                                        text: "料线用尽后将自动切换到属性完全相同的耗材。"
                                    }
                                }
                                Item{
                                    Layout.fillWidth:true
                                }

                                CusSwitchBtn{
                                    Layout.alignment:Qt.AlignRight
                                    Layout.rightMargin:20*screenScaleFactor
                                    Layout.preferredWidth: 50*screenScaleFactor
                                    Layout.preferredHeight: 28*screenScaleFactor
                                    btnHeight: 22*screenScaleFactor
                                    Layout.bottomMargin: 8*screenScaleFactor
                                    onCheckedChanged: boxMsg.autoRefill = checked
                                    onVisibleChanged: {
                                        if(visible) checked = !!+deviceItem.materialBoxList.autoRefill
                                    }
                                }

                            }
                            BasicDialogButton {
                                Layout.alignment:Qt.AlignRight
                                Layout.rightMargin:20*screenScaleFactor
                                Layout.bottomMargin:10*screenScaleFactor
                                Layout.preferredWidth: 80*screenScaleFactor
                                Layout.preferredHeight: 32*screenScaleFactor
                                text: qsTr("返回")
                                btnRadius:5*screenScaleFactor
                                btnBorderW:0
                                defaultBtnBgColor: Constants.themeGreenColor
                                hoveredBtnBgColor: Qt.lighter(Constants.themeGreenColor,1.2)
                                onSigButtonClicked:{
                                    deviceItem.materialBoxList.boxConfig(deviceItem.pcIpAddr,+boxMsg.autoRefill,+boxMsg.cAutoFeed,+boxMsg.cSelfTest)
                                    boxMsg.visible = false
                                }

                            }
                        }


                    }

                }

                Rectangle{
                    id:humidityMsg
                    anchors.fill:parent
                    color:popup_background_color
                    visible:false
                    property var autoRefill:false
                    property var cAutoFeed:false
                    property var cSelfTest:false
                    property var cAutoUpdateFilament:false
                    Rectangle{
                        width:524* screenScaleFactor
                        height:255* screenScaleFactor
                        color: "#37373B"
                        radius:5* screenScaleFactor
                        border.width:1
                        border.color: "#515157"
                        anchors.left:parent.left
                        anchors.leftMargin:60 * screenScaleFactor
                        anchors.top:parent.top
                        anchors.topMargin:35 * screenScaleFactor
                        ColumnLayout{
                            anchors.fill:parent

                            Image{
                                Layout.preferredHeight:10 * screenScaleFactor
                                Layout.preferredWidth:10 * screenScaleFactor
                                source:img_closeBtn

                                anchors.right:parent.right
                                anchors.rightMargin:10 * screenScaleFactor
                                anchors.top:parent.top
                                anchors.topMargin:10 * screenScaleFactor
                                MouseArea{
                                    anchors.fill:parent
                                    onClicked:humidityMsg.visible = false
                                }
                            }
                            RowLayout{
                                Layout.leftMargin:20 * screenScaleFactor
                                Layout.rightMargin:20 * screenScaleFactor
                                ColumnLayout{
                                    Layout.preferredHeight:30 * screenScaleFactor
                                    spacing:2
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: text_normal_color
                                        text: "CFS湿度"
                                    }
                                    RowLayout{
                                        Text {
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: "#999999"
                                            text: "范围说明（单位:"
                                        }
                                        ColumnLayout{
                                            spacing:0
                                            Text {
                                                color: "#999999"
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                text: "%"
                                            }
                                            Text {
                                                color: "#999999"
                                                font.weight: Font.Bold
                                                font.family: Constants.mySystemFont.name
                                                font.pointSize: Constants.labelFontPointSize_9
                                                text: "RH"
                                            }
                                        }
                                        Text {
                                            color: "#999999"
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            text: ")"
                                        }
                                    }
                                }
                                Item{
                                    Layout.fillWidth:true
                                }
                                RowLayout{
                                    Layout.rightMargin:45* screenScaleFactor
                                    property var humidity:+deviceItem.materialBoxList.humidity
                                    Text {
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_9
                                        color: text_normal_color
                                        text: "当前"
                                    }
                                    Image{
                                        width:11* screenScaleFactor
                                        height:14* screenScaleFactor
                                        source:{
                                            if(parent.humidity>=0&&parent.humidity<39) return humidity_normal
                                            if(parent.humidity>=40&&parent.humidity<59) return humidity_height
                                            if(parent.humidity>=59&&parent.humidity<=100) return humidity_heightest
                                        }
                                        Layout.leftMargin:8* screenScaleFactor
                                        Layout.topMargin: 3* screenScaleFactor
                                    }
                                    Text{
                                        text: parent.humidity
                                        color: Constants.themeGreenColor
                                        font.weight: Font.Bold
                                        font.family: Constants.mySystemFont.name
                                        font.pointSize: Constants.labelFontPointSize_8
                                        Layout.topMargin: 3* screenScaleFactor
                                    }
                                    ColumnLayout{
                                        Text {
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: text_normal_color
                                            text: "%"
                                        }
                                        Text {
                                            font.weight: Font.Bold
                                            font.family: Constants.mySystemFont.name
                                            font.pointSize: Constants.labelFontPointSize_9
                                            color: text_normal_color
                                            text: "RH"
                                        }
                                    }
                                }
                            }
                            RowLayout{
                                Text {
                                    Layout.leftMargin:64 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "正常"
                                }
                                Text {
                                    Layout.leftMargin:154 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "高"
                                }
                                Text {
                                    Layout.leftMargin:154 * screenScaleFactor
                                    font.weight: Font.Bold
                                    font.family: Constants.mySystemFont.name
                                    font.pointSize: Constants.labelFontPointSize_9
                                    color: text_normal_color
                                    text: "过高"
                                }
                            }
                            Image{
                                Layout.leftMargin:40 * screenScaleFactor
                                Layout.bottomMargin:50 * screenScaleFactor
                                Layout.preferredHeight:80 * screenScaleFactor
                                Layout.preferredWidth:439 * screenScaleFactor
                                source:humidity
                            }
                        }
                    }

                }
            }
        }
    }
}
