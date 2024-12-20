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

    /* 索引转换，cpp中的工具枚举是不连贯的，和Qml中的工具排序不一致 */
    function toCpp(i) {
        if (i == 3) return 5;
        else if (i == 4) return 3;
        else if (i == 5) return 4; 
        else return i
    }

    function toQml(i) {
        if (i == 5) return 3;
        else if (i == 3) return 4;
        else if (i == 4) return 5; 
        else return i
    }

    function update() {
        var button = r.itemAt(toQml(method))
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
        columns: 7

        Repeater {
            id: r
            model: [
                {
                    normalImage:  Constants.currentTheme?"qrc:/UI/photo/paint/triangle_l.svg":"qrc:/UI/photo/paint/triangle_n.png",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/paint/triangle_p.png" : "qrc:/UI/photo/paint/triangle_l.svg",
                    pressImage: "qrc:/UI/photo/paint/triangle_p.png",
                    tips: qsTr("Triangle")
                },
                {
                    normalImage: Constants.currentTheme?"qrc:/UI/photo/paint/ring_l.svg":"qrc:/UI/photo/paint/ring_n.png",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/paint/ring_p.png" : "qrc:/UI/photo/paint/ring_l.svg",
                    pressImage: "qrc:/UI/photo/paint/ring_p.png",
                    tips: qsTr("Circle")
                },
                {
                    normalImage: Constants.currentTheme?"qrc:/UI/photo/paint/fill_l.svg":"qrc:/UI/photo/paint/fill_n.png",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/paint/fill_p.png" : "qrc:/UI/photo/paint/fill_l.svg",
                    pressImage: "qrc:/UI/photo/paint/fill_p.png",
                    tips: qsTr("Fill")
                },
                {
                    normalImage: Constants.currentTheme?"qrc:/UI/photo/paint/height_paint_l.svg":"qrc:/UI/photo/paint/height_paint_n.svg",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/paint/height_paint_p.svg" : "qrc:/UI/photo/paint/height_paint_l.svg",
                    pressImage: "qrc:/UI/photo/paint/height_paint_p.svg",
                    tips: qsTr("Height Range")
                },
                {
                    normalImage: Constants.currentTheme?"qrc:/UI/photo/paint/gap_fill_l.svg":"qrc:/UI/photo/paint/gap_fill_n.svg",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/paint/gap_fill_p.svg" : "qrc:/UI/photo/paint/gap_fill_l.svg",
                    pressImage: "qrc:/UI/photo/paint/gap_fill_p.svg",
                    tips: qsTr("Gap Fill")
                },
                {
                    normalImage:  Constants.currentTheme?"qrc:/UI/photo/paint/sphere_l.svg":"qrc:/UI/photo/paint/sphere_n.svg",
                    hoverImage: Constants.currentTheme == 0 ? "qrc:/UI/photo/paint/sphere_p.svg" : "qrc:/UI/photo/paint/sphere_l.svg",
                    pressImage: "qrc:/UI/photo/paint/sphere_p.svg",
                    tips: qsTr("Sphere")
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
                    method = toCpp(index)
                }

                background: Rectangle {
                    anchors.fill: parent
                    border.width: btn.hovered || btn.checked ? 3 : 1
                    border.color: btn.hovered || btn.checked ? (btn.enabled ? Qt.rgba(0.09, 0.80, 0.37, 1.0) : Qt.rgba(0.09, 0.80, 0.37, 0.3)) : Constants.lpw_BtnBorderColor
                    // border.color: border.width === 3 ? "#1E9BE2" : "#6E6E72"
                    color: btn.checked ? (btn.enabled ? Qt.rgba(0.09, 0.80, 0.37, 1.0) : Qt.rgba(0.09, 0.80, 0.37, 0.3)) : "transparent"
                    // color: btn.checked ? Qt.rgba(0.09, 0.80, 0.37, 1.0) : "transparent"
                    radius: 5

                    Image {
                        anchors.centerIn: parent
                        source:  btn.checked ? modelData.pressImage : modelData.normalImage
                        opacity: btn.enabled ? 1.0 : 0.3
                    }
                }

                Component.onCompleted: {
                    if (index === toQml(method))
                        btn.checked = true
                    if (index === 2) { 
                        updateMethod()
                    }
                }
            }
        }
    }
}