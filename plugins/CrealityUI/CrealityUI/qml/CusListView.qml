import QtQuick 2.12
import QtQml 2.3
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.3


ListView {
    id: control

    //按住shift时，连续多选的起点
    property int mulBegin: 0
    property string delegateItemColor: Constants.themeColor_New
    property var colorsList:kernel_parameter_manager.currentMachineObject.extrudersModel
    property real leftRecWidth: 300 * screenScaleFactor


    signal sigClicke(var cIndex, var x, var y)
    //截取超出部分
    clip: true
    //不要橡皮筋效果
    boundsBehavior: Flickable.StopAtBounds
    //Item间隔
    spacing: 2 * screenScaleFactor
    delegate: Rectangle{
        property bool chosed: fileData.isChosed
        id: item_delegate
        visible: model.fileItemVisible
        width: control.width - 2
        height: 24 * screenScaleFactor
        color: fileData.isChosed ? delegateItemColor : "transparent"
        property string normalSource: "qrc:/UI/photo/modelLIstIcon.svg"

        MouseArea{
            id: item_mousearea
            anchors.fill: parent

            Item{
                id: leftItem
                width: control.leftRecWidth
                height: item_delegate.height

                Row {
                    spacing: 6 * screenScaleFactor
                    anchors.left: parent.left
                    anchors.leftMargin: 20 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter

                    Image{//放在mouseArea里面防止鼠标事件覆盖
                        id: idAreaImage
                        property string visImg: Constants.currentTheme? "qrc:/UI/photo/rightDrawer/update/show_light_default.svg" : "qrc:/UI/photo/modelVis_d.svg"
                        property string noVisImg: Constants.currentTheme? "qrc:/UI/photo/rightDrawer/update/hidden_light_default.svg" : "qrc:/UI/photo/modelNoVis_d.svg"
                        property string imgPath: fileData.isVisible ? visImg : noVisImg
                        sourceSize: Qt.size(12, 7)
                        source: imgPath
                        anchors.verticalCenter: parent.verticalCenter
                        //                        Layout.leftMargin: 25 * screenScaleFactor
                        MouseArea{
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                control.model.setItemVisible(index, !fileVisible);
                            }
                        }
                    }

                    CusText {
                        hAlign: 0
                        needRuningManual: true
                        fontWidth: leftItem.width - 40
                        font.family: Constants.mySystemFont.name
                        font.pointSize: Constants.labelFontPointSize_9
                        fontColor: fileData.isChosed ? (Constants.currentTheme ? Constants.themeTextColor : "white") : Constants.themeTextColor
                        fontText: fileName
                    }
                }
            }
            Item{
                width: item_delegate.width - leftItem.width
                height: item_delegate.height
                anchors.left: leftItem.right
                anchors.verticalCenter: leftItem.verticalCenter
                Rectangle{
                    id: colorPickBtn
                    property bool isHover: false
                    radius: 9*screenScaleFactor
                    border.width: fileData.isChosed ? 1*screenScaleFactor : 0*screenScaleFactor
                    border.color: "#ffffff"
                    width: 30*screenScaleFactor
                    height: 18*screenScaleFactor
                    anchors.left: parent.left
                    anchors.leftMargin: 10 *screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    color: fileData.modelColor
                    function setTextColor(baseColor) {
                        let bgColor = [baseColor.r * 255, baseColor.g * 255,baseColor.b * 255];
                        let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255]);
                        let blackContrast = Constants.getContrast(bgColor, [0, 0, 0]);
                        if (whiteContrast > blackContrast) {
                            console.log("color = ",  "#ffffff")
                            numText.color = "#ffffff";
                        } else {
                            console.log("color = ",  "#000000")
                            numText.color = "#000000";
                        }
                    }

                    Text{
                        id: numText
                        anchors.centerIn: parent
                        text: fileData.colorIndex
                        Component.onCompleted: {
                            colorPickBtn.setTextColor(fileData.modelColor)
                        }
                        onTextChanged: {
                            colorPickBtn.setTextColor(fileData.modelColor)
                        }
                    }
                    Popup {
                        property real modelIndex: 0
                        id:colorPic
                        width: colorListView.count *34*screenScaleFactor + 15*screenScaleFactor
                        height: 50*screenScaleFactor
                        x: colorPickBtn.x + width
                        y: colorPickBtn.y + colorPickBtn.height
                        contentItem: ListView{
                            id: colorListView
                            clip: true
                            spacing: 5*screenScaleFactor
                            model: colorsList
                            orientation: ListView.Horizontal
                            delegate: Rectangle{
                                id: rec
                                property bool isHover: false
                                width: 27*screenScaleFactor
                                height: 27*screenScaleFactor
                                radius:3*screenScaleFactor
                                color: !isHover ? "transparent" : Constants.themeGreenColor
                                function setBorderColor() {
                                    let bgColor = [model.extruderColor.r * 255, model.extruderColor.g * 255, model.extruderColor.b * 255];
                                    let whiteContrast = Constants.getContrast(bgColor, [Constants.right_panel_bgColor.r*255, Constants.right_panel_bgColor.g*255, Constants.right_panel_bgColor.b*255]);
                                    if(whiteContrast < 1.04){
                                        borderRec.border.width = 1 * screenScaleFactor
                                    }else{
                                        borderRec.border.width = 0 * screenScaleFactor
                                    }
                                }
                                Rectangle{
                                    id: borderRec
                                    anchors.fill: parent
                                    anchors.margins: 2*screenScaleFactor
                                    border.width: 1*screenScaleFactor
                                    border.color: "#000000"
                                    color: model.extruderColor
                                    radius:3*screenScaleFactor

                                    Text{
                                        anchors.centerIn: parent
                                        text: model.index + 1
                                        Component.onCompleted:  color = rec.setTextColor(parent.color)
                                    }
                                    Component.onCompleted: {
                                        setBorderColor()
                                    }
                                }

                                function setTextColor(color){
                                    let calcColor = ""
                                    let bgColor = [color.r * 255, color.g * 255, color.b * 255]
                                    let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
                                    let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
                                    if(whiteContrast > blackContrast)
                                        calcColor = "#ffffff"
                                    else
                                        calcColor =  "#000000"

                                    return calcColor
                                }

                                MouseArea{
                                    hoverEnabled: true
                                    anchors.fill: parent
                                    onClicked: {
                                        control.model.setColor(colorPic.modelIndex, model.extruderColor, model.index)
                                        colorPic.close()
                                    }
                                    onExited: {
                                        rec.isHover = false
                                    }
                                    onEntered: {
                                        rec.isHover = true
                                    }
                                }
                            }
                        }
                        background: Rectangle{
                            radius: 5 * screenScaleFactor
                            color:Constants.dialogItemRectBgColor
                            border.width:1 * screenScaleFactor
                            border.color: "#262626"
                        }
                        onVisibleChanged: {
                            if(visible){
                                colorListView.model = kernel_parameter_manager.currentMachineObject.extrudersModel
                            }
                        }
                    }
                    MouseArea{
                        hoverEnabled: true
                        anchors.fill: parent
                        onClicked: {
                            colorPic.x = mouse.x - colorPic.width
                            colorPic.y = colorPickBtn.y +25* screenScaleFactor// + colorPic.height// mouse.y + colorPic.height
                            colorPic.modelIndex = model.index
                            colorPic.visible = true
                            sigClicke(model.index, mouse.x, mouse.y)
                            console.log("+++Clicked = ",colorPic.x,mouse.x)
                        }
                        onExited: {
                            colorPickBtn.isHover = false
                        }
                        onEntered: {
                            colorPickBtn.isHover = true
                            console.log("+++index = ", model.index)
                        }
                    }
                }
                Row{
                    anchors.right: parent.right
                    anchors.rightMargin: 10 * screenScaleFactor
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10 * screenScaleFactor
                    enabled: kernel_kernel.currentPhase === 0

                    Image{
                        property string defaultImg : Constants.currentTheme == 1 ? "qrc:/UI/photo/cToolBar/support_light_default.svg" : "qrc:/UI/photo/leftBar/support_pressed.svg"
                        id: supportImage
                        visible: fileData.supportVisible
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.width: 18 * screenScaleFactor
                        sourceSize.height: 18 * screenScaleFactor
                        source: defaultImg
                        MouseArea{
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                control.model.checkOne(index)
                                topToolBar.switchMakeSupport()
                            }
                            onEntered: {
                                supportImage.source = "qrc:/UI/photo/leftBar/support_dark.svg"
                            }
                            onExited: {
                                supportImage.source = supportImage.defaultImg
                            }
                        }
                    }

                    Image{
                        property string defaultImg : Constants.currentTheme == 1 ? "qrc:/UI/photo/cToolBar/zDraw_light_default.svg" : "qrc:/UI/photo/leftBar/seam_painting_p.svg"
                        id: zImage
                        visible: fileData.zSeamVisible
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.width: 18 * screenScaleFactor
                        sourceSize.height: 18 * screenScaleFactor
                        source: defaultImg
                        MouseArea{
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                control.model.checkOne(index)
                                standaloneWindow.topToolBar.switchMakeZ()
                            }
                            onEntered: {
                                zImage.source = "qrc:/UI/photo/leftBar/seam_painting_n.svg"
                            }
                            onExited: {
                                zImage.source = zImage.defaultImg
                            }
                        }
                    }

                    Image{
                        property string defaultImg : Constants.currentTheme == 1 ?
                                                         "qrc:/UI/photo/cToolBar/color_light_default.svg" : "qrc:/UI/photo/colorMode.svg"
                        id: colorImage
                        visible: fileData.colors2Facets
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.width: 18 * screenScaleFactor
                        sourceSize.height: 18 * screenScaleFactor
                        source:defaultImg
                        MouseArea{
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                control.model.checkOne(index);
                                topToolBar.switchMakeColor()
                            }
                            onEntered: {
                                colorImage.source = "qrc:/UI/photo/colorMode_d.svg"
                            }
                            onExited: {
                                colorImage.source = colorImage.defaultImg
                            }
                        }
                    }

                    Image{
                        id: adaptiveLayerImage
                        property string defaultImg : Constants.currentTheme == 1 ?
                                                         "qrc:/UI/photo/leftBar/adaptivelayer_d.svg" :"qrc:/UI/photo/leftBar/adaptivelayer_p.svg"
                        visible: fileData.modelAdptiveLayer
                        anchors.verticalCenter: parent.verticalCenter
                        sourceSize.width: 14 * screenScaleFactor
                        sourceSize.height: 14 * screenScaleFactor
                        source: defaultImg
                        MouseArea{
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                control.model.checkOne(index);
                                topToolBar.switchMakeAdaptiveLayer()
                            }
                            onEntered: {
                                adaptiveLayerImage.source = "qrc:/UI/photo/leftBar/adaptivelayer_n.svg"
                            }
                            onExited: {
                                adaptiveLayerImage.source = adaptiveLayerImage.defaultImg
                            }
                        }
                    }
                }
            }

            onClicked: {
                //ctrl+多选，shift+连选，默认单选
                switch(mouse.modifiers){
                case Qt.ControlModifier:
                    if(!fileData.File_Checked)
                        control.model.sourceModelData.checkAppendOne(index);
                    break;
                case Qt.ShiftModifier:
                    control.model.sourceModelData.checkRange(control.mulBegin, index);
                    break;
                default:
                    console.log("+++++++control.mode = ", control.mode)
                    control.model.checkOne(index);
                    control.mulBegin=index;
                    break;
                }
            }
        }

    }

    ScrollBar.vertical: ScrollBar {
    }
}
