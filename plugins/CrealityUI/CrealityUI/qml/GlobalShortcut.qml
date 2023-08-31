import QtQuick 2.13

Item{
    property alias funcItem: loader.funcItem
    property int shortcutType: 0 //0: leftBtnsShortcut, 1: otherBtnsShortcut
    Loader{
        id: loader
        property var funcItem
        sourceComponent: shortcutType ? otherBtnsShortcutCom : leftBtnsShortcutCom
    }

    Component{
        id: leftBtnsShortcutCom
        Item{
            id: leftBtnsShortcut
            visible: shortcutType == 0
            Shortcut{
                context: Qt.WindowShortcut
                sequence: "M"

                onActivated: {
                    funcItem.switchMode(0)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "S"

                onActivated: {
                    funcItem.switchMode(1)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "R"

                onActivated: {
                    funcItem.switchMode(2)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "F"

                onActivated: {
                    funcItem.switchMode(3)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "L"

                onActivated: {
                    funcItem.switchMode(4)
                }
            }
        }
    }


    Component{
        id: otherBtnsShortcutCom
        Item{
            id: otherBtnsShortcut
            visible: shortcutType == 1
            Shortcut{
                context: Qt.WindowShortcut
                sequence: "Alt+L"

                onActivated: {
                    funcItem.bindBtn.checked = true
                    funcItem.switchMode(0)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "C"

                onActivated: {
                    funcItem.bindBtn.checked = true
                    funcItem.switchMode(2)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "T"

                onActivated: {
                    funcItem.bindBtn.checked = true
                    funcItem.switchMode(7)
                }
            }
        }

    }

}


