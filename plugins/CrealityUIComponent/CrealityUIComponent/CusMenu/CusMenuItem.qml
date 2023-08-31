import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Shapes 1.12
import CrealityUIComponent 1.0
import "qrc:/qml/CrealityUIComponent"

MenuItem {
    property alias actionShortcut: action.shortcut
    property alias actionName: action.text
    property bool separatorVisible
    id: control
    Action{
        id: action
    }
}
