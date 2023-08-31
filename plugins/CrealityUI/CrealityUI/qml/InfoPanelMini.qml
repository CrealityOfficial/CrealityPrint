import QtQuick 2.0
import QtQuick.Controls 2.0
import "qrc:/CrealityUI"
Item {
    property var errorModel : 0

    //color:"red"
    //visible: modelXSize>0?true:false
    Column
    {
        spacing: 6
        Row {
            Image {
                id : logoImage
                height:sourceSize.height
                width: sourceSize.width
                source: "qrc:/UI/photo/upload_msg.png"
            }
        }

        Row {
            id:model_r
            StyledLabel {
                  text: errorModel + " " + qsTr("Model(s) Invalid")
                  font.family:  Constants.labelFontFamily
                  font.pointSize: Constants.labelFontPointSize_6
                  color: Constants.infoPanelColor                 
            }
        }
    }
}
