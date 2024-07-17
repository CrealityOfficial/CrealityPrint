import CrealityUI 1.0
import QtQuick 2.0
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.13
import "qrc:/CrealityUI"
import "qrc:/CrealityUI/components"
import "qrc:/CrealityUI/qml"
import "qrc:/CrealityUI/secondqml"

Rectangle {
    id: idConfigPanel

    property var letterCommand
    property var curSharp

    color: "transparent"

    ScrollView {
        id: idScrollView

        clip: true
        anchors.fill: parent
        anchors.margins: 10 * screenScaleFactor
        ScrollBar.vertical.policy:   (contentHeight > height) ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff

        Column {
            anchors.fill: parent
            spacing: 10 * screenScaleFactor
            height: idFontSettings.height + 10 + idLaserShapeItem.height

            LetterConfigGroup {
                id: idFontSettings

                title: qsTr("Word")
                width: idScrollView.width - 10 * screenScaleFactor
                enabled: parent.enabled

                accordionContent: Rectangle {
                    width: idFontSettings.width
                    height: 125 * screenScaleFactor
                    color: "transparent"

                    LetterFontConfigs {
                        id: idfontSetting

                        anchors.fill: parent
                        onFontFamilyChanged: {
                            if (fontFamily.toLowerCase().indexOf("normal") !== -1)
                                curSharp.fontfamily = Constants.labelFontFamily;
                            else
                                curSharp.fontfamily = fontFamily;
                            curSharp.fontsize = fontSize;
                        }
                        onFontTextChanged: {
                            curSharp.text = fontText;
                        }
                        onFontSizeChanged: {
                            curSharp.fontfamily = fontFamily;
                            curSharp.fontsize = fontSize;
                        }
                    }

                }

            }

            LetterConfigGroup {
                id: idLaserShapeItem

                title: qsTr("Parameter")
                width: idScrollView.width - 10 * screenScaleFactor
                height: 240 * screenScaleFactor
                enabled: parent.enabled

                accordionContent: Rectangle {
                    // anchors.fill: parent
                    color: "transparent"

                    LetterShapeConfigs {
                        id: idLaserShapeSettings

                        com: letterCommand
                        selShape: curSharp
                    }

                }

            }

        }

    }

}
