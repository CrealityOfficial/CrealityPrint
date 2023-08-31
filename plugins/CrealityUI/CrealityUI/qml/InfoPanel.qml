import QtQuick 2.0
import QtQuick.Controls 2.0
Item {
    property alias modelName : model_name.text
    property alias modelNameWidth : model_name.width
    property var modelXSize
    property var modelYSize
    property var modelZSize
    property var displayFormat: "{0}x{1}x{2}mm"
    property alias modelSize : model_size.text
    property var isImage : false
    property var displayFormatImage: "{0}x{1}mm"


    //color:"red"
    //visible: modelXSize>0?true:false
    Column
    {
        spacing: 6
        Row {
            id:model_r
            StyledLabel {
                text: qsTr("Model:")
                font.family:  Constants.labelFontFamily
                font.pointSize: Constants.labelFontPointSize_6
                color: Constants.infoPanelColor
            }
            StyledLabel {
                id:model_name
                text: "zhu.stl"
                elide: Text.ElideRight
                font.pointSize: Constants.labelFontPointSize_6
                color: Constants.infoPanelColor//"#5C5F63"
            }
        }
        Row {
            id:model_r2
            //            anchors.top: model_r.bottom

            StyledLabel {
                text: qsTr("size:")
                font.pointSize: Constants.labelFontPointSize_6
                color: Constants.infoPanelColor
            }
            StyledLabel {
                id:model_size
                text: {isImage ? (modelXSize && modelYSize)? displayFormatImage.replace("{0}",modelXSize.toFixed(2)).replace("{1}",modelYSize.toFixed(2)) : "" :(modelXSize && modelYSize && modelZSize )? displayFormat.replace("{0}",modelXSize.toFixed(2)).replace("{1}",modelYSize.toFixed(2)).replace("{2}",modelZSize.toFixed(2)) : ""}
                font.pointSize: Constants.labelFontPointSize_6
                color: Constants.infoPanelColor
            }
        }
    }
}
