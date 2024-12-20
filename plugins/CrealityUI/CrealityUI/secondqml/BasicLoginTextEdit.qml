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

    // padding: 8
    leftPadding: 16* screenScaleFactor + headImage.width
    rightPadding: 16* screenScaleFactor + idtailBtn.width
    property var radius : 8* screenScaleFactor
	property var keyStr: ""
	property var errorCode: 0x0000
    property var headImageSrc: ""
    property var tailImageSrc: ""
    property var hoveredTailImageSrc: ""
    property var imgPadding:  8*screenScaleFactor
    signal tailBtnClicked()

    placeholderTextColor: Color.transparent(control.color, 0.5)
    selectionColor: control.palette.highlight
    selectedTextColor: control.palette.highlightedText
    property var strToolTip: ""
    property alias unitChar: idUnitChar.text
	property alias baseValidator: control.validator
    property color defaultBackgroundColor: Constants.dialogItemRectBgColor
    property  color  itemBorderColor: Constants.dialogItemRectBgBorderColor
    property color itemBorderHoverdColor: Constants.textRectBgHoveredColor
    opacity : control.enabled ? 1 : 0.3
    color: Constants.textColor
    //validator: RegExpValidator { regExp: /(\d{1,6})([.,]\d{1,2})?$/ }
//    placeholderText: "hello"
    selectByMouse  : true

    Image {
        id : headImage
        y: (control.height - headImage.width)/2
        x: imgPadding
        height:sourceSize.height
        width: sourceSize.width
        visible: headImageSrc=="" ? false : true
        source: headImageSrc ? headImageSrc : ""
    }
	
    PlaceholderText {
        id: placeholder
        x: control.leftPadding
        y: control.topPadding
        width: control.width - (control.leftPadding + control.rightPadding)
        height: control.height - (control.topPadding + control.bottomPadding)

        text: control.placeholderText
        font: control.font
        color: control.placeholderTextColor
        verticalAlignment: control.verticalAlignment
        visible: !control.length && !control.preeditText && (!control.activeFocus || control.horizontalAlignment !== Qt.AlignHCenter)
        elide: Text.ElideRight
        renderType: control.renderType
        opacity : control.enabled ? 1 : 0.7
    }
    Label
    {
        id : idUnitChar
        anchors.bottom: placeholder.bottom
        anchors.right: placeholder.right
        text: ""
        width: contentWidth
        height: placeholder.height
        color: enabled ? "#999999" : Qt.lighter("#999999",0.7)
        opacity : control.enabled ? 1 : 0.3

    }

    BasicButton 
    {
        id: idtailBtn
        height:imgitem.sourceSize.height
        width: imgitem.sourceSize.width
        btnRadius: 0
        btnBorderW: 0
        visible: tailImageSrc=="" ? false : true
        defaultBtnBgColor : "transparent"
        hoveredBtnBgColor: "transparent"
        anchors.bottom: control.bottom
        anchors.bottomMargin: (control.height - idtailBtn.height)/2
        anchors.right: control.right
        anchors.rightMargin: imgPadding
        Image{
            id: imgitem
            anchors.centerIn: parent
            fillMode: Image.Pad
            source: idtailBtn.hovered ? hoveredTailImageSrc : tailImageSrc
        }
        onSigButtonClicked:
        {
            tailBtnClicked()
        }
    }

    QQC2.ToolTip {
        id: tipCtrl
        visible: hovered&&(strToolTip!="") ? true : false
        //timeout: 2000
		delay: 100
		width: 400
		
		background: Rectangle {
            anchors.fill: parent
            color: "transparent"
        }
		
		contentItem: QQC2.TextArea{
			text: strToolTip
			width: parent.width
			wrapMode: TextEdit.WordWrap
			color: Constants.textColor
			font.family: Constants.labelFontFamily
			font.weight: Constants.labelFontWeight
            font.pointSize: Constants.labelFontPointSize_12
			readOnly: true
			background: Rectangle
			{
				anchors.fill : parent
				color: Constants.tooltipBgColor
				border.width:1
				//border.color:hovered ? "lightblue" : "#bdbebf"
			}
        }
    }
	
    background: Rectangle
    {
        border.width:1
        border.color:hovered ? itemBorderHoverdColor: itemBorderColor
        radius : control.radius
        color : defaultBackgroundColor
        opacity : control.enabled ? 1 : 0.7
    }
}
