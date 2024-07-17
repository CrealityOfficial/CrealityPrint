import CrealityUI 1.0
import QtQml 2.13
import QtQuick 2.13
import QtQuick.Layouts 1.3

import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/secondqml/frameless"
import "qrc:/CrealityUI/secondqml/printer"

Rectangle {
    id: idMessageStack
    anchors.bottom: parent.bottom
    width: 200
    height: 0
    color: "red"

    property bool isErrorExist: errorMap && errorMap.size > 0
    property var errorMap

    // onIsErrorExistChanged: {
    //     console.log("error status change : " + isErrorExist)
    // }

    function _destroyMessage(code) {
        var childList = children;

        for (let i = 0; i < childList.length; ++i) {
            var item = childList[i]
            if (item.messageObj && item.messageObj.code() == code) {
                item.visible = false
                item.destroy()
            }
        }
    }

    function destroyMessage(code) {
        _destroyMessage(code)

        if (errorMap) {
            errorMap.delete(code);
            isErrorExist = (errorMap.size > 0)
        }

        layout()
    }

    function requestMessage(obj) {
        var item = Qt.createComponent("MessageItem.qml");
        if (item.status == Component.Ready) {
            _destroyMessage(obj.code())
            item.createObject(idMessageStack, {width: width - 20, messageObj: obj});
        } else {
            console.log("error: " + item.errorString())
        }

        if (!errorMap)
            errorMap = new Set()

        if (obj.level() == 2 && obj.code() < 100) { /* 出现需要禁止切片的错误 */   
            // console.log("error occured")
            errorMap.add(obj.code())
            isErrorExist = true
        }

        layout()
    }

    function layout() {
        var childList = children;
        height: 0

        var h = 0
        if (childList.length == 0) {
            // height = h
        } else {
            var lastItem
            var currItem
            for (let i = 0; i < childList.length; ++i) {
                currItem = childList[i]

                if (!currItem.visible)
                    continue

                if (!lastItem) {
                    currItem.anchors.bottom = idMessageStack.bottom
                    h += currItem.height
                } else {
                    currItem.anchors.bottom = lastItem.top
                    currItem.anchors.bottomMargin = 10
                    h += 10
                    h += currItem.height
                }
                lastItem = currItem
            }
            // height = h
        }
        height = 0

    }
}