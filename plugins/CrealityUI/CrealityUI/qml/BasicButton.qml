import QtQuick 2.0
import QtQuick.Controls 2.5
Item {
    id: basicButton
    implicitWidth: 32
    implicitHeight: 32
    property alias text: propertyButton.text
    //property alias tooltip: propertyButton.tooltip
    property bool btnEnabled:true
    property bool btnSelected:false
    property color defaultBtnBgColor:  Constants.buttonColor
//    property color pressBtnBgColor : "transparent"
    property color hoveredBtnBgColor: Constants.hoveredColor
    property color selectedBtnBgColor: Constants.selectionColor//"transparent"

    property color borderColor: Constants.rectBorderColor
    property color borderHoverColor:  "transparent"// Constants.lpw_BtnBorderHoverColor
    property color btnTextColor: enabled? Constants.textColor : Constants.textColor_disabled//"#E3EBEE"
    property alias btnText: btnTxt

    property var btnRadius: 14
    property var btnBorderW: 1
    property var pointSize: Constants.labelFontPointSize_9
    property var fontFamily: Constants.labelFontFamily
    property var keyStr: ""
    property var textBold: false

    property alias hovered: propertyButton.hovered
    property alias down: propertyButton.down
    signal sigButtonClicked()
    signal sigButtonClickedWithKey(var keystr)
    Button {
        id : propertyButton
        width: parent.width
        height: parent.height
        font.family: fontFamily
        font.weight: Constants.labelFontWeight
        font.pointSize: pointSize
        font.bold:textBold
        contentItem: Item {
            Text {
                  id: btnTxt
                  color:  btnTextColor
                  anchors.centerIn: parent
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
                if(btnSelected)
                {
                  return selectedBtnBgColor
                }
               return propertyButton.hovered ?hoveredBtnBgColor:defaultBtnBgColor
            }
            border.width: btnBorderW/*1*/
            border.color: propertyButton.hovered? borderHoverColor : borderColor
        }
        onClicked:
        {
            if(keyStr == "")
            {
                sigButtonClicked()
            }
            else
            {
                sigButtonClickedWithKey(keyStr)
            }
        }
    }
}

