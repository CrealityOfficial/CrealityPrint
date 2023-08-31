import QtQuick 2.12
import QtQml 2.3
import QtQuick.Controls 2.5


ListView {
    id: control

    //按住shift时，连续多选的起点
    property int mulBegin: 0

    //截取超出部分
    clip: true
    //不要橡皮筋效果
    boundsBehavior: Flickable.StopAtBounds
    //Item间隔
    spacing: 2

    model: 20
    delegate: Rectangle{
        property bool chosed: fileData.isChosed
        id: item_delegate
        width: parent.width - 2
        height: 20 * screenScaleFactor
        color: fileData.isChosed ? checkedColor : "transparent"
        anchors.horizontalCenter: parent.horizontalCenter
        property color checkedColor: Constants.currentTheme ? "#B7E5FF" : "#739AB0"
        property string normalSource: "qrc:/UI/photo/modelLIstIcon.svg"

        Row {
            spacing: 6 * screenScaleFactor
            anchors.left: parent.left
            anchors.leftMargin: 10 * screenScaleFactor
            anchors.verticalCenter: parent.verticalCenter

            Image {
                width: 12* screenScaleFactor
                height: 12* screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: Constants.currentTheme ? normalSource : (fileData.isChosed ? "qrc:/UI/photo/modelLIstIcon_h.svg" : normalSource)
            }

            Text {
                width: 190 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                verticalAlignment: Text.AlignVCenter
                font.family: Constants.mySystemFont.name
                font.pointSize: Constants.labelFontPointSize_9
                color: fileData.isChosed ? (Constants.currentTheme ? Constants.themeTextColor : "white") : Constants.themeTextColor
                text: fileName
                elide: Text.ElideMiddle
            }

            Item{
                width: 100 * screenScaleFactor
            }
        }

        MouseArea{
            id: item_mousearea
            anchors.fill: parent

            Image{//放在mouseArea里面防止鼠标事件覆盖
                id: idAreaImage
                property string visImg: "qrc:/UI/photo/modelVis_normal.svg"
                property string noVisImg: "qrc:/UI/photo/modelNoVis_normal.svg"
                property string imgPath: fileData.isVisible ? visImg : noVisImg
                anchors.right: parent.right
                anchors.rightMargin: 16 * screenScaleFactor
                anchors.verticalCenter: parent.verticalCenter
                source: imgPath
                MouseArea{
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        control.model.setItemVisible(index, !fileVisible);
                    }
                    onEntered: {
                        idAreaImage.visImg = "qrc:/UI/photo/modelVis_d.svg"
                        idAreaImage.noVisImg = "qrc:/UI/photo/modelNoVis_d.svg"
                    }
                    onExited: {
                        idAreaImage.visImg = "qrc:/UI/photo/modelVis_normal.svg"
                        idAreaImage.noVisImg = "qrc:/UI/photo/modelNoVis_normal.svg"
                    }
                }
            }

            onClicked: {
                //ctrl+多选，shift+连选，默认单选
                switch(mouse.modifiers){
                case Qt.ControlModifier:
                    if(!fileData.File_Checked)
                        control.model.checkAppendOne(index);
                    break;
                case Qt.ShiftModifier:
                    control.model.checkRange(control.mulBegin, index);
                    break;
                default:
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
