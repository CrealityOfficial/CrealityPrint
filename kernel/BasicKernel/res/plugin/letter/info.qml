import QtQuick 2.0
import QtQuick.Controls 2.5
import CrealityUI 1.0
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/components"

LeftPanelDialog
{
    id:idControl
    width: 300* screenScaleFactor
    height: 510* screenScaleFactor
    title: qsTr("Letter")
    onVisibleChanged:
    {
        idLetterScene.visible = visible
    }
    property var com

    QtObject
    {
        id: __textParaObj
        property var textName: "TEXT"
        property var familyName: "Microsoft YaHei"
        property var textSize: 50
    }
    property alias gTextPara: __textParaObj

    function execute()
    {
    }
    Component.onCompleted:
    {
        idLetterScene.visible = true
    }
    LetterScene
    {
        id : idLetterScene
        parent: gGlScene
        anchors.fill: parent
        winWidth: parent.width
        winHeight: parent.height
//        visible: idControl.visible

        MouseArea{
            anchors.fill: parent
            propagateComposedEvents: true
            onPressed: {
                mouse.accepted = false
            }

            onReleased: {
                mouse.accepted = false
            }
        }
    }

    Item {
        anchors.fill: parent.panelArea

        LetterConfigPanel {
            id: idLetterConfigPanel
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: idLetter.top
            anchors.bottomMargin: 10
            letterCommand: com
            curSharp: idLetterScene.curSharp ? idLetterScene.curSharp : {}

        }

        BasicButton {
            id : idLetter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20 * screenScaleFactor
            property var bottonSelected: true
            width: 258* screenScaleFactor
            height: 28* screenScaleFactor
            text : qsTr("Generate Word")
            btnRadius:14
            btnBorderW:1
            borderColor: Constants.lpw_BtnBorderColor
            borderHoverColor: Constants.lpw_BtnBorderHoverColor
            defaultBtnBgColor: Constants.leftToolBtnColor_normal
            hoveredBtnBgColor: Constants.leftToolBtnColor_hovered
            anchors.horizontalCenter: parent.horizontalCenter
            pointSize: 9
            onSigButtonClicked: {
                var arry = idLetterScene.allArry()
                com.generatePolygonData(arry)
            }
        }
    }

    BasicButton {
        id : idImportFont
        width: 190* screenScaleFactor
        height: 32* screenScaleFactor
        text : qsTr("import font")
        btnRadius:3
        btnBorderW:1
        pointSize: 9
        borderColor: Constants.lpw_BtnBorderColor
        borderHoverColor: Constants.lpw_BtnBorderHoverColor
        defaultBtnBgColor: Constants.lpw_BtnColor
        hoveredBtnBgColor: Constants.lpw_BtnHoverColor
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter

        onSigButtonClicked:
        {
            com.loadFont();
        }
    }
}
