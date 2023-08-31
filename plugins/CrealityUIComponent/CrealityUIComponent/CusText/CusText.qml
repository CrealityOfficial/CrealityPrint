import QtQuick 2.9
import QtQuick.Controls 2.2
import "../"
import "../CusTip"
Rectangle{
    property real heightOffset: 0
    property real widthOffset: 0
    property int hAlign: 0 //0： 字体左对齐 1： 字体居中 2:右对齐
    property alias fontPointSize: idContentText.font.pointSize
    property alias fontWeight: idContentText.font.weight
    property alias fontFamily: idContentText.font.family
    property real fontWidth: 150
    property alias fontBold: idContentText.font.bold
    property alias fontHeight: idContentText.height
    property alias text: idContentText.text
    property alias fontColor: idContentText.color
    property alias fontOpacity: idContentText.opacity
    property alias fontWrapMode: idContentText.wrapMode
    property alias fontMaximumLineCount: idContentText.maximumLineCount
    property alias font_font: idContentText.font
    property alias fontSizeMode: idContentText.fontSizeMode
    property alias fontContentWidth: idContentText.contentWidth
    property alias font: idContentText.font
    property alias elide: idContentText.elide
    property bool needRuning: false
    property bool isDefault: !(elide !== Text.ElideNone || needRuning)

    id:recTxt
    height: idContentText.contentHeight + heightOffset//adapt some font
    width: isDefault ? idContentText.contentWidth : fontWidth
    clip: true
    color: "transparent"
    opacity: enabled ? 1 : 0.7
    BasicTooltip {
        visible:elide !== Text.ElideNone && isHover
        text: idContentText.text
    }
    property bool isHover: false
    MouseArea{
       hoverEnabled: true
       anchors.fill: parent
       onEntered: isHover = true
       onExited: isHover = false
       enabled: elide !== Text.ElideNone
    }
    Text {
        id: idContentText
        width: fontWidth
        x: hAlign == 1 ? Math.abs(idContentText.width - idContentText.contentWidth)/2 :
                         (hAlign == 0 ? 0:idContentText.width - idContentText.contentWidth)
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        anchors.verticalCenter: parent.verticalCenter
        padding: 0
        verticalAlignment: Qt.AlignVCenter
        opacity: parent.opacity
        //动画解释 1. 左对齐 2. 等待两秒 3.然后开始滚动 4. 等待一秒，归零 5. 等待3秒
        SequentialAnimation on x {//内容长度大于设置的text的长度才滚动
            running: needRuning
            loops: Animation.Infinite
            PropertyAnimation { to: 0; duration: 0 }
            PropertyAnimation { duration: 2000 }
            PropertyAnimation { to: idContentText.width - idContentText.contentWidth + widthOffset; duration: 5000 }
            PropertyAnimation { duration: 1000 }
            PropertyAnimation { to: 0; duration: 0 }
            PropertyAnimation { duration: 2000 }
        }

        onTextChanged: {
            width = width
            if(!!idContentText.contentWidth && needRuning)
                needRuning = idContentText.width - idContentText.contentWidth < 0
        }

        Component.onCompleted:{
            if(needRuning)
            needRuning = idContentText.width - idContentText.contentWidth < 0
        }

    }
}


