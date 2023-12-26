import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Rectangle {
    id: root
    color: "transparent"

    property var method: 0
    property var enableList: [true, true, true]

    function update() {
        var button = r.itemAt(method)
        button.checked = true
    }

    function setMethodEnable(index, enable) {
        enableList[index] = enable
        updateMethod()
    }

    function updateMethod() {
        for (let i = 0; i < enableList.length; ++i) {
            var button = r.itemAt(i)
            button.visible = enableList[i]
        }
    }

    CusText {
        id: idTitle
        fontText: qsTr("Select Tools")
        hAlign: 0
        fontColor: Constants.textColor
        fontPointSize: Constants.labelFontPointSize_9
    }

    Grid {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: idTitle.bottom
        anchors.topMargin: 6
        spacing: 10

        Repeater {
            id: r
            model: [
                {
                    normalImage: "qrc:/UI/photo/triangle_n.png",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/triangle_p.png" : "qrc:/UI/photo/triangle_n.png",
                    pressImage: "qrc:/UI/photo/triangle_p.png",
                    tips: qsTr("Triangle")
                },
                {
                    normalImage: "qrc:/UI/photo/ring_n.png",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/ring_p.png" : "qrc:/UI/photo/ring_n.png",
                    pressImage: "qrc:/UI/photo/ring_p.png",
                    tips: qsTr("Circle")
                },
                {
                    normalImage: "qrc:/UI/photo/fill_n.png",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/fill_p.png" : "qrc:/UI/photo/fill_n.png",
                    pressImage: "qrc:/UI/photo/fill_p.png",
                    tips: qsTr("Fill")
                }
            ]

            delegate: CusImglButton {
                id: btn
                checkable: true
                width: 30 * screenScaleFactor
                height: 30 * screenScaleFactor
                autoExclusive: true
            
                cusTooltip.text: modelData.tips

                onCheckedChanged: {
                    method = index
                }

                background: Rectangle {
                    anchors.fill: parent
                    border.width: btn.hovered || btn.checked ? 3 : 1
                    border.color: btn.hovered || btn.checked ? "#1E9BE2" : Constants.lpw_BtnBorderColor
                    // border.color: border.width === 3 ? "#1E9BE2" : "#6E6E72"
                    color: btn.checked ? "#1E9BE2" : "transparent"
                    radius: 5

                    Image {
                        anchors.centerIn: parent
                        source: btn.hovered ? modelData.hoverImage : btn.checked ? modelData.pressImage : modelData.normalImage
                    }
                }

                Component.onCompleted: {
                    if (index === method)
                        btn.checked = true
                    if (index === 2) { 
                        updateMethod()
                    }
                }
            }
        }
    }
}