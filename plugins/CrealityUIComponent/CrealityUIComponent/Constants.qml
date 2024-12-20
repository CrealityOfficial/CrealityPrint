pragma Singleton
import QtQuick 2.10
import QtQml 2.3

QtObject {
    property int screenScaleFactor: 1
    readonly property string labelFontFamily: "Source Han Sans CN Normal"
    readonly property string labelFontFamilyBold: "Source Han Sans CN Bold"
    readonly property string panelFontFamily: "Microsoft YaHei UI"
    readonly property int labelFontWeight: Font.Normal

    readonly property color textColor: "#ffffff"

    readonly property int labelFontPointSize_6: Qt.platform.os === "windows" ? 6 : 8
    readonly property int labelFontPointSize_8: Qt.platform.os === "windows" ? 8 : 10
    readonly property int labelFontPointSize_9: Qt.platform.os === "windows" ? 9 : 11
    readonly property int labelFontPointSize_10: Qt.platform.os === "windows" ? 10 : 12
    readonly property int labelFontPointSize_11: Qt.platform.os === "windows" ? 11 : 13
    readonly property int labelFontPointSize_12: Qt.platform.os === "windows" ?12 : 14
    readonly property int labelFontPointSize_14: Qt.platform.os === "windows" ?14 : 16
    readonly property int labelFontPointSize_16: Qt.platform.os === "windows" ?16 : 18
    readonly property int labelFontPointSize_18: Qt.platform.os === "windows" ?18 : 20


    property color themeColor_primary: "#ffffff"
    property color themeColor_secondary: "#dddde1"
    property color themeColor_third: "#999999"


    property color textNormalColor: themeColor_third
    property color textHoverColor: Qt.lighter(themeColor_third, 0.7)
    property color textDisableColor: Qt.lighter(themeColor_third, 0.5)

    property color btnNormalColor: themeColor_secondary
    property color btnHoverColor: Qt.lighter(themeColor_secondary, 0.5)
    property color btnCheckedColor: Qt.lighter(themeColor_secondary, 0.7)
    property color btnDisableColor: Qt.lighter(themeColor_secondary, 0.7)
}
