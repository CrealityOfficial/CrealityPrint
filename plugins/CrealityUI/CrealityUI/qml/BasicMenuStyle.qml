import QtQuick 2.0
import QtQml 2.3
import QtQuick.Controls 2.12
Menu {
    id: control
    x:2.5
    topPadding: -4
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    property bool highlightSeparator: false
    property bool hasIcon: true
    property bool centerText: false

    delegate: BasicMenuItemStyle
    {
        highlightSeparator: control.highlightSeparator
        hasIcon: control.hasIcon
        centerText: control.centerText
    }

    contentWidth : 200 * screenScaleFactor

}
