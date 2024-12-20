import QtQuick 2.0
import QtQuick.Controls 2.12
//BasicComButton
//{
//    id:idBtn
//}


Button {
    id : propertyButton
    width: parent.width
    height: parent.height
    property bool bottonSelected:false
    property color defaultBtnBgColor: "#1E9BE2"
    property color hoveredBtnBgColor: "#1EB6E2"
    property color selectedBtnBgColor: "transparent"

    property color btnTextColor:"white"
    property var btnRadius: 25

    visible: width > 10 ? true:false
    //anchors.horizontalCenter: parent.horizontalCenter
    signal sigButtonClicked()

    contentItem: Item {
        anchors.fill: parent
        //Image {
        //     id: icon_image
        //     anchors.centerIn: parent
        //     //  Distance between img and text  is 8px
        //     anchors.verticalCenterOffset:  -(idContentText.height + 8 )/2
        //     width: 44
        //     height: 44
        //     source: bottonSelected?enabledIconSource:pressedIconSource
        //     //autoTransform: true
        //     visible:
        //         (enabledIconSource.length > 0 || pressedIconSource.length > 0) ? true : false
		//
        //     fillMode: Image.Pad
        // }
        Text {
            id: idContentText
              color:  btnTextColor
              anchors.centerIn: parent
              //  Distance between img and text  is 4px
//              anchors.verticalCenterOffset: (enabledIconSource.length > 0 || pressedIconSource.length > 0)? (icon_image.height + 8 )/2 : 0
              elide: Text.ElideRight
              text: propertyButton.text
              //font: propertyButton.font
              font.pointSize: Constants.labelFontPointSize_14
              font.family: "Microsoft YaHei UI"//"Source Han Sans CN"
        }
    }

    background: Rectangle {
        implicitWidth: parent.width
        implicitHeight: parent.height
        radius: btnRadius
        opacity: enabled ? 1 : 0.3
//        color: {
//            if(bottonSelected)
//            {
//              return selectedBtnBgColor
//            }
//           return propertyButton.hovered ?hoveredBtnBgColor:defaultBtnBgColor
//        }
//        gradient: Gradient {
//            GradientStop { position: 0.0; color: "#3AC2D7" }//"white" }
//            GradientStop { position: 1; color: "white" }
//        }
        color : !enabled? "#787878"/*hoveredBtnBgColor*/ : (propertyButton.hovered? hoveredBtnBgColor:defaultBtnBgColor)

    }
    onClicked:
    {
        sigButtonClicked()
    }
}
