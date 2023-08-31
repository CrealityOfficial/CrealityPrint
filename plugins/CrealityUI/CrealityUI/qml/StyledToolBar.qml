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
ToolBar {
    id: control
    property alias mymodel: buttonlist.model
    background: Rectangle {
        implicitHeight: 40
        color: "#42bdd8"

        Rectangle {
            width: parent.width
            height: 1
            anchors.bottom: parent.bottom
            color: "transparent"
           // border.color: "#21be2b"
        }
    }

    RowLayout {
        id: rowLay
        anchors.fill: parent
//        Text {
//            id: logo
//            text: qsTr("CREALITY")
//            font.family: "Helvetica"; font.pointSize: 13; font.bold: true
//            color: "#ffffff"

//        }
        Rectangle
        {
            color: "transparent"
            height:  parent.height
            width : 12
        }

        Image {
            id : logoImage
            anchors.leftMargin: 12
            source: "qrc:/UI/images/logo.png"
        }
        ListView {
            id: buttonlist
            orientation: Qt.Horizontal
          //  anchors.left: logoImage.right
            width: parent.width-logoImage.width-12
            delegate:ToolbarButton {
                enabledIconSource:enabledIcon
                pressedIconSource:pressedIcon
                //qmlSource:source
                //itemSource:item
            }

        }
        Rectangle {
          Layout.fillWidth:true
          color: "red"
        }
    }
}
