import QtQuick 2.0
import QtQuick.Layouts 1.13
import QtQuick.Controls 2.5
import QtQuick.Window 2.12
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"
import "qrc:/CrealityUI/components"

Item {
    id: root
    property var contextList: []
    signal clicked()
    Window {
        id: menu
        width: 205 * screenScaleFactor + (textLength > 8 ? 100 * screenScaleFactor : 0)
        height: (34 + (root.contextList.length / 2) * 18)* screenScaleFactor
        visible: idTipsBtn.hovered
        flags: Qt.FramelessWindowHint
        color:"transparent"
        property var textLength : 0

        function trackTo(target) {
            if (!target)
                return

            let globalPos = target.mapToGlobal(0, 0)
            x = globalPos.x - 11
            y = globalPos.y + target.height + 5
        }

        Component.onCompleted: {
            menu.trackTo(idTipsBtn)
        }

        onVisibleChanged: {
            if (visible) {
                menu.trackTo(idTipsBtn)
            }
        }
        Rectangle {
            anchors.fill: parent
            color: Constants.themeColor
            // color: Constants.currentTheme == 0 ? "#2B2B2D" : "#DADADA"
            border.color: Constants.lpw_BtnBorderColor
            border.width: 1
            radius: 10

            Grid {
                anchors.centerIn: parent
                columns: 2
                rowSpacing: 5
                columnSpacing: 15

                Repeater {
                    id: r
                    model: contextList

                    delegate: Text {
                        text: modelData
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignHCenter
                        color: index % 2 ? Qt.rgba(0.09, 0.80, 0.37) : Constants.textColor
                        font.pointSize: Constants.labelFontPointSize_9
                        font.family: Constants.labelFontFamilyBold
                        font.weight: Font.Medium

                        Component.onCompleted: {
                            if (index === 0)
                                menu.textLength = text.length
                        }
                    }
                }

            }
        }
    }

    Button {
        id: idTipsBtn
        width: 20 * screenScaleFactor
        height: parent.height
        visible: contextList.length > 0
        background: Image {
            anchors.centerIn: parent
            source: idTipsBtn.hovered ? "qrc:/UI/photo/control_p.svg" :  Constants.currentTheme? "qrc:/UI/photo/control_l.svg" : "qrc:/UI/photo/control_n.svg"

            opacity: root.enabled ? 1.0 : 0.3
        }


    }

    BasicDialogButton
    {
        anchors.centerIn: parent
        width : 210  * screenScaleFactor
        height: parent.height
        text: qsTranslate("MovePanel","Reset")
        btnRadius: height/2
        opacity: enabled ? 1 : 0.8
        defaultBtnBgColor: Constants.leftToolBtnColor_normal
        hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
//        font.family: Constants.labelFontFamilyBold
//        font.weight: Font.Medium
//        font.pointSize: Constants.labelFontPointSize_9
        onSigButtonClicked:
        {
            root.clicked()
//            com.smooth()
        }
    }

}
