/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.impl 2.12
import QtQuick.Templates 2.12 as T
import QtQuick.Controls 2.0 as QQC2

import "../qml"
TextField{
    id: control
    implicitWidth: Math.max(contentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset,
                            placeholder.implicitWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(contentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset,
                             placeholder.implicitHeight + topPadding + bottomPadding)
    padding: 2 * screenScaleFactor
    leftPadding: padding + 4* screenScaleFactor
    rightPadding: idUnitChar.contentWidth + 5* screenScaleFactor
    property int radius : 5 * screenScaleFactor
    property var keyStr: ""
    property var errorCode: 0x0000
    property var limitEnable: true
    property var onlyPositiveNum: true

    placeholderTextColor: Color.transparent(control.color, 0.5)
    selectionColor: control.palette.highlight
    selectedTextColor: control.palette.highlightedText
    property var strToolTip: ""
    property var borderW: 1
    property alias unitChar: idUnitChar.text
    property alias baseValidator: control.validator
    //property var baseValidator: ""//control.validator
    property color defaultBackgroundColor: "transparent" // Constants.dialogItemRectBgColor
    property color  itemBorderColor: Constants.dialogItemRectBgBorderColor
    property color itemBorderHoverdColor: Constants.textRectBgHoveredColor
    opacity : control.enabled ? 1 : 0.3
    color: enabled ? Constants.textColor : Constants.disabledTextColor
    font.family: Constants.labelFontFamily
    font.weight: Constants.labelFontWeight
    font.pointSize: Constants.labelFontPointSize_9
    validator: RegExpValidator { regExp: limitEnable ? (onlyPositiveNum ? /(\d{1,6})([.,]\d{1,2})?$/ : /^([\+ \-]?(([1-9]\d*)|(0)))([.]\d{0,2})?$/): /.*/}
    selectByMouse  : true

    signal editingFieldFinished(var key, var value)
    signal textFieldChanged(var key, var value)

    signal styleTextFieldFinished(var key, var item)
    signal styleTextFieldChanged(var key, var item)

    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        font: control.font
        text: control.placeholderText
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
        opacity : control.enabled ? 1 : 0.7
    }

    QQC2.ToolTip {
        id: tipCtrl
        visible: hovered&&(strToolTip!="") ? true : false
        //timeout: 2000
        delay: 100
        width: 400*screenScaleFactor
        implicitHeight: idTextArea.contentHeight + 40*screenScaleFactor
        font: control.font

        background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }

        contentItem: QQC2.TextArea{
            id: idTextArea
            text: strToolTip
            wrapMode: TextEdit.WordWrap
            color: enabled ? Constants.textColor : Constants.disabledTextColor
            font: control.font
            readOnly: true
            background: Rectangle{
                anchors.fill : parent
                color: Constants.tooltipBgColor
                border.width:1
                //border.color:hovered ? "lightblue" : "#bdbebf"
            }
        }
    }


    background: Rectangle
    {
        border.width: borderW
        border.color:hovered ? itemBorderHoverdColor : itemBorderColor
        radius : control.radius
        color :  defaultBackgroundColor
        opacity : control.enabled ? 1 : 0.7

        Label {
            id : idUnitChar
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 4*screenScaleFactor
            text: ""
            color: enabled ? "#999999" : Qt.lighter("#999999",0.7)
            opacity : control.enabled ? 1 : 0.3
            font: control.font
        }
    }
    onEditingFinished:
    {
        editingFieldFinished(keyStr, control.text)
        styleTextFieldFinished(keyStr, this)
    }

    onTextChanged:
    {
        textFieldChanged(keyStr, control.text)
        styleTextFieldChanged(keyStr, this)
    }
}
