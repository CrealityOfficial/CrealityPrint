import QtQuick 2.9
import QtQuick.Controls 2.2
import "../secondqml"


Rectangle{
    property real heightOffset: 0
    property real widthOffset: 0
    property int hAlign: 1 //0： 字体左对齐 1： 字体居中 2:右对齐
    property alias fontPointSize: idContentText.font.pointSize
    property alias fontWeight: idContentText.font.weight
    property alias fontFamily: idContentText.font.family
    property alias fontElide: idContentText.elide
    property real fontWidth: 150 * screenScaleFactor
    property alias fontBold: idContentText.font.bold
    property alias fontHeight: idContentText.height
    property alias fontText: idContentText.text
    property alias fontColor: idContentText.color
    property alias fontOpacity: idContentText.opacity
    property alias fontWrapMode: idContentText.wrapMode
    property alias fontMaximumLineCount: idContentText.maximumLineCount
    property alias font_font: idContentText.font
    property alias fontSizeMode: idContentText.fontSizeMode
    property alias fontContentWidth: idContentText.contentWidth
    property alias font: idContentText.font
    property alias aliasText: idContentText

    property bool needRuningManual: false
    property bool needRuning: false
    property bool isDefault: false
    property bool isHovered: false
    property bool needToolTip: false

    id:recTxt
    height: idContentText.contentHeight + heightOffset//adapt some font
    width: isDefault ? idContentText.contentWidth : fontWidth
    clip: true
    color: "transparent"
    opacity: enabled ? 1 : 0.7
    Text {
        id: idContentText
        width: isDefault ? contentWidth : fontWidth
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
            id: textAnimation
            running: needRuning && needRuningManual
            loops: Animation.Infinite
            PropertyAnimation { to: 0; duration: 0 }
            PropertyAnimation { duration: 1000 }
            PropertyAnimation { to: idContentText.width - idContentText.contentWidth + widthOffset; duration: 2500 }
            PropertyAnimation { duration: 500 }
            PropertyAnimation { to: 0; duration: 0 }
            PropertyAnimation { duration: 1000 }
        }

        onTextChanged: {
            width = width
            if(idContentText.contentWidth !== 0)
                needRuning = idContentText.width - idContentText.contentWidth < 0
        }

        Component.onCompleted:{
            needRuning = idContentText.width - idContentText.contentWidth < 0
        }
    }

    onFontWidthChanged:{
        if(idContentText.contentWidth !== 0){
            idContentText.width = width
            needRuning = idContentText.width - idContentText.contentWidth < 0
            if(!needRuning)
                idContentText.x = 0
            else{
                textAnimation.stop()
                textAnimation.restart()
            }
        }
    }

    BasicTooltip {
        id: idTooptip
        visible: needToolTip && isHovered && idContentText.width < idContentText.contentWidth
    }

//    MouseArea{
//        anchors.fill: parent
//        hoverEnabled: true
//        onEntered: {
//            isHovered = true
//            console.log("111111111111")
//        }

//        onExited: {
//            isHovered = false
//             console.log("2222222222")
//        }
//    }
}


