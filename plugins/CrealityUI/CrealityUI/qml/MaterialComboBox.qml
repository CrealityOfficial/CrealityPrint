import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/CrealityUI/qml"
Rectangle{
    id:root
    property int curIndex:0
    property var materialmodel;
    property var pcPrinterID ;
    property var mapGcodeType:"";
    property string materialDisableBg: "qrc:/UI/photo/upload3mf/materialDisableBg.png"
    color:"transparent"
    function setTextColor(color) {
        let result = "";
        let bgColor = [color.r * 255, color.g * 255, color.b * 255];
        let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
        let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
        if (whiteContrast > blackContrast)
            result = "#ffffff";
        else
            result = "#000000";
        return result;
    }

    Rectangle {
        id: mapId
        property var extruderColor: materialmodel.getMaterialColor(pcPrinterID,root.curIndex)
        width:parent.width
        height: parent.height
        color:curIndex!==-1  ? extruderColor:"#707070"
        radius: 5 * screenScaleFactor
        border.width: curIndex === -1?1:0
        border.color:isHover?Constants.themeGreenColor:"#373739"
        property var isHover:false
        MouseArea {
            anchors.fill: parent
            hoverEnabled:true
            onEntered:parent.isHover=true
            onExited:parent.isHover = false
            onClicked:materialPopup.visible = true
        }

        CusText{
             anchors.centerIn: parent
             fontPointSize: Constants.labelFontPointSize_9
             fontText:"--"
             visible: curIndex===-1
             fontColor:"#ffffff"
        }


        Row {
            visible: curIndex!==-1
            anchors.fill: parent
            anchors.leftMargin: 7 * screenScaleFactor
            spacing: 7 * screenScaleFactor
            Column {
                width: parent.width - 20 * screenScaleFactor
                height: parent.height
                spacing: 2 * screenScaleFactor

                CusText {
                    property int materialId:materialmodel.getMaterialId(pcPrinterID,root.curIndex)
                    property int boxId: materialmodel.getMaterialBoxId(pcPrinterID,root.curIndex)
                    fontText: boxId + ["A","B","C", "D"][materialId]
                    fontColor:  root.setTextColor(mapId.extruderColor)
                    fontPointSize: Constants.labelFontPointSize_9
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Rectangle {
                    width: 40 * screenScaleFactor
                    height: 1
                    color: root.setTextColor(mapId.extruderColor)
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                CusText {
                    property var materialState: materialmodel.getMaterialState(pcPrinterID,root.curIndex)
                    property var materialType: materialmodel.getMaterialType(pcPrinterID,root.curIndex)
                    fontText:{
                        switch(+materialState){
                            case 0:
                                return "/";
                            case 1:
                                return  !!materialType?materialType:"?";
                            case 2:
                                 return materialmodel.getMaterialType(pcPrinterID,root.curIndex);
                        }
                    }
                    fontColor: root.setTextColor(mapId.extruderColor)
                    fontPointSize: Constants.labelFontPointSize_9
                    anchors.horizontalCenter: parent.horizontalCenter
                }

            }

            Connections{
                target:root
                function onCurIndexChanged(){
                    arrowType.requestPaint()
                }
            }

            Canvas {
                id:arrowType
                anchors.verticalCenter: parent.verticalCenter
                width: 7 * screenScaleFactor
                height: 4 * screenScaleFactor
                contextType: "2d"
                readonly property color arrowColor: root.setTextColor(mapId.extruderColor)
                onArrowColorChanged: requestPaint()
                onPaint: {
                    const context = getContext("2d")
                    context.clearRect(0, 0, width, height);
                    context.beginPath();
                    context.moveTo(0, 0);
                    context.lineTo(width,0)
                    context.lineTo(width/2,height)
                    context.closePath();
                    context.fillStyle = arrowColor
                    context.fill();
                }
            }

            Item {
                width: 5 * screenScaleFactor
            }

        }

    }
    Popup{
        id:materialPopup
        width: 342 * screenScaleFactor
        height:flowLayout.height +50 * screenScaleFactor
        visible:false
        y:43 * screenScaleFactor
        background:Rectangle{
             anchors.fill: parent
             color:"transparent"
        }
        contentItem:Rectangle{
            anchors.fill: parent
            color: Constants.themeColor
            border.width:1
            border.color:"#262626"
            radius:5 * screenScaleFactor
            Column{
                width: parent.width
                spacing: 5 * screenScaleFactor
                y: 10 * screenScaleFactor
                Text {
                    id: tipText
                    text: qsTr("CFS slot selection")
                    color: Constants.textColor
                    font.pointSize: Constants.labelFontPointSize_9
                    font.family: Constants.labelFontFamilyBold
                    anchors.left: parent.left
                    anchors.leftMargin: 10* screenScaleFactor
                }
                Rectangle {
                    width: materialPopup.width - 30 * screenScaleFactor
                    height: flowLayout.height
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "transparent"
                    Flow{
                        id:flowLayout
                        width:parent.width
                        spacing:5
                        Repeater{
                            model:materialmodel
                            delegate: CusImglButton {
                                id:materialItem
                                property bool isDisable:+model.state === 0|| (+model.state === 1&& !materialType) || root.mapGcodeType !== materialType
                                width: 70 * screenScaleFactor
                                height: 43 * screenScaleFactor
                                defaultBtnBgColor:"transparent"
                                hoveredBtnBgColor: "transparent"
                                borderColor: "transparent"
                                hoverBorderColor:isDisable? "transparent" : Constants.themeGreenColor
                                selectedBtnBgColor: "transparent"
                                selectBorderColor:"transparent"

                                onClicked: {
                                    if(materialItem.isDisable) return
                                    root.curIndex = index
                                    materialPopup.visible = false

                                }
                                Rectangle {
                                    id: __mapId
                                    width: parent.width - 6 * screenScaleFactor
                                    height: parent.height - 6 * screenScaleFactor
                                    anchors.centerIn: parent
                                    color: materialColor
                                    radius: 5 * screenScaleFactor
                                    Image{
                                        anchors.fill:parent
                                        fillMode: Image.PreserveAspectCrop
                                        source:root.materialDisableBg
                                        visible:materialItem.isDisable

                                    }

                                    Column {
                                        anchors.fill: parent
                                        spacing: 2 * screenScaleFactor
                                        property var textColor:materialItem.isDisable?"#000000": root.setTextColor(materialColor)
                                        CusText {
                                            property int materialId:materialmodel.getMaterialId(pcPrinterID,index)
                                            property int boxId: materialmodel.getMaterialBoxId(pcPrinterID,index)

                                            fontText: boxId + ["A","B","C", "D"][materialId]
                                            // fontText: materialId
                                            fontColor:parent.textColor
                                            fontPointSize: Constants.labelFontPointSize_9
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        Rectangle {
                                            width: 60 * screenScaleFactor
                                            height: 1
                                            color: parent.textColor
                                            opacity:0.5
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        CusText {
                                            property var materialState: materialmodel.getMaterialState(pcPrinterID,index)
                                            fontText:{
                                                switch(+materialState){
                                                    case 0:
                                                        return "/";
                                                    case 1:
                                                        return materialType?materialType:"?";
                                                    case 2:
                                                         return materialType?materialType:"?";
                                                }
                                            }
                                            fontColor: parent.textColor
                                            fontPointSize: Constants.labelFontPointSize_9
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                    }

                                }

                            }
                        }
                    }

                }
            }

        }

    }

}
