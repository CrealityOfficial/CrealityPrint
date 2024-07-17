import QtQuick 2.13

Item{
    property alias funcItem: loader.funcItem
    property int shortcutType: 0 //0: leftBtnsShortcut, 1: otherBtnsShortcut
    property var listView
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
                sequence: "T"
                onActivated: { 
                    funcItem.switchToolMode(listView,0)
                }
            }

            
            Shortcut{
                context: Qt.WindowShortcut
                sequence: "R"

                onActivated: {
                    funcItem.switchToolMode(listView,1)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "Shift+A"

                onActivated: {
                    funcItem.switchToolMode(listView,2)
                }
            }


            Shortcut{
                context: Qt.WindowShortcut
                sequence: "B"
                onActivated: {
                    funcItem.switchToolMode(listView,3)
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
                sequence: "Ctrl+R"
                onActivated: {
                    // funcItem.bindBtn.checked = true
                    funcItem.switchToolMode(listView,0)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "M"
                onActivated: {
                    funcItem.switchToolMode(listView,1)
                }
            }

            Shortcut{
                context: Qt.WindowShortcut
                sequence: "Alt+C"
                onActivated: {
                    // funcItem.bindBtn.checked = true
                    funcItem.switchToolMode(listView,2)
                }
            }
            Shortcut{
                context: Qt.WindowShortcut
                sequence: "C"
                onActivated: {
                    // funcItem.bindBtn.checked = true
                    funcItem.switchToolMode(listView,6)
                }
            }
        }

    }

}


