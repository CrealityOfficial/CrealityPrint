import QtQuick 2.13
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.12

Rectangle{
    id:rootRec
    implicitWidth: 100
    implicitHeight: 100
    property bool shadowEnabled: true
    color: "#3F3F44"
    radius: 5
    border.width: 1
    border.color: "#0A0A0A"
    layer.enabled: shadowEnabled
    layer.effect:
        DropShadow {
        verticalOffset: 7
        samples: 6
        color: Constants.dropShadowColor
    }
}
