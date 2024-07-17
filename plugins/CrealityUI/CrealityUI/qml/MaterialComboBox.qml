import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/CrealityUI/qml"
Rectangle{
    id:root
    property int curIndex:0
    property var materialmodel;
    property var pcPrinterID ;
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
        color:extruderColor
        radius: 5 * screenScaleFactor

        MouseArea {
            anchors.fill: parent
            onClicked: materialPopup.visible = true
        }

        Row {
            anchors.fill: parent

            ColumnLayout {
                width: parent.width - 15 * screenScaleFactor
                height: parent.height
                spacing: 2 * screenScaleFactor

                CusText {
                    fontText:  materialmodel.getMaterialId(pcPrinterID,root.curIndex)
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
                    fontText: materialmodel.getMaterialType(pcPrinterID,root.curIndex)
                    fontColor: root.setTextColor(mapId.extruderColor)
                    fontPointSize: Constants.labelFontPointSize_9
                    anchors.horizontalCenter: parent.horizontalCenter
                }

            }
            
            Canvas {
                anchors.verticalCenter: parent.verticalCenter
                width: 7 * screenScaleFactor
                height: 4 * screenScaleFactor
                contextType: "2d"
                onPaint: {
                const context = getContext("2d")
                context.clearRect(0, 0, width, height);
                context.beginPath();
                context.moveTo(0, 0);
                context.lineTo(width,0)
                context.lineTo(width/2,height)
                context.closePath();
                context.fillStyle = root.setTextColor(mapId.extruderColor)
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
        y:43

        contentItem:Rectangle{
            anchors.fill: parent
            color: Constants.themeColor
            border.width:1
            border.color:"#262626"
            radius:5
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
                                width: 70 * screenScaleFactor
                                height: 43 * screenScaleFactor
                                defaultBtnBgColor:"transparent"
                                hoveredBtnBgColor: "transparent"
                                borderColor: "transparent"
                                hoverBorderColor: Constants.themeGreenColor
                                selectedBtnBgColor: "transparent"
                                selectBorderColor: Constants.themeGreenColor
                                onClicked: {
                                    root.curIndex = index
                                    materialPopup.visible = false
                                   
                                }
                                Rectangle {
                                    id: mapId
                                    width: parent.width - 6 * screenScaleFactor
                                    height: parent.height - 6 * screenScaleFactor
                                    anchors.centerIn: parent
                                    color: materialColor
                                    radius: 5 * screenScaleFactor

                                    ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 2 * screenScaleFactor

                                        CusText {
                                            fontText: materialId
                                            fontColor: root.setTextColor(materialColor)
                                            fontPointSize: Constants.labelFontPointSize_9
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        Rectangle {
                                            width: 60 * screenScaleFactor
                                            height: 1
                                            color: root.setTextColor(materialColor)
                                            opacity:0.5
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }

                                        CusText {
                                            fontText: materialType
                                            fontColor: root.setTextColor(materialColor)
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
