import QtQuick 2.0
import QtQuick.Controls 2.5
//import CrealityUI 1.0
//import "qrc:/CrealityUI"
import "../qml"
//Item {
//    id: basicButton
//    implicitWidth: 32
//    implicitHeight: 32

    Button {
        id : propertyButton

        property var pointSize: Constants.labelFontPointSize_9
        property string enabledIconSource
        property string disabledIconSource
        property string pressedIconSource
     //   property alias text: propertyButton.text
        property bool btnEnabled:true
        property bool bottonSelected:false
        property bool idAnimatedEnbale: false
//        property color defaultBtnBgColor: "white"
//        property color hoveredBtnBgColor: "#214076"
//        property color selectedBtnBgColor: "transparent"

//        property color btnTextColor:propertyButton.hovered ? "white":"#3968E9"
//        property color borderColor: "#3968E9"
        property color defaultBtnBgColor: Constants.buttonColor//"transparent"//"white"
        property color hoveredBtnBgColor: Constants.hoveredColor//"#3AC2D7"//"gray"
        property color selectedBtnBgColor:Constants.selectionColor// "transparent"

        property color btnTextColor: Constants.textColor //"white"
        property color borderColor: Constants.rectBorderColor  //"#929292"
         property color borderHoverColor: "transparent" // Constants.lpw_BtnBorderHoverColor
        property var btnRadius: 14
        property var btnBorderW: 1
        property var strTooptip: ""
        property var positionTooptip: BasicTooltip.Position.TOP
        property alias imgWidth: icon_image.width
        property alias imgHeight: icon_image.height
        property alias btnHovred: propertyButton.hovered
        property var isAnimatedImage: false
        signal sigButtonClicked()

        property alias iconImage: icon_image

        function setAnimatedImageStatus(value)
        {
            idAnimatedImage.visible = value
            icon_image.visible = !value
            if(!idAnimatedEnbale)
                this.enabled = !value
        }

        implicitWidth: 32
        implicitHeight: 32
//        width: parent.width
//        height: parent.height
        font.family: Constants.labelFontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: pointSize
        BasicTooltip {
            id: idTooptip
            visible: text !== "" && propertyButton.hovered
            text: strTooptip
        }

        contentItem: Item {
            anchors.fill: parent
            Image {
                 id: icon_image
                 anchors.centerIn: parent
                 //  Distance between img and text  is 4px
                 anchors.horizontalCenterOffset:idContentText.width ===0? 0 :  -(idContentText.width + 4 )/2
                 width: 24
                 height: 24
                 source: bottonSelected?enabledIconSource:pressedIconSource
                 //autoTransform: true
                 visible:
                     (enabledIconSource.length > 0 || pressedIconSource.length > 0) ? true : false

                 fillMode: Image.Pad
             }
             AnimatedImage{
                 id: idAnimatedImage
                 anchors.centerIn: parent
                 anchors.horizontalCenterOffset:idContentText.width ===0? 0 :  -(idContentText.width + 4 )/2
                 visible: false
                 width: 16
                 height: 16
                 source: "qrc:/UI/photo/loading.gif"
             }
            Text {
                id: idContentText
                  color:  btnTextColor
                  anchors.centerIn: parent
                  //  Distance between img and text  is 4px
                  anchors.horizontalCenterOffset: (enabledIconSource.length > 0 || pressedIconSource.length > 0)? (icon_image.width + 4 )/2 : 0
                  elide: Text.ElideRight
                  text: propertyButton.text
                  font: propertyButton.font
            }
        }

        background: Rectangle {
            implicitWidth: parent.width
            implicitHeight: parent.height
            radius: btnRadius
            opacity: enabled ? 1 : 0.3
            color: {
                if(bottonSelected)
                {
                  return selectedBtnBgColor
                }
               return propertyButton.hovered ?hoveredBtnBgColor:defaultBtnBgColor
            }
            border.width: btnBorderW
            border.color: propertyButton.hovered? borderHoverColor : borderColor
        }
        onClicked:
        {
          //  idTooptip.visible = false
            sigButtonClicked()
            if(isAnimatedImage){
                setAnimatedImageStatus(true)
            }
            
        }
    }
//}


/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
