import QtQuick 2.10
import QtQuick.Window 2.13
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.12
import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.12

import "../../qml"
import "../../secondqml"

Rectangle {
    id: root
    color: "transparent"
    border.color: Constants.dialogItemRectBgBorderColor
    // border.width: 1

    // property var colors: ["red", "yellow", "black", "orange", "blue", "red", "yellow", "black", "orange", "blue", "red", "yellow", "black", "orange", "blue"]
    property var colors
    // property var sequenceString: "1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15"
    property var sequenceString
    property var sequenceList: []

    signal sequenceChanged()

    onSequenceChanged: {
        getSequenceString()
    }

    function init(sequenceStr) {
        console.log("sequence init " + sequenceStr)

        colors = []
        var model = kernel_parameter_manager.currentMachineObject.extrudersModel
        var rowCount = model.rowCount();
        for (var i = 0; i < rowCount; ++i) {
            var data = model.get(i);
            colors.push(data.extruderColor)
        }

        if (sequenceString != sequenceStr)
            sequenceString = sequenceStr
        parseSequenceString()
    }

    function getSequenceString() {
        let str = ''
        for (let i = 0, count = sequenceList.length; i < count; ++i) {
            str += (sequenceList[i] + ',')
        }
        if (str[str.length - 1] === ',')
            str = str.slice(0, str.length - 1)
            
        return str
    }

    function parseSequenceString() {
        sequenceList = []
        if (sequenceString == "0")
            return;

        var numberArray = sequenceString.split(",");
        for (var i = 0; i < numberArray.length; ++i) {
            let number = parseInt(numberArray[i])
            sequenceList.push(number)
        }

        console.log("parseSequenceString********************")
        sequenceChanged()
        repeater.reload(getSequenceModel())
    }

    function swapSequenceList(number1, number2) {
        let index1, index2
        for (let i = 0; i < sequenceList.length; ++i) {
            if (sequenceList[i] == number1)
                index1 = i
            if (sequenceList[i] == number2)
                index2 = i
        }
        sequenceList[index1] = sequenceList.splice(index2, 1, sequenceList[index1])[0];
        console.log("swapSequenceList********************")
        sequenceChanged()
    }

    function getSequenceModel() {
        var model = []
        for (var i = 0; i < sequenceList.length; ++i) {
            model.push(sequenceList[i])
            if (i < sequenceList.length - 1)
                model.push('|')
        }
        return model;
    }

    Item {
        id: dragItem
        anchors.fill: parent
        clip: true

        Image {
            id: shadow
            width: 20
            height: 20
            z: 1
            opacity: 0.7
            x: handler.mouseX - width / 2
            y: handler.mouseY - height / 2
            visible: handler.pressed && source

            property var urlCache: { "-1" : "" }
            property var localCache: { "-1" : Qt.rect(-1, -1, -1, -1)}
        }


        MouseArea {
            id: handler
            anchors.fill: parent
            property var from

            onPressed: {
                var number = findItemNumber(mouseX, mouseY)
                if (number > 0) {
                    from = number
                    if (shadow.urlCache[from]) {
                        shadow.source = shadow.urlCache[from]
                    } else {
                        shadow.source = ""
                        let targetItem = repeater.getItemByNumber(number)
                        targetItem.updateImage()
                    }
                } else {
                    shadow.source = ""
                    from = -1
                }
            }

            onReleased: {
                var number = findItemNumber(mouseX, mouseY)
                if (number > 0 && from > 0) {
                    swapSequenceList(from, number)
                    repeater.reload(getSequenceModel());
                }
            }

            onPositionChanged: {

            }

            function findItemNumber(x, y) {
                for (var key in shadow.localCache) {
                    let rect = shadow.localCache[key]
                    if (rect.x <= x &&
                        rect.x + rect.width >= x &&
                        rect.y <= y &&
                        rect.y + rect.height >= y) {
                        return key
                    }
                }
                return -1
            }

        }

        Flow {
            id: flow
            x: 10
            y: 6
            width: parent.width - 20
            // anchors.fill: parent
            // anchors.margins: 10
            property var itemCache

            onPositioningComplete: {
                let str = ''
                for (let i = 0; i < repeater.count; ++i) {
                    let item = repeater.itemAt(i);
                    let number = item.getNumber()
                    if (number !== '|') {
                        let pos = item.mapToItem(flow, Qt.point(0, 0))
                        let offset = item.width / 2
                        shadow.localCache[number] = Qt.rect(pos.x - offset, 
                                                            pos.y - offset, 
                                                            item.width + offset * 2, 
                                                            item.height + offset * 2)
                    }
                }
                root.height = height + 12
            }

            Repeater{
                id:repeater

                model: []

                function reload(m) {
                    model = []
                    model = m
                }

                function getItemByNumber(number) {
                    for (let i = 0; i < repeater.count; ++i) {
                        let item = repeater.itemAt(i)
                        if (item && item.getNumber && item.getNumber() == number) {
                            return item
                        }
                    }
                    return undefined
                }

                delegate: Item {
                    width: 20
                    height: 20

                    property bool isArrow: modelData === '|'

                    function updateImage() {
                        numberItem.grabToImage(function (result) {
                            shadow.source = result.url
                            shadow.urlCache[getNumber()] = result.url
                        })
                    }

                    function getNumber() {
                        return modelData
                    }

                    Rectangle {
                        id: numberItem
                        width: 20
                        height: 20
                        color: isArrow ? "transparent" :root.colors[modelData - 1]
                        visible: !isArrow

                        function setTextColor(color){
                            let calcColor = ""
                            let bgColor = [color.r * 255, color.g * 255, color.b * 255]
                            let whiteContrast = Constants.getContrast(bgColor, [255, 255, 255])
                            let blackContrast = Constants.getContrast(bgColor, [0, 0, 0])
                            if(whiteContrast > blackContrast)
                                calcColor = "#ffffff"
                            else
                                calcColor =  "#000000"

                            return calcColor
                        }

                        Text {
                            anchors.fill: parent
                            id: text
                            text: modelData
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignHCenter
                            color: parent.setTextColor(parent.color)
                            // {
                            //     var bgColor = parent.color
                            //     var brightness = 0.299 * bgColor.r + 0.587 * bgColor.g + 0.114 * bgColor.b
                            //     return brightness > 0.5 ? "black" : "white"
                            // }
                        }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 16
                        height: 8
                        source: "qrc:/UI/photo/group_print_task_create_img_right_arrow.png"
                        visible: isArrow
                        
                    }
                }
            }
        }
    }

}