/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Charts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtCharts 2.0

Rectangle {
    id: legend
    color: "lightgray"

    property int seriesCount: 0
    property variant seriesNames: []
    property variant seriesColors: []
    signal entered(string seriesName)
    signal exited(string seriesName)
    signal selected(string seriesName)

    function addSeries(seriesName, color) {
        var names = seriesNames;
        names[seriesCount] = seriesName;
        seriesNames = names;

        var colors = seriesColors;
        colors[seriesCount] = color;
        seriesColors = colors;

        seriesCount++;
    }

    Gradient {
        id: buttonGradient
        GradientStop { position: 0.0; color: "#F0F0F0" }
        GradientStop { position: 1.0; color: "#A0A0A0" }
    }

    Gradient {
        id: buttonGradientHovered
        GradientStop { position: 0.0; color: "#FFFFFF" }
        GradientStop { position: 1.0; color: "#B0B0B0" }
    }

    //![2]
    Component {
        id: legendDelegate
        Rectangle {
            id: rect
    //![2]
            property string name: seriesNames[index]
            property color markerColor: seriesColors[index]
            gradient: buttonGradient
            border.color: "#A0A0A0"
            border.width: 1
            radius: 4

            implicitWidth: label.implicitWidth + marker.implicitWidth + 30
            implicitHeight: label.implicitHeight + marker.implicitHeight + 10

            Row {
                id: row
                spacing: 5
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 5
                Rectangle {
                    id: marker
                    anchors.verticalCenter: parent.verticalCenter
                    color: markerColor
                    opacity: 0.3
                    radius: 4
                    width: 12
                    height: 10
                }
                Text {
                    id: label
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: -1
                    text: name
                }
            }

    //![3]
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    rect.gradient = buttonGradientHovered;
                    legend.entered(label.text);
                }
                onExited: {
                    rect.gradient = buttonGradient;
                    legend.exited(label.text);
                    marker.opacity = 0.3;
                    marker.height = 10;
                }
                onClicked: {
                    legend.selected(label.text);
                    marker.opacity = 1.0;
                    marker.height = 12;
                }
            }
    //![3]
        }
    }

    //![1]
    Row {
        id: legendRow
        anchors.centerIn: parent
        spacing: 10

        Repeater {
            id: legendRepeater
            model: seriesCount
            delegate: legendDelegate
        }
    }
    //![1]
}
