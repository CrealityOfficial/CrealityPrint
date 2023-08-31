/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D Editor of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.5
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.12
//import QtQuick.Controls.Styles 1.4

Item {
    width: 16
    height: 16
    id: enableButton

    property int defultTooltipWidth: 180
    property bool bottonSelected: false
    property string enabledIconSource
    property string disabledIconSource
    property string pressedIconSource
    property alias text: propertyButton.text
    //property alias tooltip: propertyButton.tooltip
    property color defaultBgColor: "transparent"
    property color hoveredBgColor: "transparent"
    property color selectedBgColor: "transparent"

    signal enabledButtonClicked()
	
    Button {
        id: propertyButton
        width: parent.width
        height: parent.height - 1
        activeFocusOnTab: false
        icon.source: enabledIconSource//enabled?enabledIconSource:disabledIconSource

        contentItem: Item {
            anchors.fill: parent
            Image {
                id: icon_image
                //anchors.verticalCenter : enableButton.verticalCenter
                anchors.centerIn: parent
                //anchors.verticalCenter:parent.verticalCenter
                width: 48 //图像宽度
                height: 48 //图像高度

                source: enabled ? ((propertyButton.hovered || bottonSelected)?pressedIconSource:enabledIconSource) : enabledIconSource
                autoTransform: true //图像是否自动变换，默认为false
                fillMode: Image.Pad //图像的填充方式-保持图片原样
            }
        }

        background: Rectangle {
            radius: 6
            implicitWidth: 48
            implicitHeight: 48
            opacity: enabled ? 1 : 0.3
            color: bottonSelected ? selectedBgColor : (propertyButton.hovered?hoveredBgColor:defaultBgColor)
        }

        onClicked: enabledButtonClicked()

        ToolTip
        {
            id: idTooltip
            delay: 100
            timeout: propertyButton.down ? 100 : 2000
            visible: propertyButton.hovered
            x: 38
            y: 45

            background: Item{
                anchors.fill: parent
                Rectangle {
                    id: backgroundRect
                    anchors.fill: parent
                    color: "white"
                    border.width: 1
                    border.color: "#767676"
                }

                DropShadow {
                    anchors.fill: backgroundRect
                    radius: 8
                    samples: 17
                    source: backgroundRect
                    horizontalOffset:3
                    verticalOffset:3
                    color: Constants.dropShadowColor
                }
            }

            contentItem: Item {
                id: idRect
                height: 24
                width:idText.width + 20
                Text {
                    id:idText
                    width: contentWidth
                    anchors.verticalCenter:parent.verticalCenter
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignHCenter
                    text: propertyButton.text
                    color: "#333333"
                    font.pointSize: Constants.labelFontPointSize_10
                    font.family: Constants.labelFontFamily
                }
            }
        }
    }
}
