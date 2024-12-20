import QtQuick 2.13
import QtQml 2.3
import QtQuick.Controls 2.5
import "../qml"

Rectangle{
    property color bgColor
    property alias contentText: cusText.fontText
    property color textColor

    width: 20 * screenScaleFactor
    height: 20 * screenScaleFactor
    radius: 5 * screenScaleFactor
    color: styleData.value

    CusText {
        id:cusText
        fontColor: textColor
        anchors.centerIn: parent
        fontWidth: parent.width
        fontPointSize: Constants.labelFontPointSize_9
    }

    onBgColorChanged:{
        let bgColori = [bgColor.r * 255, bgColor.g * 255, bgColor.b * 255]
        let color1 = [255, 255, 255]
        let color2 = [0, 0, 0]

        let ct1 = Constants.getContrast(bgColori, color1)
        let ct2 = Constants.getContrast(bgColori, color2)

        textColor = ct1 > ct2 ? "#ffffff" : "#000000"
    }
}
